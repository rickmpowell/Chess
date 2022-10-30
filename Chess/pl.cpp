/*
 *
 *	pl.cpp
 * 
 *	Code for the player class
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
const uint16_t dcYield = 1024;
#endif


XT xt;	/* transposition table is big, so we share it with all AIs */



PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName), level(3),
		mvNext(MV()), spmvNext(SPMV::Animate), fDisableMvLog(false)
{
}


PL::~PL(void)
{
}


wstring SzFromEv(EV ev)
{
	wchar_t sz[20], * pch = sz;
	if (ev >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		ev = -ev;
	}
	if (ev == evInf)
		*pch++ = L'\x221e';
	else if (FEvIsAbort(abs(ev))) {
		lstrcpy(pch, L"(interrupted)");
		pch += lstrlen(pch);
	}
	else if (FEvIsMate(ev)) {
		*pch++ = L'#';
		pch = PchDecodeInt((PlyFromEvMate(ev) + 1) / 2, pch);
	}
	else if (ev > evInf)
		*pch++ = L'*';
	else {
		pch = PchDecodeInt(ev / 100, pch);
		ev %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + ev / 10;
		*pch++ = L'0' + ev % 10;
	}
	*pch = 0;
	return wstring(sz);
}


wstring SzPercent(uint64_t wNum, uint64_t wDen)
{
	wchar_t sz[20], *pch = sz;

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
	if (!ga.fInPlay) {
		ga.MakeMv(mv, spmv);
		ga.Play();
	}
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
	EV mpapcev[APC::ActMax] = { 0, 100, 275, 300, 500, 900, 200 };
	return mpapcev[apc];
}


bool PL::FDepthLog(LGT lgt, int& depth)
{
	return ga.FDepthLog(lgt, depth);
}


void PL::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	ga.AddLog(lgt, lgf, depth, tag, szData);
}


int PL::DepthLog(void) const
{
	return ga.DepthLog();
}


void PL::SetDepthLog(int depthNew)
{
	ga.SetDepthLog(depthNew);
}


/*
 *
 *	PLAI
 * 
 *	AI computer players. Base PLAI implements alpha-beta pruned look ahead tree.
 * 
 */


