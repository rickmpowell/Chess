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
	int imvmBreakLast;
	vector<MVM>* pvmvmBreak;
	int cBreakRepeat;

public:
	PL(GA& ga, wstring szName);
	virtual ~PL(void);
	wstring& SzName(void) { return szName; }
	void SetName(const wstring& szNew) { szName = szNew; }

	virtual void StartGame(void) { }
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
	TSC tscCur;	/* the score type we're currently enumerating */
	PLAI* pplai;

public:
	VEMVSS(BDG& bdg, PLAI* pplai) noexcept;
	bool FMakeMvNext(BDG& bdg, EMV*& pemv) noexcept;
	void Reset(BDG& bdg) noexcept;

private:
	EMV* PemvBestFromTscCur(int iemvFirst) noexcept;
	void PrepTscCur(BDG& bdg, int iemvFirst) noexcept;
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

	inline AB(EV evAlpha, EV evBeta) noexcept : evAlpha(evAlpha), evBeta(evBeta) { assert(FValid()); }
	bool FValid(void) const noexcept { return evAlpha < evBeta; }

	
	/*	AB unary - operator
	 *
	 *	Inverts the alpha-beta window, negating both sides and flipping. This is what
	 *	you want to do for negamax searching
	 */
	inline AB operator-(void) const noexcept {
		assert(FValid());
		return AB(-evBeta, -evAlpha);
	}


	/*	AB::AdjMissLow
	 *
	 *	Increases the size of the alpha-beta window by widening the alpha (low)
	 *	side. Works by doubling the size, until it gets pretty big, then we just
	 *	give up and go to infinity.
	 * 
	 *	This code is called on low search failure, so we also adjust the beta (high)
	 *	cut-off down a little, since we probably won't be hitting it in subsequent
	 *	searches.
	 */
	inline void AdjMissLow(void) noexcept {
		assert(evAlpha != -evInf);	// widening won't work if we're already at inf
		int dev = evBeta - evAlpha;
		evBeta -= dev / 2;
		evAlpha = dev > 200 ? -evInf : max(evAlpha - dev, -evInf);
		assert(FValid());
	}


	/*	AB::AdjMissHigh
	 *
	 *	Increases the size of the a-b window by widening the beta (high) side.
	 *	Doubles the size, until it gets pretty big, when it just gives up and
	 *	grows it to infinity.
	 * 
	 *	This is called on fail high searches, so we also adjust alpha up a little,
	 *	since we probably won't be failing low.
	 */
	inline void AdjMissHigh(void) noexcept {
		assert(evAlpha != evInf);	// widening doesn't work if we're already at inf
		int dev = evBeta - evAlpha;
		evAlpha += dev / 2;
		evBeta = dev > 200 ? evInf : min(evBeta + dev, evInf);
		assert(FValid());
	}


	/*	AB::NarrowAlpha
	 *
	 *	Sets the new bottom range of the alpha-beta window, but only shrinks if the
	 *	new alpha is actually bigger than old alpha.
	 */
	inline void NarrowAlpha(EV ev) noexcept {
		assert(ev < evBeta);
		evAlpha = max(evAlpha, ev);
	}


	/*	AB::NarrowBeta
	 *
	 *	Sets the top range of the alpha-beta window, but only shrinks if the new 
	 *	beta is actually below the old beta
	 */
	inline void NarrowBeta(EV ev) noexcept {
		assert(ev > evAlpha);
		evBeta = min(evBeta, ev);
	}


	/*	AB::AbAspiration
	 *
	 *	Returns a narrow alpha-beta window centered on the given evaluation. This
	 *	is the beginning window for the aspiration window optimization.
	 */
	inline AB AbAspiration(EV ev, EV dev) const noexcept {
		return AB(max(ev - dev, -evInf), min(ev + dev, evInf));
	}


	/*	AB::AbNull
	 *
	 *	Returns a minimal-sized window at alpha. Used for PV search optimization.
	 */
	inline AB AbNull(void) const noexcept {
		return AB(evAlpha, evAlpha + 1);
	}

	/* define inequality operators of an eval vs. an interval to be less
	   than if it's below the bottom value, and greater than if it's above
	   the top value */

	inline bool operator>(EV ev) const noexcept {
		assert(FValid());
		return ev <= evAlpha;
	}
	inline bool operator<(EV ev) const noexcept {
		assert(FValid());
		return ev >= evBeta;
	}

	inline bool FIncludes(EV ev) const noexcept {
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

	operator wstring() const noexcept { 
		assert(FValid());
		return wstring(L"(") + SzFromEv(evAlpha) + L" " + SzFromEv(evBeta) + L")";
	}
};

inline wstring to_wstring(AB ab) noexcept { return (wstring)ab; }


/*
 *	TTM - time management styles
 */

