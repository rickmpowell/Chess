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

const unsigned long dcYield = 1000L;



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


/*
 *
 *	PLAI and PLAI2
 * 
 *	AI computer players. Base PLAI implements alpha-beta pruned look ahead tree.
 * 
 */


PLAI::PLAI(GA& ga) : PL(ga, L"Mobly"), cYield(0), cbdgmvEval(0), cbdgmvGen(0), cbdgmvPrune(0)
{
	rgfAICoeff[0] = 1.0f;
	rgfAICoeff[1] = 5.0f;
}

PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	rgfAICoeff[1] = 0.1f;
	SetName(L"Mathilda");
}

int PLAI::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenRgmvColor(gmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = 0.5e+6f;	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}

int PLAI2::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenRgmvColor(gmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = 2.0e+6f;	// approximate number of moves to analyze
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

const float evalInf = 10000.0f;
const float evalMate = 9999.0f;
const float evalMateMin = evalMate - 80.0f;

MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	time_point tpStart = high_resolution_clock::now();

	cbdgmvEval = 0L;
	cbdgmvPrune = 0L;

	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	ga.Log(LGT::Open, LGF::Normal, 
			ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", wstring(L"(") + szName + L")");
	BDG bdg = ga.bdg;
	static GMV gmv;
	bdg.GenRgmv(gmv, RMCHK::NoRemove);
	if (gmv.cmv() == 0)
		return MV();
	int depthMax = DepthMax(bdg, gmv);
	ga.Log(LGT::Data, LGF::Normal, wstring(L"Search depth:"), to_wstring(depthMax));
	
	/* and find the best move */

	bdg.GenRgmv(gmv, RMCHK::Remove);
	vector<BDGMV> vbdgmv;
	PreSortMoves(bdg, gmv, vbdgmv);

	cYield = 0;
	MV mvBest;
	float evalAlpha = -evalInf, evalBeta = evalInf;
	cbdgmvGen = vbdgmv.size();
	for (BDGMV& bdgmv : vbdgmv) {
		ga.Log(LGT::Open, LGF::Normal, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(bdgmv.eval));
		float eval = -EvalBdgDepth(bdgmv, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		LGF lgf = LGF::Normal;
		if (eval > evalAlpha) {
			lgf = LGF::Bold;
			evalAlpha = eval;
			mvBest = bdgmv.mv;
			if (eval > evalMateMin) {
				int depthMate = (int)roundf(evalMate - eval);
				if (depthMate < depthMax)
					depthMax = depthMate;
			}
		}
		ga.Log(LGT::Close, lgf, bdg.SzDecodeMvPost(bdgmv.mv), SzFromEval(eval));
	}

	chrono::time_point tpEnd = chrono::high_resolution_clock::now();

	/* log some stats */

	ga.Log(LGT::Data, LGF::Normal, wstring(L"Evaluated"), to_wstring(cbdgmvEval) + L" boards");
	ga.Log(LGT::Data, LGF::Normal, wstring(L"Pruned:"),
			to_wstring((int)roundf(100.f*(float)cbdgmvPrune/(float)cbdgmvGen)) + L"%");
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	ga.Log(LGT::Data, LGF::Normal, L"Time:", to_wstring(ms.count()) + L"ms");
	float sp = (float)cbdgmvEval / (float)ms.count();
	ga.Log(LGT::Data, LGF::Normal, L"Speed:", to_wstring((int)round(sp)) + L" nodes/ms");
	ga.Log(LGT::Close, LGF::Normal, L"", L"");

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
float PLAI::EvalBdgDepth(BDGMV& bdgmvEval, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdgmvEval, depth, evalAlpha, evalBeta);

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	vector<BDGMV> vbdgmv;
	PreSortMoves(bdgmvEval, bdgmvEval.gmvReplyAll, vbdgmv);

	int cmv = 0;
	cbdgmvGen += vbdgmv.size();
	float evalBest = -evalInf;
	for (BDGMV& bdgmv : vbdgmv) {
		if (bdgmv.FInCheck(~bdgmv.cpcToMove))
			continue;
		ga.Log(LGT::Open, LGF::Normal, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(bdgmv.eval));
		float eval = -EvalBdgDepth(bdgmv, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		cmv++;
		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cbdgmvPrune += vbdgmv.size() - cmv;
			ga.Log(LGT::Close, lgf, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(eval) + L" [Pruning]");
			return eval;
		}
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				if (eval > evalMateMin) {
					int depthMate = (int)roundf(evalMate - eval);
					if (depthMate < depthMax)
						depthMax = depthMate;
				}
				lgf = LGF::Bold;
			}
		}
		ga.Log(LGT::Close, lgf, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(eval));
	}

	/* if we could find no legal moves, we either have checkmate or stalemate */

	if (cmv == 0) {
		if (bdgmvEval.FInCheck(bdgmvEval.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0f;
	}

	/* checkmates were detected already, so GsTestGameOver can only return draws */
	GS gs = bdgmvEval.GsTestGameOver(bdgmvEval.gmvReplyAll, *ga.prule);
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
float PLAI::EvalBdgQuiescent(BDGMV& bdgmvEval, int depth, float evalAlpha, float evalBeta)
{
	/* we need to evaluate the board before we remove moves from the move list */
	float eval = EvalBdg(bdgmvEval, true);
	
	bdgmvEval.RemoveInCheckMoves(bdgmvEval.gmvReplyAll, bdgmvEval.cpcToMove);
	if (bdgmvEval.gmvReplyAll.cmv() == 0) {
		if (bdgmvEval.FInCheck(bdgmvEval.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0;
	}

	bdgmvEval.RemoveQuiescentMoves(bdgmvEval.gmvReplyAll, bdgmvEval.cpcToMove);
	if (bdgmvEval.gmvReplyAll.cmv() == 0) {
		ga.Log(LGT::Data, LGF::Normal, L"Total", SzFromEval(eval));
		return -eval;
	}

	if (++cYield % dcYield == 0)
		ga.PumpMsg();

	vector<BDGMV> vbdgmv;
	FillRgbdgmv(bdgmvEval, bdgmvEval.gmvReplyAll, vbdgmv);
	
	cbdgmvGen += vbdgmv.size();
	float evalBest = -evalInf;
	int cmv = 0;
	for (BDGMV& bdgmv : vbdgmv) {
		ga.Log(LGT::Open, LGF::Normal, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(bdgmv.eval));
		float eval = -EvalBdgQuiescent(bdgmv, depth + 1, -evalBeta, -evalAlpha);
		cmv++;
		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cbdgmvPrune += vbdgmv.size() - cmv;
			ga.Log(LGT::Close, LGF::Normal, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(eval) + L" [Pruning]");
			return eval;
		}
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				lgf = LGF::Bold;
			}
		}
		ga.Log(LGT::Close, lgf, bdgmv.SzDecodeMvPost(bdgmv.mv), SzFromEval(eval));
	}

	return evalBest;
}


/*	PLAI::FillRgbdgmv
 *
 *	Fills the BDGMV array with the moves in gmv.
 */
void PLAI::FillRgbdgmv(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv)
{
	vbdgmv.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		BDGMV bdgmv(bdg, gmv[imv]);
		bdgmv.eval = EvalBdg(bdgmv, false);
		vbdgmv.push_back(move(bdgmv));
	}
}


/*	PLAI::PreSortMoves
 *
 *	Using generated moves in gmv (generated from bdg),  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted BDGMV
 *	list is returned in vbdgmv.
 */
void PLAI::PreSortMoves(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv)
{
	/* insertion sort */

	vbdgmv.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		BDGMV bdgmv(bdg, gmv[imv]);
		bdgmv.eval = EvalBdg(bdgmv, false);
		size_t imvFirst, imvLim;
		for (imvFirst = 0, imvLim = vbdgmv.size(); imvFirst != imvLim; ) {
			size_t imvMid = (imvFirst + imvLim) / 2;
			assert(imvMid < imvLim);
			if (vbdgmv[imvMid].eval < bdgmv.eval)
				imvLim = imvMid;
			else
				imvFirst = imvMid+1;
		}
		vbdgmv.insert(vbdgmv.begin() + imvFirst, move(bdgmv));
	}
}


