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


mt19937 rgen(0);


PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName)
{
}


PL::~PL(void)
{
}


wstring SzFromEval(float eval)
{

	wchar_t sz[10], *pch = sz;
	if (eval < 0) {
		*pch++ = L'-';
		eval = -eval;
	}
	eval = round(eval * 1000.0f) / 1000.0f;
	float w = floor(eval);
	eval -= w;
	pch = PchDecodeInt((int)w, pch);
	*pch++ = L'.';
	for (int ich = 0; ich < 2; ich++) {
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

/*
 *
 * 
 */

PLAI::PLAI(GA& ga) : PL(ga, L"Mobly"), cYield(0), cbdgmvevEval(0), cbdgmvevPrune(0), cbdgmvevGen(0)
{
	rgfAICoeff[0] = 5.0f;
	rgfAICoeff[1] = 2.0f;
}


PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	rgfAICoeff[1] = 0.1f;
	SetName(L"Mathilda");
}

float PLAI2::EvalBdg(const BDGMVEV& bdgmvev, bool fFull)
{
	float eval = PLAI::EvalBdg(bdgmvev, fFull);
	if (fFull) {
		normal_distribution<float> flDist(0.0, 0.1f);
		eval += flDist(rgen);
	}
	return eval;
}

int PLAI2::DepthMax(const BDG& bdg, const vector<MV>& rgmv) const
{
	static RGMV rgmvOpp;
	bdg.GenRgmvColor(rgmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = 0.10e+6f;	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(rgmv.size() * rgmvOpp.size());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return 4;
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

MV PLAI::MvGetNext(void)
{
	time_point tpStart = high_resolution_clock::now();

	cbdgmvevEval = 0L;
	cbdgmvevPrune = 0L;

	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	ga.Log(LGT::Open, LGF::Normal, 
			ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", wstring(L"(") + szName + L")");
	BDG bdg = ga.bdg;
	static RGMV rgmv;
	bdg.GenRgmv(rgmv, RMCHK::NoRemove);
	if (rgmv.size() == 0)
		return MV();
	int depthMax = DepthMax(bdg, rgmv);
	ga.Log(LGT::Data, LGF::Normal, wstring(L"Search depth:"), to_wstring(depthMax));
	
	/* and find the best move */

	bdg.GenRgmv(rgmv, RMCHK::Remove);
	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdg, rgmv, rgbdgmvev);

	cYield = 0;
	MV mvBest;
	float evalAlpha = -evalInf, evalBeta = evalInf;
	cbdgmvevGen = rgbdgmvev.size();
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		ga.Log(LGT::Open, LGF::Normal, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(bdgmvev.eval));
		float eval = -EvalBdgDepth(bdgmvev, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		LGF lgf = LGF::Normal;
		if (eval > evalAlpha) {
			lgf = LGF::Bold;
			evalAlpha = eval;
			mvBest = bdgmvev.mv;
			if (eval > evalMateMin) {
				int depthMate = (int)roundf(evalMate - eval);
				if (depthMate < depthMax)
					depthMax = depthMate;
			}
		}
		ga.Log(LGT::Close, lgf, bdg.SzDecodeMvPost(bdgmvev.mv), SzFromEval(eval));
	}

	chrono::time_point tpEnd = chrono::high_resolution_clock::now();

	/* log some stats */

	ga.Log(LGT::Data, LGF::Normal, wstring(L"Evaluated"), to_wstring(cbdgmvevEval) + L" positions");
	ga.Log(LGT::Data, LGF::Normal, wstring(L"Pruned:"),
			to_wstring((int)roundf(100.f*(float)cbdgmvevPrune/(float)cbdgmvevGen)) + L"%");
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	ga.Log(LGT::Data, LGF::Normal, L"Time:", to_wstring(ms.count()) + L"ms");
	float sp = (float)cbdgmvevEval / (float)ms.count();
	ga.Log(LGT::Data, LGF::Normal, L"Speed:", to_wstring((int)round(sp)) + L" nodes/ms");
	ga.Log(LGT::Close, LGF::Normal, L"", L"");

	/* and return the best move */

	assert(!mvBest.fIsNil());
	return mvBest;
}


int PLAI::DepthMax(const BDG& bdg, const vector<MV>& rgmv) const
{
	static RGMV rgmvOpp;
	bdg.GenRgmvColor(rgmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = 0.50e+6f;	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(rgmv.size() * rgmvOpp.size());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return 4; 
	return depthMax;
}


/*	PLAI::EvalBdgDepth
 *
 *	Evaluates the board from the point of view of the person who last moved,
 *	i.e., the previous move.
 */
float PLAI::EvalBdgDepth(BDGMVEV& bdgmvevEval, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdgmvevEval, depth, evalAlpha, evalBeta);

	if (++cYield % 100 == 0)
		ga.PumpMsg();

	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);

	int cmv = 0;
	cbdgmvevGen += rgbdgmvev.size();
	float evalBest = -evalInf;
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		if (bdgmvev.FInCheck(~bdgmvev.cpcToMove))
			continue;
		ga.Log(LGT::Open, LGF::Normal, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(bdgmvev.eval));
		float eval = -EvalBdgDepth(bdgmvev, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		cmv++;
		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cbdgmvevPrune += rgbdgmvev.size() - cmv;
			ga.Log(LGT::Close, lgf, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(eval) + L" [Pruning]");
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
		ga.Log(LGT::Close, lgf, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(eval));
	}

	/* if we could find no legal moves, we either have checkmate or stalemate */

	if (cmv == 0) {
		if (bdgmvevEval.FInCheck(bdgmvevEval.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0f;
	}

	/* checkmates were detected already, so GsTestGameOver can only return draws */
	GS gs = bdgmvevEval.GsTestGameOver(bdgmvevEval.rgmvReplyAll, *ga.prule);
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
float PLAI::EvalBdgQuiescent(BDGMVEV& bdgmvevEval, int depth, float evalAlpha, float evalBeta)
{
	/* we need to evaluate the board before we remove moves from the move list */
	float eval = EvalBdg(bdgmvevEval, true);
	
	bdgmvevEval.RemoveInCheckMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);
	if (bdgmvevEval.rgmvReplyAll.size() == 0) {
		if (bdgmvevEval.FInCheck(bdgmvevEval.cpcToMove))
			return -(evalMate - (float)depth);
		else
			return 0.0;
	}

	bdgmvevEval.RemoveQuiescentMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);
	if (bdgmvevEval.rgmvReplyAll.size() == 0) {
		ga.Log(LGT::Data, LGF::Normal, L"Total", SzFromEval(eval));
		return -eval;
	}

	if (++cYield % 100 == 0)
		ga.PumpMsg();

	vector<BDGMVEV> rgbdgmvev;
	FillRgbdgmvev(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);
	
	cbdgmvevGen += rgbdgmvev.size();
	float evalBest = -evalInf;
	int cmv = 0;
	for (BDGMVEV bdgmvev : rgbdgmvev) {
		ga.Log(LGT::Open, LGF::Normal, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(bdgmvev.eval));
		float eval = -EvalBdgQuiescent(bdgmvev, depth + 1, -evalBeta, -evalAlpha);
		cmv++;
		LGF lgf = LGF::Normal;
		if (eval >= evalBeta) {
			cbdgmvevPrune += rgbdgmvev.size() - cmv;
			ga.Log(LGT::Close, LGF::Normal, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(eval) + L" [Pruning]");
			return eval;
		}
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				lgf = LGF::Bold;
			}
		}
		ga.Log(LGT::Close, lgf, bdgmvev.SzDecodeMvPost(bdgmvev.mv), SzFromEval(eval));
	}

	return evalBest;
}


void PLAI::FillRgbdgmvev(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev)
{
	rgbdgmvev.reserve(rgmv.size());
	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev, false);
		rgbdgmvev.push_back(move(bdgmvev));
	}
}


