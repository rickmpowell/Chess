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


wstring SzFromEval(float eval)
{
	wchar_t sz[20], *pch = sz;
	if (eval < 0) {
		*pch++ = L'-';
		eval = -eval;
	}
	else
		*pch++ = L'+';
	int cchFrac = 2;
	float tens = pow(10.0f, (float)cchFrac);
	eval = round(eval * tens) / tens;
	float w = floor(eval);
	eval -= w;
	pch = PchDecodeInt((int)w, pch);
	*pch++ = L'.';
	for (int ich = 0; ich < cchFrac-1; ich++) {
		eval *= 10.0f;
		w = floor(eval);
		eval -= w;
		*pch++ = L'0' + (int)w;
	}
	eval *= 10.0f;
	w = floor(eval);
	eval -= w;
	*pch++ = L'0' + (int)w;
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
	rgfAICoeff[0] = 1.0f;
	rgfAICoeff[1] = 5.0f;
}

PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	rgfAICoeff[1] = 0.1f;
	SetName(L"SQ Mathilda");
}


float PLAI::CmvFromLevel(int level) const
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
	bdg.GenRgmvColor(gmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}

int PLAI2::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenRgmvColor(gmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}



/*	PLAI::MvGetNext
 *
 *	Returns the next move the player wants to make on the given board. Returns mvNil if
 *	there are no moves.
 * 
 *	This is a computer AI implementation.
 */

const float evalInf = 10000.0f;	/* infinity */
const float evalMate = 9999.0f;	/* checkmates are given evals of evalMate minus moves to mate */
const float evalMateMin = evalMate - 80.0f;

bool FEvalIsMate(float eval)
{
	return eval >= evalMateMin;
}

MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	time_point tpStart = high_resolution_clock::now();

	cmvevEval = 0L;
	cmvevPrune = 0L;

	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	LogOpen(TAG(ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", ATTR(L"FEN", ga.bdg)),
			L"(" + szName + L")");
	BDG bdg = ga.bdg;
	static GMV gmv;
	bdg.GenRgmv(gmv, RMCHK::NoRemove);
	if (gmv.cmv() == 0)
		return MV();
	int depthMax = peg(DepthMax(bdg, gmv), 2, 12);
	LogData(L"Search depth: " + to_wstring(depthMax));
	
	/* and find the best move */

	bdg.GenRgmv(gmv, RMCHK::Remove);
	vector<MVEV> vmvev;
	PreSortMoves(bdg, gmv, vmvev);

	cYield = 0;
	MV mvBest;
	float evalAlpha = -evalInf, evalBeta = evalInf;
	cmvevGen = vmvev.size();
	for (MVEV& mvev : vmvev) {
		bdg.MakeMv(mvev.mv);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)), SzFromEval(mvev.eval));
		float eval = -EvalBdgDepth(bdg, mvev, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		bdg.UndoMv();
		LGF lgf = LGF::Normal;
		if (eval > evalAlpha) {
			lgf = LGF::Bold;
			evalAlpha = eval;
			mvBest = mvev.mv;
			if (FEvalIsMate(eval))
				depthMax = min(depthMax, (int)roundf(evalMate - eval));
		}
		LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval), lgf);
	}

	time_point tpEnd = chrono::high_resolution_clock::now();

	/* log some stats */

	LogData(L"Evaluated " + to_wstring(cmvevEval) + L" boards");
	LogData(L"Pruned: " + to_wstring((int)roundf(100.f*(float)cmvevPrune/(float)cmvevGen)) + L"%");
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cmvevEval / (float)ms.count();
	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" nodes/ms");
	LogClose(L"", L"", LGF::Normal);

	/* and return the best move */

	assert(!mvBest.fIsNil());
	return mvBest;
}

bool PLAI::FHasLevel(void) const
{
	return true;
}

void PLAI::SetLevel(int level)
{
	this->level = peg(level, 1, 10);
}

/*	PLAI::EvalBdgDepth
 *
 *	Evaluates the board from the point of view of the person who last moved,
 *	i.e., the previous move.
 */
