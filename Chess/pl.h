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

	virtual void SetFecoRandom(uint16_t) noexcept
	{
	}

	void ReceiveMv(MV mv, SPMV spmv);

	virtual EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	virtual EV EvBaseApc(APC apc) const noexcept;

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
	/* piece value tables */
	EV mpapcsqevOpening[APC::ActMax][sqMax];
	EV mpapcsqevMiddleGame[APC::ActMax][sqMax];
	EV mpapcsqevEndGame[APC::ActMax][sqMax];
	/* piece weight tables used to initialize piece tables above */
#include "eval_plai.h"

	/* coefficients this divided by 100 */
	uint16_t fecoMaterial, fecoMobility, fecoKingSafety, fecoPawnStructure, fecoTempo, fecoRandom;
	mt19937 rgen;	/* random number generator */
	uint64_t habdRand;	/* random number generated at the start of every search used to add randomness
						   to board eval - which is generated from the Zobrist hash */
	uint16_t cYield;
	bool fAbort;

	/* logging statistics */
	time_point<high_resolution_clock> tpStart;
	size_t cemvEval;
	size_t cemvNode;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(SPMV& spmv);
	virtual bool FHasLevel(void) const noexcept;
	virtual void SetLevel(int level) noexcept;
	virtual void SetFecoRandom(uint16_t fecoRandom) noexcept { this->fecoRandom = fecoRandom; }

	EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;

protected:
	EV EvBdgDepth(BDG& bdg, const EMV& emvPrev, int ply, int plyLim, EV evAlpha, EV evBeta) noexcept;
	EV EvBdgQuiescent(BDG& bdg, const EMV& emvPrev, int ply, EV evAlpha, EV evBeta) noexcept; 
	void PreSortGemv(BDG& bdg, GEMV& gemv) noexcept;
	void FillGemv(BDG& bdg, GEMV& gemv) noexcept;
	inline bool FAlphaBetaPrune(EMV& emv, EV& evBest, EV& evAlpha, EV evBeta, const EMV*& pemvBest, int& plyLim) const noexcept;
	inline void CheckForMates(BDG& bdg, EV& evBest, int cmvLegal, int ply) const noexcept;

	void PumpMsg(void) noexcept;

	virtual int PlyLim(const BDG& bdg, const GEMV& gemv) const;
	virtual EV EvBdgStatic(BDG& bdg, MV mv, bool fFull) noexcept;
	EV EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept; 
	EV EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept;
	float CmvFromLevel(int level) const noexcept;

	void StartMoveLog(void);
	void EndMoveLog(void);

	virtual void InitWeightTables(void);
	void InitWeightTable(const EV mpapcev[APC::ActMax], const EV mpapcsqdev[APC::ActMax][64], EV mpapcsqev[APC::ActMax][64]);
	EV EvFromPst(const BDG& bdg) const noexcept;
	inline EV EvInterpolate(GPH gph, EV ev1, GPH gph1, EV ev2, GPH gph2) const noexcept;
	EV EvBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept;
	EV EvTempo(const BDG& bdg, CPC cpc) const noexcept;

	int CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept;

};


class PLAI2 : public PLAI
{
protected:
#include "eval_plai2.h"

public:
	PLAI2(GA& ga);
	virtual int PlyLim(const BDG& bdg, const GEMV& gemv) const;
protected:
	virtual void InitWeightTables(void);
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