PLAI::PLAI(GA& ga) : PL(ga, L"SQ Mobly"), rgen(372716661UL), habdRand(0), 
		cYield(0), cemvEval(0), cemvNode(0)
{
	fecoMaterial = 1*100;
	fecoMobility = 3*100;	/* this is small because Material already includes
							   some mobility in the piece value tables */
	fecoKingSafety = 10*100;
	fecoPawnStructure = 10*100;
	fecoTempo = 1*100;
	fecoRandom = 0*100;
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


/*	PLAI::CmvFromLevel
 *
 *	Converts quality of play level into number of moves to consider when
 *	making a move.
 */
float PLAI::CmvFromLevel(int level) const noexcept
{
	switch (level) {
	case 1: return 100.0f;
	case 2: return 8000.0f;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		return 4.0f * CmvFromLevel(level - 1);
	default:
		return CmvFromLevel(5);
	}
}


int PLAI::PlyLim(const BDG& bdg, const GEMV& gemv) const
{
	static GEMV gemvOpp;
	bdg.GenGemvColor(gemvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gemv.cemv() * gemvOpp.cemv());
	int plyLim = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return plyLim;
}


/*	PLAI::StartMoveLog
 *
 *	Opens a log entry for AI move generation. Must be paired with a corresponding
 *	EndMoveLog
 */
void PLAI::StartMoveLog(void)
{
	tpStart = high_resolution_clock::now();
	cemvEval = 0L;
	cemvNode = 0L;
	LogOpen(TAG(ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", ATTR(L"FEN", ga.bdg)),
		L"(" + szName + L")");
}


/*	PLAI::EndMoveLog
 *
 *	Closes the move generation logging entry for AI move generation.
 */
void PLAI::EndMoveLog(void)
{
	/* eval stats */
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	LogData(L"Evaluated " + to_wstring(cemvEval) + L" boards");
	LogData(L"Nodes: " + to_wstring(cemvNode));

	/* cache stats */
	LogData(L"Cache Fill: " + SzPercent(xt.cxevInUse, xt.cxevMax));
	LogData(L"Cache Probe Hit: " + SzPercent(xt.cxevProbeHit, xt.cxevProbe));
	LogData(L"Cache Probe Collision: " + SzPercent(xt.cxevProbeCollision, xt.cxevProbe));
	LogData(L"Cache Save Collision: " + SzPercent(xt.cxevSaveCollision, xt.cxevSave));
	LogData(L"Cache Save Replace: " + SzPercent(xt.cxevSaveReplace, xt.cxevSave));

	/* time stats */
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cemvNode / (float)ms.count();
 	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" n/ms");
	LogClose(L"", L"", LGF::Normal);
}


/*
 *
 *	LOGOPENEMV
 * 
 *	Little open/close log wrapper. Saves a bunch of state so we can automatically
 *	close the log using the destructor without explicitly calling it.
 * 
 */


class LOGOPENEMV
{
	PL& pl;
	const BDG& bdg;
	const EMV* pemv;
	wstring szData;
	LGF lgf;
	int depthLogSav;
	int imvExpandSav;
	const EV& evBest;

	static int imvExpand;
	static MV rgmv[8];

public:
	inline LOGOPENEMV(PL& pl, const BDG& bdg, const EMV* pemv, 
			const EV& evBest, EV evAlpha, EV evBeta, int ply, wchar_t chType=L' ') : 
		pl(pl), bdg(bdg), pemv(pemv), evBest(evBest), depthLogSav(0), imvExpandSav(0), 
		szData(L""), lgf(LGF::Normal)
	{
		if (pl.FDisabledMvLog())
			return;
		depthLogSav = DepthLog();
		imvExpandSav = imvExpand;
		if (FExpandLog(pemv))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(pemv->mv), ATTR(L"FEN", bdg)),
				wstring(1, chType) + to_wstring(ply) + L" " + SzFromEv(pemv->ev) + 
					L" [" + SzFromEv(evAlpha) + L", " + SzFromEv(evBeta) + L"] ");
	}

	inline void SetData(const wstring& szNew, LGF lgfNew)
	{
		szData = szNew;
		lgf = lgfNew;
	}

	inline ~LOGOPENEMV()
	{
		if (pl.FDisabledMvLog())
			return;
		LogClose(bdg.SzDecodeMvPost(pemv->mv), 
				 szData.size() > 0 ? szData : SzFromEv(-evBest),	 
				 lgf);
		SetDepthLog(depthLogSav);
		imvExpand = imvExpandSav;
	}

	inline bool FExpandLog(const EMV* pemv) const
	{
		if (pemv->mv.sqFrom() != rgmv[imvExpand].sqFrom() ||	
				pemv->mv.sqTo() != rgmv[imvExpand].sqTo() ||
				pemv->mv.apcPromote() != rgmv[imvExpand].apcPromote())
			return false;
		imvExpand++;
		return true;
	}

	inline void SetDepthLog(int depth)
	{
		pl.SetDepthLog(depth);
	}

	inline int DepthLog(void) const
	{
		return pl.DepthLog();
	}

	inline bool FDepthLog(LGT lgt, int& depth)
	{
		return pl.FDepthLog(lgt, depth);
	}

	inline void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
	{
		pl.AddLog(lgt, lgf, depth, tag, szData);
	}
};

/* a little debugging aid to trigger a change in log depth after a 
   specific sequence of moves */
int LOGOPENEMV::imvExpand = 0;
MV LOGOPENEMV::rgmv[] = { /*
	   MV(sqD2, sqD4, pcWhitePawn),
	   MV(sqD7, sqD6, pcBlackPawn),
	   MV(sqC1, sqG5, pcWhiteBishop),
	   MV(sqE7, sqE5, pcBlackPawn),
	   MV(sqG5, sqD8, pcWhiteBishop), */
	   MV()
};


