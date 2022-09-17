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

const EVAL evalInf = 1000*100;	/* largest possible evaluation */
const EVAL evalMate = evalInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EVAL evalMateMin = evalMate - 63;
const EVAL evalTempo = 33;	/* evaluation of a single move advantage */
const EVAL evalDraw = 0;	/* evaluation of a draw */
const EVAL evalQuiescent = evalInf + 1;


bool FEvalIsMate(EVAL eval)
{
	return eval >= evalMateMin;
}


wstring SzFromEval(EVAL eval)
{
	wchar_t sz[20], *pch = sz;
	if (eval >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		eval = -eval;
	}
	if (eval == evalInf)
		*pch++ = L'\x221e';
	else if (eval == evalQuiescent)
		*pch++ = L'Q';
	else if (eval > evalInf)
		*pch++ = L'*';
	else if (FEvalIsMate(eval)) {
		*pch++ = L'#';
		pch = PchDecodeInt((evalMate - eval + 1) / 2, pch);
	}
	else {
		pch = PchDecodeInt(eval / 100, pch);
		eval %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + eval / 10;
		*pch++ = L'0' + eval % 10;
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


PLAI::PLAI(GA& ga) : PL(ga, L"SQ Mobly"), cYield(0), cmvevEval(0), cmvevNode(0)
{
	rgfAICoeffNum[0] = 100;
	rgfAICoeffNum[1] = 20;
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
	this->level = peg(level, 1, 10);
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
	case 2: return 8.0e+3f;
	case 3: return 3.2e+4f;
	case 4: return 1.2e+5f;
	default:
	case 5: return 5.0e+5f;
	case 6: return 2.0e+6f;
	case 7: return 8.0e+6f;
	case 8: return 3.2e+7f;
	case 9: return 1.2e+8f;
	case 10: return 5.0e+8f;
	}
}

int PLAI::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}


/*	PLAI::StartMoveLog
 *
 *	Opens a log entry for AI move generation. Must be paired with a corresponding
 *	EndMoveLog
 */
void PLAI::StartMoveLog(void)
{
	tpStart = high_resolution_clock::now();
	cmvevEval = 0L;
	cmvevNode = 0L;
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
	LogData(L"Evaluated " + to_wstring(cmvevEval) + L" boards");
	LogData(L"Nodes: " + to_wstring(cmvevNode));
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cmvevNode / (float)ms.count();
 	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" n/ms");
	LogClose(L"", L"", LGF::Normal);
}


/*
 *
 *	LOGOPENMVEV
 * 
 *	Little open/close log wrapper
 * 
 */
class LOGOPENMVEV
{
	PL& pl;
	const BDG& bdg;
	MVEV& mvev;
	wstring szData;
	LGF lgf;
	int depthLogSav;
	int imvExpandSav;

