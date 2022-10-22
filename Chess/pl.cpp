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



PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName), level(3),
		mvNext(MV()), spmvNext(SPMV::Animate)
{
}


PL::~PL(void)
{
}


/*
 *	Evaluation manipulators and constants
 */

const int plyMax = 127;
const EV evInf = 640*100;	/* largest possible evaluation, in centi-pawns */
const EV evMate = evInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EV evMateMin = evMate - plyMax;
const EV evTempo = 33;	/* evaluation of a single move advantage */
const EV evDraw = 0;	/* evaluation of a draw */
const EV evAbort = evInf + 1;

inline EV EvMate(int ply)
{
	return evMate - ply;
}

inline bool FEvIsMate(EV ev)
{
	return ev >= evMateMin;
}

inline bool FEvIsAbort(EV ev)
{
	return ev == evAbort;
}

inline int PlyFromEvMate(EV ev)
{
	return evMate - ev;
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
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	LogData(L"Evaluated " + to_wstring(cemvEval) + L" boards");
	LogData(L"Nodes: " + to_wstring(cemvNode));
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cemvNode / (float)ms.count();
 	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" n/ms");
	LogClose(L"", L"", LGF::Normal);
}


/*
 *
 *	LOGOPENMVEV
 * 
 *	Little open/close log wrapper. Saves a bunch of state so we can automatically
 *	close the log using the destructor without explicitly calling it.
 * 
 */
class LOGOPENMVEV
{
	PL& pl;
	const BDG& bdg;
	const EMV& emv;
	wstring szData;
	LGF lgf;
	int depthLogSav;
	int imvExpandSav;
	const EV& evBest;