/*
 *
 *	GEMVS class
 * 
 *	Move enumeration base class.
 * 
 */


GEMVS::GEMVS(BDG& bdg) noexcept : GEMV(), iemvNext(0), cmvLegal(0)
{
	bdg.GenGemv(*this, GG::Pseudo);
}


/*	GSEMV::FMakeMvNext
 *
 *	Finds the next move in the move list, returning false if there is no such
 *	move. The move is returned in pemv. The move is actually made on the board
 *	and illegal moves are checked for
 */
bool GEMVS::FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept
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

void GEMVS::UndoMv(BDG& bdg) noexcept
{
	bdg.UndoMv();
}


/*
 *
 *	GEMVSS class
 *
 *	Alpha-beta sorted movelist enumerator. These moves are pre-evaluated in order
 *	to improve alpha-beta pruning behavior. This is our most used enumerator.
 * 
 */


GEMVSS::GEMVSS(BDG& bdg, PLAI* pplai) noexcept : GEMVS(bdg)
{
	/* pre-evaluate all the moves */

	for (EMV& emv : *this) {
		bdg.MakeMvSq(emv.mv);	
		XEV* pxev = xt.Find(bdg, 0);	/* use transposition table if available */
		if (pxev != nullptr && pxev->evt() == EVT::Equal)
			emv.ev = -pxev->ev();
		else
			emv.ev = -pplai->EvBdgStatic(bdg, emv.mv, false);
		bdg.UndoMvSq(emv.mv);
	}
}


/*	GEMVSS::FMakeMvNext
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
 */
bool GEMVSS::FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept
{
	while (iemvNext < cemv()) {
		pemv = &(*this)[iemvNext++];

		/* swap the best move into the next emv to return */

		EMV* pemvBest = pemv;
		for (int iemvBest = iemvNext; iemvBest < cemv(); iemvBest++)
			if ((*this)[iemvBest].ev > pemvBest->ev)
				pemvBest = &(*this)[iemvBest];
		swap(*pemv, *pemvBest);

		/* make sure move is legal */

		bdg.MakeMv(pemv->mv);
		if (!bdg.FInCheck(~bdg.cpcToMove)) {
			cmvLegal++;
			return true;
		}
		bdg.UndoMv();
	}
	return false;
}


/*
 *
 *	GEMVSQ
 *
 *	Quiescent move list enumerator
 *
 */


GEMVSQ::GEMVSQ(BDG& bdg) noexcept : GEMVS(bdg)
{
}

bool GEMVSQ::FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept
{
	while (GEMVS::FMakeMvNext(bdg, pemv)) {
		if (!bdg.FMvIsQuiescent(pemv->mv))
			return true;
		bdg.UndoMv();
	}
	return false;
}


/*	PLAI::FAlphaBetaPrune
 *
 *	Helper function to perform standard alpha-beta pruning operations after evaluating a
 *	board position. Keeps track of alpha and beta cut offs, the best move (in evBest and 
 *	pemvBest), and adjusts search depth when mates are found. mvev has the move and
 *	evaluation in it on entry. Returns true if we should prune.
 */
bool PLAI::FAlphaBetaPrune(EMV* pemv, EV& evBest, EV& evAlpha, EV evBeta, const EMV*& pemvBest, int& plyLim) const noexcept
{
	if (pemv->ev > evBest) {
		evBest = pemv->ev;
		pemvBest = pemv;
	}
	if (pemv->ev >= evBeta) 
		return true;
	if (pemv->ev > evAlpha) {
		evAlpha = pemv->ev;
		if (FEvIsMate(pemv->ev))
			plyLim = PlyFromEvMate(pemv->ev);
	}
	if (fAbort) {
		pemv->ev = evBest = evAbort;
		pemvBest = nullptr;
		return true;
	}
	return false;
}


