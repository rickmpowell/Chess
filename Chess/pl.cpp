/*
 *
 *	pl.cpp
 * 
 *	Code for the player class. This includes the base class for all players,
 *	a player/UI class, and the AI player classes. 
 * 
 */

#include "pl.h"
#include "ga.h"
#include "debug.h"
#include "Resources/Resource.h"

/* these must be powers of two */
#ifndef NDEBUG
const uint16_t dcYield = 8;
#else
const uint16_t dcYield = 2048;
#endif


/*
 *
 *	PL base class
 * 
 *	The base class for all chess game players. 
 * 
 */


PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName),
		mveNext(mveNil), spmvNext(spmvAnimate), 
		pvmvBreak(nullptr), imvBreakLast(-1), cBreakRepeat(0)
{
}


PL::~PL(void)
{
}


wstring SzPercent(uint64_t wNum, uint64_t wDen)
{
	assert(wNum <= wDen);
	wchar_t sz[20] = L"", *pch = sz;
	if (wDen == 0)
		return wstring(L"/0");
	unsigned w1000 = (unsigned)((1000 * wNum) / wDen);
	pch = PchDecodeInt(w1000 / 10, pch);
	*pch++ = L'.';
	pch = PchDecodeInt(w1000 % 10, pch);
	*pch++ = L'%';
	*pch = 0;
	return wstring(sz);
}


/*	PL::ReceiveMv
 *
 *	receives a move from an external source, like the human UI, or by playing from
 *	a file.
 */
void PL::ReceiveMv(MVE mve, SPMV spmv)
{
	if (!ga.puiga->fInPlay)
		ga.puiga->Play(mve, spmv);
	else {
		mveNext = mve;
		spmvNext = spmv;
	}
}


EV PL::EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept
{
	return EvBaseApc(apc);
}


EV PL::EvBaseApc(APC apc) const noexcept
{
	EV mpapcev[apcMax] = { 0, 100, 275, 300, 500, 900, 200 };
	return mpapcev[apc];
}


void PL::SetAIBreak(const vector<MV>& vmv)
{
	if (pvmvBreak == nullptr) {
		delete pvmvBreak;
		pvmvBreak = nullptr;
	}
	pvmvBreak = new vector<MV>;
	*pvmvBreak = vmv;
}


void PL::InitBreak(void)
{
	cBreakRepeat = 0;
	imvBreakLast = -1;
}


class AIBREAK
{
public:
	PL& pl;
	int d;

	inline AIBREAK(PL& pl, const MVE& mve, int d) : pl(pl), d(d)
	{
		if (pl.pvmvBreak == nullptr || pl.imvBreakLast < d - 2)
			return;

		if (mve != (*pl.pvmvBreak)[(size_t)d - 1])
			return;

		pl.imvBreakLast++;
		if (pl.imvBreakLast == pl.pvmvBreak->size()-1)
			DebugBreak();
	}

	inline ~AIBREAK(void)
	{
		if (pl.pvmvBreak == nullptr || pl.imvBreakLast < d - 1)
			return;
		pl.imvBreakLast--;
	}
};


/*
 *
 *	PLAI
 * 
 *	AI computer players. Base PLAI implements alpha-beta pruned look ahead tree.
 * 
 */


PLAI::PLAI(GA& ga) : PL(ga, L"AI"), rgen(372716661UL), habdRand(0), 
		ttm(IfReleaseElse(ttmSmart, ttmConstDepth)), level(3),
		cYield(0), dSel(0)
{
	fecoPsqt = 1*fecoScale;
	fecoMaterial = 0*fecoScale;
	fecoMobility = 5*fecoScale;		/* scales up to about a bishop */
	fecoKingSafety = 10*fecoScale;
	fecoPawnStructure = 10*fecoScale;
	fecoTempo = 1*fecoScale;
	fecoRandom = 0*fecoScale;
	InitWeightTables();
}
 

/*	PLAI::FHasLevel
 *
 *	Returns true if the player has a quality of play level control.
 */
bool PLAI::FHasLevel(void) const noexcept
{
	return true;
}


/*	PLAI::SetLevel
 *
 *	Sets the quality of play level for the AI.
 */
void PLAI::SetLevel(int level) noexcept
{
	this->level = clamp(level, 1, 10);
}


void PLAI::SetTtm(TTM ttm) noexcept
{
	this->ttm = ttm;
}


void PLAI::StartGame(void)
{
	xt.Init(64 * 0x100000UL);

	/* initialize killers */

	for (int imvGame = 0; imvGame < 256; imvGame++)
		for (int imv = 0; imv < cmvKillers; imv++)
			amvKillers[imvGame][imv] = mvNil;

	InitHistory();
}


/*	PLAI::StartMoveLog
 *
 *	Opens a log entry for AI move generation. Must be paired with a corresponding
 *	EndMoveLog
 */
void PLAI::StartMoveLog(void)
{
	stbfMainTotal.Init(); stbfMainAndQTotal.Init();
	LogOpen(TAG(wjoin(to_wstring(ga.bdg.vmveGame.size()/2+1) + L".", ga.bdg.cpcToMove),  
				ATTR(L"FEN", ga.bdg)),
			L"(" + szName + L")", lgfBold);
}


/*	PLAI::EndMoveLog
 *
 *	Closes the move generation logging entry for AI move generation.
 */
void PLAI::EndMoveLog(void)
{
#ifndef NOSTATS
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	/* cache stats */
	LogData(wjoin(L"Cache Fill:", SzPercent(xt.cxevInUse, xt.cxevMax)));
	LogData(wjoin(L"Cache Probe Hit:", SzPercent(xt.cxevProbeHit, xt.cxevProbe)));
	LogData(wjoin(L"Cache Save Replace:", SzPercent(xt.cxevSaveReplace, xt.cxevSave)));
	LogData(wjoin(L"Cache Save Collision:", SzPercent(xt.cxevSaveCollision, xt.cxevSave)));

	/* time stats */
	duration dtp = tpEnd - tpMoveStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(wjoin(L"Branch factor:", (wstring)stbfMainTotal));
#endif
	LogClose(L"", L"", lgfNormal);
}


void PLAI::BuildPvSz(BDG& bdg, wstring& sz)
{
	XEV* pxev = xt.Find(bdg, 0, 0);
	if (pxev == nullptr || pxev->fVisited())
		return;
	MVE mve = bdg.MveFromMv(pxev->mv());
	sz += L" " + to_wstring(mve);
	if (mve.fIsNil())
		return;
	pxev->SetFVisited(true);
	bdg.MakeMv(mve);
	BuildPvSz(bdg, sz);
	bdg.UndoMv();
	pxev->SetFVisited(false);
}


void PLAI::LogInfo(BDG& bdg, EV ev, int d, int dSel)
{
	wstring sz;
	BuildPvSz(bdg, sz);
	/* TODO: hashfull. */
	DWORD dmsec = DmsecMoveSoFar();
	bool fMate = FEvIsMate(abs(ev));
	EV evScore = ev;
	if (fMate) {
		evScore = (DFromEvMate(abs(ev)) + 1) / 2;
		if (ev < 0)
			evScore = -evScore;
	}
	LogData(wjoin(L"info", 
				  L"score", fMate ? L"mate" : L"cp", evScore,
				  L"depth", d,
				  L"seldepth", dSel,
				  L"nodes", stbfMainAndQTotal.cmveNode,
				  L"time", dmsec,
				  L"nps", stbfMainAndQTotal.cmveNode * 1000 / dmsec,
				  L"pv", sz));
}


void PLAI::LogBestMove(BDG& bdg, MVE mveBest, int d, int dSel)
{
	LogInfo(bdg, mveBest.ev, d, dSel);
	LogData(wjoin(L"bestmove", to_wstring(mveBest)));
}


/*
 *
 *	LOGMVE
 * 
 *	Little open/close log wrapper. Saves a bunch of state so we can automatically
 *	close the log using the destructor without explicitly calling it.
 * 
 */


enum PRR : int {	/* pruning reason, for stats */
	prrNone,
	prrXt,
	prrBetaPrune,
	prrFutility,
	prrNullMove,
	prrPV
};

