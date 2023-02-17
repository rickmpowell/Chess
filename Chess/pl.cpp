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
const uint16_t dcYield = 512;
#endif


/*
 *
 *	PL base class
 * 
 *	The base class for all chess game players. 
 * 
 */


PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName), level(3),
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
	wchar_t sz[20], *pch = sz;
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
	int depth;

	inline AIBREAK(PL& pl, const MVE& mve, int depth) : pl(pl), depth(depth)
	{
		if (pl.pvmvBreak == nullptr || pl.imvBreakLast < depth - 2)
			return;

		if ((MV)mve != (*pl.pvmvBreak)[depth - 1])
			return;

		pl.imvBreakLast++;
		if (pl.imvBreakLast == pl.pvmvBreak->size()-1)
			DebugBreak();
	}

	inline ~AIBREAK(void)
	{
		if (pl.pvmvBreak == nullptr || pl.imvBreakLast < depth - 1)
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


PLAI::PLAI(GA& ga) : PL(ga, L"SQ Mobly"), rgen(372716661UL), habdRand(0), 
		ttm(IfReleaseElse(ttmSmart, ttmConstDepth)),
		//ttm(ttmSmart),
#ifndef NOSTATS
		cmveTotalEval(0), 
#endif
		cYield(0)
{
	fecoMaterial = 1*fecoScale;
	fecoMobility = 3*fecoScale;	/* this is small because Material already includes
								   some mobility in the piece-square tables */
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


void PLAI::StartGame(void)
{
	xt.Init();
}


/*	PLAI::StartMoveLog
 *
 *	Opens a log entry for AI move generation. Must be paired with a corresponding
 *	EndMoveLog
 */
void PLAI::StartMoveLog(void)
{
#ifndef NOSTATS
	tpStart = high_resolution_clock::now();
	cmveTotalEval = 0L;
#endif
	stbfTotal.Init();
	LogOpen(TAG(wjoin(to_wstring(ga.bdg.vmveGame.size()/2+1) + L".", SzFromCpc(ga.bdg.cpcToMove)),  
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
	/* eval stats */
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	LogData(wjoin(L"Evaluated", SzCommaFromLong(cmveTotalEval), L"boards"));
	LogData(wjoin(L"Nodes:", SzCommaFromLong(stbfTotal.cmveNode)));
	/* cache stats */
	LogData(wjoin(L"Cache Fill:", SzPercent(xt.cxevInUse, xt.cxevMax)));
	LogData(wjoin(L"Cache Probe Hit:", SzPercent(xt.cxevProbeHit, xt.cxevProbe)));
	LogData(wjoin(L"Cache Save Replace:", SzPercent(xt.cxevSaveReplace, xt.cxevSave)));
	LogData(wjoin(L"Cache Save Collision:", SzPercent(xt.cxevSaveCollision, xt.cxevSave)));

	/* time stats */
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(wjoin(L"Time:", SzCommaFromLong(ms.count()), L"ms"));
	LogData(wjoin(L"Speed:", to_wstring((int)round((float)stbfTotal.cmveNode / (float)ms.count())), L"kn/sec"));
	LogData(wjoin(L"Branch factor:", (wstring)stbfTotal));
#endif
	LogClose(L"", L"", lgfNormal);
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
	const BDG& bdg;
	wstring szData;
	PRR prr;

public:
	inline LOGSEARCH(PLAI& pl, const BDG& bdg) noexcept : pl(pl), bdg(bdg), prr(prrNone) { }
	inline void SetData(const wstring& szNew) noexcept { szData = szNew; }
	inline void SetReason(PRR prr) { this->prr = prr; }
};

class LOGMVE : public LOGSEARCH, public AIBREAK
{
	const MVE& mvePrev;
	int depthLogSav;
	int imvExpandSav;
	const MVE& mveBest;
	const AB& abInit;

	static int imvExpand;
	static MV amv[20];

public:
	inline LOGMVE(PLAI& pl, const BDG& bdg,
				  const MVE& mvePrev, const MVE& mveBest, const AB& abInit, int depth, wchar_t chType = L' ') noexcept :
		LOGSEARCH(pl, bdg), AIBREAK(pl, mvePrev, depth),
		mvePrev(mvePrev), mveBest(mveBest), abInit(abInit), depthLogSav(0), imvExpandSav(0)
	{
		depthLogSav = DepthShow();
		imvExpandSav = imvExpand;
		if (FExpandLog(mvePrev))
			SetDepthShow(depthLogSav + 1);
		int depthLog;
		if (!papp->FDepthLog(lgtOpen, depthLog))
			pl.PumpMsg(false);
		else {
			pl.PumpMsg(true);
			papp->AddLog(lgtOpen, lgfNormal, depthLog,
						 TAG(bdg.SzDecodeMvPost(mvePrev), ATTR(L"FEN", bdg)),
						 wjoin(wstring(1, chType) + to_wstring(depth),
							   SzFromTsc(mvePrev.tsc()),
							   SzFromEv(-mvePrev.ev), abInit));

		}
	}

	inline ~LOGMVE() noexcept
	{
		LogClose(bdg.SzDecodeMvPost(mvePrev), wjoin(SzFromEv(mveBest.ev), SzEvt()), LgfEvt());
		SetDepthShow(depthLogSav);
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
		if (mveBest.ev < abInit)
			return L"Low";
		else if (mveBest.ev > abInit)
			return L"Cut";
		else
			return L"PV";
	}

	inline LGF LgfEvt(void) const noexcept
	{
		if (mveBest.ev < abInit)
			return lgfNormal;
		else if (mveBest.ev > abInit)
			return lgfItalic;
		else
			return lgfBold;
	}
};

class LOGMVES : public LOGMVE
{
public:
	inline LOGMVES(PLAI& pl, const BDG& bdg,
				   const MVE& mvePrev, const MVE& mveBest, const AB& abInit, int depth) noexcept :
		LOGMVE(pl, bdg, mvePrev, mveBest, abInit, depth, L' ')
	{
	}
};


class LOGMVEQ : public LOGMVE
{
public:
	inline LOGMVEQ(PLAI& pl, const BDG& bdg,
				   const MVE& mvePrev, const MVE& mveBest, const AB& abInit, int depth) noexcept :
		LOGMVE(pl, bdg, mvePrev, mveBest, abInit, depth, L'Q')
	{
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
	LOGITD(PLAI& pl, const BDG& bdg, const MVE& mveBest, AB ab, int depth) noexcept : 
			LOGSEARCH(pl, bdg), mveBest(mveBest)
	{
		LogOpen(L"Depth", wjoin(depth, ab), lgfNormal);
	}

	~LOGITD() noexcept
	{
		LogClose(L"Depth", wjoin(L"[BF=" + (wstring)pl.stbfMain + L"]",
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


VMVES::VMVES(BDG& bdg, PLAI* pplai, GG gg) noexcept : VMVE(), pplai(pplai), pmveNext(begin())
{
	bdg.GenVmve(*this, gg);
	Reset(bdg);
}


void VMVES::Reset(BDG& bdg) noexcept
{
	pmveNext = begin();
	cmvLegal = 0;
}


/*	VSMVE::FMakeMveNext
 *
 *	Finds the next move in the move list, returning false if there is no such
 *	move. The move is returned in pmve. The move is actually made on the board
 *	and illegal moves are checked for
 */
bool VMVES::FMakeMveNext(BDG& bdg, MVE*& pmve) noexcept
{
	while (pmveNext < end()) {
		pmve = &*pmveNext++;
		bdg.MakeMv(*pmve);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			return true;
		}
		bdg.UndoMv();
		/* TODO: should we remove the in-check move from the list? */
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
	if (size() != 1)
		return false;
	mve = (*this)[0];
	return true;
}


/*
 *
 *	VMVESS class
 *
 *	Alpha-beta sorted movelist enumerator. These moves are pre-evaluated in order
 *	to improve alpha-beta pruning behavior. This is our most used enumerator.
 * 
 */


VMVESS::VMVESS(BDG& bdg, PLAI* pplai, GG gg) noexcept : VMVES(bdg, pplai, gg), tscCur(tscPrincipalVar)
{
	Reset(bdg);
}


/*	VMVESS::Reset
 *
 *	Resets the iteration of the sorted move list, preparing us for a full 
 *	enumeration of the list.
 */
void VMVESS::Reset(BDG& bdg) noexcept
{
	VMVES::Reset(bdg);
	tscCur = tscPrincipalVar;
	PrepTscCur(bdg, begin());
}


/*	VMVESS::FMakeMveNext
 *
 *	Gets the next legal move, sorted by evaluation. Return false if we're done. 
 *	Keeps track of count of legal moves we've enumerated. The move will be made 
 *	on the board when it is returned.
 * 
 *	We do not actually pre-sort the array, but do a quick scan to find the best
 *	move and swap it into position as we go. This is a good optimization if
 *	we get an early prune because we avoid a full O(n*log(n)) sort and only pay
 *	for the O(n) scan. The worst case this degenerates into O(n^2), but n is 
 *	relatively small, so maybe not a big deal.
 * 
 *	We lazy evaluate any moves that aren't found in the transposition table. 
 */
bool VMVESS::FMakeMveNext(BDG& bdg, MVE*& pmve) noexcept
{
	while (pmveNext < end()) {

		/* swap the best move into the next mve to return */

		MVE* pmveBest;
		for ( ; (pmveBest = PmveBestFromTscCur(pmveNext)) == nullptr; 
				tscCur++, PrepTscCur(bdg, pmveNext))
			;
		pmve = &*pmveNext;
		swap(*pmveNext, *pmveBest);
		pmveNext++;

		/* make sure move is legal */

		bdg.MakeMv(*pmve);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			return true;
		}
		bdg.UndoMv();
	}
	return false;
}


/*	VMVESS::PmveBestFromTscCur
 *
 *	Finds the best move from the movelist that matches the current score type
 *	that we're enumerating. Returns nullptr if no matches.
 */
MVE* VMVESS::PmveBestFromTscCur(VMVE::it pmveFirst) noexcept
{
	MVE* pmveBest = nullptr; 
	for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
		if (pmve->tsc() == tscCur &&
				(pmveBest == nullptr || pmve->ev > pmveBest->ev))
			pmveBest = &(*pmve);
	}
	return pmveBest;
}


/*	VMVESS::PrepTscCur
 *
 *	Prepares the enumeration for the next score type. We try to lazy evaluate as 
 *	many moves as possible so we don't waste time evaluating positions that might
 *	never be visited because of pruning. 
 * 
 *	Currently we break moves into ... (1) the principal variation, (2) any other 
 *	move in the transposition table that was fully evaluated, (3) captures, and
 *	and (4) everything else.
 */
void VMVESS::PrepTscCur(BDG& bdg, VMVE::it pmveFirst) noexcept
{
	switch (tscCur) {
	case tscPrincipalVar:
	{
		/* first time through the enumeration, snag the principal variation. This can
		   be done very quickly with just a transposition table probe. While we're at
		   it, go ahead and reset all the other moves to nil so we re-score them on
		   subsequent passes */
		XEV* pxev = pplai->xt.Find(bdg, 0);
		MVE mvePV = pxev && pxev->tev() == tevEqual ? bdg.MveFromMv(pxev->mv()) : mveNil;
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			assert(!pmve->fIsNil());
			pmve->SetTsc(tscNil);
			if (mvePV == *pmve) {
				assert(pxev != nullptr);
				pmve->SetTsc(tscPrincipalVar);
				pmve->ev = -pxev->ev();
			}
		}
		break;
	}

	case tscXTable:
	{
		/* get the eval of any move that has an entry in the tranposition table */
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			assert(pmve->tsc() == tscNil);	// should all be marked nil by tscPrincipalVar pass
			bdg.MakeMvSq(*pmve);
			XEV* pxev = pplai->xt.Find(bdg, 0);
			if (pxev && pxev->tev() == tevEqual) {
				pmve->SetTsc(tscXTable);
				pmve->ev = -pxev->ev();
			}
			bdg.UndoMvSq(*pmve);
		}
		break;
	}

	case tscEvCapture:
	case tscEvOther:
	{
		/* the remainder of the moves need to compute an actual board score */
		for (VMVE::it pmve = pmveFirst; pmve < end(); pmve++) {
			assert(pmve->tsc() == tscNil);
			if (tscCur == tscEvCapture && !pmve->fIsCapture())
				continue;
			bdg.MakeMvSq(*pmve);
			pmve->SetTsc(tscCur);
			pmve->ev = -pplai->EvBdgScore(bdg, *pmve);
			bdg.UndoMvSq(*pmve);
		}
		break;
	}

	default:
		assert(false);
		break;
	}
}


/*
 *
 *	VMVESQ
 *
 *	Quiescent move list enumerator
 *
 */


VMVESQ::VMVESQ(BDG& bdg, PLAI* pplai, GG gg) noexcept : VMVES(bdg, pplai, gg)
{
}


/*	VMVESQ::FMakeMveNext
 *
 *	Finds the next noisy move in the quiescent move list and makes it on the
 *	board.
 */
bool VMVESQ::FMakeMveNext(BDG& bdg, MVE*& pmve) noexcept
{
	while (VMVES::FMakeMveNext(bdg, pmve)) {
		if (!bdg.FMvIsQuiescent(*pmve))
			return true;
		bdg.UndoMv();
	}
	return false;
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

	mveBestOverall = MVE(mvuNil, -evInf);
	VMVESS vmvess(bdg, this, ggLegal);
	stbfTotal.Init();

	InitWeightTables();
	InitTimeMan(bdg);

	/* main iterative deepening and aspiration window loop */

	MVE mveBest;
	AB ab(-evInf, evInf);
	int depthLim = 2;
	do {
		mveBest = MVE(mvuNil, -evInf);
		stbfMain.Init(); stbfQuiescent.Init();
		LOGITD logitd(*this, bdg, mveBest, ab, depthLim);
		vmvess.Reset(bdg);
		stbfMain.AddGen(1);
		FSearchMveBest(bdg, vmvess, mveBest, ab, 0, depthLim, tsAll);
		SaveXt(bdg, mveBest, ab, depthLim);
		stbfTotal += stbfMain;
	} while (!vmvess.FOnlyOneMove(mveBestOverall) && FDeepen(mveBest, ab, depthLim) && 
			 FBeforeDeadline(depthLim));

	EndMoveLog();
	return mveBestOverall;
}


/*	PLAI::EvBdgSearch
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth depthLim; depth is current. Uses alpha-beta pruning.
 * 
 *	The board will be modified, but it will be restored to its original state in 
 *	all cases.
 * 
 *	On entry mvePrev has the last move made.
 * 
 *	Returns board evaluation in centi-pawns.
 */
EV PLAI::EvBdgSearch(BDG& bdg, const MVE& mvePrev, AB abInit, int depth, int depthLim, TS ts) noexcept
{
	stbfMain.AddNode(1);

	/* we're at depth, go to quiescence to get static eval */

	if (depth >= depthLim)
		return EvBdgQuiescent(bdg, mvePrev, abInit, depth, ts);

	MVE mveBest;
	LOGMVES logmve(*this, bdg, mvePrev, mveBest, abInit, depth);

	/* check transposition table, evaluate the board, then try futility pruning and 
	   null move heuristic pruning tricks */

	if (FLookupXt(bdg, mveBest, abInit, depthLim - depth))
		return mveBest.ev;	
	/*
	mveBest = MVE(mvNil, EvBdgStatic(bdg, mvePrev.mv, true));
	SaveXt(bdg, mveBest, AB(-evInf, evInf), 0);
	if (FTryFutility(bdg, mveBest, abInit, depth, depthLim, ts))
		return mveBest.ev;
	*/
	if (FTryNullMove(bdg, mveBest, abInit, depth, depthLim, ts))
		return mveBest.ev;

	/* if none of those optimizations work, do a full search */

	mveBest.ev = -evInf;
	VMVESS vmvess(bdg, this, ggPseudo);
	stbfMain.AddGen(1);
	if (!FSearchMveBest(bdg, vmvess, mveBest, abInit, depth, depthLim, ts))
		TestForMates(bdg, vmvess, mveBest, depth);
	SaveXt(bdg, mveBest, abInit, depthLim - depth);
	return mveBest.ev;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the "quiet" evaluation after the given board/last move from the point 
 *	of view of the player next to move; it only considers captures and other "noisy" 
 *	moves. Alpha-beta prunes within the abInit window.
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, const MVE& mvePrev, AB abInit, int depth, TS ts) noexcept
{
	stbfQuiescent.AddNode(1);

	int depthLim = depthMax;
	MVE mveBest;
	LOGMVEQ logmve(*this, bdg, mvePrev, mveBest, abInit, depth);

	if (FLookupXt(bdg, mveBest, abInit, 0))
		return mveBest.ev;

	/* first off, the player may refuse the capture, so get full, slow static eval 
	   and check if we're already in a pruning situation */

	AB ab = abInit;
	mveBest = MVE(mvuNil, EvBdgStatic(bdg, mvePrev));
//	SaveXt(bdg, mveBest, AB(-evInf, evInf), 0);
	if (FPrune(&mveBest, mveBest, ab, depthLim))
		return mveBest.ev;

	/* then recursively evaluate noisy moves */
		
	VMVESQ vmvesq(bdg, this, ggPseudo);
	stbfQuiescent.AddGen(1);
	for (MVE* pmve = nullptr; vmvesq.FMakeMveNext(bdg, pmve); ) {
		pmve->ev = -EvBdgQuiescent(bdg, *pmve, -ab, depth + 1, ts);
		vmvesq.UndoMv(bdg);
		if (FPrune(pmve, mveBest, ab, depthLim))
			goto Done;
	}
	TestForMates(bdg, vmvesq, mveBest, depth);

Done:
//	SaveXt(bdg, mveBest, abInit, 0);
	return mveBest.ev;
}


/*	PLAI::FSearchMveBest
 *
 *	Finds the best move for the given board and sorted movelist, using the given
 *	alpha-beta window and search depth. Returns true the search should be pruned.
 *
 *	Handles the principal value search optimization, which tries to bail out of
 *	non-PV searches quickly by doing a quick null-window search on the PV valuation.
 */
bool PLAI::FSearchMveBest(BDG& bdg, VMVESS& vmvess, MVE& mveBest, AB ab, int depth, int& depthLim, TS ts) noexcept
{
	/* do first move (probably a PV move) with full a-b window */
	
	MVE* pmve;
	if (!vmvess.FMakeMveNext(bdg, pmve))
		return false;
	pmve->ev = -EvBdgSearch(bdg, *pmve, -ab, depth + 1, depthLim, ts);
	vmvess.UndoMv(bdg);
	if (FPrune(pmve, mveBest, ab, depthLim))
		return true;

	/* subsequent moves get the PV pre-search optimization which uses an 
	   ultra-narrow window. We pray that narrow window quickly fails low; 
	   if we don't, redo the search with the full window */

	while (vmvess.FMakeMveNext(bdg, pmve)) {
		pmve->ev = -EvBdgSearch(bdg, *pmve, -ab.AbNull(), depth + 1, depthLim, ts);
		if (!(pmve->ev < ab))
			pmve->ev = -EvBdgSearch(bdg, *pmve, -ab, depth + 1, depthLim, ts);
		vmvess.UndoMv(bdg);
		if (FPrune(pmve, mveBest, ab, depthLim))
			return true;
	}
	return false;
}


/*	PLAI::FLookupXt
 *
 *	Checks the transposition table for a board entry at the given search depth. Returns 
 *	true if we should stop the search at this point, either because we found an exact 
 *	match of the board/depth, or the inexact match is outside the alpha/beta interval.
 *
 *	mveBest will contain the evaluation we should use if we stop the search.
 */
bool PLAI::FLookupXt(BDG& bdg, MVE& mveBest, AB ab, int depth) noexcept
{
	/* look for the entry in the transposition table */

	XEV* pxev = xt.Find(bdg, depth);
	if (pxev == nullptr)
		return false;
	
	/* adjust the alpha-beta interval based on the entry */

	switch (pxev->tev()) {
	case tevEqual:
		mveBest.ev = pxev->ev();
		break;
	case tevHigher:
		if (!(pxev->ev() > ab))
			return false;
		mveBest.ev = ab.evBeta;
		break;
	case tevLower:
		if (!(pxev->ev() < ab))
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
XEV* PLAI::SaveXt(BDG& bdg, MVE mveBest, AB ab, int depth) noexcept
{
	/* don't save cancels or timeouts */
	if (FEvIsInterrupt(mveBest.ev))
		return nullptr;

	if (mveBest.ev < ab)
		return xt.Save(bdg, mveBest, tevLower, depth);
	if (mveBest.ev > ab)
		return xt.Save(bdg, mveBest, tevHigher, depth);
	return xt.Save(bdg, mveBest, tevEqual, depth);
}


bool PLAI::FTryFutility(BDG& bdg, MVE& mveBest, AB ab, int depth, int depthLim, TS ts) noexcept
{
	if (depthLim - depth >= 8)	// only works on frontier nodes
		return false;
	if (mveBest.ev + EV(300 * (depthLim - depth)) > ab)
		return false;
	if (mveBest.ev <= evSureWin)
		return false;

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
bool PLAI::FTryNullMove(BDG& bdg, MVE& mveBest, AB ab, int depth, int depthLim, TS ts) noexcept
{
	int R = 2;
	if (depth + 1 >= depthLim - R ||		// don't bother if search is going this deep anyway
		bdg.gph >= gphMidMax ||				// lame zugzwang estimate
		(ts & tsNoNullMove) ||
		bdg.FInCheck(bdg.cpcToMove))
		return false;

	mveBest.SetMvu(mvuNil);
	bdg.MakeMvNull();
	mveBest.ev = -EvBdgSearch(bdg, mveBest, (-ab).AbNull(), depth + 1, depthLim - R, ts + tsNoNullMove);
	bdg.UndoMv();
	return mveBest.ev > ab;
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
bool PLAI::FPrune(MVE* pmve, MVE& mveBest, AB& ab, int& depthLim) const noexcept
{
	/* keep track of best move */

	if (pmve->ev > mveBest.ev)
		mveBest = *pmve;
	
	/* above beta, prune */

	if (pmve->ev > ab) {
		mveBest = *pmve;
		return true;
	}

	/* if inside the alpha-beta window. Narrow alpha down with what is now probably 
	   the best move's eval; if we're mating the other guy, we can adjust the search
	   depth since from now on, we're only looking for quicker mates */
	
	if (!(pmve->ev < ab)) {	
		ab.NarrowAlpha(pmve->ev);
		if (FEvIsMate(pmve->ev))	
			depthLim = DepthFromEvMate(pmve->ev);
	}

	/* If Esc is hit (set by message pump), or if we're taking too damn long to do
	   the search, force the search to prune all the way back to root, where we'll 
	   abort the search */

	if (sint != sintNull) {
		pmve->ev = mveBest.ev = (sint == sintCanceled) ? evCanceled : evTimedOut;
		mveBest.SetMvu(mvuNil);
		return true;
	}

	return false;
}


/*	PLAI::TestForMates
 *
 *	Helper function that adjusts evaluations in checkmates and stalemates. Modifies
 *	evBest to be the checkmate/stalemate if we're in that state. cmvLegal is the
 *	count of legal moves in the move list, and vmves is the move enumerator, which 
 *	kept track of how many legal moves there were.
 */
void PLAI::TestForMates(BDG& bdg, VMVES& vmves, MVE& mveBest, int depth) const noexcept
{
	/* no legal moves - checkmate or stalemate */

	if (vmves.cmvLegal == 0) {
		mveBest.ev = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(depth) : evDraw;
		mveBest.SetMvu(mvuNil);
	}

	/* GsTestGameOver checks for various non-stalemate draws; we cut off 3-repeated
	   position draws after just 2 repeated positions, since the idea is the same and
	   it avoids the AI making annoying oscillating move circles */
	
	else if (bdg.GsTestGameOver(vmves.cmvLegal, 2) != gsPlaying) { 
		mveBest.ev = evDraw;
		mveBest.SetMvu(mvuNil);
	}
}


/*	PLAI::FDeepen
 *
 *	Just a little helper to set us up for the next round of the iterative deepening
 *	loop. Our loop is set up to handle both deepening and aspiration windows, so
 *	sometimes we increase the depth, other times we widen the aspiration window.
 *
 *	Returns true if we successfully progressed the depth/window; returns false if we 
 *	should we stop the search, which happens on an abort/interrupt, or a forced mate.
 */
bool PLAI::FDeepen(MVE mveBest, AB& ab, int& depth) noexcept
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

	if (mveBest.ev < ab)
		ab.AdjMissLow();
	else if (mveBest.ev > ab)
		ab.AdjMissHigh();
	else {
		/* yay, we found a move - go deeper in the next pass, but use a tight
		   a-b window (the aspiration window optimization) at first in hopes we'll 
		   get lots of pruning */

		mveBestOverall = mveBest;
		if (FEvIsMate(mveBest.ev) || FEvIsMate(-mveBest.ev))
			return false;
		ab = ab.AbAspiration(mveBest.ev, 20);
		depth += 1;
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
	tpMoveFirst = high_resolution_clock::now();
	
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
	static EV mpapcev[apcMax] = { 0, 100, 300, 300, 500, 900, 100 };
	EV ev = 0;
	for (CPC cpc = cpcWhite; cpc < cpcMax; cpc++)
		for (APC apc = apcPawn; apc < apcMax; apc++)
			ev += mpapcev[apc] * bdg.mppcbb[PC(cpc, apc)].csq();
	return ev;
}


/*	PLAI::FBeforeDeadline
 *
 *	Lets us know when we should stop searching for moves. This is where our time management
 *	happens. Returns true if we should keep searching.
 * 
 *	Currently very unsophisticated. Doesn't take clock or timing rules into account.
 */
bool PLAI::FBeforeDeadline(int depthLim) noexcept
{
	if (mveBestOverall.fIsNil())
		return true;

	switch (ttm) {

	case ttmSmart:
	case ttmTimePerMove:
		return DmsecMoveSoFar() < dmsecDeadline;
	case ttmConstDepth:
		/* different levels get different depths */
		return depthLim <= level + 1;
	}

	return false;
}

long long PLAI::DmsecMoveSoFar(void) const noexcept
{
	time_point<high_resolution_clock> tp = high_resolution_clock::now();
	duration dtp = tp - tpMoveFirst;
	milliseconds dmsec = duration_cast<milliseconds>(dtp);
	return dmsec.count();
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

	if (ttm == ttmConstDepth || mveBestOverall.fIsNil())
		return sintNull;
	
	/* the deadline is just a suggestion - we'll actually abort the search if we go
	   50% beyond it */

	long long dmsec = DmsecMoveSoFar();
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
 *	for pre-sorting alpha-beta pruning, because it's somewhat more accurate than not
 *	doing it at all, but it's not good enough for full static board evaluation. 
 *	Someday, a better heuristic would be to exchange until quiescence.
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


/*	PLAI::EvBdgScore
 *
 *	Scores the board in an efficient way, for alpha-beta pre-sorting
 */
EV PLAI::EvBdgScore(BDG& bdg, MVE mvePrev) noexcept
{
	EV evMaterial = 0, evTempo = 0;

	if (fecoMaterial)
		evMaterial = EvFromPst(bdg) + EvBdgAttackDefend(bdg, mvePrev);
	if (fecoTempo)
		evTempo = EvTempo(bdg);

	EV ev = (fecoMaterial * evMaterial +
			 fecoTempo * evTempo +
			 fecoScale/2) / fecoScale;

	return ev;
}


/*	PLAI::EvBdgStatic
 *
 *	Evaluates the board from the point of view of the color with the 
 *	move. Previous move is in mvevPrev, which should be pre-populated 
 *	with the legal move list for the player with the move.
 * 
 */
EV PLAI::EvBdgStatic(BDG& bdg, MVE mvePrev) noexcept
{
	EV evMaterial = 0, evMobility = 0, evKingSafety = 0, evPawnStructure = 0;
	EV evTempo = 0, evRandom = 0;

	if (fecoMaterial)
		evMaterial = EvFromPst(bdg);
	if (fecoRandom) {
		/* if we want randomness in the board eval, we need it to be stable randomness,
		   so we use the Zobrist hash of the board and xor it with a random number we
		   generate at the start of search and use that to generate our random number
		   within the range */
		uint64_t habdT = (bdg.habd ^ habdRand);
		evRandom = (EV)(habdT % (fecoRandom * 2 + 1)) - fecoRandom; /* +/- fecoRandom */
	}
	if (fecoTempo)
		evTempo = EvTempo(bdg);

	/* slow factors aren't used for pre-sorting */

	EV evPawnToMove, evPawnDef;
	EV evKingToMove, evKingDef;
	static VMVE vmveSelf, vmvePrev;
	if (fecoMobility) {
		bdg.GenVmveColor(vmvePrev, bdg.cpcToMove);
		bdg.GenVmveColor(vmveSelf, ~bdg.cpcToMove);
		evMobility = vmvePrev.cmve() - vmveSelf.cmve();
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
	
	EV ev = (fecoMaterial * evMaterial +
			fecoMobility * evMobility +
			fecoKingSafety * evKingSafety +
			fecoPawnStructure * evPawnStructure +
			fecoTempo * evTempo +
			evRandom +
		fecoScale/2) / fecoScale;

#ifndef NOSTATS
	cmveTotalEval++;
#endif
#ifdef EVALSTATS
	LogData(bdg.cpcToMove == cpcWhite ? L"White" : L"Black");
	if (fecoMaterial)
		LogData(wjoin(L"Material", SzFromEv(evMaterial)));
	if (fecoMobility)
		LogData(wjoin(L"Mobility", vmvePrev.cmve(), L"-", vmveSelf.cmve()));
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
	LogData(wjoin(SzFromCpc(bdg.cpcToMove), L"eval:", SzFromEv(ev)));
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
	for (int file = 0; file < fileMax; file++, bbFile = BbEastOne(bbFile)) {
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
	for (int file = 0; file < fileMax; file++, bbFile = BbEastOne(bbFile))
		cfile += (bbPawn & bbFile) && !(bbPawn & (BbEastOne(bbFile) | BbWestOne(bbFile)));
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
	fecoMaterial = 1 * fecoScale;	
	fecoMobility = 0 * fecoScale;	
	fecoKingSafety = 0 * fecoScale;	
	fecoPawnStructure = 0 * fecoScale;	
	fecoTempo = 1 * fecoScale;
	fecoRandom = 0 * fecoScale;	
	SetName(L"SQ Mathilda");
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
	vinfopl.push_back(INFOPL(idclassplAI, tplAI, L"SQ Mobly", 5));
	vinfopl.push_back(INFOPL(idclassplAI, tplAI, L"Max Mobly", 10));
	vinfopl.push_back(INFOPL(idclassplAI2, tplAI, L"SQ Mathilda", 3));
	vinfopl.push_back(INFOPL(idclassplAI2, tplAI, L"Max Mathilda", 10));
	vinfopl.push_back(INFOPL(idclassplHuman, tplHuman, L"Rick Powell"));
	vinfopl.push_back(INFOPL(idclassplHuman, tplHuman, L"Hazel the Dog"));
	vinfopl.push_back(INFOPL(idclassplHuman, tplHuman, L"Allain de Leon"));
}


AINFOPL::~AINFOPL(void) 
{
}


/*	AINFOPL::PplFactory
 *
 *	Creates a player for the given game using the given player index. The
 *	player is not added to the game yet. 
 */
PL* AINFOPL::PplFactory(GA& ga, int iinfopl) const
{
	PL* ppl = nullptr;
	switch (vinfopl[iinfopl].idclasspl) {
	case idclassplAI:
		ppl = new PLAI(ga);
		break;
	case idclassplAI2:
		ppl = new PLAI2(ga);
		break;
	case idclassplHuman:
		ppl = new PLHUMAN(ga, vinfopl[iinfopl].szName);
		break;
	default:
		assert(false);
		return nullptr;
	}
	ppl->SetLevel(vinfopl[iinfopl].level);
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


