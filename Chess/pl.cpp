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


XT xt;	/* transposition table is big, so we share it with all AIs */


/*
 *
 *	PL base class
 * 
 *	The base class for all chess game players. 
 * 
 */


PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName), level(3),
		mvNext(MV()), spmvNext(spmvAnimate), 
		pvmvmBreak(nullptr), imvmBreakLast(-1), cBreakRepeat(0)
{
}


PL::~PL(void)
{
}


wstring SzPercent(uint64_t wNum, uint64_t wDen)
{
	wchar_t sz[20], *pch = sz;

	if (wDen == 0)
		return wstring(L"[0/0]");
	int w1000 = (int)((1000 * wNum) / wDen);
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
void PL::ReceiveMv(MV mv, SPMV spmv)
{
	if (!ga.puiga->fInPlay)
		ga.puiga->Play(mv, spmv);
	else {
		mvNext = mv;
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


void PL::SetAIBreak(const vector<MVM>& vmvm)
{
	if (pvmvmBreak == nullptr) {
		delete pvmvmBreak;
		pvmvmBreak = nullptr;
	}
	pvmvmBreak = new vector<MVM>;
	*pvmvmBreak = vmvm;
}


void PL::InitBreak(void)
{
	cBreakRepeat = 0;
	imvmBreakLast = -1;
}


class AIBREAK
{
public:
	PL& pl;
	int ply;

	inline AIBREAK(PL& pl, const EMV& emv, int ply) : pl(pl), ply(ply)
	{
		if (pl.pvmvmBreak == nullptr || pl.imvmBreakLast < ply - 2)
			return;

		if (emv.mv != (*pl.pvmvmBreak)[ply - 1])
			return;

		pl.imvmBreakLast++;
		if (pl.imvmBreakLast == pl.pvmvmBreak->size()-1)
			DebugBreak();
	}

	inline ~AIBREAK(void)
	{
		if (pl.pvmvmBreak == nullptr || pl.imvmBreakLast < ply - 1)
			return;
		pl.imvmBreakLast--;
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
		tms(IfReleaseElse(tmsSmart, tmsConstDepth)),
#ifndef NOSTATS
		cemvEval(0), cemvNode(0),
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
	cemvEval = 0L;
	cemvNode = 0L;
#endif
	LogOpen(TAG(wjoin(to_wstring(ga.bdg.vmvGame.size()/2+1) + L".", SzFromCpc(ga.bdg.cpcToMove)),  
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
	LogData(wjoin(L"Evaluated", SzCommaFromLong(cemvEval), L"boards"));
	LogData(wjoin(L"Nodes:", SzCommaFromLong(cemvNode)));
	/* cache stats */
	LogData(wjoin(L"Cache Fill:", SzPercent(xt.cxevInUse, xt.cxevMax)));
	LogData(wjoin(L"Cache Probe Hit:", SzPercent(xt.cxevProbeHit, xt.cxevProbe)));
	LogData(wjoin(L"Cache Probe Collision:", SzPercent(xt.cxevProbeCollision, xt.cxevProbe)));
	LogData(wjoin(L"Cache Save Collision:", SzPercent(xt.cxevSaveCollision, xt.cxevSave)));
	LogData(wjoin(L"Cache Save Replace:", SzPercent(xt.cxevSaveReplace, xt.cxevSave)));

	/* time stats */
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(wjoin(L"Time:", SzCommaFromLong(ms.count()), L"ms"));
	float sp = (float)cemvNode / (float)ms.count();
 	LogData(wjoin(L"Speed:", to_wstring((int)round(sp)), L"n/ms"));
#endif
	LogClose(L"", L"", lgfNormal);
}


/*
 *
 *	LOGEMV
 * 
 *	Little open/close log wrapper. Saves a bunch of state so we can automatically
 *	close the log using the destructor without explicitly calling it.
 * 
 */


class LOGSEARCH
{
protected:
	PL& pl;
	const BDG& bdg;
	wstring szData;

public:
	LOGSEARCH(PL& pl, const BDG& bdg) noexcept : pl(pl), bdg(bdg)
	{
	}

	inline void SetData(const wstring& szNew) noexcept
	{
		szData = szNew;
	}
};

class LOGEMV : public LOGSEARCH, public AIBREAK
{
	const EMV& emvPrev;
	int depthLogSav;
	int imvmExpandSav;
	const EMV& emvBest;
	const AB& abInit;

	static int imvmExpand;
	static MVM rgmvm[20];

public:
	inline LOGEMV(PLAI& pl, const BDG& bdg, 
			const EMV& emvPrev, const EMV& emvBest, const AB& abInit, int ply, wchar_t chType=L' ') noexcept : 
		LOGSEARCH(pl, bdg), AIBREAK(pl, emvPrev, ply),
		emvPrev(emvPrev), emvBest(emvBest), abInit(abInit), depthLogSav(0), imvmExpandSav(0)
	{
#ifndef NOSTATS
		pl.cemvNode++;
#endif
		pl.PumpMsg(false);

		depthLogSav = DepthLog();
		imvmExpandSav = imvmExpand;
		if (FExpandLog(emvPrev))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(emvPrev.mv), ATTR(L"FEN", bdg)),
				wjoin(wstring(1, chType) + to_wstring(ply), SzFromSct(emvPrev.sct()), SzFromEv(-emvPrev.ev), abInit),
				lgfNormal);
	}

	inline ~LOGEMV() noexcept
	{
		LogClose(bdg.SzDecodeMvPost(emvPrev.mv), wjoin(SzFromEv(emvBest.ev), SzEvt()), LgfEvt());
		SetDepthLog(depthLogSav);
		imvmExpand = imvmExpandSav;
	}

	inline bool FExpandLog(const EMV& emv) const noexcept
	{
		if (rgmvm[imvmExpand] != mvmAll) {
			if (emv.mv.sqFrom() != rgmvm[imvmExpand].sqFrom() ||
					emv.mv.sqTo() != rgmvm[imvmExpand].sqTo() ||
					emv.mv.apcPromote() != rgmvm[imvmExpand].apcPromote())
				return false;
		}
		imvmExpand++;
		return true;
	}

	inline wstring SzEvt(void) const noexcept
	{
		if (emvBest.ev < abInit)
			return L"Low";
		else if (emvBest.ev > abInit)
			return L"Cut";
		else
			return L"PV";
	}

	inline LGF LgfEvt(void) const noexcept
	{
		if (emvBest.ev < abInit)
			return lgfNormal;
		else if (emvBest.ev > abInit)
			return lgfItalic;
		else
			return lgfBold;
	}
};


/* a little debugging aid to trigger a change in log depth after a 
   specific sequence of moves */
int LOGEMV::imvmExpand = 0;
MVM LOGEMV::rgmvm[] = { /*
	   MVM(sqC2, sqC3),
	   MVM(sqB8, sqC6),
	   MVM(sqD1, sqB3),
	   MVM(sqG8, sqF6),
	   MVM(sqB3, sqB7),
	   mvmAll,	*/
	   mvmNil
};


/*
 *
 *	LOGITD
 * 
 *	Iterative deepening loop logging. Displays search depth and a-b window,
 *	along with the result of the cycle.
 * 
 */


class LOGITD : public LOGSEARCH
{
	const EMV& emvBest;
public:
	LOGITD(PLAI& pl, const BDG& bdg, const EMV& emvBest, AB ab, int ply) noexcept : 
			LOGSEARCH(pl, bdg), emvBest(emvBest)
	{
		LogOpen(L"Depth", wjoin(ply, ab), lgfNormal);
	}

	~LOGITD() noexcept
	{
		LogClose(L"Depth", wjoin(bdg.SzDecodeMvPost(emvBest.mv), SzFromEv(emvBest.ev)), lgfNormal);
	}
};


/*
 *
 *	VEMVS class
 * 
 *	Move enumeration base class.
 * 
 */


VEMVS::VEMVS(BDG& bdg) noexcept : VEMV(), iemvNext(0), cmvLegal(0)
{
	bdg.GenVemv(*this, ggPseudo);
}


void VEMVS::Reset(BDG& bdg) noexcept
{
	iemvNext = 0;
	cmvLegal = 0;
}


/*	VSEMV::FMakeMvNext
 *
 *	Finds the next move in the move list, returning false if there is no such
 *	move. The move is returned in pemv. The move is actually made on the board
 *	and illegal moves are checked for
 */
bool VEMVS::FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept
{
	while (iemvNext < cemv()) {
		pemv = &(*this)[iemvNext++];
		bdg.MakeMv(pemv->mv);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			return true;
		}
		bdg.UndoMv();
	}
	return false;
}


/*	VEMVS::UndoMv
 *
 *	Undoes the move made by FMakeMvNext
 */
void VEMVS::UndoMv(BDG& bdg) noexcept
{
	bdg.UndoMv();
}


/*
 *
 *	VEMVSS class
 *
 *	Alpha-beta sorted movelist enumerator. These moves are pre-evaluated in order
 *	to improve alpha-beta pruning behavior. This is our most used enumerator.
 * 
 */


VEMVSS::VEMVSS(BDG& bdg, PLAI* pplai) noexcept : VEMVS(bdg), pplai(pplai), sctCur(sctPrincipalVar)
{
	Reset(bdg);
}


/*	VEMVSS::Reset
 *
 *	Resets the iteration of the sorted move list, preparing us for a full 
 *	enumeration of the list.
 */
void VEMVSS::Reset(BDG& bdg) noexcept
{
	VEMVS::Reset(bdg);
	sctCur = sctPrincipalVar;
	InitSctCur(bdg, 0);
}


/*	VEMVSS::FMakeMvNext
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
bool VEMVSS::FMakeMvNext(BDG& bdg, EMV*& pemvNext) noexcept
{
	while (iemvNext < cemv()) {

		/* swap the best move into the next emv to return */

		EMV* pemvBest;
		for ( ; (pemvBest = PemvBestFromSctCur(iemvNext)) == nullptr; 
				sctCur++, InitSctCur(bdg, iemvNext))
			;
		pemvNext = &(*this)[iemvNext++];
		swap(*pemvNext, *pemvBest);

		/* make sure move is legal */

		bdg.MakeMv(pemvNext->mv);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			return true;
		}
		bdg.UndoMv();
	}
	return false;
}