class LOGSEARCH
{
protected:
	PLAI& pl;
	BDG& bdg;
	wstring szData;
	PRR prr;

public:
	inline LOGSEARCH(PLAI& pl, BDG& bdg) noexcept : pl(pl), bdg(bdg), prr(prrNone) { }
	inline void SetData(const wstring& szNew) noexcept { szData = szNew; }
	inline void SetReason(PRR prr) { this->prr = prr; }
};

class LOGMVE : public LOGSEARCH, public AIBREAK
{
	const MVE& mvePrev;
	int lgdSav;
	int imvExpandSav;
	const MVE& mveBest;
	AB abInit;

	static int imvExpand;
	static MV amv[20];

public:
	inline LOGMVE(PLAI& pl, BDG& bdg,
				  const MVE& mvePrev, const MVE& mveBest, const AB& ab, int d, 
				  wchar_t chType = L' ') noexcept :
		LOGSEARCH(pl, bdg), AIBREAK(pl, mvePrev, d),
		mvePrev(mvePrev), mveBest(mveBest), abInit(-ab), lgdSav(0), imvExpandSav(0)
	{
		lgdSav = LgdShow();
		imvExpandSav = imvExpand;
		if (FExpandLog(mvePrev))
			SetLgdShow(lgdSav + 1);
		int lgd;
		if (!papp->FDepthLog(lgtOpen, lgd))
			pl.PumpMsg(false);
		else {
			pl.PumpMsg(true);
			papp->AddLog(lgtOpen, lgfNormal, lgd,
						 TAG(bdg.SzDecodeMvPost(mvePrev), ATTR(L"FEN", bdg)),
						 wjoin(wstring(1, chType) + to_wstring(d),
							   to_wstring(mvePrev.tsc()),
							   SzFromEv(mvePrev.ev), abInit));
		}
	}

	inline ~LOGMVE() noexcept
	{
		LogClose(bdg.SzDecodeMvPost(mvePrev), wjoin(SzFromEv(-mveBest.ev), SzEvt()), LgfEvt());
		SetLgdShow(lgdSav);
		imvExpand = imvExpandSav;
	}

	inline bool FExpandLog(const MVE& mve) const noexcept
	{
		if (mve.fIsNil())
			return false;
		if (amv[imvExpand] != mvAll) {
			if (mve != amv[imvExpand])
				return false;
		}
		imvExpand++;
		return true;
	}

	inline wstring SzEvt(void) const noexcept
	{
		if (abInit.FEvIsBelow(-mveBest.ev))
			return L"Low";
		else if (abInit.FEvIsAbove(-mveBest.ev))
			return L"Cut";
		else
			return L"PV";
	}

	inline LGF LgfEvt(void) const noexcept
	{
		if (abInit.FEvIsBelow(-mveBest.ev))
			return lgfNormal;
		else if (abInit.FEvIsAbove(-mveBest.ev))
			return lgfItalic;
		else // PV
			return lgfBold;
	}
};


/* a little debugging aid to trigger a change in log depth after a 
   specific sequence of moves */
int LOGMVE::imvExpand = 0;
MV LOGMVE::amv[] = { /*
	   MV(sqC2, sqC3),
	   MV(sqB8, sqC6),
	   MV(sqD1, sqB3),
	   MV(sqG8, sqF6),
	   MV(sqB3, sqB7),
	   mvAll,	*/
	   mvNil
};


/*
 *
 *	LOGITD
 * 
 *	Iterative deepening loop logging. Displays search depth and a-b window,
 *	along with the result of the cycle and branch factor.
 * 
 */


class LOGITD : public LOGSEARCH
{
	const MVE& mveBest;
public:
	LOGITD(PLAI& pl, BDG& bdg, const MVE& mveBest, AB ab, int d) noexcept : 
			LOGSEARCH(pl, bdg), mveBest(mveBest)
	{
		LogOpen(L"Depth", wjoin(d, ab), lgfNormal);
	}

	~LOGITD() noexcept
	{
		LogClose(L"Depth", wjoin(L"[BF=" + (wstring)pl.stbfMainTotal + L"]",
								 bdg.SzDecodeMvPost(mveBest),
								 SzFromEv(mveBest.ev)), 
				 lgfNormal);
	}
};


/*
 *
 *	VMVES class
 * 
 *	Move enumeration base class.
 * 
 */


VMVES::VMVES(BDG& bdg, PLAI* pplai, int d, GG gg) noexcept : VMVE(), pplai(pplai), gg(gg), d(d), pmveNext(begin()), tscCur(tscPrincipalVar)
{
	bdg.GenMoves(*this, gg == ggNoisyAndChecks ? ggAll : gg);
	Reset(bdg);
}


void VMVES::Reset(BDG& bdg) noexcept
{
	pmveNext = begin();
	cmvLegal = 0;
	tscCur = tscPrincipalVar;
	PrepTscCur(bdg, begin());
}


/*	VSMVE::FEnumMvNext
 *
 *	Finds the next move in the move list, returning false if there is no such
 *	move. The move is returned in pmve. The move is actually made on the board
 *	and illegal moves are checked for
 */
bool VMVES::FEnumMvNext(BDG& bdg, MVE*& pmve) noexcept
{
	while (pmveNext < end()) {

		/* swap the best move into the next mve to return */

		MVE* pmveBest;
		for (; (pmveBest = PmveBestFromTscCur(pmveNext)) == nullptr;
			 tscCur++, PrepTscCur(bdg, pmveNext))
			;
		pmve = &*pmveNext;
		swap(*pmveNext, *pmveBest);
		pmveNext++;

		/* make sure move is legal */

		bdg.MakeMv(*pmve);
		pplai->xt.Prefetch(bdg);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			if (GgType(gg) != ggNoisyAndChecks)
				return true;
			if (pmve->apcCapture() || pmve->apcPromote() || bdg.FInCheck(bdg.cpcToMove))
				return true;
		}
		bdg.UndoMv();
	}
	return false;
}


/*	VMVES::UndoMv
 *
 *	Undoes the move made by FMakeMvNext
 */
void VMVES::UndoMv(BDG& bdg) noexcept
{
	bdg.UndoMv();
}


/*	VMVES::FOnlyOneMove
 *
 *	Returns true if there is only one legal move. The legal move is returned
 *	in mve.
 * 
 *	WARNING! For now, this only works if the VMVES constructor was called with 
 *	ggLegal.
 */
bool VMVES::FOnlyOneMove(MVE& mve) const noexcept
{
	switch (size()) {
	case 0:
		mve = mveNil;
		break;
	case 1:
		mve = (*this)[0];
		break;
	default:
		return false;
	}
	return true;
}


/*	VMVES::PmveBestFromTscCur
 *
 *	Finds the best move from the movelist that matches the current score type
 *	that we're enumerating. Returns nullptr if no matches.
 */
MVE* VMVES::PmveBestFromTscCur(VMVE::it pmveFirst) noexcept
{
	/* find first one */

	VMVE::it pmve = pmveFirst;
	for ( ; pmve < end(); ++pmve) {
		if (pmve->tsc() == tscCur)
			goto KeepGoing;
	}
	return nullptr;

	/* and find the best of the rest */

KeepGoing:
	MVE* pmveBest = &(*pmve);
	while (++pmve < end()) {
		if (pmve->tsc() == tscCur && pmve->ev > pmveBest->ev)
			pmveBest = &(*pmve);
	}
	return pmveBest;
}


/*	VMVES::PrepTscCur
 *
 *	Prepares the enumeration for the next score type. We try to lazy evaluate as 
 *	many moves as possible so we don't waste time evaluating positions that might
 *	never be visited because of pruning. 
 * 
 *	Currently we break moves into ... (1) the principal variation, (2) any other 
 *	move in the transposition table that was fully evaluated, (3) captures, and
 *	and (4) everything else.
 */
