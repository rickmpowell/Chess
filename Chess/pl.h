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


/*
 *
 *	MVEV structure
 * 
 *	Just a little holder of move evaluation information which is used for 
 *	generating AI move lists and alpha-beta pruning.
 * 
 */


class MVEV 
{
public:
	MV mv;
	float eval;
	GMV gmvReplyAll;	// all reply moves, including illegal moves that leave king in check

	MVEV(void) : mv(MV()), eval(0.0f) { }

	MVEV(BDG& bdg, MV mv) : mv(mv), eval(0.0f)
	{
		bdg.MakeMv(mv);
		gmvReplyAll.Reserve(50);
		bdg.GenGmvColor(gmvReplyAll, bdg.cpcToMove);
	}

	bool operator<(const MVEV& mvev) const
	{
		return eval < mvev.eval;
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

	bool FDepthLog(LGT lgt, int& depth);
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
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
	uint16_t cYield;

	/* logging */
	time_point<high_resolution_clock> tpStart;
	size_t cmvevEval;
	size_t cmvevGen;
	size_t cmvevPrune;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(SPMV& spmv);
	virtual bool FHasLevel(void) const;
	virtual void SetLevel(int level);

protected:
	float EvalBdgDepth(BDG& bdg, MVEV& mvev, int depth, int depthMax, float evalAlpha, float evalBeta, const RULE& rule);
	float EvalBdgQuiescent(BDG& bdg, MVEV& mvev, int depth, float evalAlpha, float evalBeta);
	void PreSortVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev);
	void FillVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev);

	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual float EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull);
	float CmvFromLevel(int level) const;

	void StartMoveLog(void);
	void EndMoveLog(void);

	float VpcFromCpc(const BDG& bdg, CPC cpcMove) const;
	float VpcOpening(const BDG& bdg, CPC cpcMove) const;
	float VpcEarlyMidGame(const BDG& bdg, CPC cpcMove) const;
	float VpcMiddleGame(const BDG& bdg, CPC cpcMove) const;
	float VpcLateMidGame(const BDG& bdg, CPC cpcMove) const;
	float VpcEndGame(const BDG& bdg, CPC cpcMove) const;
	float VpcLateEndGame(const BDG& bdg, CPC cpcMove) const;
	float VpcWeightTable(const BDG& bdg, CPC cpcMove, const float mpapcsqeval[APC::ActMax][64]) const;
};


class PLAI2 : public PLAI
{
public:
	PLAI2(GA& ga);
	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual float EvalBdg(BDG& bdg, const MVEV& mvev, bool fFull);
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