/*	VEMVSS::PemvBestFromSctCur
 *
 *	Finds the best move from the movelist that matches the current score type
 *	that we're enumerating. Returns nullptr if no matches.
 */
EMV* VEMVSS::PemvBestFromSctCur(int iemvFirst) noexcept
{
	EMV* pemvBest = nullptr; 
	for (int iemvBest = iemvFirst; iemvBest < cemv(); iemvBest++) {
		if ((*this)[iemvBest].sct() == sctCur && 
				(pemvBest == nullptr || (*this)[iemvBest].ev > pemvBest->ev))
			pemvBest = &(*this)[iemvBest];
	}
	return pemvBest;
}


/*	VEMVSS::InitSctCur
 *
 *	Prepares the enumeration for the next score type. We try to lazy evaluate as 
 *	many moves as possible so we don't waste time evaluating positions that might
 *	never be visited because of pruning. 
 * 
 *	Currently we break moves into ... (1) the principal variation, (2) any other 
 *	move in the transposition table that was fully evaluated, (3) captures, and
 *	and (4) everything else.
 */
void VEMVSS::InitSctCur(BDG& bdg, int iemvFirst) noexcept
{
	switch (sctCur) {
	case sctPrincipalVar:
	{
		/* first time through the enumeration, snag the principal variation. This can
		   be done very quickly with just a transposition table probe. While we're at
		   it, go ahead and reset all the other moves */
		XEV xevT(0, mvNil, evtNull, evCanceled, 0);
		XEV* pxev = xt.Find(bdg, 0);
		if (pxev == nullptr || pxev->evt() != evtEqual)
			pxev = &xevT;
		for (int iemv = iemvFirst; iemv < cemv(); iemv++) {
			EMV& emv = (*this)[iemv];
			if (pxev->mv() != emv.mv)
				emv.SetSct(sctNil);
			else {
				emv.SetSct(sctPrincipalVar);
				emv.ev = -pxev->ev();
			}
		}
		break;
	}

	case sctXTable:
	{
		/* get the eval of any element we find in the tranposition table */
		for (int iemv = iemvFirst; iemv < cemv(); iemv++) {
			EMV& emv = (*this)[iemv];
			assert(emv.sct() == sctNil);	// should all be marked nil by pv pass
			bdg.MakeMvSq(emv.mv);
			XEV* pxev = xt.Find(bdg, 0);
			if (pxev != nullptr && pxev->evt() != evtHigher) {
				emv.SetSct(sctXTable);
				emv.ev = -pxev->ev();
			}
			bdg.UndoMvSq(emv.mv);
		}
		break;
	}

	case sctEvCapture:
	case sctEvOther:
	{
		/* the remainder of the moves need static eval */
		for (int iemv = iemvFirst; iemv < cemv(); iemv++) {
			EMV& emv = (*this)[iemv];
			assert(emv.sct() == sctNil);
			if (sctCur == sctEvCapture && !emv.mv.fIsCapture())
				continue;
			bdg.MakeMvSq(emv.mv);
			emv.SetSct(sctCur);
			emv.ev = -pplai->EvBdgStatic(bdg, emv.mv, false);
			bdg.UndoMvSq(emv.mv);
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
 *	VEMVSQ
 *
 *	Quiescent move list enumerator
 *
 */


VEMVSQ::VEMVSQ(BDG& bdg) noexcept : VEMVS(bdg)
{
}


/*	VEMVSQ::FMakeMvNext
 *
 *	Finds the next noisy move in the quiescent move list and makes it on the
 *	board.
 */
bool VEMVSQ::FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept
{
	while (VEMVS::FMakeMvNext(bdg, pemv)) {
		if (!bdg.FMvIsQuiescent(pemv->mv))
			return true;
		bdg.UndoMv();
	}
	return false;
}


/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. Assumes we've already checked for
 *	mates.
 *
 *	Returns information in spmv for how the board should be display the move,
 *	but this isn't used in AI players. 
 * 
 *	For an AI, this is the root node of the alpha-beta search, 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = spmvAnimate;
	cYield = 0; sint = sintNull;
	StartMoveLog();

	BDG bdg = ga.bdg;
	habdRand = genhabd.HabdRandom(rgen);
	InitBreak();
	xt.Init();	// should we just clear this?

	emvBestOverall = EMV(mvNil, -evInf);
	VEMVSS vemvss(bdg, this);

	InitWeightTables();
	InitTimeMan(bdg);

	/* main iterative deepening and aspiration window loop */

	AB ab(-evInf, evInf);
	for (int plyLim = 2; !FStopSearch(plyLim); ) {
		vemvss.Reset(bdg);
		EMV emvBest = EMV(MV(), -evInf);
		LOGITD logitd(*this, bdg, emvBest, ab, plyLim);
		FSearchEmvBest(bdg, vemvss, emvBest, ab, 0, plyLim);
		SaveXt(bdg, emvBest, ab, plyLim);
		if (FDeepen(emvBest, ab, plyLim))
			break;
	}

	EndMoveLog();
	return emvBestOverall.mv;
}


/*	PLAI::EvBdgDepth
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth plyLim; ply is current. Uses alpha-beta pruning.
 * 
 *	The board will be modified, but it will be restored to its original state in 
 *	all cases.
 * 
 *	On entry emvPrev has the last move made.
 * 
 *	Returns board evaluation in centi-pawns.
 */
EV PLAI::EvBdgDepth(BDG& bdg, const EMV& emvPrev, AB abInit, int ply, int plyLim) noexcept
{
	/* we're at depth, go to quiescence */

	if (ply >= plyLim)
		return EvBdgQuiescent(bdg, emvPrev, abInit, ply);

	EMV emvBest(MV(), -evInf);
	LOGEMV logemv(*this, bdg, emvPrev, emvBest, abInit, ply, L' ');
	
	/* check transposition table, check futility pruning, and try the null move
	   pruning trick */

	if (FLookupXt(bdg, emvBest, abInit, plyLim - ply))
		return emvBest.ev;
	/* TODO: futility pruning */
	/* TODO: null move pruning */

	/* full search */

	VEMVSS vemvss(bdg, this);
	if (!FSearchEmvBest(bdg, vemvss, emvBest, abInit, ply, plyLim))
		TestForMates(bdg, vemvss, emvBest, ply);
	
	SaveXt(bdg, emvBest, abInit, plyLim - ply);
	return emvBest.ev;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the "quiet" evaluation after the given board/last move from the point 
 *	of view of the player next to move; it only considers captures and other "noisy" 
 *	moves. Alpha-beta prunes within the abInit window.
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, const EMV& emvPrev, AB abInit, int ply) noexcept
{
	int plyLim = plyMax;
	EMV emvBest;
	LOGEMV logemv(*this, bdg, emvPrev, emvBest, abInit, ply, L'Q');

	if (FLookupXt(bdg, emvBest, abInit, 0))
		return emvBest.ev;

	/* first off, the player may refuse the capture, so get full, slow static eval 
	   and check if we're already in a pruning situation */

	AB ab = abInit;
	emvBest.mv = MV();
	emvBest.ev = EvBdgStatic(bdg, emvPrev.mv, true);
	if (FPrune(&emvBest, emvBest, ab, plyLim))
		return emvBest.ev;

	/* recursively evaluate noisy moves */
	
	VEMVSQ vemvsq(bdg);
	for (EMV* pemv; vemvsq.FMakeMvNext(bdg, pemv); ) {
		pemv->ev = -EvBdgQuiescent(bdg, *pemv, -ab, ply + 1);
		vemvsq.UndoMv(bdg);
		if (FPrune(pemv, emvBest, ab, plyLim))
			goto Done;
	}
	TestForMates(bdg, vemvsq, emvBest, ply);

Done:
	SaveXt(bdg, emvBest, abInit, 0);
	return emvBest.ev;
}


/*	PLAI::FSearchEmvBest
 *
 *	Finds the best move for the given board and sorted movelist, using the given
 *	alpha-beta window and search depth. Returns true the search should be pruned.
 *
 *	Handles the principal value search optimization, which tries to bail out of
 *	non-PV searches quickly by doing a quick null-window search on the PV valuation.
 */
bool PLAI::FSearchEmvBest(BDG& bdg, VEMVSS& vemvss, EMV& emvBest, AB ab, int ply, int& plyLim) noexcept
{
	/* do first move (probably a PV move) with full a-b window */
	
	EMV* pemv;
	if (!vemvss.FMakeMvNext(bdg, pemv))
		return false;
	pemv->ev = -EvBdgDepth(bdg, *pemv, -ab, ply + 1, plyLim);
	vemvss.UndoMv(bdg);
	if (FPrune(pemv, emvBest, ab, plyLim))
		return true;

	/* subsequent moves get the PV pre-search optimization which uses an 
	   ultra-narrow window. We pray we quickly fail low, but if we don't, 
	   we have to redo the search with the full window */

	while (vemvss.FMakeMvNext(bdg, pemv)) {
		pemv->ev = -EvBdgDepth(bdg, *pemv, -ab.AbNull(), ply + 1, plyLim);
		if (!(pemv->ev < ab))
			pemv->ev = -EvBdgDepth(bdg, *pemv, -ab, ply + 1, plyLim);
		vemvss.UndoMv(bdg);
		if (FPrune(pemv, emvBest, ab, plyLim))
			return true;
	}
	return false;
}


/*	PLAI::FLookupXt
 *
 *	Checks the transposition table for a board entry at the given search depth. Returns 
 *	true if we should stop the search at this point, either because we found an exact 
 *	match of the board/ply, or the inexact match is outside the alpha/beta interval.
 *
 *	emvBest will contain the evaluation we should use if we stop the search.
 */
bool PLAI::FLookupXt(BDG& bdg, EMV& emvBest, AB ab, int ply) const noexcept
{
	/* look for the entry in the transposition table */

	XEV* pxev = xt.Find(bdg, ply);
	if (pxev == nullptr)
		return false;
	
	/* adjust the alpha-beta interval based on the entry */

	switch (pxev->evt()) {
	case evtEqual:
		emvBest.ev = pxev->ev();
		break;
	case evtHigher:
		if (!(pxev->ev() > ab))
			return false;
		emvBest.ev = ab.evBeta;
		break;
	case evtLower:
		if (!(pxev->ev() < ab))
			return false;
		emvBest.ev = ab.evAlpha;
		break;
	}
	
	emvBest.mv = pxev->mv();
	return true;
}


/*	PLAI::SaveXt
 *
 *	Saves the evaluated board in the transposition table, including the depth,
 *	best move, and making sure we keep track of whether the eval was outside
 *	the a-b window.
 */
XEV* PLAI::SaveXt(BDG& bdg, EMV emvBest, AB ab, int ply) const noexcept
{
	if (emvBest.ev < ab)
		return xt.Save(bdg, emvBest, evtLower, ply);
	else if (emvBest.ev > ab)
		return xt.Save(bdg, emvBest, evtHigher, ply);
	else
		return xt.Save(bdg, emvBest, evtEqual, ply);
}


/*	PLAI::FPrune
 *
 *	Helper function to perform standard alpha-beta pruning operations after evaluating a
 *	board position. Keeps track of alpha and beta cut offs, the best move (in evmBest), 
 *	and adjusts search depth when mates are found. Returns true if we should prune.
 * 
 *	Also returns true if the search should be interrupted for some reason. pemv->ev will 
 *	be modified with the reason for the interruption.
 */
bool PLAI::FPrune(EMV* pemv, EMV& emvBest, AB& ab, int& plyLim) const noexcept
{
	if (pemv->ev > emvBest.ev) // keep track of best move so far
		emvBest = *pemv;
	if (pemv->ev > ab) {	// above the beta cut-off, we prune
		emvBest = *pemv;
		return true;
	}
	if (!(pemv->ev < ab)) {	// we're inside the a-b window
		ab.NarrowAlpha(pemv->ev);
		if (FEvIsMate(pemv->ev))	// optimization to limit search on forced mates
			plyLim = PlyFromEvMate(pemv->ev);
	}

	/* If Esc is hit (set by message pump), or if we're taking too damn long to do
	 * the search, force the search to prune all the way back to root, where we'll 
	 * abort the search */

	if (sint != sintNull) {
		pemv->ev = emvBest.ev = (sint == sintCanceled) ? evCanceled : evTimedOut;
		emvBest.mv = MV();
		return true;
	}
	return false;
}


/*	PLAI::TestForMates
 *
 *	Helper function that adjusts evaluations in checkmates and stalemates. Modifies
 *	evBest to be the checkmate/stalemate if we're in that state. cmvLegal is the
 *	count of legal moves in the move list, and vemvs is the move enumerator, which 
 *	kept sstrack of how many legal moves there were..
 */
void PLAI::TestForMates(BDG& bdg, VEMVS& vemvs, EMV& emvBest, int ply) const noexcept
{
	if (vemvs.cmvLegal == 0) {
		emvBest.ev = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(ply) : evDraw;
		emvBest.mv = MV();
	}
	else if (bdg.GsTestGameOver(vemvs.cmvLegal, *ga.prule) != gsPlaying) {
		emvBest.ev = evDraw;
		emvBest.mv = MV();
	}
}


/*	PLAI::FDeepen
 *
 *	Just a little helper to set us up for the next round of the iterative deepening
 *	loop. Our loop is set up to handle both deepening and aspiration windows, so
 *	sometimes we increase the depth, other times we widen the aspiration window.
 *
 *	Returns true if we should abort the search, which happens if we find a forced
 *	mate or some interrupt occurs.
 */
bool PLAI::FDeepen(EMV emvBest, AB& ab, int& ply) noexcept
{
	if (emvBest.ev == evCanceled) {
		emvBestOverall.mv = mvNil;
		return true;
	}
	if (emvBest.ev == evTimedOut) {
		assert(!emvBestOverall.mv.fIsNil());
		return true;
	}

	/* If the search failed with a narrow a-b window, widen the window up some and
	   try again */
	if (emvBest.ev < ab)
		ab.AdjMissLow();
	else if (emvBest.ev > ab)
		ab.AdjMissHigh();
	else {
		/* yay, we found a move - go deeper in the next pass, but use a tight
		   a-b window at first in hopes we'll get lots of pruning which will help
		   search very quickly */
		emvBestOverall = emvBest;
		if (FEvIsMate(emvBest.ev) || FEvIsMate(-emvBest.ev))
			return true;
		ab = ab.AbAspiration(emvBest.ev, 20);
		ply++;
	}
	return false;
}


/*	PLAI::InitTimeMan
 *
 *	Gets us ready for the intelligence that will determine how long we spend analyzing the
 *	move.
 */
void PLAI::InitTimeMan(BDG& bdg) noexcept
{
	static const DWORD mpleveldmsec[] = { 0,
		msecHalfSec, 1*msecSec, 2*msecSec, 5*msecSec, 10*msecSec, 15*msecSec, 30*msecSec,
		1*msecMin, 2*msecMin, 5*msecMin
	};

	CPC cpc = bdg.cpcToMove;
	dmsecFlag = -1;
	tpMoveFirst = high_resolution_clock::now();
	switch (tms) {
	case tmsSmart:
		if (!ga.prule->FUntimed()) {
			dmsecFlag = ga.DmsecRemaining(cpc);
			DWORD dmsecMove = ga.prule->DmsecAddMove(cpc, ((int)bdg.imvCurLast+1)/2+1);
			int cmvDen = (50 - 10) * (EvMaterialTotal(bdg) - 200) / (7800 - 200) + 10;
			dmsecDeadline = dmsecFlag / cmvDen;
			if (dmsecDeadline + dmsecMove < dmsecFlag)
				dmsecDeadline += dmsecMove;
			dmsecDeadline = min(dmsecDeadline, mpleveldmsec[level]);
			LogData(wjoin(L"Time target:", SzCommaFromLong(dmsecDeadline), L"ms"));
			break;
		}
		tms = tmsTimePerMove;
		[[fallthrough]];
	case tmsTimePerMove:
		dmsecDeadline = mpleveldmsec[level];
		break;
	default:
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


/*	PLAI::FStopSearch
 *
 *	Lets us know when we should stop searching for moves. This is where our time management
 *	happens. Returns true if we should stop searching.
 * 
 *	Currently very unsophisticated. Doesn't take clock or timing rules into account.
 */
bool PLAI::FStopSearch(int plyLim) noexcept
{
	if (emvBestOverall.mv.fIsNil())
		return false;

	switch (tms) {

	case tmsSmart:
	case tmsTimePerMove:
	{
		time_point<high_resolution_clock> tp = high_resolution_clock::now();
		duration dtp = tp - tpMoveFirst;
		microseconds usec = duration_cast<microseconds>(dtp);
		return usec.count() >= usecMsec * dmsecDeadline;
	}
	case tmsConstDepth:
	{
		/* different levels get different depths */
		return plyLim > level + 1;
	}
	}

	return true;
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
		if (ga.bdg.gs != gsPlaying)
			sint = sintCanceled;		// someone has flagged the game, abort the search
		else if (tms != tmsConstDepth && !emvBestOverall.mv.fIsNil()) {
			/* interrupt searches that have been going on way too long, or if we're about
			   to flag */
			time_point<high_resolution_clock> tp = high_resolution_clock::now();
			duration dtp = tp - tpMoveFirst;
			microseconds usec = duration_cast<microseconds>(dtp);
			if (usec.count() > (msecSec * (dmsecDeadline+dmsecDeadline/2))) // we've taken 1.5 times too long... timeout
				sint = sintTimedOut;
			/* if we're within a half-second of flagging, bail out right away */
			else if (dmsecFlag != -1 && usec.count() > usecMsec * dmsecFlag - msecHalfSec)
				sint = sintTimedOut;
		}
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
EV PLAI::EvBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept
{
	APC apcMove = bdg.ApcFromSq(mvPrev.sqTo());
	APC apcAttacker = bdg.ApcSqAttacked(mvPrev.sqTo(), bdg.cpcToMove);
	if (apcAttacker != apcNull) {
		if (apcAttacker < apcMove)
			return EvBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(mvPrev.sqTo(), ~bdg.cpcToMove);
		if (apcDefended == apcNull)
			return EvBaseApc(apcMove);
	}
	return 0;
}


/*	PLAI::EvBdgStatic
 *
 *	Evaluates the board from the point of view of the color with the 
 *	move. Previous move is in mvevPrev, which should be pre-populated 
 *	with the legal move list for the player with the move.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
EV PLAI::EvBdgStatic(BDG& bdg, MV mvPrev, bool fFull) noexcept
{
	EV evMaterial = 0, evMobility = 0, evKingSafety = 0, evPawnStructure = 0;
	EV evTempo = 0, evRandom = 0;

	if (fecoMaterial) {
		evMaterial = EvFromPst(bdg);
		if (!fFull)
			evMaterial += EvBdgAttackDefend(bdg, mvPrev);
	}
	if (fecoRandom) {
		/* if we want randomness in the board eval, we need it to be stable randomness,
		 * so we use the Zobrist hash of the board and xor it with a random number we
		 * generate at the start of search and use that to generate our random number
		 * within the range */
		uint64_t habdT = (bdg.habd ^ habdRand);
		evRandom = (EV)(habdT % (fecoRandom * 2 + 1)) - fecoRandom; /* +/- fecoRandom */
	}
	if (fecoTempo)
		evTempo = EvTempo(bdg);

	/* slow factors aren't used for pre-sorting */

	EV evPawnToMove, evPawnDef;
	EV evKingToMove, evKingDef;
	static VEMV vemvSelf, vemvPrev;
	if (fFull) {
		if (fecoMobility) {
			bdg.GenVemvColor(vemvPrev, bdg.cpcToMove);
			bdg.GenVemvColor(vemvSelf, ~bdg.cpcToMove);
			evMobility = vemvPrev.cemv() - vemvSelf.cemv();
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
	}
	
#ifndef NOSTATS
	cemvEval++;
#endif
	EV ev = (fecoMaterial * evMaterial +
			fecoMobility * evMobility +
			fecoKingSafety * evKingSafety +
			fecoPawnStructure * evPawnStructure +
			fecoTempo * evTempo +
			evRandom +
		fecoScale/2) / fecoScale;

	if (fFull) {
#ifndef NOSTATS
		cemvEval++;
#endif
#ifdef EVALSTATS
		LogData(bdg.cpcToMove == cpcWhite ? L"White" : L"Black");
		if (fecoMaterial)
			LogData(wjoin(L"Material", SzFromEv(evMaterial)));
		if (fecoMobility)
			LogData(wjoin(L"Mobility", vemvPrev.cemv(), L"-", vemvSelf.cemv()));
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
	}

	return ev;
}


/*	PLAI::EvInterpolate
 *
 *	Interpolate the piece value evaluation based on game phase
 */
EV PLAI::EvInterpolate(GPH gph, EV ev1, GPH gphFirst, EV ev2, GPH gphLim) const noexcept
{
	assert(gphFirst < gphLim);
	assert(in_range(gph, gphFirst, gphLim));
	return (ev1 * ((int)gphLim - (int)gph) + ev2 * ((int)gph - (int)gphFirst)) / 
		((int)gphLim - (int)gphFirst);
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
	GPH gph = min(bdg.GphCur(), gphMax);	// can exceed Max with promotions

	/* opening */

	if (FInOpening(gph)) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevOpening);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	if (FInEndGame(gph)) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevEndGame);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

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


MV PLHUMAN::MvGetNext(SPMV& spmv)
{
	mvNext = MV();
	do
		ga.puiga->PumpMsg();
	while (mvNext.fIsNil());
	MV mv = mvNext;
	spmv = spmvNext;
	mvNext = MV();
	return mv;
}


/*
 *
 *	RGINFOPL
 * 
 *	The list of players we have available to play. This is used as a player
 *	picker, and includes a factory for creating players.
 * 
 */


RGINFOPL::RGINFOPL(void) 
{
	vinfopl.push_back(INFOPL(IDCLASSPL::AI, TPL::AI, L"SQ Mobly", 5));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI, TPL::AI, L"Max Mobly", 10));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI2, TPL::AI, L"SQ Mathilda", 3));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI2, TPL::AI, L"Max Mathilda", 10));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Rick Powell"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Hazel the Dog"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Allain de Leon"));
}


RGINFOPL::~RGINFOPL(void) 
{
}


/*	RGINFOPL::PplFactory
 *
 *	Creates a player for the given game using the given player index. The
 *	player is not added to the game yet. 
 */
PL* RGINFOPL::PplFactory(GA& ga, int iinfopl) const
{
	PL* ppl = nullptr;
	switch (vinfopl[iinfopl].idclasspl) {
	case IDCLASSPL::AI:
		ppl = new PLAI(ga);
		break;
	case IDCLASSPL::AI2:
		ppl = new PLAI2(ga);
		break;
	case IDCLASSPL::Human:
		ppl = new PLHUMAN(ga, vinfopl[iinfopl].szName);
		break;
	default:
		assert(false);
		return nullptr;
	}
	ppl->SetLevel(vinfopl[iinfopl].level);
	return ppl;
}


/*	RGINFOPL::IdbFromInfopl
 *
 *	Returns an icon resource ID to use to help identify the players.
 */
int RGINFOPL::IdbFromInfopl(const INFOPL& infopl) const
{
	switch (infopl.tpl) {
	case TPL::AI:
		return idbAiLogo;
	default:
		break;
	}
	return idbHumanLogo;
}


