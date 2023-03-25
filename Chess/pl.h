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
  *	TTM - time management styles
  */

enum TTM : int
{
	ttmNil = -1,
	ttmFirst = 0,
	ttmConstDepth = 0,
	ttmTimePerMove,
	ttmSmart,
	ttmInfinite,
	ttmMax
};

wstring to_wstring(TTM ttm);

__forceinline TTM& operator++(TTM& ttm)
{
	ttm = static_cast<TTM>(ttm + 1);
	return ttm;
}

__forceinline TTM operator++(TTM& ttm, int)
{
	TTM ttmT = ttm;
	ttm = static_cast<TTM>(ttm + 1);
	return ttmT;
}


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

	MVE mveNext;
	SPMV spmvNext;
	int imvBreakLast;
	vector<MV>* pvmvBreak;
	int cBreakRepeat;

public:
	PL(GA& ga, wstring szName);
	virtual ~PL(void);
	wstring& SzName(void) { return szName; }
	void SetName(const wstring& szNew) { szName = szNew; }

	virtual void StartGame(void) { }
	virtual MVE MveGetNext(SPMV& spmv) noexcept = 0;
	void ReceiveMv(MVE mve, SPMV spmv);

	virtual bool FHasLevel(void) const noexcept { return false; }
	virtual int Level(void) const noexcept { return -1; }
	virtual void SetLevel(int level) noexcept { }
	virtual void SetTtm(TTM ttm) noexcept { }
	virtual void SetFecoRandom(uint16_t) noexcept { }

	virtual EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	virtual EV EvBaseApc(APC apc) const noexcept;

	bool FDepthLog(LGT lgt, int& lgd) noexcept;
	void AddLog(LGT lgt, LGF lgf, int lgd, const TAG& tag, const wstring& szData) noexcept;
	int DepthLog(void) const noexcept;
	void SetDepthLog(int lgdNew) noexcept;
	void SetAIBreak(const vector<MV>& vmv);
	void InitBreak(void);
};


/*
 *
 *	VMVES class
 *
 *	A move list class that has smart enumeration with delayed scoring,
 *	to be used during alpha-beta search.
 *
 */

class PLAI;

class VMVES : public VMVE
{
public:
	VMVE::it pmveNext;
	int cmvLegal;
	PLAI* pplai;
	int d;
	TSC tscCur;	/* the score type we're currently enumerating */

public:
	inline VMVES(BDG& bdg, PLAI* pplai, int d, GG gg) noexcept;
	inline void Reset(BDG& bdg) noexcept;
	inline bool FEnumMvNext(BDG& bdg, MVE*& pmve) noexcept;
	inline void UndoMv(BDG& bdg) noexcept;
	bool FOnlyOneMove(MVE& mve) const noexcept;

private:
	MVE* PmveBestFromTscCur(VMVE::it pmveFirst) noexcept;
	void PrepTscCur(BDG& bdg, VMVE::it pmveFirst) noexcept;
};


/*
 *
 *	AB class
 * 
 *	Alpha-beta window. Rather than keep alpha/beta values separately, we combine
 *	them in this "window" class.
 * 
 */


class AB
{
public:
	EV evAlpha;
	EV evBeta;

	inline AB(EV evAlpha, EV evBeta) noexcept : evAlpha(evAlpha), evBeta(evBeta) { assert(FValid()); }
	inline bool FValid(void) const noexcept { return evAlpha < evBeta; }

	
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


	/*	AB::RaiseAlpha
	 *
	 *	Sets the new bottom range of the alpha-beta window, but only shrinks if the
	 *	new alpha is actually bigger than old alpha.
	 */
	inline void RaiseAlpha(EV ev) noexcept {
		assert(ev < evBeta);
		evAlpha = max(evAlpha, ev);
	}