/*	PLAI::CheckForMates
 *
 *	Helper function that adjusts evaluations in checkmates and stalemates. Modifies
 *	evBest to be the checkmate/stalemate if we're in that state. cmvLegal is the 
 *	count of legal moves in vmvev, and vmvev is the full pseudo-legal move list.
 */
void PLAI::CheckForMates(BDG& bdg, GEMVS& gemvs, EV& evBest, int ply) const noexcept
{
	if (gemvs.cmvLegal == 0)
		evBest = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(ply) : evDraw;
	else if (bdg.GsTestGameOver(gemvs.cmvLegal, *ga.prule) != GS::Playing)
		evBest = evDraw;
}


/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. 
 *
 *	Returns information in spmv for how the board should be display the move,
 *	but this isn't used in AI players. 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	BDG bdg = ga.bdg;

	cYield = 0;
	fAbort = false;
	xt.Init();
	InitWeightTables();
	habdRand = genhabd.HabdRandom(rgen);

	GEMVSS gemvss(bdg, this);
	int plyLim = clamp(PlyLim(bdg, gemvss), 2, 14);

	StartMoveLog();
	LogData(L"Search depth: " + to_wstring(plyLim));

	EV evAlpha = -evInf, evBeta = evInf;
	EV evBest = -evInf;
	const EMV* pemvBest = nullptr;

	for (EMV* pemv; gemvss.FMakeMvNext(bdg, pemv); ) {
		pemv->ev = -EvBdgDepth(bdg, pemv, 1, plyLim, -evBeta, -evAlpha);
		gemvss.UndoMv(bdg);
		if (FAlphaBetaPrune(pemv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			break;
	}
	
	EndMoveLog();
	return pemvBest == nullptr ? MV() : pemvBest->mv;
}


/*	PLAI::EvBdgDepth
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth plyMax; ply is current. Uses alpha-beta pruning.
 * 
 *	The board will be modified, but it will be restored to its original state in 
 *	all cases.
 * 
 *	On entry pemvPrev has the last move made.
 * 
 *	Returns board evaluation in centi-pawns.
 */
EV PLAI::EvBdgDepth(BDG& bdg, const EMV* pemvPrev, int ply, int plyLim, EV evAlpha, EV evBeta) noexcept
{
	if (ply >= plyLim)
		return EvBdgQuiescent(bdg, pemvPrev, ply, evAlpha, evBeta);

	const EMV* pemvBest = nullptr;
	EV evBest = -evInf;

	cemvNode++;
	LOGOPENEMV logopenemv(*this, bdg, pemvPrev, evBest, evAlpha, evBeta, ply, L' ');
	PumpMsg();

	GEMVSS gemvss(bdg, this);
	for (EMV* pemv; gemvss.FMakeMvNext(bdg, pemv); ) {
		pemv->ev = -EvBdgDepth(bdg, pemv, ply + 1, plyLim, -evBeta, -evAlpha);
		gemvss.UndoMv(bdg);
		if (FAlphaBetaPrune(pemv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			return evBest;
	}

	CheckForMates(bdg, gemvss, evBest, ply);
	return evBest;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the quiet evaluation after the given board/last move from the point of view 
 *	of the player next to move, i.e., it only considers captures and other "noisy" moves.
 *	Alpha-beta prunes. 
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, const EMV* pemvPrev, int ply, EV evAlpha, EV evBeta) noexcept
{
	cemvNode++;

	const EMV* pemvBest = nullptr;
	EV evBest = -evInf;

	LOGOPENEMV logopenemv(*this, bdg, pemvPrev, evBest, evAlpha, evBeta, ply, L'Q');
	PumpMsg();

	int plyLim = plyMax;

	/* first off, get full, slow static eval and check current board already in a pruning 
	   situations; use transposition table eval if it's available */

	XEV* pxev = xt.Find(bdg, 0);
	if (pxev != nullptr && pxev->evt() == EVT::Equal)
		evBest = pxev->ev();
	else {
		evBest = EvBdgStatic(bdg, pemvPrev->mv, true);
		xt.Save(bdg, evBest, EVT::Equal, 0);
	}

	if (evBest >= evBeta)
		return evBest;
	if (evBest > evAlpha)
		evAlpha = evBest;

	/* recursively evaluate more moves */

	GEMVSQ gemvsq(bdg);
	for (EMV* pemv; gemvsq.FMakeMvNext(bdg, pemv); ) {
		pemv->ev = -EvBdgQuiescent(bdg, pemv, ply + 1, -evBeta, -evAlpha);
		gemvsq.UndoMv(bdg);
		if (FAlphaBetaPrune(pemv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			return evBest;
	}

	CheckForMates(bdg, gemvsq, evBest, ply);
	return evBest;
}


void PLAI::PumpMsg(void) noexcept
{
	if (++cYield % dcYield == 0) {
		try {
			ga.PumpMsg();
		}
		catch (...) {
			fAbort = true;
		}
	}
}


EV PLAI::EvTempo(const BDG& bdg) const noexcept
{
	if (FInOpening(bdg.GphCur()))
		return evTempo;
	if (bdg.GphCur() < GPH::MidMid)
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
	if (apcAttacker != APC::Null) {
		if (apcAttacker < apcMove)
			return EvBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(mvPrev.sqTo(), ~bdg.cpcToMove);
		if (apcDefended == APC::Null)
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

	/* slow factors aren't used for pre-sorting */

	EV evPawnToMove, evPawnDef;
	EV evKingToMove, evKingDef;
	static GEMV gemvSelf, gemvPrev;
	if (fFull) {
		if (fecoMobility) {
			bdg.GenGemvColor(gemvPrev, bdg.cpcToMove);
			bdg.GenGemvColor(gemvSelf, ~bdg.cpcToMove);
			evMobility = gemvPrev.cemv() - gemvSelf.cemv();
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
		if (fecoTempo)
			evTempo = EvTempo(bdg);
	}
	
	cemvEval++;
	EV ev = (fecoMaterial * evMaterial +
			fecoMobility * evMobility +
			fecoKingSafety * evKingSafety +
			fecoPawnStructure * evPawnStructure +
			fecoTempo * evTempo +
			evRandom +
		50) / 100;

	if (fFull) {
		cemvEval++;
		if (!fDisableMvLog) {
			LogData(bdg.cpcToMove == CPC::White ? L"White" : L"Black");
			if (fecoMaterial)
				LogData(L"Material " + SzFromEv(evMaterial));
			if (fecoMobility)
				LogData(L"Mobility " + to_wstring(gemvPrev.cemv()) + L" - " + to_wstring(gemvSelf.cemv()));
			if (fecoKingSafety)
				LogData(L"King Safety " + to_wstring(evKingToMove) + L" - " + to_wstring(evKingDef));
			if (fecoPawnStructure)
				LogData(L"Pawn Structure " + to_wstring(evPawnToMove) + L" - " + to_wstring(evPawnDef));
			if (fecoTempo)
				LogData(L"Tempo " + SzFromEv(evTempo));
			if (fecoRandom)
				LogData(L"Random " + SzFromEv(evRandom / 100));
			LogData(L"Total " + SzFromEv(ev));
		}
	}

	return ev;
}


/*	PLAI::EvInterpolate
 *
 *	Interpolate the piece value evvaluation based on game phase
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
void PLAI::ComputeMpcpcev1(const BDG& bdg, EV mpcpcev[], const EV mpapcsqev[APC::ActMax][sqMax]) const noexcept
{
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow())
			mpcpcev[CPC::White] += mpapcsqev[apc][bb.sqLow().sqFlip()];
		for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow())
			mpcpcev[CPC::Black] += mpapcsqev[apc][bb.sqLow()];
	}
}


void PLAI::ComputeMpcpcev2(const BDG& bdg, EV mpcpcev1[], EV mpcpcev2[], 
						const EV mpapcsqev1[APC::ActMax][sqMax], 
						const EV mpapcsqev2[APC::ActMax][sqMax]) const noexcept
{
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow().sqFlip();
			mpcpcev1[CPC::White] += mpapcsqev1[apc][sq];
			mpcpcev2[CPC::White] += mpapcsqev2[apc][sq];
		}
		for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow();
			mpcpcev1[CPC::Black] += mpapcsqev1[apc][sq];
			mpcpcev2[CPC::Black] += mpapcsqev2[apc][sq];
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
	GPH gph = min(bdg.GphCur(), GPH::Max);	// can exceed Max with promotions

	/* opening */

	if (FInOpening(gph)) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevOpening);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	if (FInEndGame(gph)) {
		ComputeMpcpcev1(bdg, mpcpcev1, mpapcsqevEndGame);
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	if (gph <= GPH::MidMid) {
		ComputeMpcpcev2(bdg, mpcpcev1, mpcpcev2, mpapcsqevOpening, mpapcsqevMiddleGame);
		return EvInterpolate(gph, mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], GPH::MidMin,
							 mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], GPH::MidMid);
	}
	else {
		ComputeMpcpcev2(bdg, mpcpcev1, mpcpcev2, mpapcsqevMiddleGame, mpapcsqevEndGame);
		return EvInterpolate(gph, mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], GPH::MidMid,
								mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], GPH::MidMax);
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


void PLAI::InitWeightTable(const EV mpapcev[APC::ActMax], const EV mpapcsqdev[APC::ActMax][sqMax], EV mpapcsqev[APC::ActMax][sqMax])
{
	memset(&mpapcsqev[APC::Null], 0, sqMax * sizeof(EV));
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
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
	BB bbPawn = bdg.mppcbb[PC(cpc, APC::Pawn)];
	BB bbFile = bbFileA;
	for (int file = 0; file < fileMax; file++, bbFile <<= 1) {
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
	BB bbPawn = bdg.mppcbb[PC(cpc, APC::Pawn)];

	cfile += (bbPawn & bbFileA) && !(bbPawn & bbFileB);
	cfile += (bbPawn & bbFileH) && !(bbPawn & bbFileG);
	BB bbFile = bbFileB;
	BB bbAdj = bbFileA | bbFileC;
	for (int file = fileA+1; file < fileMax-1; file++, bbFile <<= 1, bbAdj <<= 1)
		cfile += (bbPawn & bbFile) && !(bbPawn & bbAdj);
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

	/* overextended pawns */

	/* backwards pawns */
	/* passed pawns */
	/* connected pawns */
	
	/* open and half-open files */

	return ev;
}


/*
 *
 *	PLAI2
 * 
 */


PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	fecoMaterial = 100;	
	fecoMobility = 0;	
	fecoKingSafety = 0;	
	fecoPawnStructure = 0;	
	fecoTempo = 100;
	fecoRandom = 10*100;	/* +/-10 centipawns */
	SetName(L"SQ Mathilda");
	InitWeightTables();
}


int PLAI2::PlyLim(const BDG& bdg, const GEMV& gemv) const
{
	static GEMV gemvOpp;
	bdg.GenGemvColor(gemvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gemv.cemv() * gemvOpp.cemv());
	int plyLim = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return plyLim;
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
		ga.PumpMsg();
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
 *	The list of players wew have available to play.
 * 
 */


RGINFOPL::RGINFOPL(void) 
{
	vinfopl.push_back(INFOPL(IDCLASSPL::AI, TPL::AI, L"SQ Mobly", 5));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI2, TPL::AI, L"SQ Mathilda", 3));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Rick Powell"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Hazel the Dog"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Allain de Leon"));
}


RGINFOPL::~RGINFOPL(void) 
{
}


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