void VMVES::PrepTscCur(BDG& bdg, VMVE::it pmveFirst) noexcept
{
	switch (tscCur) {
	case tscPrincipalVar:
	{
		/* first time through the enumeration, snag the principal variation. This can
		   be done very quickly with just a transposition table probe. While we're at
		   it, go ahead and reset all the other moves to nil so we re-score them on
		   subsequent passes */
		XEV* pxev = pplai->xt.Find(bdg, d, d);
		MVE mvePV;
		if (pxev && pxev->tev() == tevEqual) {
			mvePV = bdg.MveFromMv(pxev->mv());
			mvePV.ev = -pxev->ev(d);
		}
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			assert(!pmve->fIsNil());
			pmve->SetTsc(tscNil);
			if (mvePV == *pmve) {
				assert(pxev != nullptr);
				pmve->SetTsc(tscPrincipalVar);
				pmve->ev = mvePV.ev;
			}
		}
		break;
	}

	case tscGoodCapture:
	{
		/* we can score captures quickly without making the move, so do them early */
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			assert(pmve->tsc() == tscNil);
			bdg.FillUndoMvSq(*pmve);
			if (!pmve->fIsCapture() && !pmve->apcPromote())
				continue;
			pmve->ev = pplai->ScoreCapture(bdg, *pmve);
			/* cut-off determined experimentally */
			pmve->SetTsc(pmve->ev < -200 ? tscBadCapture : tscGoodCapture);
		}
		break;
	}

	case tscKiller:
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			if (pmve->tsc() != tscNil)
				continue;
			if (pplai->FScoreKiller(bdg, *pmve))
				pmve->SetTsc(tscKiller);
			else if (pplai->FScoreHistory(bdg, *pmve))
				pmve->SetTsc(tscHistory);
		}
		break;

	case tscHistory:
		/* scored in tscKiller */
		break;

	case tscXTable:
	{
#ifdef LATER
		/* this actually makes it worse */
		/* get the eval of any move that has an entry in the tranposition table */
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			if (pmve->tsc() != tscNil)
				continue;
			bdg.MakeMvSq(*pmve);
			XEV* pxev = pplai->xt.Find(bdg, d, d);
			if (pxev && pxev->tev() == tevEqual) {
				pmve->SetTsc(tscXTable);
				pmve->ev = -pxev->ev(d);
			}
			bdg.UndoMvSq(*pmve);
		}
#endif
		break;
	}

	case tscBadCapture:
		/* these were already scored by tscGoodCapture */
		break;

	case tscEvOther:
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			if (pmve->tsc() != tscNil)
				continue;
			bdg.MakeMvSq(*pmve);
			pmve->SetTsc(tscEvOther);
			pmve->ev = -pplai->ScoreMove(bdg, *pmve);
			bdg.UndoMvSq(*pmve);
		}
		break;
	}
}


/*	PLAI::MveGetNext
 *
 *	Returns the next move for the player. Assumes we've already checked for
 *	mates on entry. If the game cancels or times out, returns a cancel move
 *	and the game state will be changed.
 *
 *	Returns information in spmv for how the board should be display the move,
 *	but this isn't used in AI players. 
 * 
 *	For an AI, this is the root node of the alpha-beta search.
 */
MVE PLAI::MveGetNext(SPMV& spmv) noexcept
{
	spmv = spmvAnimate;
	cYield = 0; sint = sintNull;
	StartMoveLog();

	BDG bdg = ga.bdg;
	habdRand = genhabd.HabdRandom(rgen);
	InitBreak();
	xt.Clear();		
	xt.BumpAge();
	AgeHistory();

	mveBestOverall = MVE(mvuNil, -evInf);
	VMVES vmves(bdg, this, 0, ggLegal);

	InitWeightTables();
	InitTimeMan(bdg);

	/* main iterative deepening and aspiration window loop */

	MVE mveBest;
	AB ab(-evInf, evInf);
	int dLim = 2;
	dSel = 0;
	do {
		mveBest = MVE(mvuNil, -evInf);

		/* stats and logging */
		stbfMain.Init(); stbfMainAndQ.Init();
		LOGITD logitd(*this, bdg, mveBest, ab, dLim);
		stbfMain.IncGen(); stbfMainAndQ.IncGen();

		/* do search for each move at current depth/aspiration window */
		vmves.Reset(bdg);
		FSearchMveBest(bdg, vmves, mveBest, ab, 0, dLim, tsAll);
		SaveXt(bdg, mveBest, ab, 0, dLim);
		
		stbfMainTotal += stbfMain; stbfMainAndQTotal += stbfMainAndQ;
	} while (!vmves.FOnlyOneMove(mveBestOverall) && 
			 FDeepen(bdg, mveBest, ab, dLim) && 
			 FBeforeDeadline(dLim));

	EndMoveLog();
	LogBestMove(ga.bdg, mveBestOverall, dLim, dSel);
	return mveBestOverall;
}


/*	PLAI::FSearchMveBest
 *
 *	Finds the best move for the given board and sorted movelist, using the given
 *	alpha-beta window and search depth. Returns true the search should be pruned.
 *
 *	Handles the principal value search optimization.
 */
bool PLAI::FSearchMveBest(BDG& bdg, VMVES& vmves, MVE& mveBest, AB ab, int d, int& dLim, TS ts) noexcept
{
	/* do first move (probably a PV move) with full a-b window */

	MVE* pmve;
	if (!vmves.FEnumMvNext(bdg, pmve))
		return false;
	pmve->ev = -EvBdgSearch(bdg, *pmve, -ab, d + 1, dLim, ts);
	vmves.UndoMv(bdg);
	if (FPrune(bdg, *pmve, mveBest, ab, d, dLim))
		return true;

	/* subsequent moves get the PV pre-search optimization which uses an
	   ultra-narrow window. We pray that narrow window quickly fails low;
	   if we don't, redo the search with the full window */

	while (vmves.FEnumMvNext(bdg, pmve)) {
		pmve->ev = -EvBdgSearch(bdg, *pmve, -ab.AbNull(), d + 1, dLim, 
								ts+tsNoPruneNullMove+tsNoPruneFutility+tsNoPruneRazoring);
 		if (!ab.FEvIsBelow(pmve->ev) && !ab.fIsNull())
			pmve->ev = -EvBdgSearch(bdg, *pmve, -ab, d + 1, dLim, ts);
		vmves.UndoMv(bdg);
		if (FPrune(bdg, *pmve, mveBest, ab, d, dLim))
			return true;
	}
	return false;
}


/*	PLAI::EvBdgSearch
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth dLim; d is current. Uses alpha-beta pruning.
 * 
 *	The board will be modified, but it will be restored to its original state in 
 *	all cases.
 * 
 *	On entry mvePrev has the last move made.
 * 
 *	Returns board evaluation in centi-pawns.
 */
EV PLAI::EvBdgSearch(BDG& bdg, const MVE& mvePrev, AB abInit, int d, int dLim, TS ts) noexcept
{
	stbfMain.IncNode();

	/* if we're at depth, go to quiescence to get static eval */

	if (d >= dLim)
		return EvBdgQuiescent(bdg, mvePrev, abInit, d, ts);
	
	stbfMainAndQ.IncNode();
	MVE mveBest;
	LOGMVE logmve(*this, bdg, mvePrev, mveBest, abInit, d, ' ');

	if (FTestForDraws(bdg, mveBest))
		return mveBest.ev;

	stbfMain.IncGen(); 
	stbfMainAndQ.IncGen();

	/* check transposition table, evaluate the board, then try futility pruning and
	   null move heuristic pruning tricks */

	if (FLookupXt(bdg, mveBest, abInit, d, dLim))
		return mveBest.ev;
	bool fInCheck = bdg.FInCheck(bdg.cpcToMove);
	GG gg = ggAll + ggPseudo;
	if (fInCheck)
		dLim++;
	else {
		EV evStatic = EvBdgStatic(bdg, mvePrev);
		if (FTryStaticNullMove(bdg, mveBest, evStatic, abInit, d, dLim, ts))
			return mveBest.ev;
		if (FTryNullMove(bdg, mveBest, abInit, d, dLim, ts))
			return mveBest.ev;
		if (FTryRazoring(bdg, mveBest, evStatic, abInit, d, dLim, ts))
			return mveBest.ev;
		if (FTryFutility(bdg, mveBest, evStatic, abInit, d, dLim, ts))
			gg = ggNoisyAndChecks + ggPseudo;
	}

	/* if none of those optimizations work, generate moves and do a full search */

	mveBest.ev = -evInf;
	VMVES vmves(bdg, this, d, gg);
	if (!FSearchMveBest(bdg, vmves, mveBest, abInit, d, dLim, ts) && vmves.cmvLegal == 0)
		mveBest = MVE(mvuNil, fInCheck ? -EvMate(d) : evDraw);
	SaveXt(bdg, mveBest, abInit, d, dLim);
	return mveBest.ev;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the "quiet" evaluation after the given board/last move from the point 
 *	of view of the player next to move; it only considers captures and other "noisy" 
 *	moves. Alpha-beta prunes within the abInit window.
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, const MVE& mvePrev, AB abInit, int d, TS ts) noexcept
{
	stbfMainAndQ.IncNode();
	dSel = max(dSel, d);

	int dLim = dMax;
	MVE mveBest;
	LOGMVE logmve(*this, bdg, mvePrev, mveBest, abInit, d, 'Q');

	if (FLookupXt(bdg, mveBest, abInit, d, d))
		return mveBest.ev;

	/* first off, the player may refuse the capture, so get full, slow static eval 
	   and check if we're already in a pruning situation */

	AB ab = abInit;
	mveBest = MVE(mvuNil, EvBdgStatic(bdg, mvePrev), tscEvOther);
	{ LOGMVE logmve(*this, bdg, mveBest, mveBest, abInit, d, 'S'); }
	if (FPrune(bdg, mveBest, mveBest, ab, d, dLim))
		return mveBest.ev;

	/* then recursively evaluate noisy moves */
		
	VMVES vmves(bdg, this, d, ggPseudo + ggNoisy);
	stbfMainAndQ.IncGen();
	for (MVE* pmve = nullptr; vmves.FEnumMvNext(bdg, pmve); ) {
		pmve->ev = -EvBdgQuiescent(bdg, *pmve, -ab, d + 1, ts);
		vmves.UndoMv(bdg);
		if (FPrune(bdg, *pmve, mveBest, ab, d, dLim))
			break;
	}

	return mveBest.ev;
}


