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
	int level;

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
	
	virtual bool FHasLevel(void) const
	{
		return false;
	}

	virtual int Level(void) const
	{
		return level;
	}

	virtual void SetLevel(int level)
	{
		this->level = level;
	}

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
	virtual bool FHasLevel(void) const;
	virtual void SetLevel(int level);

protected:
	float EvalBdgDepth(BDGMV& bdgmv, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule);
	float EvalBdgQuiescent(BDGMV& bdgev, int depth, float evalAlpha, float evalBeta);
	void PreSortMoves(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv);
	void FillRgbdgmv(const BDG& bdg, const GMV& gmv, vector<BDGMV>& vbdgmv);

	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual float EvalBdg(const BDGMV& bdgmv, bool fFull);
	float CmvFromLevel(int level) const;


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



/*
 *
 *	RGINFOPL
 * 
 *	The list of available players we have to play a game.
 * 
 */

enum class TPL	// types of players
{
	AI,
	Human,
	Stream
};

enum class IDCLASSPL	// player classes
{
	AI,
	AI2,
	Human
};

struct INFOPL
{
	IDCLASSPL idclasspl;
	TPL tpl;
	wstring szName;
	int level;
	INFOPL(IDCLASSPL idclasspl, TPL tpl, const wstring& szName, int level = 0) : idclasspl(idclasspl), tpl(tpl), szName(szName), level(level)
	{
	}
};


class RGINFOPL
{
public:
	vector <INFOPL> vinfopl;

	RGINFOPL(void);
	~RGINFOPL(void);
	PL* PplFactory(GA& ga, int iinfopl) const;
	int IdbFromInfopl(const INFOPL& infopl) const;
};

extern RGINFOPL rginfopl;