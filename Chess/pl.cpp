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

const EVAL evalInf = 10000 * 100;	/* largest possible evaluation */
const EVAL evalMate = evalInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EVAL evalMateMin = evalMate - 100;
const EVAL evalTempo = 33;	/* evaluation of a single move advantage */
const EVAL evalDraw = 0;	/* evaluation of a draw */


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
	if (FEvalIsMate(eval)) {
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

/*
 *
 *	PLAI and PLAI2
 * 
 *	AI computer players. Base PLAI implements alpha-beta pruned look ahead tree.
 * 
 */


PLAI::PLAI(GA& ga) : PL(ga, L"SQ Mobly"), cYield(0), cmvevEval(0), cmvevGen(0), cmvevPrune(0)
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
	case 1: return 500.0f;
	case 2: return 2000.0f;
	case 3: return 1.0e+5f;
	case 4: return 1.0e+6f;
	case 5: return 2.0e+6f;
	case 6: return 5.0e+6f;
	case 7: return 8.0e+6f;
	case 8: return 1.0e+7f;
	case 9: return 2.0e+7f;
	case 10: return 5.0e+7f;
	default:
		return 1.0e+6f;
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
	cmvevPrune = 0L;
	LogOpen(TAG(ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", ATTR(L"FEN", ga.bdg)),
		L"(" + szName + L")");
}


/*	PLAI::EndMoveLog
 *
 *	Closes the move generation logging entry for A?I move generation.
 */
void PLAI::EndMoveLog(void)
{
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	LogData(L"Evaluated " + to_wstring(cmvevEval) + L" boards");
	LogData(L"Pruned: " + to_wstring((int)roundf(100.f * (float)cmvevPrune / (float)cmvevGen)) + L"%");
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cmvevEval / (float)ms.count();
	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" nodes/ms");
	LogClose(L"", L"", LGF::Normal);
}


/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. Returns information for how the board 
 *	should display the move in spmv. 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;

	StartMoveLog();

	/* generate all moves without removing checks. we'll use this as a heuristic for the 
	   amount of mobillity on the board, which we can use to estimate the depth we can 
	   search */

	BDG bdg = ga.bdg;
	GMV gmv;
	bdg.GenGmv(gmv, RMCHK::NoRemove);
	if (gmv.cmv() == 0) {
		EndMoveLog();
		return MV();
	}
	int depthMax = peg(DepthMax(bdg, gmv), 2, 13);
	LogData(L"Search depth: " + to_wstring(depthMax));
	
	/* generate all legal moves and do a quick pre-sort on them to improve quality
	   of our alpha-beta pruning */

	bdg.GenGmv(gmv, RMCHK::Remove);
	vector<MVEV> vmvev;
	PreSortVmvev(bdg, gmv, vmvev);

	cYield = 0;
	MV mvBest;
	EVAL evalAlpha = -evalInf, evalBeta = evalInf;
	cmvevGen = vmvev.size();

	for (MVEV& mvev : vmvev) {

		/* recursively evaluate the move to target depth */

		bdg.MakeMv(mvev.mv);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)), SzFromEval(mvev.eval));
		EVAL eval;
		try {
			eval = -EvalBdgDepth(bdg, mvev, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		}
		catch (...) {
			LogClose(bdg.SzDecodeMvPost(mvev.mv), L"(interrupted)", LGF::Italic);
			if (mvBest.fIsNil()) {
				ga.bdg.RemoveInCheckMoves(gmv, ga.bdg.cpcToMove);
				mvBest = gmv[0];
			}
			break;
		}
		bdg.UndoMv();

		/* keep track of best move */

		LGF lgf = LGF::Normal;
		if (eval > evalAlpha) {
			lgf = LGF::Bold;
			evalAlpha = eval;
			mvBest = mvev.mv;
			if (FEvalIsMate(eval))
				depthMax = min(depthMax, evalMate - eval);
		}
		
		LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval), lgf);
	}

	EndMoveLog();

	/* and return the best move */

	assert(!mvBest.fIsNil());
	return mvBest;
}