/*	PLAI::FLookupXt
 *
 *	Checks the transposition table for a board entry at the given search depth. Returns 
 *	true if we should stop the search at this point, either because we found an exact 
 *	match of the board/depth, or the inexact match is outside the alpha/beta interval.
 *
 *	mveBest will contain the evaluation we should use if we stop the search.
 */
bool PLAI::FLookupXt(BDG& bdg, MVE& mveBest, AB ab, int d, int dLim) noexcept
{
	/* look for the entry in the transposition table */

	XEV* pxev = xt.Find(bdg, d, dLim);
	if (pxev == nullptr)
		return false;
	
	/* adjust the value based on alpha-beta interval */

	switch (pxev->tev()) {
	case tevEqual:
		mveBest.ev = pxev->ev(d);
		break;
	case tevHigher:
		if (!ab.FEvIsAbove(pxev->ev(d)))
			return false;
		mveBest.ev = ab.evBeta;
		break;
	case tevLower:
		if (!ab.FEvIsBelow(pxev->ev(d)))
			return false;
		mveBest.ev = ab.evAlpha;
		break;
	}
	
	mveBest.SetMvu(bdg.MveFromMv(pxev->mv()));
	return true;
}


/*	PLAI::SaveXt
 *
 *	Saves the evaluated board in the transposition table, including the depth,
 *	best move, and making sure we keep track of whether the eval was outside
 *	the a-b window.
 */
XEV* PLAI::SaveXt(BDG& bdg, MVE mveBest, AB ab, int d, int dLim) noexcept
{
	/* don't save cancels or timeouts */
	if (FEvIsInterrupt(mveBest.ev))
		return nullptr;

	if (ab.FEvIsBelow(mveBest.ev))
		return xt.Save(bdg, mveBest, tevLower, d, dLim);
	if (ab.FEvIsAbove(mveBest.ev))
		return xt.Save(bdg, mveBest, tevHigher, d, dLim);
	return xt.Save(bdg, mveBest, tevEqual, d, dLim);
}


/*	PLAI::SaveKiller
 *
 *	Remember killer moves (non-captures that caused a beta cut-off), indexed by 
 *	the move number of the game. Killer moves are used for improving move ordering 
 *	by making a pretty good guess what moves are cut moves.
 */
void PLAI::SaveKiller(BDG& bdg, MVE mve) noexcept
{
	if (mve.fIsCapture() || mve.apcPromote())
		return;
	int imveLim = bdg.imveCurLast + 1;
	if (imveLim >= 256 || amvKillers[imveLim][0] == mve)
		return;
	for (int imv = cmvKillers-1; imv >= 1; imv--)
		amvKillers[imveLim][imv] = amvKillers[imveLim][imv-1];
	amvKillers[imveLim][0] = mve;
}


/*	PLAI::AddHistory
 *
 *	Bumps the move history count, which is non-captures that cause beta cut-offs, indexed
 *	by the source/destination squares of the move
 */
void PLAI::AddHistory(BDG& bdg, MVE mve, int d, int dLim) noexcept
{
	if (mve.fIsCapture() || mve.apcPromote())
		return;
	int& csqHistory = mppcsqcHistory[bdg.PcFromSq(mve.sqFrom())][mve.sqTo()];
	csqHistory += (dLim - d) * (dLim - d);
	if (csqHistory >= evMateMin)
		AgeHistory();
}


/*	PLAI::SubtractHistory
 *
 *	Lowers history count in the history table, which is done on non-beta cut-offs.
 *	Note that bumping is much faster than decaying.
 */
void PLAI::SubtractHistory(BDG& bdg, MVE mve) noexcept
{
	if (mve.fIsCapture() || mve.apcPromote())
		return;
	int& csqHistory = mppcsqcHistory[bdg.PcFromSq(mve.sqFrom())][mve.sqTo()];
	if (csqHistory > 0)
		csqHistory--;
}


/*	PLAI::InitHistory
 *
 *	Zero out the history to start a game
 */
void PLAI::InitHistory(void) noexcept
{
	for (PC pc = 0; pc < pcMax; pc++)
		for (SQ sqTo = 0; sqTo < sqMax; sqTo++)
			mppcsqcHistory[pc][sqTo] = 0;
}


/*	PLAI::AgeHistory
 *
 *	Redece old history's impact with each move
 */
void PLAI::AgeHistory(void) noexcept
{
	for (PC pc = 0; pc < pcMax; pc++)
		for (SQ sqTo = 0; sqTo < sqMax; sqTo++)
			mppcsqcHistory[pc][sqTo] /= 8;
}


/*	PLAI::FTryFutility
 *
 *	Futility pruning works by checking a move that can't possibly be made good
 *	enough to exceed alpha. 
 */
bool PLAI::FTryFutility(BDG& bdg, MVE& mveBest, EV evStatic, AB ab, int d, int dLim, TS ts) noexcept
{
	const int ddFutility = 4;
	if (dLim - d >= ddFutility)
		return false;

	static const EV mpdddevFutility[ddFutility] = { 
		0, 
		200, 300, 500  
	};
	EV devMargin = mpdddevFutility[dLim - d];
	return evStatic + devMargin <= ab.evAlpha;
}


/*	PLAI::FTryStaticNullMove
 *
 *	If our static material score is so large that a big hit is still greater than 
 *	beta, we assume we're in a cut situation.
 */
bool PLAI::FTryStaticNullMove(BDG& bdg, MVE& mveBest, EV evStatic, AB ab, int d, int dLim, TS ts) noexcept
{
	if (ts & tsNoPruneNullMove)
		return false;
	EV devMargin = evPawn * (dLim - d);
	if (!ab.FEvIsAbove(evStatic - devMargin))
		return false;
	mveBest = MVE(mvuNil, evStatic - devMargin, tscEvOther);
	return true;
}


/*	PLAI::FTryNullMove
 *
 *	The null-move pruning heuristic. This heuristic has primary assumption that at
 *	least one move improves the player's position. So we try just skipping our turn
 *	(the null move) and continue with a quick search with a narrow a-b window and
 *	reduced depth and see if we get a prune. If we do, then this is probably a shitty
 *	position.
 *
 *	This trick doesn't work if we're in check because the null move would be illegal.
 *	Zugzwang positions violate the primary assumption - if either occur, this technique
 *	would cause improper evals.
 *
 *	Returns true if we successfully found a pruning situation and search can be 
 *	stopped. We do not have a proper evaluated move in this situation, but we do have 
 *	a high/cut/prune evaluation.
 */