	static int imvExpand;
	static MV rgmv[8];

public:
	inline LOGOPENMVEV(PL& pl, const BDG& bdg, MVEV& mvev, 
			EVAL evalAlpha, EVAL evalBeta, int depth, wchar_t chType=L' ') : 
		pl(pl), bdg(bdg), mvev(mvev), szData(L""), lgf(LGF::Normal)
	{
		depthLogSav = DepthLog();
		imvExpandSav = imvExpand;
		if (FExpandLog(mvev))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)),
				to_wstring(chType) + to_wstring(depth) + L" " + SzFromEval(mvev.eval) + 
					L" [" + SzFromEval(evalAlpha) + L", " + SzFromEval(evalBeta) + L"] ");
	}

	inline void SetData(const wstring& szNew, LGF lgfNew)
	{
		szData = szNew;
		lgf = lgfNew;
	}

	inline ~LOGOPENMVEV()
	{
		LogClose(bdg.SzDecodeMvPost(mvev.mv), 
			szData.size() > 0 ? szData : 
				SzFromEval(mvev.eval) /*+ L" (" + bdg.SzDecodeMvPost(mvev.mvReplyBest) + L")"*/,
			lgf);
		SetDepthLog(depthLogSav);
		imvExpand = imvExpandSav;
	}

	inline bool FExpandLog(MVEV& mvev) const
	{
		if (mvev.mv.sqFrom() != rgmv[imvExpand].sqFrom() ||	
				mvev.mv.sqTo() != rgmv[imvExpand].sqTo() ||
				mvev.mv.apcPromote() != rgmv[imvExpand].apcPromote())
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



/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. Returns information for how the board 
 *	should display the move in spmv. 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	cYield = 0;

	StartMoveLog();

	MV mvBest;
	EVAL evalAlpha = -evalInf, evalBeta = evalInf;
	int depthMax;

	GMV gmv;
	BDG bdg = ga.bdg;
	vector<MVEV> vmvev;
	const MVEV* pmvevBest = nullptr;

	/* figure out how deep we're going to search */

	bdg.GenGmv(gmv, GG::Pseudo);
	depthMax = peg(DepthMax(bdg, gmv), 2, 14);
	LogData(L"Search depth: " + to_wstring(depthMax));

	/* pre-sort to improve quality of alpha-beta pruning */

	PreSortVmvev(bdg, gmv, vmvev);
		
	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makemv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			EVAL eval = -EvalBdgDepth(bdg, mvev, 1, depthMax, -evalBeta, -evalAlpha);
			if (eval > evalAlpha) {
				evalAlpha = eval;
				pmvevBest = &mvev;
			}
		}
	}
	catch (...) {
		/* force aborted games to return nil */
		pmvevBest = nullptr;
	}

	EndMoveLog();
	return pmvevBest == nullptr ? MV() : pmvevBest->mv;
}


/*	PLAI::EvalBdgDepth
 *
 *	Evaluates the board/move from the point of view of the person who just moved,
 *	Evaluates to the target depth depthMax; depth is current. Uses alpha-beta pruning.
 * 
 *	On entry mvevLast has the move we're evaluating. It will also contain pseudo-legal 
 *	moves for the next player to move (i.e., some moves may leave king in check).
 * 
 *	Returns board evaluation in centi-pawns from the point of view of the side that
 *	made the last move.
 */
EVAL PLAI::EvalBdgDepth(BDG& bdg, MVEV& mvevLast, int depth, int depthMax, EVAL evalAlpha, EVAL evalBeta)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdg, mvevLast, depth, evalAlpha, evalBeta);

	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evalAlpha, evalBeta, depth, L' ');
	PumpMsg();

	/* pre-sort the pseudo-legal moves in mvevEval */
	vector<MVEV> vmvev;
	PreSortVmvev(bdg, mvevLast.gmvPseudoReply, vmvev);
	int cmvLegal = 0;

	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makevmv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			cmvLegal++;
			EVAL eval = -EvalBdgDepth(bdg, mvev, depth + 1, depthMax, -evalBeta, -evalAlpha);
			if (eval >= evalBeta)
				return mvevLast.eval = evalBeta;
			if (eval > evalAlpha)
				evalAlpha = eval;
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* test for checkmates, stalemates, and other draws */

	if (cmvLegal == 0) 
		evalAlpha = bdg.FInCheck(bdg.cpcToMove) ? -(evalMate - depth) : evalDraw;
	else if (bdg.GsTestGameOver(mvevLast.gmvPseudoReply, *ga.prule) != GS::Playing)
		evalAlpha = evalDraw;
	return mvevLast.eval = evalAlpha;
}


/*	PLAI::EvalBdgQuiescent
 *
 *	Returns the evaluation after the given board/last move from the point of view of the
 *	player that made the last move. Alpha-beta prunes. 
 */