/*	PLAI::EvalBdg
 *
 *	Evaluates the board from the point of view of the color that just moved.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
float PLAI::EvalBdg(const BDGMV& bdgmv, bool fFull)
{
	cbdgmvEval++;

	float vpcNext = VpcFromCpc(bdgmv, bdgmv.cpcToMove);
	float vpcSelf = VpcFromCpc(bdgmv, ~bdgmv.cpcToMove);
	if (!fFull) {
		/* make a guess that this is a bad move if we're moving to an attacked square,
		   which will improve alpha-beta pruning, but not something we want to do on
		   real board evaluation */
		if (bdgmv.FSqAttacked(bdgmv.cpcToMove, bdgmv.mv.sqTo()))
			vpcSelf -= bdgmv.VpcFromSq(bdgmv.mv.sqTo());
	}
	float evalMat = vpcSelf - vpcNext;

	static GMV gmvSelf;
	bdgmv.GenRgmvColor(gmvSelf, ~bdgmv.cpcToMove, false);
	int evalMob = gmvSelf.cmv() - bdgmv.gmvReplyAll.cmv();

	if (fFull) {
		ga.Log(LGT::Data, LGF::Normal, L"Material", to_wstring((int)vpcSelf) + L"-" + to_wstring((int)vpcNext));
		ga.Log(LGT::Data, LGF::Normal, L"Mobility", to_wstring(gmvSelf.cmv()) + L"-" + to_wstring(bdgmv.gmvReplyAll.cmv()) + L"]");
	}
	return rgfAICoeff[0] * evalMat + rgfAICoeff[1] * (float)evalMob;
}