	/*	AB::AbAspiration
	 *
	 *	Returns a narrow alpha-beta window centered on the given evaluation. This
	 *	is the beginning window for the aspiration window search optimization.
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


	inline bool fIsNull(void) const noexcept {
		return evAlpha + 1 == evBeta;
	}

	/* define inequality operators of an eval vs. an interval to be less
	   than if it's below the bottom value, and greater than if it's above
	   the top value */

	inline bool FEvIsBelow(EV ev) const noexcept { return ev <= evAlpha; }
	inline bool FEvIsAbove(EV ev) const noexcept { return ev >= evBeta; }
	inline bool FIncludes(EV ev) const noexcept { return ev > evAlpha && ev < evBeta; }

	operator wstring() const noexcept { 
		assert(FValid());
		return wstring(L"(") + SzFromEv(evAlpha) + L" " + SzFromEv(evBeta) + L")";
	}
};

inline wstring to_wstring(AB ab) noexcept { return (wstring)ab; }


/*
 *	SINT search interrupt types
 */

enum SINT : int {
	sintNull = 0,
	sintCanceled,
	sintTimedOut
};


/*
 *	TS search options, can be or'ed together
 */

enum TS : unsigned {
	tsAll = 0,
	
	tsNoPruneNullMove = 0x0001,
	tsNoPruneFutility = 0x0002,
	tsNoPruneNullPV = 0x0004,
	
	tsNoOrderPV = 0x0010,
	tsNoOrderCapt = 0x0020,
	tsNoOrderKillers = 0x0040,
	tsNoOrderHistory = 0x0080,
	tsNoOrderXT = 0x0100,

	tsNoIterDeepending = 0x2000,
	tsNoAspiration = 0x4000,
	
	tsNoTransTable = 0x8000
};

inline TS operator+(TS ts1, TS ts2) noexcept { return static_cast<TS>(ts1 | ts2); }
inline TS operator-(TS ts1, TS ts2) noexcept { return static_cast<TS>(ts1 & ~ts2); }
inline bool operator&(TS ts1, TS ts2) noexcept { return (static_cast<unsigned>(ts1) & static_cast<unsigned>(ts2)) != 0; }


/*
 *
 *	STBF class
 * 
 *	Branch factor stats. Probably overkill to make this a class, but it bundles 
 *	some things up nice.
 * 
 */


class STBF {
#ifndef NOSTATS
public:
	unsigned long long cmveNode;
	unsigned long long cmveGen;
public:
	STBF(unsigned long long cmveNode, unsigned long long cmveGen) noexcept : cmveNode(cmveNode), cmveGen(cmveGen) {}
	STBF(void) noexcept : cmveNode(0), cmveGen(0) {}
	inline void Init(void) noexcept { cmveNode = 0; cmveGen = 0; }
	STBF operator+(const STBF& stbf) const noexcept { return STBF(cmveNode + stbf.cmveNode, cmveGen + stbf.cmveGen); }
	inline STBF& operator+=(STBF& stbf) noexcept { cmveNode += stbf.cmveNode; cmveGen += stbf.cmveGen; return *this; }
	inline void IncNode(void) noexcept { ++cmveNode; }
	inline void IncGen(void) noexcept { ++cmveGen; }
	
	operator wstring() noexcept { 
		if (cmveGen == 0)
			return wstring(L"/0");
		int w100 = (int)round(100.0 * (double)cmveNode / (double)cmveGen);
		wchar_t sz[12] = L"", * pch = sz;
		int wInt = w100 / 100;
		pch = PchDecodeInt(wInt, pch);
		w100 = w100 % 100;
		*pch++ = L'.';
		*pch++ = L'0' + w100 / 10;
		*pch++ = L'0' + w100 % 10;
		*pch = 0;
		return wstring(sz);
	}
#else
public:
	inline void Init(void) noexcept { }
	inline STBF& operator+=(STBF& stbf) noexcept { return *this; }
	__forceinline void AddGen(int cmve) noexcept { }
	__forceinline void AddNode(int cmve) noexcept { }
	__forceinline operator wstring() noexcept { return L""; }
#endif
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
	friend class VMVES;
	friend class LOGMVE;
	friend class LOGMVES;
	friend class LOGMVEQ;
	friend class LOGITD;

protected:
	/* piece value tables */
	EV mpapcsqevOpening[apcMax][sqMax];
	EV mpapcsqevMiddleGame[apcMax][sqMax];
	EV mpapcsqevEndGame[apcMax][sqMax];
	/* piece weight tables used to initialize piece tables above */
#include "eval_plai.h"