void PLAI::PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev)
{
	/* insertion sort */

	rgbdgmvev.reserve(rgmv.size());
	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev, false);
		unsigned imvFirst, imvLim;
		for (imvFirst = 0, imvLim = (unsigned)rgbdgmvev.size(); ; ) {
			if (imvFirst == imvLim) {
				rgbdgmvev.insert(rgbdgmvev.begin()+imvFirst, move(bdgmvev));
				break;
			}
			unsigned imvMid = (imvFirst + imvLim) / 2;
			assert(imvMid < imvLim);
			if (rgbdgmvev[imvMid].eval < bdgmvev.eval)
				imvLim = imvMid;
			else
				imvFirst = imvMid+1;
		}
	}
}


/*	PLAI::EvalBdg
 *
 *	Evaluates the board from the point of view of the color that just moved.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
float PLAI::EvalBdg(const BDGMVEV& bdgmvev, bool fFull)
{
	cbdgmvevEval++;

	float vpcNext = VpcFromCpc(bdgmvev, bdgmvev.cpcToMove);
	float vpcSelf = VpcFromCpc(bdgmvev, ~bdgmvev.cpcToMove);
	float evalMat = (float)(vpcSelf - vpcNext) / (float)(vpcSelf + vpcNext);

	static RGMV rgmvSelf;
	bdgmvev.GenRgmvColor(rgmvSelf, ~bdgmvev.cpcToMove, false);
	float evalMob = (float)((int)rgmvSelf.size() - (int)bdgmvev.rgmvReplyAll.size()) /
		(float)((int)rgmvSelf.size() + (int)bdgmvev.rgmvReplyAll.size());

	if (fFull) {
		ga.Log(LGT::Data, LGF::Normal, L"Material", to_wstring((int)vpcSelf) + L"-" + to_wstring((int)vpcNext));
		ga.Log(LGT::Data, LGF::Normal, L"Mobility", to_wstring(rgmvSelf.size()) + L"-" + to_wstring(bdgmvev.rgmvReplyAll.size()) + L"]");
	}
	float evalControl = 0.0f;
	return rgfAICoeff[0] * evalMat + rgfAICoeff[1] * evalMob;
}


/*	PLAI:VpcFromCpc
 *
 *	Returns the board evaluation for the given color. Basically determines what
 *	stage of the game we're in and dispatches to the appropriate evaluation
 *	function (opening, middle game, endgame
 */
