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


PL::PL(wstring szName, const float* rgfAICoeef) : szName(szName)
{
	memcpy(this->rgfAICoeef, rgfAICoeef, sizeof(this->rgfAICoeef));
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
	const float cmvSearch = 500000.0f;
	const float fracAlphaBeta = 3.0f; // cuts moves we analyze by this factor
	float size2 = (float)(rgmv.size() * rgmvOpp.size());
	int depthMax = (int)round(2.0f*log(cmvSearch) / (log(size2)-log(2.0f*fracAlphaBeta)));
	if (depthMax < 4)
		depthMax = 4;

	/* and find the best move */

	bdg.GenRgmv(rgmv, RMCHK::Remove);
	vector<BDGMVEV> rgbdgmvev;
	PreSortMoves(bdg, rgmv, rgbdgmvev);

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
float PL::EvalBdgDepth(BDGMVEV& bdgmvevEval, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule) const
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdgmvevEval, depth, evalAlpha, evalBeta);

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

	/* TODO: check for 3-position repeat draw */

	return evalAlpha;
}


/*	PL::EvalBdgQuiescent
 *
 *	Returns the quiescent evaluation of the board from the point of view of the 
 *	previous move player, i.e., it evaluates the previous move.
 */
float PL::EvalBdgQuiescent(BDGMVEV& bdgmvevEval, int depth, float evalAlpha, float evalBeta) const
{
	float eval = -bdgmvevEval.eval;
	
	bdgmvevEval.RemoveInCheckMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);
	bdgmvevEval.RemoveQuiescentMoves(bdgmvevEval.rgmvReplyAll, bdgmvevEval.cpcToMove);

	if (bdgmvevEval.rgmvReplyAll.size() == 0)
		return eval;

#ifdef NOPE
	/* include no-move as possible quiescent stopping point */

	if (eval >= evalBeta)
		return evalBeta;
	if (eval > evalAlpha)
		evalAlpha = eval;
#endif

	vector<BDGMVEV> rgbdgmvev;
	FillRgbdgmvev(bdgmvevEval, bdgmvevEval.rgmvReplyAll, rgbdgmvev);

	for (BDGMVEV bdgmvev : rgbdgmvev) {
		eval = -EvalBdgQuiescent(bdgmvev, depth + 1, -evalBeta, -evalAlpha);
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
		bdgmvev.eval = EvalBdg(bdgmvev);
		rgbdgmvev.push_back(bdgmvev);
	}
}


void PL::PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev) const
{
	/* insertion sort */

	for (MV mv : rgmv) {
		BDGMVEV bdgmvev(bdg, mv);
		bdgmvev.eval = EvalBdg(bdgmvev);
		unsigned imvFirst, imvLim;
		for (imvFirst = 0, imvLim = (unsigned)rgbdgmvev.size(); ; ) {
			if (imvFirst == imvLim) {
				rgbdgmvev.insert(rgbdgmvev.begin()+imvFirst, bdgmvev);
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


void PL::SortRgbdgmvev(vector<BDGMVEV>& rgbdg, vector<BDGMVEV>& rgbdgScratch, unsigned ibdgFirst, unsigned ibdgLim) const
{
	if (ibdgLim - ibdgFirst <= 1)
		return;
	unsigned ibdgMid = (ibdgLim + ibdgFirst) / 2;
	SortRgbdgmvev(rgbdg, rgbdgScratch, ibdgFirst, ibdgMid);
	SortRgbdgmvev(rgbdg, rgbdgScratch, ibdgMid, ibdgLim);

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
 *	Evaluates the board from the point of view of the color that just moved.
 */
float PL::EvalBdg(const BDGMVEV& bdgmvev) const
{
	float vpcNext = bdgmvev.VpcTotalFromCpc(bdgmvev.cpcToMove);
	float vpcSelf = bdgmvev.VpcTotalFromCpc(CpcOpposite(bdgmvev.cpcToMove));
	float evalMat = (float)(vpcSelf - vpcNext) / (float)(vpcSelf + vpcNext);

	static vector<MV> rgmvSelf;
	bdgmvev.GenRgmvColor(rgmvSelf, CpcOpposite(bdgmvev.cpcToMove), false);
	float evalMob = (float)((int)rgmvSelf.size() - (int)bdgmvev.rgmvReplyAll.size()) /
		(float)((int)rgmvSelf.size() + (int)bdgmvev.rgmvReplyAll.size());

	return rgfAICoeef[0] * evalMat + rgfAICoeef[1] * evalMob;
}


