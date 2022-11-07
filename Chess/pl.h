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
#include "xt.h"



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
	friend class AIBREAK;

protected:
	GA& ga;
	wstring szName;
	int level;

	MV mvNext;
	SPMV spmvNext;
	bool fDisableMvLog;
	int imvmBreakLast;
	vector<MVM>* pvmvmBreak;
	int cBreakRepeat;

public:
	PL(GA& ga, wstring szName);
	virtual ~PL(void);
	wstring& SzName(void) { return szName; }
	void SetName(const wstring& szNew) { szName = szNew; }

	virtual MV MvGetNext(SPMV& spmv) = 0;
	void ReceiveMv(MV mv, SPMV spmv);

	virtual bool FHasLevel(void) const noexcept { return false; }
	virtual int Level(void) const noexcept { return level; }
	virtual void SetLevel(int level) noexcept { this->level = level; }
	virtual void SetFecoRandom(uint16_t) noexcept { }

	virtual EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	virtual EV EvBaseApc(APC apc) const noexcept;

	bool FDepthLog(LGT lgt, int& depth) noexcept;
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;
	int DepthLog(void) const noexcept;
	void SetDepthLog(int depthNew) noexcept;
	void DisableMvLog(bool fDisableMvLogNew) noexcept { fDisableMvLog = fDisableMvLogNew; }
	inline bool FDisabledMvLog(void) const noexcept { return fDisableMvLog; }
	void SetAIBreak(const vector<MVM>& vmvm);
	void InitBreak(void);
};


/*
 *
 *	VEMVS class
 *
 *	A move list class that has smart enumeration used in alpha-beta search.
 *	Variants for the various types of enumerations we have to do, including
 *	regular pre-sorted evals and unsorted quiescent enumerations.
 *
 *	Note that we do not use polymorphism in these classes. We only use them
 *	in a known context.
 *
 */


class VEMVS : public VEMV
{
public:
	int iemvNext;
	int cmvLegal;

public:
	inline VEMVS(BDG& bdg) noexcept;
	inline void Reset(BDG& bdg) noexcept;
	inline bool FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept;
	inline void UndoMv(BDG& bdg) noexcept;
};


/*
 *
 *	VEMVSS
 *
 *	Sorted move enumerator, used for optimizing alpha-beta search orer.
 *
 */


class PLAI;
class VEMVSS : public VEMVS
{
public:
	VEMVSS(BDG& bdg, PLAI* pplai) noexcept;
	bool FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept;
	void Reset(BDG& bdg, PLAI* pplai) noexcept;
};


/*
 *
 *	VEMVSQ enumeration
 *
 *	Enumerates noisy moves
 *
 */


class VEMVSQ : public VEMVS
{
public:
	VEMVSQ(BDG& bdg) noexcept;
	bool FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept;
};


/*
 *
 *	AB class
 * 
 *	Alpha-beta window used
 * 
 */
class AB
{
public:
	EV evAlpha;
	EV evBeta;

	inline AB(EV evAlpha, EV evBeta) : evAlpha(evAlpha), evBeta(evBeta) {
	}

	bool FValid(void) const {
		return evAlpha <= evBeta;
	}

	inline AB operator-(void) const {
		assert(FValid());
		return AB(-evBeta, -evAlpha);
	}

	inline AB& WidenLower(void) {
		evAlpha = max(evAlpha - (evBeta - evAlpha), -evInf);
		assert(FValid());
		return *this;
	}

	inline AB& WidenUpper(void) {
		evBeta = min(evBeta + (evBeta - evAlpha), evInf);
		assert(FValid());
		return *this;
	}

	inline AB& NarrowLower(EV ev) {
		assert(ev <= evBeta);
		evAlpha = max(evAlpha, ev);
		return *this;
	}

	inline AB& NarrowUpper(EV ev) {
		assert(ev >= evAlpha);
		evBeta = min(evBeta, ev);
		return *this;
	}

	/* define inequality operators of an eval vs. an interval to be less
	   than if it's below the bottom value, and greater than if it's above
	   the top value */

	inline bool operator>(EV ev) const {
		assert(FValid());
		return ev <= evAlpha;
	}
	inline bool operator<(EV ev) const {
		assert(FValid());
		return ev >= evBeta;
	}

	inline bool FIncludes(EV ev) const {
		assert(FValid());
		return ev > evAlpha && ev < evBeta;
	}

