/*
 *
 *	pl.h
 * 
 *	Definitions for players of the game
 * 
 */
#pragma once

#include "ui.h"
#include "bd.h"


class BDGMVEV : public BDG
{
public:
	MV mv;
	float eval;
	vector<MV> rgmvReplyAll;	// all reply moves, including illegal moves that leave king in check

	inline BDGMVEV(void) : mv(MV()), eval(0.0f) { }

	inline BDGMVEV(const BDG& bdg, MV mv) : BDG(bdg), mv(mv), eval(0.0f) 
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
protected:
	GA& ga;
	wstring szName;

public:
	PL(GA& ga, wstring szName);
	~PL(void);
	wstring& SzName(void) {
		return szName;
	}

	void SetName(const wstring& szNew) {
		szName = szNew;
	}

	virtual MV MvGetNext(void) = 0;
};


/*
 *
 *	PLAI class
 * 
 *	Player using an AI (with alpha-beta pruning and static board evaluiation) 
 *	to compute moves
 * 
 */


class PLAI : public PL
{
protected:
	float rgfAICoeff[3];
	long cYield;
	size_t cbdgmvevEval;
	size_t cbdgmvevGen;
	size_t cbdgmvevPrune;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(void);

protected:
	float EvalBdgDepth(BDGMVEV& bdg, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule);
	float EvalBdgQuiescent(BDGMVEV& bdg, int depth, float evalAlpha, float evalBeta);
	void PreSortMoves(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdg);
	void FillRgbdgmvev(const BDG& bdg, const vector<MV>& rgmv, vector<BDGMVEV>& rgbdgmvev);
	float EvalBdg(const BDGMVEV& bdgmvev, bool fFull);

	float VpcFromCpc(const BDGMVEV& bdgmvev, CPC cpcMove) const;
	float VpcOpening(const BDGMVEV& bdgmvev, CPC cpcMove) const;
	float VpcMiddleGame(const BDGMVEV& bdgmvev, CPC cpcMove) const;
	float VpcEndGame(const BDGMVEV& bdgmvev, CPC cpcMove) const;
	float VpcWeightTable(const BDGMVEV& bdgmvev, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const;
};


class PLAI2 : public PLAI
{
public:
	PLAI2(GA& ga);
};


/*
 *
 *	PLHUMAN player class
 * 
 *	Prompts the user for moves.
 * 
 */


class PLHUMAN : public PL
{
public:
	PLHUMAN(GA& ga, wstring szName);
	virtual MV MvGetNext(void);
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
	virtual SIZF SizfLayoutPreferred(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void SetPl(PL* pplNew);
};