float PLAI::EvalBdgDepth(BDG& bdg, MVEV& mvevEval, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdg, mvevEval, depth, evalAlpha, evalBeta);

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	vector<MVEV> vmvev;
	PreSortMoves(bdg, mvevEval.gmvReplyAll, vmvev);

	int cmv = 0;
	cmvevGen += vmvev.size();
	float evalBest = -evalInf;
	for (MVEV& mvev: vmvev) {
		bdg.MakeMv(mvev.mv);
		if (bdg.FInCheck(~bdg.cpcToMove)) {
			bdg.UndoMv();
			continue;
		}
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", (wstring)bdg)), SzFromEval(mvev.eval));
		float eval = -EvalBdgDepth(bdg, mvev, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		bdg.UndoMv();
		cmv++;
		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cmvevPrune += vmvev.size() - cmv;
			LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval) + L" (Pruning)", lgf);
			return eval;
		}
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				if (FEvalIsMate(eval))
					depthMax = min(depthMax, (int)roundf(evalMate - eval));
				lgf = LGF::Bold;
			}
		}
		LogClose(bdg.SzDecodeMvPost(mvev.mv), SzFromEval(eval), lgf);
	}

	/* if we could find no legal moves, we either have checkmate or stalemate */

	if (cmv == 0) {
		if (bdg.FInCheck(bdg.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0f;
	}

	/* checkmates were detected already, so GsTestGameOver can only return draws */

	GS gs = bdg.GsTestGameOver(mvevEval.gmvReplyAll, *ga.prule);
	assert(gs != GS::BlackCheckMated && gs != GS::WhiteCheckMated);
	if (gs != GS::Playing)
		return 0.0f;

	return evalBest;
}


/*	PLAI::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the 
 *	previous move player, i.e., it evaluates the previous move.
 */
float PLAI::EvalBdgQuiescent(BDG& bdg, MVEV& mvevEval, int depth, float evalAlpha, float evalBeta)
{
	/* we need to evaluate the board before we remove moves from the move list, because the
	   un-processed move list is used to compute mobility */

	float eval = EvalBdg(bdg, mvevEval, true);
	
	bdg.RemoveInCheckMoves(mvevEval.gmvReplyAll, bdg.cpcToMove);
	if (mvevEval.gmvReplyAll.cmv() == 0) {
		if (bdg.FInCheck(bdg.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0;	/* must be a stalemate */
	}

	bdg.RemoveQuiescentMoves(mvevEval.gmvReplyAll, bdg.cpcToMove);
	if (mvevEval.gmvReplyAll.cmv() == 0) {
		LogData(L"Total " + SzFromEval(eval));
		return -eval;
	}

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	vector<MVEV> vmvev;
	FillRgbdgmv(bdg, mvevEval.gmvReplyAll, vmvev);
	
	cmvevGen += vmvev.size();
	float evalBest = -evalInf;
	int cmv = 0;
	for (MVEV& mvev : vmvev) {
		bdg.MakeMv(mvev.mv);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)), SzFromEval(mvev.eval));
		float eval = -EvalBdgQuiescent(bdg, mvev, depth + 1, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		cmv++;
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


/*	PLAI::FillRgbdgmv
 *
 *	Fills the BDGMV array with the moves in gmv.
 */
void PLAI::FillRgbdgmv(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev)
{
	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(bdg, gmv[imv]);
		mvev.eval = EvalBdg(bdg, mvev, false);
		bdg.UndoMv();
		vmvev.push_back(move(mvev));
	}
}


/*	PLAI::PreSortMoves
 *
 *	Using generated moves in gmv (generated from bdg),  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted BDGMV
 *	list is returned in vbdgmv.
 */
void PLAI::PreSortMoves(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev)
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


/*	PLAI::EvalBdg
 *
 *	Evaluates the board from the point of view of the color that just moved.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
float PLAI::EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull)
{
	cmvevEval++;

	float vpcNext = VpcFromCpc(bdg, bdg.cpcToMove);
	float vpcSelf = VpcFromCpc(bdg, ~bdg.cpcToMove);
	if (!fFull) {
		/* make a guess that this is a bad move if we're moving to an attacked square
		   that isn't defended, which will improve alpha-beta pruning, but not something 
		   we want to do on real board evaluation; a better heuristic would be to 
		   exchange until quiescence */
		if (bdg.FSqAttacked(bdg.cpcToMove, mvev.mv.sqTo()) &&
				!bdg.FSqAttacked(~bdg.cpcToMove, mvev.mv.sqTo()))
			vpcSelf -= bdg.VpcFromSq(mvev.mv.sqTo());
	}
	float evalMat = vpcSelf - vpcNext;

	static GMV gmvSelf;
	bdg.GenRgmvColor(gmvSelf, ~bdg.cpcToMove, false);
	int evalMob = gmvSelf.cmv() - mvev.gmvReplyAll.cmv();

	if (fFull) {
		LogData(L"Material " + to_wstring((int)vpcSelf) + L"-" + to_wstring((int)vpcNext));
		LogData(L"Mobility " + to_wstring(gmvSelf.cmv()) + L"-" + to_wstring(mvev.gmvReplyAll.cmv()));
	}
	return rgfAICoeff[0] * evalMat + rgfAICoeff[1] * (float)evalMob;
}


/*	PLAI:VpcFromCpc
 *
 *	Returns the board evaluation for the given color. Basically determines what
 *	stage of the game we're in and dispatches to the appropriate evaluation
 *	function (opening, middle game, endgame
 */
