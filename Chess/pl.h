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
	BDGMVEV(void) : mv(MV()), eval(0.0f) { }
	BDGMVEV(const BDG& bdg, MV mv) : BDG(bdg), mv(mv), eval(0.0f) { MakeMv(mv); }
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
public:
	PL(wstring szName);
	~PL(void);
	wstring& SzName(void) {
		return szName;
	}

	void SetName(const wstring& szNew) {
		szName = szNew;
	}

	virtual MV MvGetNext(GA& ga);
	void PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdg) const;
	void PreSortRgbdgmvev(vector<BDGMVEV>& rgbdg, vector<BDGMVEV>& rgbdgScratch, unsigned ibdgFirst, unsigned ibdgLim) const;
	int CmpEvalMv(const BDG& bdg1, const BDG& bdg2) const;
	float EvalBdg(const BDG& bdg) const;
	float EvalBdgDepth(BDG& bdg, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule) const;
	float EvalBdgQuiescent(BDG& bdg, int depth, float evalAlpha, float evalBeta) const;
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