EVAL PLAI::EvalBdgQuiescent(BDG& bdg, MVEV& mvevLast, int depth, EVAL evalAlpha, EVAL evalBeta)
{
	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evalAlpha, evalBeta, depth, L'Q');
	PumpMsg();

	int cmvLegal = 0;

	/* first off, check current board already in a pruning situations and fill mvevEval 
	   with full static eval */

	EVAL eval = -EvalBdgStatic(bdg, mvevLast, true);
	if (eval >= evalBeta)
		return mvevLast.eval = evalBeta;
	if (eval > evalAlpha)
		evalAlpha = eval;

	/* don't bother pre-sorting moves during quiescent search because noisy moves are 
	   rare */

	vector<MVEV> vmvev;
	FillVmvev(bdg, mvevLast.gmvPseudoReply, vmvev);

	try {
		/* recursively evaluate more moves */

		for (MVEV& mvev : vmvev) {
			MAKEMV makemv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			cmvLegal++;
			if (bdg.FMvIsQuiescent(mvev.mv))
				continue;
			bdg.GenGmv(mvev.gmvPseudoReply, GG::Pseudo);	// these aren't generated by FillVmvev
			EVAL eval = -EvalBdgQuiescent(bdg, mvev, depth + 1, -evalBeta, -evalAlpha);
			if (eval >= evalBeta)
				return mvevLast.eval = evalBeta;
			if (eval > evalAlpha)
				evalAlpha = eval;
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* checkmate and stalemate - checking for repeat draws during quiescent search 
	   doesn't work because we don't evaluate every move */

	if (cmvLegal == 0)
		evalAlpha = bdg.FInCheck(bdg.cpcToMove) ? -(evalMate - depth) : evalDraw;

	return mvevLast.eval = evalAlpha;
}


void PLAI::PumpMsg(void)
{
	if (++cYield % dcYield == 0)
		ga.PumpMsg();
}


/*	PLAI::FillVmvev
 *
 *	Fills the BDGMV array with the moves in gmv. Does not pre-sort the array.
 *	The mvev's evaluation and reply all list are not filled in.
 */
void PLAI::FillVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept
{
	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(gmv[imv]);
		vmvev.push_back(move(mvev));
	}
}


/*	PLAI::PreSortVmvev
 *
 *	Using generated moves in gmv (generated from bdg),  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted BDGMV
 *	list is returned in vbdgmv.
 */
void PLAI::PreSortVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept
{
	/* insertion sort */

	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(gmv[imv]);
		bdg.MakeMv(mvev.mv);
		bdg.GenGmv(mvev.gmvPseudoReply, GG::Pseudo);
		mvev.eval = EvalBdgStatic(bdg, mvev, false);
		bdg.UndoMv();
		size_t imvFirst, imvLim;
		for (imvFirst = 0, imvLim = vmvev.size(); imvFirst != imvLim; ) {
			size_t imvMid = (imvFirst + imvLim) / 2;
			assert(imvMid < imvLim);
			if (vmvev[imvMid].eval < mvev.eval)
				imvLim = imvMid;
			else
				imvFirst = imvMid+1;
		}
		vmvev.insert(vmvev.begin() + imvFirst, move(mvev));
	}
}

EVAL PLAI::EvalTempo(const BDG& bdg, CPC cpc) const noexcept
{
	return cpc == bdg.cpcToMove ? evalTempo : -evalTempo;
}


/*	PLAI::EvalBdgAttackDefend
 *
 *	Little heuristic for board evaluation that tries to detect bad moves, which are
 *	if we have moved ot an attacked square that isn't defended. This is only useful
 *	for pre-sorting alpha-beta pruning, because it's somewhat more accurate, but it's 
 *	good enough for real static board evaluation. Someday, a better heuristic would
 *	be to exchange until quiescence.
 * 
 *	The board should already have the move made on it. Destination square of the move 
 *	is sqTo. Returns the amount to adjust the evaluation by.
 */
EVAL PLAI::EvalBdgAttackDefend(BDG& bdg, SQ sqTo) const noexcept
{
	APC apcMove = bdg.ApcFromSq(sqTo);
	APC apcAttacker = bdg.ApcSqAttacked(sqTo, bdg.cpcToMove);
	if (apcAttacker != APC::Null) {
		if (apcAttacker < apcMove)
			return -EvalBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(sqTo, ~bdg.cpcToMove);
		if (apcDefended == APC::Null)
			return -EvalBaseApc(apcMove);
	}
	return 0;
}