bool PLAI::FTryNullMove(BDG& bdg, MVE& mveBest, AB ab, int d, int dLim, TS ts) noexcept
{
	int R = 2 + (dLim - d) / 4;
	if (d + 1 >= dLim - R ||			// don't bother if search is going this deep anyway
		bdg.gph >= gphMidMax ||			// super lame zugzwang estimate
		(ts & tsNoPruneNullMove))		// don't do null move inside null move
		return false;

	mveBest.SetMvu(mvuNil);
	bdg.MakeMvNull();
	mveBest.ev = -EvBdgSearch(bdg, mveBest, (-ab).AbNull(), d + 1, dLim - R, ts + tsNoPruneNullMove);
	bdg.UndoMv();
	return ab.FEvIsAbove(mveBest.ev);
}


EV mpdddevFutility[9] = {
	0,
	100, 
	160, 
	220, 
	280, 
	340, 
	400, 
	460, 
	520
};


/*	PLAI::FTryRazoring
 *
 *	If we're near the horzion and static evaluation is terrible, try a quick 
 *	quiescent search to see if we'll probably fail low. If qsearch fails low, 
 *	it probably knows what it's talking about, so bail out and return alpha.  
 */
bool PLAI::FTryRazoring(BDG& bdg, MVE& mveBest, EV evStatic, AB ab, int d, int dLim, TS ts) noexcept
{
	if ((ts & tsNoPruneRazoring) || dLim - d > 2)
		return false;

	assert(dLim-d < CArray(mpdddevFutility));
	EV dev = 3 * mpdddevFutility[dLim-d];
	if (ab.FEvIsBelow(evStatic + dev)) {
		EV ev = EvBdgQuiescent(bdg, mveBest, ab, d, ts);
		if (ab.FEvIsBelow(ev)) {
			mveBest = MVE(mvuNil, ab.evAlpha, tscEvOther);
			return true;
		}
	}
	return false;
}


/*	PLAI::FPrune
 *
 *	Helper function to perform standard alpha-beta pruning operations after evaluating a
 *	board position. Keeps track of alpha and beta cut offs, the best move (in evmBest), 
 *	and adjusts search depth when mates are found. Returns true if we should prune.
 * 
 *	Also returns true if the search should be interrupted for some reason. pmve->ev will 
 *	be modified with the reason for the interruption.
 */
bool PLAI::FPrune(BDG& bdg, MVE &mve, MVE& mveBest, AB& ab, int d, int& dLim) noexcept
{
	/* keep track of best move */

	if (mve.ev > mveBest.ev)
		mveBest = mve;
	
	/* above beta, prune; pruned moves are killer moves */

	if (ab.FEvIsAbove(mve.ev)) {
		mveBest = mve;
		SaveKiller(bdg, mveBest);
		return true;
	}

	/* if inside the alpha-beta window. Narrow alpha down with what is now probably 
	   the best move's eval; if we're mating the other guy, we can adjust the search
	   depth since from now on, we're only looking for quicker mates */
	
	if (!ab.FEvIsBelow(mve.ev)) {
		ab.RaiseAlpha(mve.ev);
		if (FEvIsMate(mve.ev))	
			dLim = DFromEvMate(mve.ev);
		AddHistory(bdg, mve, d, dLim);
	}

	/* If Esc is hit (set by message pump), or if we're taking too damn long to do
	   the search, force the search to prune all the way back to root, where we'll 
	   abort the search */

	if (sint != sintNull) {
		mve.ev = mveBest.ev = (sint == sintCanceled) ? evCanceled : evTimedOut;
		mveBest.SetMvu(mvuNil);
		return true;
	}

	return false;
}




/*	PLAI::FTestForDraws
 *
 *	Checks for non-stalemate draws.
 */
bool PLAI::FTestForDraws(BDG& bdg, MVE& mveBest) noexcept
{
	if (bdg.GsTestGameOver(1, 2) == gsPlaying)
		return false;
	mveBest.ev = evDraw;
	mveBest.SetMvu(mvuNil);
	return true;
}


/*	PLAI::FDeepen
 *
 *	Just a little helper to set us up for the next round of the iterative deepening
 *	loop. Our loop is set up to handle both deepening and aspiration windows, so
 *	sometimes we increase the depth, other times we widen the aspiration window.
 *
 *	Returns true if we successfully progressed the depth/window; returns false if we 
 *	should stop the search, which happens on an abort/interrupt, or a forced mate.
 * 
 *	Keeps track of the best overall move so far, which only changes when we have a
 *	successful search.
 */
bool PLAI::FDeepen(BDG& bdg, MVE mveBest, AB& ab, int& d) noexcept
{
	if (mveBest.ev == evCanceled) {
		mveBestOverall.SetMvu(mvuNil);
		return false;
	}
	if (mveBest.ev == evTimedOut) {
		/* we only time out when we have already found at least one best overall move */
		assert(!mveBestOverall.fIsNil());
		return false;
	}	

	/* If the search failed with a narrow a-b window, widen the window up some and
	   try again */

	if (ab.FEvIsBelow(mveBest.ev))
		ab.AdjMissLow();
	else if (ab.FEvIsAbove(mveBest.ev))
		ab.AdjMissHigh();
	else {
		/* yay, we found a move - go deeper in the next pass, but use a tight
		   a-b window (the aspiration window optimization) at first in hopes we'll 
		   get lots of pruning */

		mveBestOverall = mveBest;
		LogInfo(bdg, mveBestOverall.ev, d, dSel);
		if (FEvIsMate(mveBest.ev) || FEvIsMate(-mveBest.ev))
			return false;
		ab = ab.AbAspiration(mveBest.ev, 20);
		d += 1;
	}
	return true;
}


/*	PLAI::InitTimeMan
 *
 *	Gets us ready for the intelligence that will determine how long we spend analyzing the
 *	move. Time management happens in two places: during the main iterative deepening loop,
 *	and as an interrupt on the clock tick. 
 * 
 *	This basically sets a deadline. If we ever exceed this deadline in the iterative 
 *	deepening, we immediately stop the search. However, the iterative deepening loop may 
 *	not have control for minutes at a time. So, on the clock tick, we also check; but 
 *	we'll let the AI think quite a ways past the deadline if we're in the middle of a long
 *	search.
 */
void PLAI::InitTimeMan(BDG& bdg) noexcept
{
	static const DWORD mpleveldmsec[] = { 0,
		msecHalfSec, 1*msecSec, 2*msecSec, 5*msecSec, 10*msecSec, 15*msecSec, 30*msecSec,
		1*msecMin, 2*msecMin, 5*msecMin
	};

	/* for constant depth time management, leave the flag variable -1 */

	dmsecFlag = -1;
	tpMoveStart = high_resolution_clock::now();
	
	switch (ttm) {
	default:
		break;
	case ttmSmart:
		if (!ga.prule->FUntimed()) {
			dmsecFlag = ga.DmsecRemaining(bdg.cpcToMove);
			int nmvCur = ga.bdg.vmveGame.NmvFromImv(bdg.imveCurLast+1);	/* move number of the move we're about to make */
			int nmvFlag = ga.prule->TmiFromNmv(nmvCur).nmvLast;	/* move number we must make before we flag */
			DWORD dmsecMove = ga.prule->DmsecAddMove(bdg.cpcToMove, nmvCur);
			/* number of moves left in the game based on total material on the board;
			   yeah, I know I know, really lame */
			int dnmv = WInterpolate(EvMaterialTotal(bdg), 200, 7800, 10, 50);
			if (nmvFlag > 0)
				dnmv = min(dnmv, nmvFlag - nmvCur + 1);
			assert(dnmv > 0);
			dmsecDeadline = min(dmsecFlag/dnmv + dmsecMove, dmsecFlag);
			dmsecDeadline = min(dmsecDeadline, mpleveldmsec[level]);
			LogData(wjoin(L"Time target:", SzCommaFromLong(dmsecDeadline), L"ms"));
			break;
		}
		/* games without clocks just use a constant time per move */
		ttm = ttmTimePerMove;
		[[fallthrough]];
	case ttmTimePerMove:
		dmsecDeadline = mpleveldmsec[level];
		break;
	}
}


/*	PLAI::EvMaterialTotal
 *
 *	Add up the complete material on the board. This is a pretty crude measure
 *	and is only used as a proxy for the amout of time left in the game.
 */