/*	PLAI::EvalBdgDepth
 *
 *	Evaluates the board from the point of view of the person who last moved,
 *	i.e., the previous move. Evaluates to the target depth depthMax; current depth
 *	is depth. Uses alpha-beta pruning.
 * 
 *	On entry mvevEval has the move we're evaluating. It will also contain pseudo-legal 
 *	moves for the next player to move (i.e., some moves may leave king in check).
 * 
 *	Returns board evaluation in centi-pawns.
 */
EVAL PLAI::EvalBdgDepth(BDG& bdg, MVEV& mvevEval, int depth, int depthMax, EVAL evalAlpha, EVAL evalBeta, const RULE& rule)
{
	/* if we've reached target depth, switch over to quiescent search */

	if (depth >= depthMax)
		return EvalBdgQuiescent(bdg, mvevEval, depth, evalAlpha, evalBeta);

	/* keep UI messages pumpin' */

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	/* pre-sort the pseudo-legal moves in mvevEval */

	vector<MVEV> vmvev;
	PreSortVmvev(bdg, mvevEval.gmvReplyAll, vmvev);

	/* keep track of stats, number of legal moves we've evaluated, and the best move */

	int cmvLegal = 0;
	cmvevGen += vmvev.size();
	EVAL evalBest = -evalInf;

	for (MVEV& mvev: vmvev) {
		
		/* make move and make sure it's legal */

		bdg.MakeMv(mvev.mv);
		if (bdg.FInCheck(~bdg.cpcToMove)) {
			bdg.UndoMv();
			continue;
		}
		cmvLegal++;

		/* recursively evaluate the move to target depth, using alpha-beta pruning */

		LGF lgf = LGF::Normal;
		EVAL eval;
		try {
			LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", (wstring)bdg)), SzFromEval(mvev.eval));
			eval = -EvalBdgDepth(bdg, mvev, depth + 1, depthMax, -evalBeta, -evalAlpha, rule);
			bdg.UndoMv();

			/* do our alpha-beta pruning */

			if (eval >= evalBeta) {
				cmvevPrune += vmvev.size() - cmvLegal;
				LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval) + L" (Pruning)", lgf);
				return eval;
			}
			if (eval > evalBest) {
				evalBest = eval;
				if (eval > evalAlpha) {
					evalAlpha = eval;
					if (FEvalIsMate(eval))
						depthMax = min(depthMax, evalMate - eval);
					lgf = LGF::Bold;
				}
			}
			LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval), lgf);
		}
		catch (...) {
			bdg.UndoMv();
			LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval) + L" (interrupted)", LGF::Italic);
			throw;
		}
	}

	/* if no legal moves, we either have checkmate or stalemate */

	if (cmvLegal == 0) {
		if (bdg.FInCheck(bdg.cpcToMove))
			return -(evalMate - depth);
		else
			return evalDraw;
	}

	/* checkmates were already detected, so GsTestGameOver can only find our more
	   obscure draw situations */

	GS gs = bdg.GsTestGameOver(mvevEval.gmvReplyAll, *ga.prule);
	assert(gs != GS::BlackCheckMated && gs != GS::WhiteCheckMated);
	if (gs != GS::Playing)
		return evalDraw;

	return evalBest;
}


/*	PLAI::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the 
 *	previous move player, i.e., it evaluates the previous move.
 */
EVAL PLAI::EvalBdgQuiescent(BDG& bdg, MVEV& mvevEval, int depth, EVAL evalAlpha, EVAL evalBeta)
{
	/* we need to evaluate the board before we remove moves from the move list, 
	   because the un-processed move list is used to compute mobility */

	EVAL eval = EvalBdg(bdg, mvevEval, true);
	
	/* check for mates and stalemates */

	bdg.RemoveInCheckMoves(mvevEval.gmvReplyAll, bdg.cpcToMove);
	if (mvevEval.gmvReplyAll.cmv() == 0) {
		if (bdg.FInCheck(bdg.cpcToMove))
			return -(evalMate - (EVAL)depth);
		else
			return 0;	
	}

	/* remove any "quiet" moves; once only quiet moves are left, we can
	   safely return the board evaluation */

	bdg.RemoveQuiescentMoves(mvevEval.gmvReplyAll, bdg.cpcToMove);
	if (mvevEval.gmvReplyAll.cmv() == 0) {
		LogData(L"Total " + SzFromEval(eval));
		return -eval;
	}

	/* keep messages a-pumpin' */

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	/* only noisy moves are left. there shouldn't be very many of them, so don't
	   bother with sorting */

	vector<MVEV> vmvev;
	FillVmvev(bdg, mvevEval.gmvReplyAll, vmvev);
	
	/* keepin track of stats and best move */

	cmvevGen += vmvev.size();
	EVAL evalBest = -evalInf;
	int cmv = 0;

	for (MVEV& mvev : vmvev) {
	
		/* recursively evaluate the quiescent move */

		bdg.MakeMv(mvev.mv);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)), SzFromEval(mvev.eval));
		EVAL eval = -EvalBdgQuiescent(bdg, mvev, depth + 1, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		cmv++;

		/* keep track of best continuation and alpha-beta prune */

		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cmvevPrune += vmvev.size() - cmv;
			LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval) + L" (Pruning)", LGF::Normal);
			return eval;
		}
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				lgf = LGF::Bold;
			}
		}
		LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval), lgf);
	}

	return evalBest;
}