	static int imvExpand;
	static MV rgmv[8];

public:
	inline LOGOPENMVEV(PL& pl, const BDG& bdg, const EMV& emv, 
			const EV& evBest, EV evAlpha, EV evBeta, int ply, wchar_t chType=L' ') : 
		pl(pl), bdg(bdg), emv(emv), evBest(evBest), szData(L""), lgf(LGF::Normal)
	{
		depthLogSav = DepthLog();
		imvExpandSav = imvExpand;
		if (FExpandLog(emv))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(emv.mv), ATTR(L"FEN", bdg)),
				wstring(1, chType) + to_wstring(ply) + L" " + SzFromEv(/**/-emv.ev) + 
					L" [" + SzFromEv(evAlpha) + L", " + SzFromEv(evBeta) + L"] ");
	}

	inline void SetData(const wstring& szNew, LGF lgfNew)
	{
		szData = szNew;
		lgf = lgfNew;
	}

	inline ~LOGOPENMVEV()
	{
		LogClose(bdg.SzDecodeMvPost(emv.mv), 
				 szData.size() > 0 ? 
					szData : 
					SzFromEv(-evBest),	// negate this because the opening mvev.ev is relative to caller 
				 lgf);
		SetDepthLog(depthLogSav);
		imvExpand = imvExpandSav;
	}

	inline bool FExpandLog(const EMV& emv) const
	{
		if (emv.mv.sqFrom() != rgmv[imvExpand].sqFrom() ||	
				emv.mv.sqTo() != rgmv[imvExpand].sqTo() ||
				emv.mv.apcPromote() != rgmv[imvExpand].apcPromote())
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
int LOGOPENMVEV::imvExpand = 0;
MV LOGOPENMVEV::rgmv[] = { /*
	   MV(sqD2, sqD4, pcWhitePawn),
	   MV(sqD7, sqD6, pcBlackPawn),
	   MV(sqC1, sqG5, pcWhiteBishop),
	   MV(sqE7, sqE5, pcBlackPawn),
	   MV(sqG5, sqD8, pcWhiteBishop), */
	   MV()
};


/*	PLAI::FAlphaBetaPrune
 *
 *	Helper function to perform standard alpha-beta pruning operations after evaluating a
 *	board position. Keeps track of alpha and beta cut offs, the best move (in evBest and 
 *	pemvBest), and adjusts search depth when mates are found. mvev has the move and
 *	evaluation in it on entry. Returns true if we should prune.
 */
bool PLAI::FAlphaBetaPrune(EMV& emv, EV& evBest, EV& evAlpha, EV evBeta, const EMV*& pemvBest, int& plyLim) const noexcept
{
	if (emv.ev > evBest) {
		evBest = emv.ev;
		pemvBest = &emv;
	}
	if (emv.ev >= evBeta) 
		return true;
	if (emv.ev > evAlpha) {
		evAlpha = emv.ev;
		if (FEvIsMate(emv.ev))
			plyLim = PlyFromEvMate(emv.ev);
	}
	if (fAbort) {
		emv.ev = evBest = evAbort;
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
void PLAI::CheckForMates(BDG& bdg, EV& evBest, int cmvLegal, int ply) const noexcept
{
	if (cmvLegal == 0)
		evBest = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(ply) : evDraw;
	else if (bdg.GsTestGameOver(cmvLegal, *ga.prule) != GS::Playing)
		evBest = evDraw;
}


/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. Returns information for how the board 
 *	should display the move in spmv. 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	cYield = 0;
	fAbort = false;

	StartMoveLog();

	InitWeightTables();
	/* generate a random board hash value used to add randomness to eval */ 
	uniform_int_distribution<uint32_t> grfDist(0L, 0xffffffffUL);
	habdRand = HabdFromDist(rgen, grfDist);

	BDG bdg = ga.bdg;

	/* figure out how deep we're going to search */

	GEMV gemv;
	bdg.GenGemv(gemv, GG::Pseudo);
	int plyLim = clamp(PlyLim(bdg, gemv), 2, 14);
	LogData(L"Search depth: " + to_wstring(plyLim));

	/* pre-sort to improve quality of alpha-beta pruning */

	PreSortGemv(bdg, gemv);

	EV evAlpha = -evInf, evBeta = evInf;
	EV evBest = -evInf;
	const EMV* pemvBest = nullptr;

	for (EMV& emv : gemv) {
		MAKEMV makemv(bdg, emv.mv);
		if (bdg.FInCheck(~bdg.cpcToMove))
			continue;
		emv.ev = -EvBdgDepth(bdg, emv, 1, plyLim, -evBeta, -evAlpha);
		if (FAlphaBetaPrune(emv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			break;
	}
	
	EndMoveLog();
	return pemvBest == nullptr ? MV() : pemvBest->mv;
}


/*	PLAI::EvBdgDepth
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth depthMax; depth is current. Uses alpha-beta pruning.
 * 
 *	On entry emvPrev has the last move made. It will also contain pseudo-legal 
 *	moves for the next player to move (i.e., some moves may leave king in check).
 * 
 *	Returns board evaluation in centi-pawns from the point of view of the side with
 *	the move.
 */
EV PLAI::EvBdgDepth(BDG& bdg, const EMV& emvPrev, int ply, int plyLim, EV evAlpha, EV evBeta) noexcept
{
	if (ply >= plyLim)
		return EvBdgQuiescent(bdg, emvPrev, ply, evAlpha, evBeta);

	const EMV* pemvBest = nullptr;
	EV evBest = -evInf;

	cemvNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, emvPrev, evBest, evAlpha, evBeta, ply, L' ');
	PumpMsg();

	/* pre-sort the pseudo-legal moves in mvevEval */

	GEMV gemv;
	PreSortGemv(bdg, gemv);
	
	int cmvLegal = 0;

	for (EMV& emv : gemv) {
		MAKEMV makevmv(bdg, emv.mv);
		if (bdg.FInCheck(~bdg.cpcToMove))
			continue;
		cmvLegal++;
		emv.ev = -EvBdgDepth(bdg, emv, ply + 1, plyLim, -evBeta, -evAlpha);
		if (FAlphaBetaPrune(emv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			return evBest;
	}

	CheckForMates(bdg, evBest, cmvLegal, ply);
	return evBest;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the evaluation after the given board/last move from the point of view of the
 *	player next to move. Alpha-beta prunes. 
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, const EMV& emvPrev, int ply, EV evAlpha, EV evBeta) noexcept
{
	cemvNode++;

	const EMV* pemvBest = nullptr;
	EV evBest = -evInf;

	LOGOPENMVEV logopenmvev(*this, bdg, emvPrev, evBest, evAlpha, evBeta, ply, L'Q');
	PumpMsg();

	int cmvLegal = 0;
	int plyLim = plyMax;

	/* first off, get full, slow static eval and check current board already in a pruning 
	   situations */

	evBest = EvBdgStatic(bdg, emvPrev.mv, true);
	if (evBest >= evBeta)
		return evBest;
	if (evBest > evAlpha)
		evAlpha = evBest;

	/* don't bother pre-sorting moves during quiescent search because noisy moves are 
	   rare */

	GEMV gemv;
	FillGemv(bdg, gemv);

	/* recursively evaluate more moves */

	for (EMV& emv : gemv) {
		MAKEMV makemv(bdg, emv.mv);
		if (bdg.FInCheck(~bdg.cpcToMove))
			continue;
		cmvLegal++;
		if (bdg.FMvIsQuiescent(emv.mv))
			continue;
		emv.ev = -EvBdgQuiescent(bdg, emv, ply + 1, -evBeta, -evAlpha);
		if (FAlphaBetaPrune(emv, evBest, evAlpha, evBeta, pemvBest, plyLim))
			return evBest;
	}

	CheckForMates(bdg, evBest, cmvLegal, ply);
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


/*	PLAI::FillGemv
 *
 *	Fills the EMV array with the moves in gemv. Does not pre-sort the array.
 *	The emv's evaluation is not filled in.
 */
void PLAI::FillGemv(BDG& bdg, GEMV& gemv) noexcept
{
	bdg.GenGemv(gemv, GG::Pseudo);
}


/*	PLAI::PreSortGemv
 *
 *	Generates pseudo moves in from bdg,  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted EMV
 *	list is returned in gemv.
 */
void PLAI::PreSortGemv(BDG& bdg, GEMV& gemv) noexcept
{
	gemv.Clear();
	bdg.GenGemv(gemv, GG::Pseudo);
	for (EMV& emv : gemv) {
		MAKEMV makemv(bdg, emv.mv);
		emv.ev = /*-*/EvBdgStatic(bdg, emv.mv, false);
	}
#ifdef NO
	sort(gemv.begin(), gemv.end(), 
			 [](const EMV& emv1, const EMV& emv2) 
			 {
				 return emv1 > emv2; 
			 }
		);
#endif
	sort(gemv.begin(), gemv.end());
}


EV PLAI::EvTempo(const BDG& bdg, CPC cpc) const noexcept
{
	return cpc == bdg.cpcToMove ? evTempo : -evTempo;
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
			evTempo = EvTempo(bdg, bdg.cpcToMove);
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
			LogData(L"Random " + SzFromEv(evRandom/100));
		LogData(L"Total " + SzFromEv(ev));
	}

	return ev;
}


/*	PLAI::EvInterpolate
 *
 *	Interpolate the piece value evvaluation based on game phase
 */
EV PLAI::EvInterpolate(GPH gph, EV ev1, GPH gph1, EV ev2, GPH gph2) const noexcept
{
	/* careful - phases are a decreasing function */
	assert(gph1 > gph2);
	assert(gph >= gph2);
	assert(gph <= gph1);
	int dphase = static_cast<int>(gph1) - static_cast<int>(gph2);
	return (ev1 * (static_cast<int>(gph) - static_cast<int>(gph2)) +
			ev2 * (static_cast<int>(gph1) - static_cast<int>(gph)) +
			dphase / 2) / dphase;
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
	GPH gph = min(bdg.GphCur(), GPH::Max);

	/* opening */

	if (gph > GPH::Opening) {
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow().sqFlip();
				mpcpcev1[CPC::White] += mpapcsqevOpening[apc][sq];
			}
		}
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				mpcpcev1[CPC::Black] += mpapcsqevOpening[apc][sq];
			}
		}
		return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
	}

	/* opening to mid-game */

	if (gph > GPH::MidGame) {
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow().sqFlip();
				mpcpcev1[CPC::White] += mpapcsqevOpening[apc][sq];
				mpcpcev2[CPC::White] += mpapcsqevMiddleGame[apc][sq];
			}
		}
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				mpcpcev1[CPC::Black] += mpapcsqevOpening[apc][sq];
				mpcpcev2[CPC::Black] += mpapcsqevMiddleGame[apc][sq];
			}
		}
		return EvInterpolate(gph,
							 mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], GPH::Opening,
							 mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], GPH::MidGame);
	}

	/* mid-to-end game */

	if (gph > GPH::EndGame) {
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow().sqFlip();
				mpcpcev1[CPC::White] += mpapcsqevMiddleGame[apc][sq];
				mpcpcev2[CPC::White] += mpapcsqevEndGame[apc][sq];
			}
		}
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				mpcpcev1[CPC::Black] += mpapcsqevMiddleGame[apc][sq];
				mpcpcev2[CPC::Black] += mpapcsqevEndGame[apc][sq];
			}
		}
		return EvInterpolate(gph,
							 mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove], GPH::MidGame,
							 mpcpcev2[bdg.cpcToMove] - mpcpcev2[~bdg.cpcToMove], GPH::EndGame);
	}

	/* end game */

	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(CPC::White, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow().sqFlip();
			mpcpcev1[CPC::White] += mpapcsqevEndGame[apc][sq];
		}
	}
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		for (BB bb = bdg.mppcbb[PC(CPC::Black, apc)]; bb; bb.ClearLow()) {
			SQ sq = bb.sqLow();
			mpcpcev1[CPC::Black] += mpapcsqevEndGame[apc][sq];
		}
	}
	return mpcpcev1[bdg.cpcToMove] - mpcpcev1[~bdg.cpcToMove];
}



/*	PLAI::EvFromGphApcSq
 *
 *	Gets the square evaluation of square sq in game phase gph if piece apc is in the
 *	square. Uninterpolated. This should be used for in-game eval, it's only used for
 *	debugging purpose to show the piece value table heat maps.
 */
EV PLAI::EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept
{
	if (gph <= GPH::EndGame)
		return mpapcsqevEndGame[apc][sq];
	else if (gph <= GPH::MidGame)
		return mpapcsqevMiddleGame[apc][sq];
	else
		return mpapcsqevOpening[apc][sq];
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