	/* coefficients this divided by 100 */
	uint16_t fecoPsqt, fecoMaterial, fecoMobility, fecoKingSafety, fecoPawnStructure, fecoTempo, fecoRandom;
	const uint16_t fecoScale = 10;
	mt19937_64 rgen;	/* random number generator */
	uint64_t habdRand;	/* random number generated at the start of every search used to add randomness
						   to board eval - which is generated from the Zobrist hash */
	
	MVE mveBestOverall;	/* during search, root level best move so far */
	DWORD dmsecDeadline, dmsecFlag;
	time_point<high_resolution_clock> tpMoveStart;
	
	XT xt;
	static const int cmvKillers = 2;
	MV amvKillers[256][cmvKillers];
	int mppcsqcHistory[pcMax][sqMax];

	uint16_t cYield;
	int level;
	SINT sint;
	TTM ttm;

	/* logging statistics */

	STBF stbfMain;	/* stats for main search, but not quiescent */
	STBF stbfMainAndQ;	/* stats for main + quiescent */
	STBF stbfMainTotal;	/* cummulative all stats for iterative deepening */
	STBF stbfMainAndQTotal;	/* cummulative all stats for iterative deepening */

public:
	PLAI(GA& ga);
	virtual bool FHasLevel(void) const noexcept;
	virtual void SetLevel(int level) noexcept;
	virtual int Level(void) const noexcept { return level; }
	virtual void SetFecoRandom(uint16_t fecoRandom) noexcept { this->fecoRandom = fecoRandom; }
	virtual void SetTtm(TTM ttm) noexcept;
	
	virtual void StartGame(void);
	void PumpMsg(bool fForce) noexcept;

	/* search */

public:
	virtual MVE MveGetNext(SPMV& spmv) noexcept;
protected:
	EV EvBdgSearch(BDG& bdg, const MVE& mvePrev, AB ab, int d, int dLim, TS ts) noexcept;
	EV EvBdgQuiescent(BDG& bdg, const MVE& mvePrev, AB ab, int d, TS ts) noexcept; 
	inline bool FSearchMveBest(BDG& bdg, VMVES& vmves, MVE& mveBest, AB ab, int d, int& dLim, TS ts) noexcept;
	inline bool FPrune(BDG& bdg, MVE& mve, MVE& mveBest, AB& ab, int d, int& dLim) noexcept;
	inline bool FDeepen(BDG& bdg, MVE mveBest, AB& ab, int& d) noexcept;
	inline void TestForMates(BDG& bdg, VMVES& vmves, MVE& mveBest, int d) const noexcept;
	inline bool FLookupXt(BDG& bdg, MVE& mveBest, AB ab, int d, int dLim) noexcept;
	inline XEV* SaveXt(BDG& bdg, MVE mveBest, AB ab, int d, int dLim) noexcept;
	inline void SaveKiller(BDG& bdg, MVE mve) noexcept;
	inline void InitHistory(void) noexcept;
	inline void BumpHistory(BDG& bdg, MVE mve, int d, int dLim) noexcept;
	inline void AgeHistory(void) noexcept;
	inline bool FTryFutility(BDG& bdg, MVE& mveBest, AB ab, int d, int dLim, TS ts) noexcept;
	inline bool FTryNullMove(BDG& bdg, MVE& mveBest, AB ab, int d, int dLim, TS ts) noexcept;
	inline bool FTestForDraws(BDG& bdg, MVE& mve) noexcept;

