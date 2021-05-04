/*
 *
 *	pl.cpp
 * 
 *	Code for the player class
 * 
 */

#include "pl.h"
#include "ga.h"

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
MV PL::MvGetNext(BDG bdg)
{
	/* generate all moves without removing checks. we'll use this as a heuristic for the amount of
	 * mobillity on the board, which we can use to estimate the depth we can search */

	static vector <MV> rgmv;
	bdg.GenRgmv(rgmv, RMCHK::NoRemove);
	if (rgmv.size() == 0)
		return MV();
	static vector<MV> rgmvOpp;
	bdg.GenRgmvColor(rgmvOpp, CpcOpposite(bdg.cpcToMove));
	int depth = (int)round(log(20000.0f*20000.0f) / (log((float)(rgmv.size()*rgmvOpp.size()))));
	if (depth < 3)
		depth = 3;

	/* now that we have that, get rid of those illegal check moves */

//	bdg.RemoveInCheckMoves(rgmv, bdg.cpcToMove);
//	if (rgmv.size() == 0)
//		return MV();

	/* and find the best move - we have illegal moves in this move list, so be sure to skip
	   them */
	
	MV mvBest;
	float evalBest = -1000.f;
	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		if (!bdg.FSqAttacked(bdg.SqFromTpc(tpcKing, CpcOpposite(bdg.cpcToMove)), bdg.cpcToMove)) {
			float eval = -EvalBdgDepth(bdg, depth, -1000.0, 1000.0);
			if (eval > evalBest) {
				evalBest = eval;
				mvBest = mv;
			}
		}
		bdg.UndoMv();
	}
	
	return mvBest;
}


float PL::EvalBdgDepth(BDG& bdg, int depth, float evalAlpha, float evalBeta) const
{
	vector<MV> rgmv;	/* can't be static because this is called recursively */
	rgmv.reserve(50);
	bdg.GenRgmv(rgmv, RMCHK::Remove);

	switch (bdg.GsTestGameOver(rgmv)) {

	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		return -100.0f;

	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		return -100.0f;

	default:	/* draws */
		return 0.0;

	case GS::Playing:
		if (depth == 0)
			return EvalBdg(bdg);
		break;
	}

	/* find the best move */

	for (MV mv : rgmv) {
		bdg.MakeMv(mv);
		float eval = -EvalBdgDepth(bdg, depth - 1, -evalBeta, -evalAlpha);
		bdg.UndoMv();
		/* prune if the move is too good */
		if (eval >= evalBeta)
			return evalBeta;
		if (eval > evalAlpha)
			evalAlpha = eval;
	}
	return evalAlpha;
}


/*	PL::EvalBdg
 *
 *	Evaluates the board from the point of view of the cpc color
 */
float PL::EvalBdg(const BDG& bdg) const
{
	int cSelf = bdg.CMaterial(bdg.cpcToMove);
	int cOpp = bdg.CMaterial(CpcOpposite(bdg.cpcToMove));
	float evalMat = (float)(cSelf - cOpp) / (float)(cSelf + cOpp);

	static vector<MV> rgmvOpp;
	static vector<MV> rgmv;
	rgmv.reserve(50);
	rgmvOpp.reserve(50);
	bdg.GenRgmvColor(rgmv, bdg.cpcToMove);
	bdg.GenRgmvColor(rgmvOpp, CpcOpposite(bdg.cpcToMove));
	float evalMob = (float)((int)rgmv.size() - (int)rgmvOpp.size()) /
		(float)((int)rgmv.size() + (int)rgmvOpp.size());

	return 2.5f * evalMob + 5.f * evalMat;
}