float PLAI::VpcFromCpc(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	if (bdgmvev.rgmvGame.size() < 2*15)
		return VpcOpening(bdgmvev, cpcMove);

	float vpcMove = bdgmvev.VpcTotalFromCpc(cpcMove);
	float vpcOpp = bdgmvev.VpcTotalFromCpc(~cpcMove);
	if (vpcMove < 1800.0f || vpcOpp < 1800.0f)
		return VpcEndGame(bdgmvev, cpcMove);

	return VpcMiddleGame(bdgmvev, cpcMove);
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
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
	 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
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
float PLAI::VpcOpening(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	return VpcWeightTable(bdgmvev, cpcMove, mpapcsqevalOpening);
}


/*	PLAI::VpcMiddleGame
 *
 *	Returns the board evaluation for middle game stage of the game
 */
float PLAI::VpcMiddleGame(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	return bdgmvev.VpcTotalFromCpc(cpcMove);
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
	 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
	 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f,
	 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f,
	 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f, 1.1f,
	 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
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

float PLAI::VpcEndGame(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	return VpcWeightTable(bdgmvev, cpcMove, mpapcsqevalEndGame);
}

float PLAI::VpcWeightTable(const BDGMVEV& bdgmvev, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const
{
	float vpc = 0.0f;
	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sq = bdgmvev.mptpcsq[cpcMove][tpc];
		if (sq.fIsNil())
			continue;
		APC apc = bdgmvev.ApcFromSq(sq);
		int rank = sq.rank();
		int file = sq.file();
		if (cpcMove == CPC::White)
			rank = rankMax - rank - 1;
		vpc += bdgmvev.VpcFromSq(sq) * mpapcsqeval[apc][rank * 8 + file];
	}
	return vpc;
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


MV PLHUMAN::MvGetNext(void)
{
	MV mv;
	do {
		ga.PumpMsg();
	} while (mv.fIsNil());
	return mv;
}