	/* time management */

	virtual void InitTimeMan(BDG& bdg) noexcept;
	virtual bool FBeforeDeadline(int dLim) noexcept;
	SINT SintTimeMan(void) const noexcept;
	EV EvMaterialTotal(BDG& bdg) const noexcept;
	EV EvMaterial(BDG& bdg, CPC cpc) const noexcept;
	DWORD DmsecMoveSoFar(void) const noexcept;
	
	/* scoring for move ordering */

	EV ScoreMove(BDG& bdg, MVE mvePrev) noexcept;
	EV ScoreCapture(BDG& bdg, MVE mve)  noexcept;
	bool FScoreKiller(BDG& bdg, MVE& mve) noexcept;
	bool FScoreHistory(BDG& bdg, MVE& mve) noexcept;

	/* static evaluation */

	virtual EV EvBdgStatic(BDG& bdg, MVE mve) noexcept;
	virtual void InitWeightTables(void);
	EV EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept;
	EV EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept;
	void InitWeightTable(const EV mpapcev[apcMax], const EV mpapcsqdev[apcMax][64], EV mpapcsqev[apcMax][64]);
	EV EvFromPst(const BDG& bdg) const noexcept;
	inline void ComputeMpcpcev1(const BDG& bdg, EV mpcpcev[], const EV mpapcsqev[apcMax][sqMax]) const noexcept;
	inline void ComputeMpcpcev2(const BDG& bdg, EV mpcpcev1[], EV mpcpcev2[],
					  const EV mpapcsqev1[apcMax][sqMax], const EV mpapcsqev2[apcMax][sqMax]) const noexcept;
	EV EvFromGphApcSq(GPH gph, APC apc, SQ sq) const noexcept;
	inline EV EvInterpolate(GPH gph, EV ev1, GPH gph1, EV ev2, GPH gph2) const noexcept;
	EV EvBdgAttackDefend(BDG& bdg, MVE mvePrev) const noexcept;
	EV EvTempo(const BDG& bdg) const noexcept;

	int CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept;
	int CfilePassedPawns(BDG& bdg, CPC cpc) const noexcept;
	int CsqWeak(BDG& bdg, CPC cpc) const noexcept;

	/* logging */
	
	void StartMoveLog(void);
	void EndMoveLog(void);
	void LogInfo(BDG& bdg, EV ev, int d);
	void BuildPvSz(BDG& bdg, wstring& sz);

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

	virtual MVE MveGetNext(SPMV& spmv) noexcept;
};


/*
 *
 *	AINFOPL
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

enum CLPL : int 	// player classes
{
	clplNil = -1,
	clplFirst = 0,
	clplAI = 0,
	clplAI2,
	clplHuman,
	clplMax
};

wstring to_wstring(CLPL clpl);

__forceinline CLPL& operator++(CLPL& clpl)
{
	clpl = static_cast<CLPL>(clpl + 1);
	return clpl;
}

__forceinline CLPL operator++(CLPL& clpl, int)
{
	CLPL clplT = clpl;
	clpl = static_cast<CLPL>(clpl + 1);
	return clplT;
}

struct INFOPL
{
	CLPL clpl;
	TPL tpl;
	wstring szName;
	int level;
	TTM ttm;

	INFOPL(CLPL clpl, TPL tpl, const wstring& szName, TTM ttm = ttmNil, int level = 0) : 
		clpl(clpl), tpl(tpl), szName(szName), ttm(ttm), level(level)
	{
	}
};


class AINFOPL
{
public:
	vector <INFOPL> vinfopl;

	AINFOPL(void);
	PL* PplFactory(GA& ga, int iinfopl) const;
	int IdbFromInfopl(const INFOPL& infopl) const;
};

extern AINFOPL ainfopl;