float PLAI::VpcFromCpc(const BDG& bdg, CPC cpcMove) const
{
	float vpcMove = bdg.VpcTotalFromCpc(cpcMove) + bdg.VpcTotalFromCpc(~cpcMove);
	if (vpcMove > 7000.0f)
		return VpcOpening(bdg, cpcMove);
	if (vpcMove > 6200.0f)
		return VpcEarlyMidGame(bdg, cpcMove);
	if (vpcMove > 5400.0f)
		return VpcMiddleGame(bdg, cpcMove);
	if (vpcMove > 4600.0f)
		return VpcLateMidGame(bdg, cpcMove);
	if (vpcMove > 2200.0f)
		return VpcEndGame(bdg, cpcMove);
	return VpcLateEndGame(bdg, cpcMove);
}

const float mpapcsqevalOpening[APC::ActMax][64] = {
	{0, 0, 0, 0, 0, 0, 0, 0,	// APC::Null
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Pawn
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.1f, 1.1f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.2f, 1.2f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Knight
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.1f, 1.1f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.2f, 1.2f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.2f, 1.2f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.1f, 1.1f, 1.1f, 1.1f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Bishop
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Rook
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Queen
	 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 0.9f, 0.8f, 0.8f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 0.9f, 0.8f, 0.8f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 1.0f, 1.0f, 1.0f, 1.0f, 0.9f, 0.9f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,	// APC::King
	 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
	 0.7f, 0.7f, 0.5f, 0.5f, 0.5f, 0.5f, 0.7f, 0.7f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.5f, 1.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.5f}
};
float PLAI::VpcOpening(const BDG& bdg, CPC cpcMove) const
{
	return VpcWeightTable(bdg, cpcMove, mpapcsqevalOpening);
}


float PLAI::VpcEarlyMidGame(const BDG& bdg, CPC cpcMove) const
{
	return (VpcMiddleGame(bdg, cpcMove) + VpcOpening(bdg, cpcMove)) / 2.0f;
}


/*	PLAI::VpcMiddleGame
 *
 *	Returns the board evaluation for middle game stage of the game
 */
float PLAI::VpcMiddleGame(const BDG& bdg, CPC cpcMove) const
{
	return bdg.VpcTotalFromCpc(cpcMove);
}


float PLAI::VpcLateMidGame(const BDG& bdg, CPC cpcMove) const
{
	return (VpcMiddleGame(bdg, cpcMove) + VpcEndGame(bdg, cpcMove)) / 2.0f;
}


const float mpapcsqevalEndGame[APC::ActMax][64] = {
	{0, 0, 0, 0, 0, 0, 0, 0,	// APC::Null
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0},
	{9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f,	// APC::Pawn
	 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f,
	 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f,
	 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
	 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f,
	 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f,
	 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Knight
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Bishop
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Rook
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,	// APC::Queen
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
	{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,	// APC::King
	 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f,
	 0.5f, 1.0f, 1.5f, 1.5f, 1.5f, 1.5f, 1.0f, 0.5f,
	 0.5f, 1.0f, 1.5f, 1.5f, 1.5f, 1.5f, 1.0f, 0.5f,
	 0.5f, 1.0f, 1.5f, 1.5f, 1.5f, 1.5f, 1.0f, 0.5f,
	 0.5f, 1.0f, 1.5f, 1.5f, 1.5f, 1.5f, 1.0f, 0.5f,
	 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f,
	 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}
};

float PLAI::VpcEndGame(const BDG& bdg, CPC cpcMove) const
{
	return VpcWeightTable(bdg, cpcMove, mpapcsqevalEndGame);
}


float PLAI::VpcLateEndGame(const BDG& bdg, CPC cpcMove) const
{
	return VpcEndGame(bdg, cpcMove);
}


/*	PLAI::VpcWeightTable
 *
 *	Uses a weight table to compute piece values. Weight tables are multpliers
 *	on the base value of the piece.
 */
float PLAI::VpcWeightTable(const BDG& bdg, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const
{
	float vpc = 0.0f;
	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sq = bdg.mptpcsq[cpcMove][tpc];
		if (sq.fIsNil())
			continue;
		APC apc = bdg.ApcFromSq(sq);
		int rank = sq.rank();
		int file = sq.file();
		if (cpcMove == CPC::White)
			rank = rankMax - rank - 1;
		vpc += bdg.VpcFromSq(sq) * mpapcsqeval[apc][rank * 8 + file];
	}
	return vpc;
}


/*
 *
 *	PLAI2
 * 
 */


float PLAI2::EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull)
{
	float eval;
	eval = PLAI::EvalBdg(bdg, mvev, fFull);
	if (fFull) {
		normal_distribution<float> flDist(0.0, 25.0f);
		eval += flDist(rgen);
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
	vinfopl.push_back(INFOPL(IDCLASSPL::AI, TPL::AI, L"SQ Mobly", 4));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI2, TPL::AI, L"SQ Mathilda", 2));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Rick Powell"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"My Dog Is the Best Dog"));
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