EV PLAI::EvMaterialTotal(BDG& bdg) const noexcept
{
	EV ev = 0;
	for (CPC cpc = cpcWhite; cpc < cpcMax; cpc++)
		ev += EvMaterial(bdg, cpc);
	return ev;
}


EV PLAI::EvMaterial(BDG& bdg, CPC cpc) const noexcept
{
	static EV mpapcev[apcMax] = { 0, 100, 300, 300, 500, 900, 250 };
	EV ev = 0;
	for (APC apc = apcPawn; apc < apcMax; apc++)
		ev += mpapcev[apc] * bdg.mppcbb[PC(cpc, apc)].csq();
	return ev;
}


/*	PLAI::FBeforeDeadline
 *
 *	Lets us know when we should stop searching for moves. This is where our time management
 *	happens. Returns true if we should keep searching.
 */
bool PLAI::FBeforeDeadline(int dLim) noexcept
{
	if (mveBestOverall.fIsNil())
		return true;

	switch (ttm) {

	case ttmSmart:
	case ttmTimePerMove:
		return DmsecMoveSoFar() < dmsecDeadline;
	case ttmConstDepth:
		/* different levels get different depths */
		return dLim <= level;
	case ttmInfinite:
		return true;
	}

	return false;
}


DWORD PLAI::DmsecMoveSoFar(void) const noexcept
{
	time_point<high_resolution_clock> tp = high_resolution_clock::now();
	duration dtp = tp - tpMoveStart;
	milliseconds dmsec = duration_cast<milliseconds>(dtp);
	return (DWORD)dmsec.count();
}



/*	PLAI::SintTimeMan
 *
 *	Returns whether we should interrupt search or not. Type of interrupt is
 *	returned. InitTimeMan sets up the variables we need in order to detect it's
 *	time to stop this search.
 * 
 *	Returns sintNull if there is no interrupt.
 */
SINT PLAI::SintTimeMan(void) const noexcept
{
	/* if someone flagged, stop the search */

	if (!ga.bdg.FGsPlaying())
		return sintCanceled;

	/* if we're doing constant depth search, we do no time management; we also must
	   have a possible best move before we can interrupt */

	if (ttm == ttmConstDepth || ttm == ttmInfinite || mveBestOverall.fIsNil())
		return sintNull;
	
	/* the deadline is just a suggestion - we'll actually abort the search if we go
	   50% beyond it */

	DWORD dmsec = DmsecMoveSoFar();
	if (dmsec > dmsecDeadline + dmsecDeadline / 2)
		return sintTimedOut;

	/* if we're within a half-second of flagging, bail out right away */

	if (dmsecFlag != -1 && dmsec > dmsecFlag - msecHalfSec)
		return sintTimedOut;
	return sintNull;
}


/*	PLAI::PumpMsg
 *
 *	Temporary message pump to keep the UI working while the AI is thinking.
 *	Eventually we'll modify the search to run in a dedicated thread and we won't
 *	need this any more.
 * 
 *	If the user aborts the AI by hitting Esc, it sets a flag in the player to 
 *	cancel the AI. The flag is checked in the pruning code, which causes a 
 *	cascade of pruning and we then abort at the top level of the search.
 */
void PLAI::PumpMsg(bool fForce) noexcept
{
	if (fForce)
		cYield = -1;
	if (++cYield % dcYield)
		return;
	try {
		ga.puiga->PumpMsg();
		sint = SintTimeMan();
	}
	catch (...) {
		sint = sintCanceled;
	}
}


/*	PLAI::EvTempo
 *
 *	The value of a tempo, used in board static evaluation.
 */
EV PLAI::EvTempo(const BDG& bdg) const noexcept
{
	if (FInOpening(bdg.GphCur()))
		return evTempo;
	if (bdg.GphCur() < gphMidMid)
		return evTempo/2;
	return 0;
}


/*	PLAI::EvBdgAttackDefend
 *
 *	Little heuristic for board evaluation that tries to detect bad moves, which are
 *	if we have moved to an attacked square that isn't defended. This is only useful
 *	for pre-sorting, because it's somewhat more accurate than not doing it at all, 
 *	but it's not nearly as good as full quiescent search.
 * 
 *	Destination square of the previous move is in sqTo. Returns the amount to adjust 
 *	the evaluation by.
 */
EV PLAI::EvBdgAttackDefend(BDG& bdg, MVE mvePrev) const noexcept
{
	APC apcMove = bdg.ApcFromSq(mvePrev.sqTo());
	APC apcAttacker = bdg.ApcSqAttacked(mvePrev.sqTo(), bdg.cpcToMove);
	if (apcAttacker != apcNull) {
		if (apcAttacker < apcMove)
			return EvBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(mvePrev.sqTo(), ~bdg.cpcToMove);
		if (apcDefended == apcNull)
			return EvBaseApc(apcMove);
	}
	return 0;
}


/*	PLAI::ScoreCapture
 *
 *	Scores capture moves on the board. The move has NOT been made on the board
 *	yet. This is only superficially scaled to approximately the same range as
 *	an EV evaluation, and should only be used to compare against other scores
 *	returned by EvCaptureScore. Do no compare it to EvBdgStatic, or ScoreMove.
 */
EV PLAI::ScoreCapture(BDG& bdg, MVE mve) noexcept
{
	static const EV mpapcev[apcMax] = { 0, 100, 275, 300, 500, 900, 1000 };
	if (!mve.apcPromote()) {
		APC apcFrom = bdg.ApcFromSq(mve.sqFrom());
		EV ev = mpapcev[bdg.ApcFromSq(mve.sqTo())];
		APC apcDefender = bdg.ApcSqAttacked(mve.sqTo(), ~bdg.cpcToMove);
		if (apcDefender && apcDefender < apcFrom)
			ev -= mpapcev[apcFrom];
		return ev;
	}
		//return mpapcev[bdg.ApcFromSq(mve.sqTo())] - mpapcev[bdg.ApcFromSq(mve.sqFrom())] */;
		//return bdg.ApcFromSq(mve.sqTo()) * evPawn + (apcKing - bdg.ApcFromSq(mve.sqFrom()));
	else
		return mpapcev[mve.apcPromote()] - evPawn;
}


/*	PLAI::ScoreMove
 *
 *	Scores the board in an efficient way, for alpha-beta pre-sorting. While this
 *	number is currently scaled to be comparable to static evaluations returned 
 *	by EvBdgStatic, that is just a coincidence. 
 */
EV PLAI::ScoreMove(BDG& bdg, MVE mvePrev) noexcept
{
	EV evPsqt = 0, evTempo = 0;

	if (fecoPsqt)
		evPsqt= EvFromPst(bdg) + EvBdgAttackDefend(bdg, mvePrev);
	if (fecoTempo)
		evTempo = EvTempo(bdg);

	EV ev = (fecoPsqt * evPsqt+
			 fecoTempo * evTempo +
			 fecoScale/2) / fecoScale;

	return ev;
}


/*	PLAI::FScoreKiller
 *
 *	Scores killer moves, which are moves we saved when they caused a beta
 *	cut-off. Returns true if he move is a killer, and the evaluation is stored
 *	in the mve. While we go to some effort to scale these evaluations to 
 *	approximate a normal EV, it is not directly comparable, and should only
 *	be compared to other killer move scores.
 */
bool PLAI::FScoreKiller(BDG& bdg, MVE& mve) noexcept
{
	int imveLim = bdg.imveCurLast + 1;
	for (int imv = 0; imv < cmvKillers; imv++) {
		if (amvKillers[imveLim][imv] == mve) {
			mve.ev = evPawn - 10 * imv;
			return true;
		}
	}
	return false;
}


bool PLAI::FScoreHistory(BDG& bdg, MVE& mve) noexcept
{
	if (mppcsqcHistory[bdg.PcFromSq(mve.sqFrom())][mve.sqTo()] == 0)
		return false;
	mve.ev = mppcsqcHistory[bdg.PcFromSq(mve.sqFrom())][mve.sqTo()];
	return true;
}


/*	PLAI::EvBdgStatic
 *
 *	Evaluates the board from the point of view of the color with the 
 *	move. Previous move is in mvevPrev, which should be pre-populated  
 */