/*	PLAI::EvalBdgStatic
 *
 *	Evaluates the board from the point of view of the color that just moved.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
EVAL PLAI::EvalBdgStatic(BDG& bdg, MVEV& mvev, bool fFull) noexcept
{
	EVAL evalMat = EvalPstFromCpc(bdg, ~bdg.cpcToMove);
	if (!fFull)
		evalMat += EvalBdgAttackDefend(bdg, mvev.mv.sqTo());

	static GMV gmvSelf;
	bdg.GenGmvColor(gmvSelf, ~bdg.cpcToMove);
	EVAL evalMob = 20*(gmvSelf.cmv() - mvev.gmvPseudoReply.cmv());
	cmvevEval++;
	EVAL eval = (rgfAICoeffNum[0] * evalMat + rgfAICoeffNum[1] * evalMob + 50) / 100 + EvalTempo(bdg, ~bdg.cpcToMove);

	if (fFull) {
		cmvevEval++;
		LogData(L"Mobility " + to_wstring(gmvSelf.cmv()) + L"-" + to_wstring(mvev.gmvPseudoReply.cmv()));
		LogData(L"Material " + SzFromEval(evalMat));
		LogData(L"Total " + SzFromEval(eval));
	}

	return eval;
}

enum PHASE {
	phaseMax = 24,
	phaseOpening = 22,
	phaseMid = 20,
	phaseEnd = 6
};


/*	PLAI:EvalPstFromCpc
 *
 *	Returns the board evaluation for the given color. 
 */
EVAL PLAI::EvalPstFromCpc(const BDG& bdg, CPC cpcEval) const noexcept
{
	static const int mpapcphase[APC::ActMax] = { 0, 0, 1, 1, 2, 4, 0 };
	int phase = 0;
	EVAL mpcpcevalOpening[2] = { 0, 0 };
	EVAL mpcpcevalMid[2] = { 0, 0 };
	EVAL mpcpcevalEnd[2] = { 0, 0 };

	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		phase += bdg.mppcbb[PC(CPC::White, apc)].csq() * mpapcphase[apc];
		phase += bdg.mppcbb[PC(CPC::Black, apc)].csq() * mpapcphase[apc];
		for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
			for (BB bb = bdg.mppcbb[PC(cpc, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				if (cpc == CPC::White)
					sq = sq ^ SQ(rankMax - 1, 0);
				mpcpcevalOpening[cpc] += mpapcsqevalOpening[apc][sq];
				mpcpcevalMid[cpc] += mpapcsqevalMiddleGame[apc][sq];
				mpcpcevalEnd[cpc] += mpapcsqevalEndGame[apc][sq];
			}
		}
	}
	phase = min(phase, phaseMax);

	if (phase > phaseOpening)
		return mpcpcevalOpening[cpcEval] - mpcpcevalOpening[~cpcEval];
	
	if (phase > phaseMid)
		return EvalInterpolate(phase, 
			mpcpcevalOpening[cpcEval] - mpcpcevalOpening[~cpcEval], phaseOpening,
			mpcpcevalMid[cpcEval] - mpcpcevalMid[~cpcEval], phaseMid);
		
	if (phase > phaseEnd)
		return EvalInterpolate(phase, 
			mpcpcevalMid[cpcEval] - mpcpcevalMid[~cpcEval], phaseMid, 
			mpcpcevalEnd[cpcEval] - mpcpcevalEnd[~cpcEval], phaseEnd); 
	
	return mpcpcevalEnd[cpcEval] - mpcpcevalEnd[~cpcEval];
}


EVAL PLAI::EvalInterpolate(int phase, EVAL eval1, int phase1, EVAL eval2, int phase2) const noexcept
{
	/* careful - phases are a decreasing function */
	assert(phase1 > phase2);
	assert(phase >= phase2);
	assert(phase <= phase1);
	int dphase = phase1 - phase2;
	return (eval1 * (phase - phase2) + eval2 * (phase1 - phase) + dphase/2) / dphase;
}