/*	PLAI:VpcFromCpc
 *
 *	Returns the board evaluation for the given color. Basically determines what
 *	stage of the game we're in and dispatches to the appropriate evaluation
 *	function (opening, middle game, endgame
 */
float PLAI::VpcFromCpc(const BDGMV& bdgmv, CPC cpcMove) const
{
	float vpcMove = bdgmv.VpcTotalFromCpc(cpcMove) + bdgmv.VpcTotalFromCpc(~cpcMove);
	if (vpcMove > 7000.0f)
		return VpcOpening(bdgmv, cpcMove);
	if (vpcMove > 6200.0f)
		return VpcEarlyMidGame(bdgmv, cpcMove);
	if (vpcMove > 5400.0f)
		return VpcMiddleGame(bdgmv, cpcMove);
	if (vpcMove > 4600.0f)
		return VpcLateMidGame(bdgmv, cpcMove);
	if (vpcMove > 2200.0f)
		return VpcEndGame(bdgmv, cpcMove);
	return VpcLateEndGame(bdgmv, cpcMove);
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
float PLAI::VpcOpening(const BDGMV& bdgmv, CPC cpcMove) const
{
	return VpcWeightTable(bdgmv, cpcMove, mpapcsqevalOpening);
}


float PLAI::VpcEarlyMidGame(const BDGMV& bdgmv, CPC cpcMove) const
{
	return (VpcMiddleGame(bdgmv, cpcMove) + VpcOpening(bdgmv, cpcMove)) / 2.0f;
}


/*	PLAI::VpcMiddleGame
 *
 *	Returns the board evaluation for middle game stage of the game
 */
float PLAI::VpcMiddleGame(const BDGMV& bdgmv, CPC cpcMove) const
{
	return bdgmv.VpcTotalFromCpc(cpcMove);
}


float PLAI::VpcLateMidGame(const BDGMV& bdgmv, CPC cpcMove) const
{
	return (VpcMiddleGame(bdgmv, cpcMove) + VpcEndGame(bdgmv, cpcMove)) / 2.0f;
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

float PLAI::VpcEndGame(const BDGMV& bdgmv, CPC cpcMove) const
{
	return VpcWeightTable(bdgmv, cpcMove, mpapcsqevalEndGame);
}


float PLAI::VpcLateEndGame(const BDGMV& bdgmv, CPC cpcMove) const
{
	return VpcEndGame(bdgmv, cpcMove);
}


/*	PLAI::VpcWeightTable
 *
 *	Uses a weight table to compute piece values. Weight tables are multpliers
 *	on the base value of the piece.
 */
float PLAI::VpcWeightTable(const BDGMV& bdgmv, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const
{
	float vpc = 0.0f;
	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sq = bdgmv.mptpcsq[cpcMove][tpc];
		if (sq.fIsNil())
			continue;
		APC apc = bdgmv.ApcFromSq(sq);
		int rank = sq.rank();
		int file = sq.file();
		if (cpcMove == CPC::White)
			rank = rankMax - rank - 1;
		vpc += bdgmv.VpcFromSq(sq) * mpapcsqeval[apc][rank * 8 + file];
	}
	return vpc;
}


/*
 *
 *	PLAI2
 * 
 */


float PLAI2::EvalBdg(const BDGMV& bdgmv, bool fFull)
{
	float eval;
	eval = PLAI::EvalBdg(bdgmv, fFull);
	if (fFull) {
		normal_distribution<float> flDist(0.0, 0.5f);
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
	mvNext = MV();
	return mv;
}