EV PLAI::EvBdgStatic(BDG& bdg, MVE mvePrev) noexcept
{
	EV evPsqt = 0, evMaterial = 0, evMobility = 0, evKingSafety = 0, evPawnStructure = 0;
	EV evTempo = 0, evRandom = 0;

	if (fecoPsqt)
		evPsqt = EvFromPst(bdg);
	if (fecoMaterial)
		evMaterial = EvMaterial(bdg, bdg.cpcToMove) - EvMaterial(bdg, ~bdg.cpcToMove);
	if (fecoRandom) {
		/* if we want randomness in the board eval, we need it to be stable randomness,
		   so we use the Zobrist hash of the board and xor it with a random number we
		   generate at the start of search and use that to generate our random number
		   within the range */
		uint64_t habdT = (bdg.habd ^ habdRand);
		evRandom = (EV)(habdT % ((HABD)fecoRandom * 2 + 1)) - fecoRandom; /* range is +/- fecoRandom */
	}
	if (fecoTempo)
		evTempo = EvTempo(bdg);

	EV evPawnToMove, evPawnDef;
	EV evKingToMove, evKingDef;
	int cmvSelf, cmvPrev;
	if (fecoMobility) {
		cmvSelf = bdg.CmvPseudo(~bdg.cpcToMove);
		cmvPrev = bdg.CmvPseudo(bdg.cpcToMove);
		/* this should create a number more-or-less between 0 and +/-100, and 
		   measures mobility as a fraction of total mobility, and should be
		   independent of the amount of material on the board */
		evMobility = (100 * cmvPrev) / (3 * EvMaterial(bdg, bdg.cpcToMove)) - 
					 (100 * cmvSelf) / (3 * EvMaterial(bdg, ~bdg.cpcToMove));
	}
	if (fecoKingSafety) {
		evKingToMove = EvBdgKingSafety(bdg, bdg.cpcToMove);
		evKingDef = EvBdgKingSafety(bdg, ~bdg.cpcToMove);
		evKingSafety = evKingToMove - evKingDef;
	}
	if (fecoPawnStructure) {
		evPawnToMove = EvBdgPawnStructure(bdg, bdg.cpcToMove);
		evPawnDef = EvBdgPawnStructure(bdg, ~bdg.cpcToMove);
		evPawnStructure = evPawnToMove - evPawnDef;
	}
	
	EV ev = (fecoPsqt * evPsqt +
			 fecoMaterial * evMaterial +
			 fecoMobility * evMobility +
			 fecoKingSafety * evKingSafety +
			 fecoPawnStructure * evPawnStructure +
			 fecoTempo * evTempo +
			 evRandom +
			 fecoScale/2) / fecoScale;

#ifdef EVALSTATS
	LogData(bdg.cpcToMove == cpcWhite ? L"White" : L"Black");
	if (fecoPsqt)
		LogData(wjoin(L"PST", SzFromEv(evPsqt)));
	if (fecoMobility)
		LogData(wjoin(L"Mobility", cmvPrev, L"-", cmvSelf));
	if (fecoKingSafety)
		LogData(wjoin(L"King Safety", evKingToMove, L"-", evKingDef));
	if (fecoPawnStructure)
		LogData(wjoin(L"Pawn Structure", evPawnToMove, L"-", evPawnDef));
	if (fecoTempo)
		LogData(wjoin(L"Tempo", SzFromEv(evTempo)));
	if (fecoRandom)
		LogData(wjoin(L"Random", SzFromEv(evRandom / fecoScale)));
	LogData(wjoin(L"Total", SzFromEv(ev)));
#else
	LogData(wjoin(bdg.cpcToMove, L"eval:", SzFromEv(ev)));
#endif

	return ev;
}


/*	PLAI::EvInterpolate
 *
 *	Interpolate the piece value evaluation based on game phase
 */
EV PLAI::EvInterpolate(GPH gph, EV ev1, GPH gphFirst, EV ev2, GPH gphLim) const noexcept
{
	return WInterpolate((int)gph, (int)gphFirst, (int)gphLim, ev1, ev2);
}


/*	PLAI::ComputeMpcpev1
 *
 *	Scans the board looking for pieces, looks them up in the piece/square valuation table,
 *	and keeps a running sum of the square/piece values for each side. Stores the result
 *	in mpcpcev.
 */
void PLAI::ComputeMpcpcev1(const BDG& bdg, EV mpcpcev[], const EV mpapcsqev[apcMax][sqMax]) const noexcept
{
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(cpcWhite, apc)]; bb; bb.ClearLow())
			mpcpcev[cpcWhite] += mpapcsqev[apc][bb.sqLow().sqFlip()];
		for (BB bb = bdg.mppcbb[PC(cpcBlack, apc)]; bb; bb.ClearLow())
			mpcpcev[cpcBlack] += mpapcsqev[apc][bb.sqLow()];
	}
}


void PLAI::ComputeMpcpcev2(const BDG& bdg, EV mpcpcev1[], EV mpcpcev2[], 
						const EV mpapcsqev1[apcMax][sqMax], 
						const EV mpapcsqev2[apcMax][sqMax]) const noexcept
{
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(cpcWhite, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow().sqFlip();
			mpcpcev1[cpcWhite] += mpapcsqev1[apc][sq];
			mpcpcev2[cpcWhite] += mpapcsqev2[apc][sq];
		}
		for (BB bb = bdg.mppcbb[PC(cpcBlack, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow();
			mpcpcev1[cpcBlack] += mpapcsqev1[apc][sq];
			mpcpcev2[cpcBlack] += mpapcsqev2[apc][sq];
		}
	}
}


/*	PLAI:EvFromPst
 *
 *	Returns the piece value table board evaluation for the side with 
 *	the move. 
 */
EV PLAI::EvFromPst(const BDG& bdg) const noexcept
{
	EV mpcpcev1[2] = { 0, 0 };
	EV mpcpcev2[2] = { 0, 0 };

	/* opening */

	if (bdg.FInOpening()) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevOpening);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	/* end game */

	if (bdg.FInEndGame()) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevEndGame);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	/* middle game, which ramps from opening to mid-mid, then ramps from mid-mid
	   to end game */

	GPH gph = min(bdg.GphCur(), gphMax);	// can exceed Max with promotions
	if (gph <= gphMidMid) {
		ComputeMpcpcev2(bdg, mpcpcev1, mpcpcev2, mpapcsqevOpening, mpapcsqevMiddleGame);
		return EvInterpolate(gph, mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], gphMidMin,
							 mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], gphMidMid);
	}
	else {
		ComputeMpcpcev2(bdg, mpcpcev1, mpcpcev2, mpapcsqevMiddleGame, mpapcsqevEndGame);
		return EvInterpolate(gph, mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], gphMidMid,
								mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], gphMidMax);
	}
}


/*	PLAI::EvFromGphApcSq
 *
 *	Gets the square evaluation of square sq in game phase gph if piece apc is in the
 *	square. Uninterpolated. This should be used for in-game eval, it's only used for
 *	debugging purpose to show the piece value table heat maps.
 */
EV PLAI::EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept
{
	if (FInEndGame(gph))
		return mpapcsqevEndGame[apc][sq];
	if (FInOpening(gph))
		return mpapcsqevOpening[apc][sq];
	return mpapcsqevMiddleGame[apc][sq];
}


/*	PLAI::InitWeightTables
 *
 *	Initializes the piece value weight tables for the different phases of the game.
 */
void PLAI::InitWeightTables(void)
{
	InitWeightTable(mpapcevOpening, mpapcsqdevOpening, mpapcsqevOpening);
	InitWeightTable(mpapcevMiddleGame, mpapcsqdevMiddleGame, mpapcsqevMiddleGame);
	InitWeightTable(mpapcevEndGame, mpapcsqdevEndGame, mpapcsqevEndGame);
}


void PLAI::InitWeightTable(const EV mpapcev[apcMax], const EV mpapcsqdev[apcMax][sqMax], EV mpapcsqev[apcMax][sqMax])
{
	memset(&mpapcsqev[apcNull], 0, sqMax * sizeof(EV));
	for (APC apc = apcPawn; apc < apcMax; ++apc)
		for (uint64_t sq = 0; sq < sqMax; sq++)
			mpapcsqev[apc][sq] = mpapcev[apc] + mpapcsqdev[apc][sq];
}


EV PLAI::EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept
{
	return 0;
}


