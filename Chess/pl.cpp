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


PL::PL(wstring szName) : szName(szName)
{
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
MV PL::MvGetNext(GA& ga)
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
	int depthMax = (int)round(log(10000000.0f*10000000.0f) / (log((float)(rgmv.size()*rgmvOpp.size()))));
	if (depthMax < 5)
		depthMax = 5;

	/* and find the best move */

	bdg.GenRgmv(rgmv, RMCHK::Remove);
	vector<BDGMVEV> rgbdg;
	PreSortMoves(bdg, rgmv, rgbdg);

	MV mvBest;
	float evalAlpha = -1000.0f, evalBeta = 1000.0f;
	for (BDGMVEV& bdgmvev : rgbdg) {
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
 *	Evaluates the board from the point of view of the person who made the previous move.
 */
float PL::EvalBdgDepth(BDG& bdg, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule) const
{
	vector<MV> rgmv;	/* can't be static because this is called recursively */
	rgmv.reserve(50);
	bdg.GenRgmv(rgmv, RMCHK::Remove);

	switch (bdg.GsTestGameOver(rgmv, rule)) {
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		return (float)-(100 - depth);
	default:	/* draws */
		return 0.0;
	case GS::Playing:
		if (depth >= depthMax)
			return EvalBdgQuiescent(bdg, depth, evalAlpha, evalBeta);
		break;
	}

	/* find the best move */

	vector<BDGMVEV> rgbdg;
	PreSortMoves(bdg, rgmv, rgbdg);

	for (BDGMVEV& bdgmvev : rgbdg) {
		float eval = -EvalBdgDepth(bdgmvev, depth+1, depthMax, -evalBeta, -evalAlpha, rule);
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}
	return evalAlpha;
}


/*	PL::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the previous 
 *	move player, i.e., it evaluates the previous move.
 */
float PL::EvalBdgQuiescent(BDG& bdg, int depth, float evalAlpha, float evalBeta) const
{
	float eval = -EvalBdg(bdg);

	vector<MV> rgmv;	/* can't be static because this is called recursively */
	rgmv.reserve(50);
	bdg.GenRgmvQuiescent(rgmv, RMCHK::Remove);

	if (rgmv.size() == 0)
		return eval;

	/* include no-move as possible quiescent stopping point */
	if (eval >= evalBeta)
		return evalBeta;
	if (eval > evalAlpha)
		evalAlpha = eval;

	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		eval = -EvalBdgQuiescent(bdg, depth + 1, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}
	return evalAlpha;
}


void PL::PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdg) const
{
	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev);
		rgbdg.push_back(bdgmvev);
	}
	static vector<BDGMVEV> rgbdgScratch;
	rgbdgScratch.resize(rgbdg.size());
	PreSortRgbdgmvev(rgbdg, rgbdgScratch, 0, (unsigned)rgbdg.size());
}


void PL::PreSortRgbdgmvev(vector<BDGMVEV>& rgbdg, vector<BDGMVEV>& rgbdgScratch, unsigned ibdgFirst, unsigned ibdgLim) const
{
	if (ibdgLim - ibdgFirst <= 1)
		return;
	unsigned ibdgMid = (ibdgLim + ibdgFirst) / 2;
	PreSortRgbdgmvev(rgbdg, rgbdgScratch, ibdgFirst, ibdgMid);
	PreSortRgbdgmvev(rgbdg, rgbdgScratch, ibdgMid, ibdgLim);

	unsigned ibdgLeft, ibdgRight, ibdgOut;
	for (ibdgOut = ibdgFirst, ibdgLeft = ibdgFirst, ibdgRight = ibdgMid; ibdgLeft < ibdgMid && ibdgRight < ibdgLim; ) {
		if (rgbdg[ibdgLeft].eval >= rgbdg[ibdgRight].eval)
			swap(rgbdgScratch[ibdgOut++], rgbdg[ibdgLeft++]);
		else
			swap(rgbdgScratch[ibdgOut++], rgbdg[ibdgRight++]);
	}
	while (ibdgLeft < ibdgMid)
		swap(rgbdgScratch[ibdgOut++], rgbdg[ibdgLeft++]);
	for (unsigned ibdg = ibdgFirst; ibdg < ibdgOut; ibdg++)
		swap(rgbdg[ibdg], rgbdgScratch[ibdg]);
}


/*	PL::EvalBdg
 *
 *	Evaluates the board from the point of view of the cpc color
 */
float PL::EvalBdg(const BDG& bdg) const
{
	float vpcNext = bdg.VpcTotalFromCpc(bdg.cpcToMove);
	float vpcSelf = bdg.VpcTotalFromCpc(CpcOpposite(bdg.cpcToMove));
	float evalMat = (float)(vpcSelf - vpcNext) / (float)(vpcSelf + vpcNext);

	static vector<MV> rgmvNext;
	static vector<MV> rgmvSelf;
	rgmvNext.reserve(50);
	rgmvSelf.reserve(50);
	bdg.GenRgmvColor(rgmvNext, bdg.cpcToMove, false);
	bdg.GenRgmvColor(rgmvSelf, CpcOpposite(bdg.cpcToMove), false);
	float evalMob = (float)((int)rgmvSelf.size() - (int)rgmvNext.size()) /
		(float)((int)rgmvSelf.size() + (int)rgmvNext.size());

	return 2.5f * evalMob + 5.f * evalMat;
}