/*	PLAI::FillVmvev
 *
 *	Fills the BDGMV array with the moves in gmv. Does not pre-sort the array.
 */
void PLAI::FillVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev)
{
	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(bdg, gmv[imv]);
		mvev.eval = EvalBdg(bdg, mvev, false);
		bdg.UndoMv();
		vmvev.push_back(move(mvev));
	}
}


/*	PLAI::PreSortVmvev
 *
 *	Using generated moves in gmv (generated from bdg),  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted BDGMV
 *	list is returned in vbdgmv.
 */
void PLAI::PreSortVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev)
{
	/* insertion sort */

	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(bdg, gmv[imv]);
		mvev.eval = EvalBdg(bdg, mvev, false);
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

inline EVAL EvalTempo(CPC cpc)
{
	return cpc == CPC::White ? evalTempo : -evalTempo;
}


/*	PLAI::EvalBdg
 *
 *	Evaluates the board from the point of view of the color that just moved.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
EVAL PLAI::EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull)
{
	cmvevEval++;

	EVAL evalNext = EvalPstFromCpc(bdg, bdg.cpcToMove);
	EVAL evalSelf = EvalPstFromCpc(bdg, ~bdg.cpcToMove);
	if (!fFull) {
		/* make a guess that this is a bad move if we're moving to an attacked square
		   that isn't defended, which will improve alpha-beta pruning, but not something 
		   we want to do on real board evaluation; someday, a better heuristic would be 
		   to exchange until quiescence */
		if (bdg.FSqAttacked(mvev.mv.shfTo(), bdg.cpcToMove) &&
				!bdg.FSqAttacked(mvev.mv.shfTo(), ~bdg.cpcToMove))
			evalSelf -= EvalBaseApc(bdg.ApcFromShf(mvev.mv.shfTo()));
	}
	EVAL evalMat = evalSelf - evalNext;

	static GMV gmvSelf;
	bdg.GenGmvColor(gmvSelf, ~bdg.cpcToMove);
	EVAL evalMob = 10*(gmvSelf.cmv() - mvev.gmvReplyAll.cmv());

	if (fFull) {
		LogData(L"Material " + to_wstring(evalSelf) + L"-" + to_wstring(evalNext));
		LogData(L"Mobility " + to_wstring(gmvSelf.cmv()) + L"-" + to_wstring(mvev.gmvReplyAll.cmv()));
	}
	return (rgfAICoeffNum[0] * evalMat + rgfAICoeffNum[1] * evalMob + 50) / 100 + EvalTempo(bdg.cpcToMove);
}

enum PHASE {
	phaseMax = 24,
	phaseOpening = 20,
	phaseMid = 16,
	phaseEnd = 6,
	phaseLateEnd = 0
};

/*	PLAI:EvalPstFromCpc
 *
 *	Returns the board evaluation for the given color. Basically determines what
 *	stage of the game we're in and dispatches to the appropriate evaluation
 *	function (opening, middle game, endgame)
 */