enum TTM : int {
	ttmConstDepth,
	ttmTimePerMove,
	ttmSmart
};

/*
 *	SINT search interrupt types
 */

enum SINT : int {
	sintNull = 0,
	sintCanceled,
	sintTimedOut
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
	friend class VEMVSS;
	friend class LOGEMV;

protected:
	/* piece value tables */
	EV mpapcsqevOpening[apcMax][sqMax];
	EV mpapcsqevMiddleGame[apcMax][sqMax];
	EV mpapcsqevEndGame[apcMax][sqMax];
	/* piece weight tables used to initialize piece tables above */
#include "eval_plai.h"

	/* coefficients this divided by 100 */
	uint16_t fecoMaterial, fecoMobility, fecoKingSafety, fecoPawnStructure, fecoTempo, fecoRandom;
	const uint16_t fecoScale = 10;
	mt19937_64 rgen;	/* random number generator */
	uint64_t habdRand;	/* random number generated at the start of every search used to add randomness
						   to board eval - which is generated from the Zobrist hash */
	
	EMV emvBestOverall;	/* during search, root level best move so far */
	TTM ttm;
	DWORD dmsecDeadline, dmsecFlag;
	time_point<high_resolution_clock> tpMoveFirst;

	uint16_t cYield;
	SINT sint;

#ifndef NOSTATS
	/* logging statistics */
	time_point<high_resolution_clock> tpStart;
	size_t cemvEval;
	size_t cemvNode;
#endif

public:
	PLAI(GA& ga);

	virtual void StartGame(void);
	virtual MV MvGetNext(SPMV& spmv);
	virtual bool FHasLevel(void) const noexcept;
	virtual void SetLevel(int level) noexcept;
	virtual void SetFecoRandom(uint16_t fecoRandom) noexcept { this->fecoRandom = fecoRandom; }

	EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	void PumpMsg(bool fForce) noexcept;

protected:
	EV EvBdgDepth(BDG& bdg, const EMV& emvPrev, AB ab, int ply, int plyLim) noexcept;
	EV EvBdgQuiescent(BDG& bdg, const EMV& emvPrev, AB ab, int ply) noexcept; 
	inline bool FSearchEmvBest(BDG& bdg, VEMVSS& vemvss, EMV& emvBest, AB ab, int ply, int& plyLim) noexcept;
	inline bool FPrune(EMV* pemv, EMV& emvBest, AB& ab, int& plyLim) const noexcept;
	inline bool FDeepen(EMV emvBest, AB& ab, int& ply) noexcept;
	inline void TestForMates(BDG& bdg, VEMVS& vemvs, EMV& emvBest, int ply) const noexcept;
	inline bool FLookupXt(BDG& bdg, EMV& emvBest, AB ab, int ply) const noexcept;
	inline XEV* SaveXt(BDG& bdg, EMV emvBest, AB ab, int ply) const noexcept;
	inline bool FTryFutility(BDG& bdg, EMV& emvBest, AB ab, int ply, int plyLim) noexcept;
	inline bool FTryNullMove(BDG& bdg, const EMV& emvPrev, EMV& emvBest, AB ab, int ply, int plyLim) noexcept;

	virtual void InitTimeMan(BDG& bdg) noexcept;
	virtual bool FStopSearch(int plyLim) noexcept;
	EV EvMaterialTotal(BDG& bdg) const noexcept;
	
	virtual EV EvBdgStatic(BDG& bdg, MV mv, bool fFull) noexcept;
	EV EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept; 
	EV EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept;

	void StartMoveLog(void);
	void EndMoveLog(void);

	virtual void InitWeightTables(void);
	void InitWeightTable(const EV mpapcev[apcMax], const EV mpapcsqdev[apcMax][64], EV mpapcsqev[apcMax][64]);
	EV EvFromPst(const BDG& bdg) const noexcept;
	inline void ComputeMpcpcev1(const BDG& bdg, EV mpcpcev[], const EV mpapcsqev[apcMax][sqMax]) const noexcept;
	inline void ComputeMpcpcev2(const BDG& bdg, EV mpcpcev1[], EV mpcpcev2[],
					  const EV mpapcsqev1[apcMax][sqMax], const EV mpapcsqev2[apcMax][sqMax]) const noexcept;
	inline EV EvInterpolate(GPH gph, EV ev1, GPH gph1, EV ev2, GPH gph2) const noexcept;
	EV EvBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept;
	EV EvTempo(const BDG& bdg) const noexcept;

	int CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfilePassedPawns(BDG& bdg, CPC cpc) const noexcept;
	int CsqWeak(BDG& bdg, CPC cpc) const noexcept;
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


enum TPL	// types of players
{
	tplAI,
	tplHuman,
	tplStream
};

enum IDCLASSPL	// player classes
{
	idclassplAI,
	idclassplAI2,
	idclassplHuman
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