/* these values are taken from "simplified evaluation function */
const EVAL mpapcevalOpening[APC::ActMax] = { 0, 100, 320, 330, 500, 900, 200 };
const EVAL mpapcsqdevalOpening[APC::ActMax][sqMax] = {
	{0, 0, 0, 0, 0, 0, 0, 0,	// APC::Null
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{  0,   0,   0,   0,   0,   0,   0,   0,	// Pawn
	  50,  50,  50,  50,  50,  50,  50,  50,
	  20,  20,  20,  20,  20,  20,  20,  20,
	  -5,   0,   5,   5,   5,   5,   0,  -5,
	   0,   0,  15,  25,  25,   0,   0,   0,
	   5,   5,   5,  10,  10,   0,   5,   5,
	   5,   5,  -5, -25, -25,  10,   5,   5,
	   0,   0,   0,   0,   0,   0,   0,   0},
	{-50, -40, -30, -30, -30, -30, -40, -50,	// Knight
	 -40, -20,   0,   0,   0,   0, -20, -40,
	 -30,   0,  10,  15,  15,  10,   0, -30,
	 -30,   0,  15,  20,  20,  15,   0, -30,
	 -30,   0,  15,  20,  20,  15,   0, -30,
	 -30,   0,  10,  15,  15,  10,   0, -30,
	 -40, -20,  0,    0,   0,   0, -20, -40,
	 -50, -40, -30, -30, -30, -30, -40, -50},
	{-20, -10, -10, -10, -10, -10, -10, -20,	// Bishop
	 -10,   0,   0,   0,   0,   0,   0, -10,
	 -10,   0,   5,  10,  10,   5,   0, -10,
	 -10,   5,   5,  10,  10,   5,   5, -10,
	 -10,   0,  10,  10,  10,  10,   0, -10,
	 -10,  10,  10,  10,  10,  10,  10, -10,
	 -10,  10,   0,   0,   0,   0,  10, -10,
	 -20, -10, -10, -10, -10, -10, -10, -20},
	{  0,   0,   0,   0,   0,   0,   0,   0,	// Rook
	   0,   0,   0,   0,   0,   0,   0,   0,
	  -5,   0,   0,   0,   0,   0,   0,  -5,
	  -5,   0,   0,   0,   0,   0,   0,  -5,
	  -5,   0,   0,   0,   0,   0,   0,  -5,
	  -5,   0,   0,   0,   0,   0,   0,  -5,
	   5,   5,   5,   5,   5,   5,   5,   5,
	   5,   5,   5,  10,   5,  10,   5,   5},
	{-20, -10, -10,  -5,  -5, -10, -10, -20,	// Queen
	 -10,   0,   5,   5,   5,   5,   0, -10,
	 -10,  -5, -10, -10, -10, -10,  -5, -10,
	  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
	  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
	  -5,   0,   0,   0,   0,   0,   0,  -5,
	   0,  10,  10,  10,  10,  10,  10,   0,
	 -10, -10, -10,   0,   0, -10, -10, -10},
	{-30, -40, -40, -50, -50, -40, -40, -30,	// King
	 -30, -40, -40, -50, -50, -40, -40, -30,
	 -30, -40, -40, -50, -50, -40, -40, -30,
	 -30, -40, -40, -50, -50, -40, -40, -30,
	 -20, -30, -30, -40, -40, -30, -30, -20,
	 -10, -20, -30, -30, -30, -30, -20, -10,
	  20,  20,   0,  -5,  -5,   0,  20,  20,
	  20,  30,  10,  -5,  -5,  10,  35,  25}
};


/* these constants are taken from pesto evaluator */
const EVAL mpapcevalMiddleGame[APC::ActMax] = { 0, 82, 337, 365, 477, 1025, 200 };
const EVAL mpapcsqdevalMiddleGame[APC::ActMax][sqMax] = {
	{0, 0, 0, 0, 0, 0, 0, 0,	// Null
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{   0,   0,   0,   0,   0,   0,   0,    0,	// Pawn
	   98, 134,  61,  95,  68, 126,  34,  -11,
	   -6,   7,  26,  31,  65,  56,  25,  -20,
	  -14,  13,   6,  21,  23,  12,  17,  -23,
	  -27,  -2,  -5,  12,  17,   6,  10,  -25,
	  -26,  -4,  -4, -10,   3,   3,  33,  -12,
	  -35,  -1, -20, -23, -15,  24,  38,  -22,
	    0,   0,   0,   0,   0,   0,   0,    0},
	{-167, -89, -34, -49,  61, -97, -15, -107,	// Knight
	  -73, -41,  72,  36,  23,  62,   7,  -17,
	  -47,  60,  37,  65,  84, 129,  73,   44,
	   -9,  17,  19,  53,  37,  69,  18,   22,
	  -13,   4,  16,  13,  28,  19,  21,   -8,
	  -23,  -9,  12,  10,  19,  17,  25,  -16,
	  -29, -53, -12,  -3,  -1,  18, -14,  -19,
	 -105, -21, -58, -33, -17, -28, -19,  -23},
	 {-29,   4, -82, -37, -25, -42,   7,   -8,	// Bishop
	  -26,  16, -18, -13,  30,  59,  18,  -47,
	  -16,  37,  43,  40,  35,  50,  37,   -2,
	   -4,   5,  19,  50,  37,  37,   7,   -2,
	   -6,  13,  13,  26,  34,  12,  10,    4,
	    0,  15,  15,  15,  14,  27,  18,   10,
	    4,  15,  16,   0,   7,  21,  33,    1,
	  -33,  -3, -14, -21, -13, -12, -39,  -21},
	 { 32,  42,  32,  51,  63,   9,  31,   43,	// Rook
	   27,  32,  58,  62,  80,  67,  26,   44,
	   -5,  19,  26,  36,  17,  45,  61,   16,
	  -24, -11,   7,  26,  24,  35,  -8,  -20,
	  -36, -26, -12,  -1,   9,  -7,   6,  -23,
	  -45, -25, -16, -17,   3,   0,  -5,  -33,
	  -44, -16, -20,  -9,  -1,  11,  -6,  -71,
	  -19, -13,   1,  17,  16,   7, -37,  -26},	
	 {-28,   0,  29,  12,  59,  44,  43,   45,	// Queen
	  -24, -39,  -5,   1, -16,  57,  28,   54,
	  -13, -17,   7,   8,  29,  56,  47,   57,
	  -27, -27, -16, -16,  -1,  17,  -2,    1,
	   -9, -26,  -9, -10,  -2,  -4,   3,   -3,
	  -14,   2, -11,  -2,  -5,   2,  14,    5,
	  -35,  -8,  11,   2,   8,  15,  -3,    1,
	   -1, -18,  -9,  10, -15, -25, -31,  -50},
	 {-65,  23,  16, -15, -56, -34,   2,   13,	// King
	   29,  -1, -20,  -7,  -8,  -4, -38,  -29,
	   -9,  24,   2, -16, -20,   6,  22,  -22,
	  -17, -20, -12, -27, -30, -25, -14,  -36,
	  -49,  -1, -27, -39, -46, -44, -33,  -51,
	  -14, -14, -22, -46, -44, -30, -15,  -27,
		1,   7,  -8, -64, -43, -16,   9,    8,
	  -15,  36,  12, -54,   8, -28,  24,   14}
};

/* these constants are taken from pesto evaluator */
const EVAL mpapcevalEndGame[APC::ActMax] = { 0, 94, 281, 297, 512, 936, 200 };
const EVAL mpapcsqdevalEndGame[APC::ActMax][sqMax] = {
	{0, 0, 0, 0, 0, 0, 0, 0,	// Null
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{  0,   0,   0,   0,   0,   0,   0,   0,	// Pawn
	 178, 173, 158, 134, 147, 132, 165, 187,
	  94, 100,  85,  67,  56,  53,  82,  84,
	  32,  24,  13,   5,  -2,   4,  17,  17,
	  13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	   4,   7,  -6,   1,   0,  -5,  -1,  -8,
	  13,   8,   8,  10,  13,   0,   2,  -7,
	   0,   0,   0,   0,   0,   0,   0,   0},
	{-58, -38, -13, -28, -31, -27, -63, -99,	// Knight
	 -25,  -8, -25,  -2,  -9, -25, -24, -52,
	 -24, -20,  10,   9,  -1,  -9, -19, -41,
	 -17,   3,  22,  22,  22,  11,   8, -18,
	 -18,  -6,  16,  25,  16,  17,   4, -18,
	 -23,  -3,  -1,  15,  10,  -3, -20, -22,
	 -42, -20, -10,  -5,  -2, -20, -23, -44,
	 -29, -51, -23, -15, -22, -18, -50, -64},
	{-14, -21, -11,  -8,  -7,  -9, -17, -24,	// Bishop
	  -8,  -4,   7, -12,  -3, -13,  -4, -14,
	   2,  -8,   0,  -1,  -2,   6,   0,   4,
	  -3,   9,  12,   9,  14,  10,   3,   2,
	  -6,   3,  13,  19,   7,  10,  -3,  -9,
	 -12,  -3,   8,  10,  13,   3,  -7, -15,
	 -14, -18,  -7,  -1,   4,  -9, -15, -27,
	 -23,  -9, -23,  -5,  -9, -16,  -5, -17},
	{ 13,  10,  18,  15,  12,  12,   8,   5,	// Rook
	  11,  13,  13,  11,  -3,   3,   8,   3,
	   7,   7,   7,   5,   4,  -3,  -5,  -3,
	   4,   3,  13,   1,   2,   1,  -1,   2,
	   3,   5,   8,   4,  -5,  -6,  -8, -11,
	  -4,   0,  -5,  -1,  -7, -12,  -8, -16,
	  -6,  -6,   0,   2,  -9,  -9, -11,  -3,
	  -9,   2,   3,  -1,  -5, -13,   4, -20},
	{ -9,  22,  22,  27,  27,  19,  10,  20,	// Queen
	 -17,  20,  32,  41,  58,  25,  30,   0,
	 -20,   6,   9,  49,  47,  35,  19,   9,
	   3,  22,  24,  45,  57,  40,  57,  36,
	 -18,  28,  19,  47,  31,  34,  39,  23,
	 -16, -27,  15,   6,   9,  17,  10,   5,
	 -22, -23, -30, -16, -16, -23, -36, -32,
	 -33, -28, -22, -43,  -5, -32, -20, -41},
	{-74, -35, -18, -18, -11,  15,   4, -17,	// King
	 -12,  17,  14,  17,  17,  38,  23,  11,
	  10,  17,  23,  15,  20,  45,  44,  13,
	  -8,  22,  24,  27,  26,  33,  26,   3,
	 -18,  -4,  21,  24,  27,  23,   9, -11,
	 -19,  -3,  11,  21,  23,  16,   7,  -9,
	 -27, -11,   4,  13,  14,   4,  -5, -17,
	 -53, -34, -21, -11, -28, -14, -24, -43}
};
		

EVAL PLAI::EvalBaseApc(APC apc) const noexcept
{
	EVAL mpapceval[APC::ActMax] = { 0, 100, 275, 300, 500, 900, 200 };
	return mpapceval[apc];
}


void PLAI::InitWeightTables(void)
{
	InitWeightTable(mpapcevalOpening, mpapcsqdevalOpening, mpapcsqevalOpening);
	InitWeightTable(mpapcevalMiddleGame, mpapcsqdevalMiddleGame, mpapcsqevalMiddleGame);
	InitWeightTable(mpapcevalEndGame, mpapcsqdevalEndGame, mpapcsqevalEndGame);
}

void PLAI::InitWeightTable(const EVAL mpapceval[APC::ActMax], const EVAL mpapcsqdeval[APC::ActMax][sqMax], EVAL mpapcsqeval[APC::ActMax][sqMax])
{
	memset(&mpapcsqeval[APC::Null], 0, sqMax * sizeof(EVAL));
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
		for (uint64_t sq = 0; sq < sqMax; sq++)
			mpapcsqeval[apc][sq] = mpapceval[apc] + mpapcsqdeval[apc][sq];
}


/*
 *
 *	PLAI2
 * 
 */


PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	rgfAICoeffNum[0] = 100;
	rgfAICoeffNum[1] = 100;
	SetName(L"SQ Mathilda");
}


int PLAI2::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}


EVAL PLAI2::EvalBdgStatic(BDG& bdg, MVEV& mvev, bool fFull) noexcept
{
	EVAL evalMat = EvalPstFromCpc(bdg, ~bdg.cpcToMove);
	if (!fFull)
		evalMat += EvalBdgAttackDefend(bdg, mvev.mv.sqTo());

	if (fFull) {
		evalMat += 1;	// just to make full evals different than partials so we can tell them apart when debugging
		cmvevEval++;
	}

	EVAL eval = evalMat + EvalTempo(bdg, ~bdg.cpcToMove);

	if (fFull) {
		LogData(L"Material " + SzFromEval(evalMat));
		LogData(L"Total " + SzFromEval(eval));
	}
	return eval;
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