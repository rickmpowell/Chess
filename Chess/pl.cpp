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


PL::PL(GA& ga, wstring szName, const float* rgfAICoeef) : ga(ga), szName(szName), cYield(0)
{
	memcpy(this->rgfAICoeff, rgfAICoeef, sizeof(this->rgfAICoeff));
}


PL::~PL(void)
{
}


/*	PL::MvGetNext
 *
 *	Returns the next move the player wants to make on the given board. Returns mvNil if
 *	there are no moves.
 * 
 *	This is a compauter AI implementation.
 */
MV PL::MvGetNext(void)
{
	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	BDG bdg = ga.bdg;
	static vector <MV> rgmv;
	bdg.GenRgmv(rgmv, RMCHK::NoRemove);
	if (rgmv.size() == 0)
		return MV();
	static vector<MV> rgmvOpp;
	bdg.GenRgmvColor(rgmvOpp, CpcOpposite(bdg.cpcToMove), false);
	const float cmvSearch = 1.00e+6f;
	const float fracAlphaBeta = 3.0f; // cuts moves we analyze by this factor
	float size2 = (float)(rgmv.size() * rgmvOpp.size());
	int depthMax = (int)round(2.0f*log(cmvSearch) / (log(size2)-log(2.0f*fracAlphaBeta)));
	if (depthMax < 4)
		depthMax = 4;

	/* and find the best move */

	bdg.GenRgmv(rgmv, RMCHK::Remove);
	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdg, rgmv, rgbdgmvev);

	cYield = 0;
	MV mvBest;
	float evalAlpha = -1000.0f, evalBeta = 1000.0f;
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		float eval = -EvalBdgDepth(bdgmvev, 0, depthMax, -evalBeta, -evalAlpha, *ga.prule);
		if (eval > evalAlpha) {
			evalAlpha = eval;
			mvBest = bdgmvev.mv;
		}
	}
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
	for (BDGMVEV& bdgmvev : rgbdgmvev) {
		if (bdgmvev.FInCheck(CpcOpposite(bdgmvev.cpcToMove)))
			continue;
		cmv++;
		float eval = -EvalBdgDepth(bdgmvev, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}

	/* if we could find no legal moves, we either have checkmate or stalemate */

	if (cmv == 0) {
		if (bdgmvevEval.FInCheck(bdgmvevEval.cpcToMove))
			return -(float)(100 - depth);
		else
			return 0.0f;
	}

#ifdef LATER
	/* checkmates were detected already, so GsTestGameOver can only return draws */
	GS gs = bdgmvevEval.GsTestGameOver(bdgmvevEval.rgmvReplyAll, *ga.prule);
	assert(gs != GS::BlackCheckMated && gs != GS::WhiteCheckMated);
	if (gs != GS::Playing)
		return 0.0f;
#endif

	return evalAlpha;
}


/*	PL::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the 
 *	previous move player, i.e., it evaluates the previous move.
 */
float PL::EvalBdgQuiescent(BDGMVEV& bdgmvevEval, int depth, float evalAlpha, float evalBeta)
{
	bdgmvevEval.RemoveInCheckMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);
	bdgmvevEval.RemoveQuiescentMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);

	if (bdgmvevEval.rgmvReplyAll.size() == 0)
		return -EvalBdg(bdgmvevEval, true);

	vector<BDGMVEV> rgbdgmvev;
	FillRgbdgmvev(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);

	for (BDGMVEV bdgmvev : rgbdgmvev) {
		float eval = -EvalBdgQuiescent(bdgmvev, depth + 1, -evalBeta, -evalAlpha);
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}

	return evalAlpha;
}


void PL::FillRgbdgmvev(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev) const
{
	rgbdgmvev.reserve(rgmv.size());
	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev, false);
		rgbdgmvev.push_back(move(bdgmvev));
	}
}


void PL::PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev) const
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
float PL::EvalBdg(const BDGMVEV& bdgmvev, bool fFull) const
{
	float vpcNext = bdgmvev.VpcTotalFromCpc(bdgmvev.cpcToMove);
	float vpcSelf = bdgmvev.VpcTotalFromCpc(CpcOpposite(bdgmvev.cpcToMove));
	float evalMat = (float)(vpcSelf - vpcNext) / (float)(vpcSelf + vpcNext);

	static vector<MV> rgmvSelf;
	bdgmvev.GenRgmvColor(rgmvSelf, CpcOpposite(bdgmvev.cpcToMove), false);
	float evalMob = (float)((int)rgmvSelf.size() - (int)bdgmvev.rgmvReplyAll.size()) /
		(float)((int)rgmvSelf.size() + (int)bdgmvev.rgmvReplyAll.size());

	float evalControl = 0.0f;
#ifdef LATER
	if (fFull) {
		if (rgfAICoeff[2])
			evalControl = EvalBdgControl(bdgmvev, rgmvSelf);
	}
#endif

	return rgfAICoeff[0] * evalMat + rgfAICoeff[1] * evalMob + rgfAICoeff[2] * evalControl;
}


float PL::EvalBdgControl(const BDGMVEV& bdgmvev, const vector<MV>& rgmvSelf) const
{
	int mpsqcmvAttack[128] = { 0 };
	for (MV mv : rgmvSelf)
		mpsqcmvAttack[mv.SqTo()]++;
	for (MV mv : bdgmvev.rgmvReplyAll)
		mpsqcmvAttack[mv.SqTo()]--;
	float eval = 0.0;
	float mpsqcoeffControl[128] = { 0.0f };
	SQ sqKing = bdgmvev.mptpcsq[cpcWhite][tpcKing];
	FillKingCoeff(mpsqcoeffControl, sqKing);
	sqKing = bdgmvev.mptpcsq[cpcBlack][tpcKing];
	FillKingCoeff(mpsqcoeffControl, sqKing);
	for (SQ sq = 0; sq < 128; sq++)
		eval += mpsqcmvAttack[sq] * mpsqcoeffControl[sq];
	return eval / 15.0f;
}

void PL::FillKingCoeff(float mpsqcoeffControl[], SQ sq) const
{
	mpsqcoeffControl[sq] = 1.0f;
	int rgdsq1[] = { -17, -16, -15, -1, 1, 15, 16, 17 };
	int rgdsq2[] = { -34, -33, -32, -31, -30, -18, -2, 2, 18, 30, 31, 32, 33, 34 };

	for (int idsq = 0; idsq < CArray(rgdsq1); idsq++) {
		int dsq = rgdsq1[idsq];
		if (((sq + dsq) & 0x88) == 0)
			mpsqcoeffControl[sq + dsq] = 1.0f;
	}
	for (int idsq = 0; idsq < CArray(rgdsq2); idsq++) {
		int dsq = rgdsq2[idsq];
		if (((sq + dsq) & 0x88) == 0)
			mpsqcoeffControl[sq + dsq] = 0.5f;
	}
} 