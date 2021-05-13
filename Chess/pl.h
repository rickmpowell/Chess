/*
 *
 *	pl.h
 * 
 *	Definitions for players of the game
 * 
 */
#pragma once

#include "framework.h"
#include "bd.h"


class BDGMVEV : public BDG
{
public:
	MV mv;
	float eval;
	vector<MV> rgmvReplyAll;	// all reply moves, including illegal moves that leave king in check

	BDGMVEV(void) : mv(MV()), eval(0.0f) { }

	BDGMVEV(const BDG& bdg, MV mv) : BDG(bdg), mv(mv), eval(0.0f) 
	{ 
		MakeMv(mv); 
		rgmvReplyAll.reserve(50);  
		GenRgmvColor(rgmvReplyAll, cpcToMove, false); 
	}
};


/*
 *
 *	PL class
 * 
 *	The player class
 * 
 */


class GA;

class PL
{
	wstring szName;
	float rgfAICoeef[2];
public:
	PL(wstring szName, const float rgfAICoeef[]);
	~PL(void);
	wstring& SzName(void) {
		return szName;
	}

	void SetName(const wstring& szNew) {
		szName = szNew;
	}

	virtual MV MvGetNext(GA& ga);
	float EvalBdgDepth(BDGMVEV& bdg, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule) const;
	float EvalBdgQuiescent(BDGMVEV& bdg, int depth, float evalAlpha, float evalBeta) const;
	void PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdg) const;
	void FillRgbdgmvev(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev) const;
	void SortRgbdgmvev(vector<BDGMVEV>& rgbdg, vector<BDGMVEV>& rgbdgScratch, unsigned ibdgFirst, unsigned ibdgLim) const;
	float EvalBdg(const BDGMVEV& bdgmvev) const;
};


#include "ui.h"


/*
 *
 *	UIPL
 *
 *	Player name UI element in the move list. Pretty simple control.
 *
 */

class UIPL : public UI
{
private:
	PL* ppl;
	CPC cpc;
public:
	UIPL(UI* puiParent, CPC cpc);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void SetPl(PL* pplNew);
};