/*	PLAI::CfileDoubledPawns
 *
 *	Returns the number of doubled pawns the side cpc has, i.e.,
 *	the number of files that contain multiple pawns.
 */
int PLAI::CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept
{
	int cfile = 0;
	BB bbPawn = bdg.mppcbb[PC(cpc, apcPawn)];
	BB bbFile = bbFileA;
	for (int file = 0; file < fileMax; file++, bbFile = BbEast1(bbFile)) {
		int csq = (bbPawn & bbFile).csq();
		if (csq)
			cfile += csq - 1;
	}
	return cfile;
}


/*	PLAI::CfileIsoPawns
 *
 *	Returns the number of isolated pawns the side cpc has; i.e., 
 *	the number of files that have pawns when neither adjacent file
 *	have pawns.
 */
int PLAI::CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept
{
	int cfile = 0;
	BB bbPawn = bdg.mppcbb[PC(cpc, apcPawn)];
	BB bbFile = bbFileA;
	for (int file = 0; file < fileMax; file++, bbFile = BbEast1(bbFile))
		cfile += (bbPawn & bbFile) && !(bbPawn & (BbEast1(bbFile) | BbWest1(bbFile)));
	return cfile;
}


/*	PLAI::CfilePassedPawns
 *
 *	Counts the number of files that have passed pawns on them.
 */
int PLAI::CfilePassedPawns(BDG& bdg, CPC cpc) const noexcept
{
	int cfile = 0;
	DIR dir = cpc == cpcWhite ? dirNorth : dirSouth;
	BB bbPawn = bdg.mppcbb[PC(cpc, apcPawn)];
	BB bbPawnOpp = bdg.mppcbb[PC(~cpc, apcPawn)];
	for (BB bb = bbPawn; bb; bb.ClearLow()) {
		SQ sqPawn = bb.sqLow();
		/* not blocked by our own pawn, and no opponent pawns in the alley */
		if (!(mpbb.BbSlideTo(sqPawn, dir) & bbPawn) && !(mpbb.BbPassedPawnAlley(sqPawn, cpc) & bbPawnOpp))
			cfile++;
	}
	return cfile;
}


/*	PLAI::CsqWeak
 *
 *	A weak square is one that can never be attacked/defended by a pawn.
 */
int PLAI::CsqWeak(BDG& bdg, CPC cpc) const noexcept
{
	DIR dir = cpc == cpcWhite ? dirNorth : dirSouth;
	BB bbNotWeak(0);
	for (BB bbPawns = bdg.mppcbb[PC(cpc, apcPawn)]; bbPawns; bbPawns.ClearLow()) {
		SQ sqPawn = bbPawns.sqLow();
		bbNotWeak |= mpbb.BbPassedPawnAlley(sqPawn, cpc) - mpbb.BbSlideTo(sqPawn, dir);
	}
	/* only count weak squares in the middle ranks */
	bbNotWeak &= bbRank3 | bbRank4 | bbRank5 | bbRank6;
	return (~bbNotWeak).csq() - 32;
}


/*	PLAI::EvBdgPawnStructure
 *
 *	Returns the pawn structure evaluation of the board position from the
 *	point of view of cpc.
 */
EV PLAI::EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept
{
	EV ev = 0;

	ev -= CfileDoubledPawns(bdg, cpc);
	ev -= CfileIsoPawns(bdg, cpc);
	ev += 5 * CfilePassedPawns(bdg, cpc);

	/* overextended pawns */

	/* backwards pawns */

	/* connected pawns */

	/* open files */

#ifdef WEAK_SQUARES
	/* weak squares - this doesn't work very well because it scales too large when
	   you use just a count of squares; need to weigh the squares by how weak they 
	   are */

	if (bdg.gph < gphMidMax) {
		int csq = CsqWeak(bdg, cpc);
		if (bdg.gph > gphMidMid)
			csq /= 2;
		ev -= csq;
	}
#endif

	return ev;
}


/*
 *
 *	PLAI2
 * 
 *	An AI with full alpha-beta search capabilities, but a stupid eval function. 
 *	Used as a foil for the smart eval AI.
 * 
 */


PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	fecoPsqt = 1 * fecoScale;	
	fecoMaterial = 0 * fecoScale;	// should never use material and psqt together 
	fecoMobility = 0 * fecoScale;	
	fecoKingSafety = 0 * fecoScale;	
	fecoPawnStructure = 0 * fecoScale;	
	fecoTempo = 1 * fecoScale;
	fecoRandom = 0 * fecoScale;	
	SetName(L"AI2");
	InitWeightTables();
}


void PLAI2::InitWeightTables(void)
{
	InitWeightTable(mpapcevOpening2, mpapcsqdevOpening2, mpapcsqevOpening);
	InitWeightTable(mpapcevMiddleGame2, mpapcsqdevMiddleGame2, mpapcsqevMiddleGame);
	InitWeightTable(mpapcevEndGame2, mpapcsqdevEndGame2, mpapcsqevEndGame);
}


/*
 *
 *	PLHUMAN class
 * 
 *	Human player, which drives the UI for getting moves.
 * 
 */


PLHUMAN::PLHUMAN(GA& ga, wstring szName) : PL(ga, szName)
{
}


MVE PLHUMAN::MveGetNext(SPMV& spmv) noexcept
{
	mveNext = mveNil;
	do {
		try {
			ga.puiga->PumpMsg();
		}
		catch (...) {
			break;
		}
	} while (mveNext.fIsNil() && ga.bdg.FGsPlaying());
	
	MVE mve = mveNext;
	spmv = spmvNext;
	mveNext = mveNil;
	
	return mve;
}


/*
 *
 *	AINFOPL
 * 
 *	The list of players we have available to play. This is used as a player
 *	picker, and includes a factory for creating players.
 * 
 */


AINFOPL::AINFOPL(void) 
{
	vinfopl.push_back(INFOPL(clplAI, tplAI, L"AI", IfDebugElse(ttmConstDepth, ttmSmart), 5));
	vinfopl.push_back(INFOPL(clplAI2, tplAI, L"AI2", IfDebugElse(ttmConstDepth, ttmSmart), 3));
	vinfopl.push_back(INFOPL(clplAI, tplAI, L"AI (Infinite)", ttmInfinite, 10));
	vinfopl.push_back(INFOPL(clplAI, tplAI, L"AI (Depth)", ttmConstDepth, 5));
	vinfopl.push_back(INFOPL(clplHuman, tplHuman, L"Rick Powell"));
	vinfopl.push_back(INFOPL(clplHuman, tplHuman, L"Hazel"));
}


/*	AINFOPL::PplFactory
 *
 *	Creates a player for the given game using the given player index. The
 *	player is not added to the game yet. 
 */
PL* AINFOPL::PplFactory(GA& ga, int iinfopl) const
{
	PL* ppl = nullptr;
	switch (vinfopl[iinfopl].clpl) {
	case clplAI:
		ppl = new PLAI(ga);
		break;
	case clplAI2:
		ppl = new PLAI2(ga);
		break;
	case clplHuman:
		ppl = new PLHUMAN(ga, vinfopl[iinfopl].szName);
		break;
	default:
		assert(false);
		return nullptr;
	}
	ppl->SetLevel(vinfopl[iinfopl].level);
	ppl->SetTtm(vinfopl[iinfopl].ttm);
	return ppl;
}


/*	AINFOPL::IdbFromInfopl
 *
 *	Returns an icon resource ID to use to help identify the players.
 */
int AINFOPL::IdbFromInfopl(const INFOPL& infopl) const
{
	switch (infopl.tpl) {
	case tplAI:
		return idbAiLogo;
	default:
		break;
	}
	return idbHumanLogo;
}


wstring to_wstring(TTM ttm)
{
	switch (ttm) {
	case ttmConstDepth:
		return L"Constant Depth";
	case ttmTimePerMove:
		return L"Constant Time";
	case ttmSmart:
		return L"Smart";
	case ttmInfinite:
		return L"Infinite";
	default:
		return L"<nil>";
	}
}

wstring to_wstring(CLPL clpl)
{
	switch (clpl) {
	case clplAI:
		return L"AI";
	case clplAI2:
		return L"AI2";
	case clplHuman:
		return L"Human";
	default:
		return L"<nil>";
	}
}