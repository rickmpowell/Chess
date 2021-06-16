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


class BDGMV : public BDG
{
public:
	MV mv;
	float eval;
	GMV gmvReplyAll;	// all reply moves, including illegal moves that leave king in check

	BDGMV(void) : mv(MV()), eval(0.0f) { }

	BDGMV(const BDG& bdg, MV mv) : BDG(bdg), mv(mv), eval(0.0f)
	{
		MakeMv(mv);
		gmvReplyAll.Reserve(50);
		GenRgmvColor(gmvReplyAll, cpcToMove, false);
	}

	bool operator<(const BDGMV& bdgmv) const
	{
		return eval < bdgmv.eval;
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

	MV mvNext;
	SPMV spmvNext;

public:
	PL(GA& ga, wstring szName);
	virtual ~PL(void);

	wstring& SzName(void) 
	{
		return szName;
	}

	void SetName(const wstring& szNew) 
	{
		szName = szNew;
	}

	virtual MV MvGetNext(SPMV& spmv) = 0;

	void ReceiveMv(MV mv, SPMV spmv);
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
	unsigned long cYield;
	size_t cbdgmvEval;
	size_t cbdgmvGen;
	size_t cbdgmvPrune;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(SPMV& spmv);

protected:
	float EvalBdgDepth(BDGMV& bdgmv, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule);
	float EvalBdgQuiescent(BDGMV& bdgev, int depth, float evalAlpha, float evalBeta);
	void PreSortMoves(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv);
	void FillRgbdgmvev(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv);

	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual float EvalBdg(const BDGMV& bdgmv, bool fFull);

	float VpcFromCpc(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcOpening(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcEarlyMidGame(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcMiddleGame(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcLateMidGame(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcEndGame(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcLateEndGame(const BDGMV& bdgmv, CPC cpcMove) const;
	float VpcWeightTable(const BDGMV& bdgmv, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const;
};


class PLAI2 : public PLAI
{
public:
	PLAI2(GA& ga);
	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual float EvalBdg(const BDGMV& bdgmv, bool fFull);
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
	virtual MV MvGetNext(SPMV& spmv);
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
	virtual void Draw(RCF rcfUpdate);
	void SetPl(PL* pplNew);
};

