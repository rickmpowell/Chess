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


float PL::EvalBdg(BDG bdg)
{
	static vector<MV> rgmvSelf;
	rgmvSelf.reserve(50);
	bdg.GenRgmv(rgmvSelf, RMCHK::Remove);
	bdg.TestGameOver(rgmvSelf);
	switch (bdg.gs) {
	case GS::Playing:
	{
		int cSelf = bdg.CMaterial(bdg.cpcToMove);
		int cOpp = bdg.CMaterial(bdg.cpcToMove ^ 1);
		float evalMat = (float)(cSelf - cOpp) / (float)(cSelf + cOpp);

		static vector<MV> rgmvOpp;
		rgmvOpp.reserve(50);
		bdg.GenRgmvColor(rgmvSelf, bdg.cpcToMove);
		bdg.GenRgmvColor(rgmvOpp, bdg.cpcToMove ^ 1);
		float evalMob = (float)((int)rgmvSelf.size() - (int)rgmvOpp.size()) /
			(float)((int)rgmvSelf.size() + (int)rgmvOpp.size());

		return 2.5f * evalMob + 5.f * evalMat;
	}
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		return bdg.cpcToMove == cpcWhite ? -100.0f : 100.0f;
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		return bdg.cpcToMove == cpcBlack ? -100.0f : 100.0f;
		break;
	default:
		return 0.0;
	}
}


MV PL::MvGetNext(const BDG& bdg)
{
	vector <MV> rgmv;
	bdg.GenRgmv(rgmv, RMCHK::Remove);
	if (rgmv.size() == 0)
		return MV();
	MV mvBest;
	float evalBest = -1000.f;
	for (MV mv : rgmv) {
		BDG bdgT = bdg;
		bdgT.MakeMv(mv);
		float eval = -EvalBdgDepth(bdgT, 3);
		if (eval > evalBest) {
			evalBest = eval;
			mvBest = mv;
		}
	}
	return mvBest;
}

float PL::EvalBdgDepth(BDG bdg, int depth)
{
	if (depth == 0)
		return EvalBdg(bdg);
	vector<MV> rgmv;
	rgmv.reserve(100);
	bdg.GenRgmv(rgmv, RMCHK::Remove);
	bdg.TestGameOver(rgmv);
	switch (bdg.gs) {
	case GS::Playing:
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		return bdg.cpcToMove == cpcWhite ? -100.0f : 100.0f;
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		return bdg.cpcToMove == cpcBlack ? -100.0f : 100.0f;
	default:	/* the rest are all draws */
		return 0.0;
	}

	float evalBest = -1000.0f;
	BDG bdgT = bdg;
	for (MV mv : rgmv) {
		bdgT.MakeMv(mv);
		float eval = -EvalBdgDepth(bdgT, depth - 1);
		if (eval > evalBest)
			evalBest = eval;
		bdgT.UndoMv();
	}
	return evalBest;
}