EVAL PLAI::EvalPstFromCpc(const BDG& bdg, CPC cpcMove) const noexcept
{
	int mpapcphase[APC::ActMax] = { 0, 0, 1, 1, 2, 4, 0 };
	int phase = 0;
	EVAL mpcpcevalOpening[2] = { 0, 0 };
	EVAL mpcpcevalMid[2] = { 0, 0 };
	EVAL mpcpcevalEnd[2] = { 0, 0 };

	for (APC apc = APC::Knight; apc <= APC::Queen; ++apc) {
		phase += bdg.mpapcbb[CPC::White][apc].csq() * mpapcphase[apc];
		phase += bdg.mpapcbb[CPC::Black][apc].csq() * mpapcphase[apc];
		for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
			for (BB bb = bdg.mpapcbb[cpc][apc]; bb; bb.ClearLow()) {
				SHF shf = bb.shfLow();
				if (cpcMove == CPC::White)
					shf = shf ^ SHF(rankMax - 1, 0);
				mpcpcevalOpening[cpc] += mpapcsqevalOpening[apc][shf];
				mpcpcevalMid[cpc] += mpapcsqevalMiddleGame[apc][shf];
				mpcpcevalEnd[cpc] += mpapcsqevalEndGame[apc][shf];
			}
		}
	}
	phase = min(phase, phaseMax);

	if (phase > phaseOpening)
		return mpcpcevalOpening[cpcMove] - mpcpcevalOpening[~cpcMove];
	
	if (phase > phaseMid)
		return EvalInterpolate(phase, 
			mpcpcevalOpening[cpcMove] - mpcpcevalOpening[~cpcMove], phaseOpening,
			mpcpcevalMid[cpcMove] - mpcpcevalMid[~cpcMove], phaseMid);
		
	if (phase > phaseEnd)
		return EvalInterpolate(phase, 
			mpcpcevalMid[cpcMove] - mpcpcevalMid[~cpcMove], phaseMid, 
			mpcpcevalEnd[cpcMove] - mpcpcevalEnd[~cpcMove], phaseEnd); 
	
	return mpcpcevalEnd[cpcMove] - mpcpcevalEnd[~cpcMove];
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
int mpapcevalOpening[APC::ActMax] = { 0, 100, 320, 330, 500, 900, 200 };
EVAL mpapcsqdevalOpening[APC::ActMax][64] = {
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
	   5,   0,  -5, -25, -25,  10,   0,   5,
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
int mpapcevalMiddleGame[APC::ActMax] = { 0, 82, 337, 365, 477, 1025, 200 };
const EVAL mpapcsqdevalMiddleGame[APC::ActMax][64] = {
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
int mpapcevalEndGame[APC::ActMax] = { 0, 94, 281, 297, 512, 936, 200 };
const EVAL mpapcsqdevalEndGame[APC::ActMax][64] = {
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
		

EVAL PLAI::EvalBaseApc(APC apc) const
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

void PLAI::InitWeightTable(const EVAL mpapceval[APC::ActMax], const EVAL mpapcshfdeval[APC::ActMax][64], EVAL mpapcshfeval[APC::ActMax][64])
{
	memset(&mpapcshfeval[APC::Null], 0, 64 * sizeof(EVAL));
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
		for (uint64_t shf = 0; shf < 64; shf++)
			mpapcshfeval[apc][shf] = mpapceval[apc] + mpapcshfdeval[apc][shf];
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


EVAL PLAI2::EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull)
{
	cmvevEval++;

	EVAL evalNext = EvalPstFromCpc(bdg, bdg.cpcToMove);
	EVAL evalSelf = EvalPstFromCpc(bdg, ~bdg.cpcToMove);
	if (!fFull) {
		/* make a guess that this is a bad move if we're moving to an attacked square
		   that isn't defended, which will improve alpha-beta pruning, but not something
		   we want to do on real board evaluation; someday, a better heuristic would be
		   to exchange until quiescence */
		if (bdg.FSqAttacked(mvev.mv.shfTo(), bdg.cpcToMove) &&
				!bdg.FSqAttacked(mvev.mv.shfTo(), ~bdg.cpcToMove))
			evalSelf -= EvalBaseApc(bdg.ApcFromShf(mvev.mv.shfTo()));
	}
	EVAL evalMat = evalSelf - evalNext;
	if (fFull) {
		LogData(L"Material " + to_wstring(evalSelf) + L"-" + to_wstring(evalNext));
	}
	return evalMat + EvalTempo(bdg.cpcToMove);
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
	do {
		ga.PumpMsg();
	} while (mvNext.fIsNil());
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
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Al de Leon"));

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