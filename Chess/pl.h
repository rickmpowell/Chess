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

	virtual MV MvGetNext(BDG bdg);
	void PreSortRgmv(BDG& bdg, vector<MV>& rgmv, vector<MV>& rgmvScratch, unsigned imvFirst, unsigned imvLim) const;
	int CmpEvalMv(BDG& bdg, MV mv1, MV mv2) const;
	float EvalBdg(const BDG& bdg) const;
	float EvalBdgDepth(BDG& bdg, int depth, int depthMax, float evalAlpha, float evalBeta) const;
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

