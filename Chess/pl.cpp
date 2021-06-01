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


PL::PL(GA& ga, wstring szName, const float* rgfAICoeef) : ga(ga), szName(szName), cYield(0), cbdgmvevEval(0)
{
	memcpy(this->rgfAICoeff, rgfAICoeef, sizeof(this->rgfAICoeff));
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


/*	PL::MvGetNext
 *
 *	Returns the next move the player wants to make on the given board. Returns mvNil if
 *	there are no moves.
 * 
 *	This is a computer AI implementation.
 */

const float evalInf = 10000.0f;
const float evalMate = 9999.0f;
const float evalMateMin = evalMate - 80.0f;

MV PL::MvGetNext(void)
{
	cbdgmvevEval = 0L;

	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	ga.Log(LGT::SearchStartAI, L"");
	BDG bdg = ga.bdg;
	static vector <MV> rgmv;
	bdg.GenRgmv(rgmv, RMCHK::NoRemove);
	if (rgmv.size() == 0)
		return MV();
	static vector<MV> rgmvOpp;
	bdg.GenRgmvColor(rgmvOpp, ~bdg.cpcToMove, false);
	const float cmvSearch = 1.00e+6f;	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(rgmv.size() * rgmvOpp.size());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2*fracAlphaBeta*fracAlphaBeta));
	ga.Log(LGT::SearchDepthAI, wstring(L"Search depth: ") + to_wstring(depthMax));

	/* and find the best move */

	bdg.GenRgmv(rgmv, RMCHK::Remove);
	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdg, rgmv, rgbdgmvev);

	cYield = 0;
	MV mvBest;
	float evalAlpha = -evalInf, evalBeta = evalInf;
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		float eval = -EvalBdgDepth(bdgmvev, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		ga.Log(LGT::SearchMoveAI, bdg.SzDecodeMvPost(bdgmvev.mv) + L" " + SzFromEval(bdgmvev.eval) + L" " + SzFromEval(eval));
		if (eval > evalAlpha) {
			evalAlpha = eval;
			mvBest = bdgmvev.mv;
			if (eval > evalMateMin) {
				int depthMate = (int)roundf(evalMate - eval);
				if (depthMate < depthMax)
					depthMax = depthMate;
			}
		}
	}

	ga.Log(LGT::SearchNodesAI, wstring(L"Searched ") + to_wstring(cbdgmvevEval) + L" nodes");
	assert(!mvBest.FIsNil());
	return mvBest;
}


/*	PL::EvalBdgDepth
 *
 *	Evaluates the board from the point of view of the person who last moved,
 *	i.e., the previous move.
 */
float PL::EvalBdgDepth(BDGMVEV& bdgmvevEval, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdgmvevEval, depth, evalAlpha, evalBeta);

	if (++cYield % 1000 == 0)
		ga.PumpMsg();

	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);

	int cmv = 0;
	float evalBest = -evalInf;
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		if (bdgmvev.FInCheck(~bdgmvev.cpcToMove))
			continue;
		cmv++;
		float eval = -EvalBdgDepth(bdgmvev, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		if (eval >= evalBeta)
			return eval;
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				if (eval > evalMateMin) {
					int depthMate = (int)roundf(evalMate - eval);
					if (depthMate < depthMax)
						depthMax = depthMate;
				}
			}
		}
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


/*	PL::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the 
 *	previous move player, i.e., it evaluates the previous move.
 */
float PL::EvalBdgQuiescent(BDGMVEV& bdgmvevEval, int depth, float evalAlpha, float evalBeta)
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
	if (bdgmvevEval.rgmvReplyAll.size() == 0)
		return -eval;

	if (++cYield % 1000 == 0)
		ga.PumpMsg();

	vector<BDGMVEV> rgbdgmvev;
	FillRgbdgmvev(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);

	float evalBest = -evalInf;
	for (BDGMVEV bdgmvev : rgbdgmvev) {
		float eval;
		eval = -EvalBdgQuiescent(bdgmvev, depth + 1, -evalBeta, -evalAlpha);
		if (eval >= evalBeta)
			return eval;
		if (eval > evalBest) {
			evalBest = eval;
			if (eval > evalAlpha)
				evalAlpha = eval;
		}
	}

	return evalBest;
}


void PL::FillRgbdgmvev(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev)
{
	rgbdgmvev.reserve(rgmv.size());
	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev, false);
		rgbdgmvev.push_back(move(bdgmvev));
	}
}


void PL::PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev)
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


/*	PL::EvalBdg
 *
 *	Evaluates the board from the point of view of the color that just moved.
 */
float PL::EvalBdg(const BDGMVEV& bdgmvev, bool fFull)
{
	cbdgmvevEval++;

	float vpcNext = VpcFromCpc(bdgmvev, bdgmvev.cpcToMove);
	float vpcSelf = VpcFromCpc(bdgmvev, ~bdgmvev.cpcToMove);
	float evalMat = (float)(vpcSelf - vpcNext) / (float)(vpcSelf + vpcNext);

	static vector<MV> rgmvSelf;
	bdgmvev.GenRgmvColor(rgmvSelf, ~bdgmvev.cpcToMove, false);
	float evalMob = (float)((int)rgmvSelf.size() - (int)bdgmvev.rgmvReplyAll.size()) /
		(float)((int)rgmvSelf.size() + (int)bdgmvev.rgmvReplyAll.size());

	float evalControl = 0.0f;
	return rgfAICoeff[0] * evalMat + rgfAICoeff[1] * evalMob;
}

float PL::VpcFromCpc(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	if (bdgmvev.rgmvGame.size() < 20)
		return VpcOpening(bdgmvev, cpcMove);

	float vpcMove = bdgmvev.VpcTotalFromCpc(cpcMove);
	float vpcOpp = bdgmvev.VpcTotalFromCpc(~cpcMove);
	if (vpcMove < 2000.0f || vpcOpp < 2000.0f)
		return VpcEndGame(bdgmvev, cpcMove);

	return VpcMiddleGame(bdgmvev, cpcMove);
}

float PL::VpcOpening(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	return bdgmvev.VpcTotalFromCpc(cpcMove);
}

float PL::VpcMiddleGame(const BDGMVEV& bdgmvev, CPC cpcMove) const
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

float PL::VpcEndGame(const BDGMVEV& bdgmvev, CPC cpcMove) const
{
	return VpcWeightTable(bdgmvev, cpcMove, mpapcsqevalEndGame);
}

float PL::VpcWeightTable(const BDGMVEV& bdgmvev, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const
{
	float vpc = 0.0f;
	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sq = bdgmvev.mptpcsq[cpcMove][tpc];
		if (sq.FIsNil())
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