	friend inline bool operator<(EV ev, const AB& ab) {
		assert(ab.FValid());
		return ev <= ab.evAlpha;
	}

	friend inline bool operator>(EV ev, const AB& ab) {
		assert(ab.FValid());
		return ev >= ab.evBeta;
	}

	operator wstring() const { 
		assert(FValid());
		return wstring(L"[") + SzFromEv(evAlpha) + L" " + SzFromEv(evBeta) + L"]";
	}
};

inline wstring to_wstring(AB ab) { return (wstring)ab; }



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
	friend class VEMVSS;
	friend class LOGEMV;

protected:
	/* piece value tables */
	EV mpapcsqevOpening[APC::ActMax][sqMax];
	EV mpapcsqevMiddleGame[APC::ActMax][sqMax];
	EV mpapcsqevEndGame[APC::ActMax][sqMax];
	/* piece weight tables used to initialize piece tables above */
#include "eval_plai.h"

	/* coefficients this divided by 100 */
	uint16_t fecoMaterial, fecoMobility, fecoKingSafety, fecoPawnStructure, fecoTempo, fecoRandom;
	const uint16_t fecoScale = 10;
	mt19937_64 rgen;	/* random number generator */
	uint64_t habdRand;	/* random number generated at the start of every search used to add randomness
						   to board eval - which is generated from the Zobrist hash */
	uint16_t cYield;
	bool fAbort;

	/* logging statistics */
	time_point<high_resolution_clock> tpStart;
	size_t cemvEval;
	size_t cemvNode;

	int plySearchMax;

public:
	PLAI(GA& ga);
	virtual MV MvGetNext(SPMV& spmv);
	virtual bool FHasLevel(void) const noexcept;
	virtual void SetLevel(int level) noexcept;
	virtual void SetFecoRandom(uint16_t fecoRandom) noexcept { this->fecoRandom = fecoRandom; }

	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;
	EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	void PumpMsg(bool fForce) noexcept;

protected:
	EV EvBdgDepth(BDG& bdg, const EMV& emvPrev, int ply, int plyLim, AB ab) noexcept;
	EV EvBdgQuiescent(BDG& bdg, const EMV& emvPrev, int ply, AB ab) noexcept; 
	inline bool FAlphaBetaPrune(EMV* pemv, EMV& emvBest, AB& ab, int& plyLim) const noexcept;
	inline bool FDeepenIt(const EMV& emvBest, AB& ab, int& ply) const noexcept;
	inline void TestForMates(BDG& bdg, VEMVS& vemvs, EMV& emvBest, int ply) const noexcept;
	inline bool FLookupXt(BDG& bdg, int ply, EMV& emvBest, AB& ab) const noexcept;

	virtual void InitSearch(BDG& bdg) noexcept;
	virtual bool FStopSearch(int& plyLim) noexcept;
	virtual EV EvBdgStatic(BDG& bdg, MV mv, bool fFull) noexcept;
	EV EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept; 
	EV EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept;
	float CmvFromLevel(int level) const noexcept;

	void StartMoveLog(void);
	void EndMoveLog(void);

	virtual void InitWeightTables(void);
	void InitWeightTable(const EV mpapcev[APC::ActMax], const EV mpapcsqdev[APC::ActMax][64], EV mpapcsqev[APC::ActMax][64]);
	EV EvFromPst(const BDG& bdg) const noexcept;
	inline void ComputeMpcpcev1(const BDG& bdg, EV mpcpcev[], const EV mpapcsqev[APC::ActMax][sqMax]) const noexcept;
	inline void ComputeMpcpcev2(const BDG& bdg, EV mpcpcev1[], EV mpcpcev2[],
					  const EV mpapcsqev1[APC::ActMax][sqMax], const EV mpapcsqev2[APC::ActMax][sqMax]) const noexcept;
	inline EV EvInterpolate(GPH gph, EV ev1, GPH gph1, EV ev2, GPH gph2) const noexcept;
	EV EvBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept;
	EV EvTempo(const BDG& bdg) const noexcept;

	int CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfilePassedPawns(BDG& bdg, CPC cpc) const noexcept;
};


/*
 *
 *	PLAI2
 * 
 *	A second AI, a little dumber than PLAI but a good opponent. Does full alpha-beta
 *	pruning but uses a less sophisticated board evaluation function.
 * 
 */


class PLAI2 : public PLAI
{
protected:
#include "eval_plai2.h"

public:
	PLAI2(GA& ga);
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
