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
	EVAL eval;
	MV mvReplyBest;
	GMV gmvPseudoReply;	// all reply pseudo moves

	MVEV(MV mv=MV()) : mv(mv), eval(0), mvReplyBest(MV())
	{
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
	
	virtual bool FHasLevel(void) const noexcept
	{
		return false;
	}

	virtual int Level(void) const noexcept
	{
		return level;
	}

	virtual void SetLevel(int level) noexcept
	{
		this->level = level;
	}

	void ReceiveMv(MV mv, SPMV spmv);

	bool FDepthLog(LGT lgt, int& depth);
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
	int DepthLog(void) const;
	void SetDepthLog(int depthNew);
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
	EVAL mpapcsqevalOpening[APC::ActMax][sqMax];
	EVAL mpapcsqevalMiddleGame[APC::ActMax][sqMax];
	EVAL mpapcsqevalEndGame[APC::ActMax][sqMax];
	uint16_t rgfAICoeffNum[3];	/* coefficient is this number divided by 100 */
	uint16_t cYield;

	/* logging statistics */
	time_point<high_resolution_clock> tpStart;
	size_t cmvevEval;
	size_t cmvevNode;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(SPMV& spmv);
	virtual bool FHasLevel(void) const noexcept;
	virtual void SetLevel(int level) noexcept;

protected:
	EVAL EvalBdgDepth(BDG& bdg, MVEV& mvev, int depth, int depthMax, EVAL evalAlpha, EVAL evalBeta);
	EVAL EvalBdgQuiescent(BDG& bdg, MVEV& mvev, int depth, EVAL evalAlpha, EVAL evalBeta); 
	bool FAlphaBetaPrune(const MVEV& mvev, MVEV& mvevBest, EVAL& evalAlpha, EVAL& evalBeta, int& depthMax) const noexcept;
	void PreSortVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept;
	void FillVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept;
	void PumpMsg(void);

	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual EVAL EvalBdgStatic(BDG& bdg, MVEV& mvev, bool fFull) noexcept;
	float CmvFromLevel(int level) const noexcept;

	void StartMoveLog(void);
	void EndMoveLog(void);

	EVAL EvalBaseApc(APC apc) const noexcept;
	void InitWeightTables(void);
	void InitWeightTable(const EVAL mpapceval[APC::ActMax], const EVAL mpapcsqdeval[APC::ActMax][64], EVAL mpapcsqeval[APC::ActMax][64]);
	EVAL EvalPstFromCpc(const BDG& bdg, CPC cpcMove) const noexcept;
	EVAL EvalInterpolate(int phase, EVAL eval1, int phase1, EVAL eval2, int phase2) const noexcept;
	EVAL EvalBdgAttackDefend(BDG& bdg, SQ sqTo) const noexcept;
	EVAL EvalTempo(const BDG& bdg, CPC cpc) const noexcept;
};


class PLAI2 : public PLAI
{
public:
	PLAI2(GA& ga);
	virtual int DepthMax(const BDG& bdg, const GMV& gmv) const;
	virtual EVAL EvalBdgStatic(BDG& bdg, MVEV& mvev, bool fFull) noexcept;
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