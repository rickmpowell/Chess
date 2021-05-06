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
	int depthMax = (int)round(log(50000.0f*50000.0f) / (log((float)(rgmv.size()*rgmvOpp.size()))));
	if (depthMax < 3)
		depthMax = 3;

	/* and find the best move - we have illegal moves in this move list, so be sure to skip
	   them */
	
	MV mvBest;
	float evalBest = -1000.f;
	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		if (!bdg.FSqAttacked(bdg.SqFromTpc(tpcKing, CpcOpposite(bdg.cpcToMove)), bdg.cpcToMove)) {
			float eval = -EvalBdgDepth(bdg, 0, depthMax, -1000.0, 1000.0);
			ga.uidb.AddEval(bdg, mv, eval);
			if (eval > evalBest) {
				evalBest = eval;
				mvBest = mv;
			}
		}
		bdg.UndoMv();
	}
	
	return mvBest;
}


float PL::EvalBdgDepth(BDG& bdg, int depth, int depthMax, float evalAlpha, float evalBeta) const
{
	vector<MV> rgmv;	/* can't be static because this is called recursively */
	rgmv.reserve(50);
	bdg.GenRgmv(rgmv, RMCHK::Remove);

	switch (bdg.GsTestGameOver(rgmv)) {
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

//	vector<MV> rgmvScratch;
//	PreSortRgmv(bdg, rgmv, rgmvScratch, 0, (unsigned)rgmv.size());

	/* find the best move */

	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		float eval = -EvalBdgDepth(bdg, depth+1, depthMax, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		/* prune if the move is too good */
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}
	return evalAlpha;
}


float PL::EvalBdgQuiescent(BDG& bdg, int depth, float evalAlpha, float evalBeta) const
{
	float eval = EvalBdg(bdg);

	vector<MV> rgmv;	/* can't be static because this is called recursively */
	rgmv.reserve(50);
	bdg.GenRgmvQuiescent(rgmv, RMCHK::Remove);

	if (rgmv.size() == 0)
		return eval;

	if (eval >= evalBeta)
		return evalBeta;
	if (eval > evalAlpha)
		evalAlpha = eval;

	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		eval = -EvalBdgQuiescent(bdg, depth + 1, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		/* prune if the move is too good */
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}
	return evalAlpha;
}


int PL::CmpEvalMv(BDG& bdg, MV mv1, MV mv2) const
{
	bdg.MakeMv(mv1);
	float eval1 = EvalBdg(bdg);
	bdg.UndoMv();
	bdg.MakeMv(mv2);
	float eval2 = EvalBdg(bdg);
	bdg.UndoMv();
	return eval1 < eval2 ? -1 : (eval2 > eval2 ? 1 : 0);
}


void PL::PreSortRgmv(BDG& bdg, vector<MV>& rgmv, vector<MV>& rgmvScratch, unsigned imvFirst, unsigned imvLim) const
{
	if (imvLim - imvFirst <= 1)
		return;
	unsigned imvMid = (imvLim + imvFirst) / 2;
	PreSortRgmv(bdg, rgmv, rgmvScratch, imvFirst, imvMid);
	PreSortRgmv(bdg, rgmv, rgmvScratch, imvMid + 1, imvLim);
	rgmvScratch.clear();

	unsigned imvLeft, imvRight, imvOut;
	for (imvOut = imvFirst, imvLeft = imvFirst, imvRight = imvMid+1; imvLeft < imvMid && imvRight < imvLim; ) {
		if (CmpEvalMv(bdg, rgmvScratch[imvLeft], rgmvScratch[imvRight]) > 0)
			rgmvScratch[imvOut++] = rgmv[imvLeft++];
		else
			rgmvScratch[imvOut++] = rgmv[imvRight++];
	}
	while (imvLeft < imvMid)
		rgmvScratch[imvOut++] = rgmv[imvLeft++];
	while (imvRight < imvLim)
		rgmvScratch[imvOut++] = rgmv[imvRight++];
	rgmv = rgmvScratch;
}


/*	PL::EvalBdg
 *
 *	Evaluates the board from the point of view of the cpc color
 */
float PL::EvalBdg(const BDG& bdg) const
{
	float vpcSelf = bdg.VpcFromCpc(bdg.cpcToMove);
	float vpcOpp = bdg.VpcFromCpc(CpcOpposite(bdg.cpcToMove));
	float evalMat = (float)(vpcSelf - vpcOpp) / (float)(vpcSelf + vpcOpp);

	static vector<MV> rgmvOpp;
	static vector<MV> rgmv;
	rgmv.reserve(50);
	rgmvOpp.reserve(50);
	bdg.GenRgmvColor(rgmv, bdg.cpcToMove, false);
	bdg.GenRgmvColor(rgmvOpp, CpcOpposite(bdg.cpcToMove), false);
	float evalMob = (float)((int)rgmv.size() - (int)rgmvOpp.size()) /
		(float)((int)rgmv.size() + (int)rgmvOpp.size());

	return 2.5f * evalMob + 5.f * evalMat;
}


