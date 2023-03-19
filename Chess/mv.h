/*
 *
 *	mv.h
 * 
 *	Move definitions, includes mini-moves, an undoable move, and the move
 *	list. Because our AI is more efficient if we can save an evaluation along
 *	with the move, we actually generate moves into a move list that has space
 *	for an evaluation. So our evaluation types are also defined here.
 * 
 *	These structures and classes are critical to overall speed of the AI.
 *	There's a lot of careful sizing and inlining and assumptions about how
 *	words and bytes line up within the structure. Modify this code with care.
 * 
 *	There are three basic move types: MV (a mini-move), a MVU (contains undo
 *	information), and an MVE (an evaluated move).
 *	
 */
#pragma once
#include "pc.h"
#include "bb.h"


/*
 *
 *	MVU and MV type
 *
 *	A move is a from and to square, with a little extra info for
 *	weird moves, along with enough information to undo the move.
 *
 *	Low word is the from/to square, along with the type of piece
 *	that is moving. We store the piece with apc and color
 *
 *	The high word is mostly used for undo information and includes
 *	previous castle state, the type of piece captured (if a capture),
 *	and en passant state. It also includes the piece promoted to for
 *	pawn promotions.
 *
 *   15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *  +--+--------+--------+--------+--------+--------+
 *  |XX|apc prom|       to        |      from       |  <<= minimum amount needed to recreate the move
 *  +--+--------+--------+--------+--------+--------+
 *  +--+-----+-----+--------+--+--------+--+--------+
 *  |XX| cs state  |ep file |ep|apc capt|  pc move  |  <<= mostly undo information
 *  +--+-----+-----+--------+--+--------+--+--------+
 *   31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 *
 *	The first word of the move is the mini-move (MV), which is the
 *	minimal amount of information needed, along with the board state,
 *	to specify a move. The redundant information could be filled in
 *	from the BD before the move is made.
 */

class MV {
protected:
	uint16_t usqFrom : 6,
		usqTo : 6,
		uapcPromote : 3,
		unused1 : 1;
public:
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	__forceinline MV(void) noexcept { *(uint16_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	__forceinline MV(uint16_t u) noexcept { *(uint16_t*)this = u; }
	__forceinline MV(SQ sqFrom, SQ sqTo) noexcept { *(uint16_t*)this = 0;	usqFrom = sqFrom; usqTo = sqTo; }
	__forceinline operator uint16_t() const noexcept { return *(uint16_t*)this; }

	__forceinline SQ sqFrom(void) const noexcept { return usqFrom; }
	__forceinline SQ sqTo(void) const noexcept { return usqTo; }
	__forceinline APC apcPromote(void) const noexcept { return static_cast<APC>(uapcPromote); }
	__forceinline void SetApcPromote(APC apc) noexcept { uapcPromote = static_cast<uint16_t>(apc); }
	__forceinline bool fIsNil(void) const noexcept { return usqFrom == 0 && usqTo == 0; }
};

static_assert(sizeof(MV) == sizeof(uint16_t));


class MVU : public MV {
protected:
	uint16_t
		upcMove : 4,	// the piece that is moving
		uapcCapt : 3,	// for captures, the piece we take
		ufEnPassant : 1,	// en passant state
		ufileEnPassant : 3,
		ucs : 4,		// saved castle state
		unused2 : 1;

public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	__forceinline MVU(void) noexcept { *(uint32_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	__forceinline MVU(uint32_t u) noexcept { *(uint32_t*)this = u; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	__forceinline MVU(MV mv) noexcept { *(uint32_t*)this = 0; *(uint16_t*)this = *(uint16_t*)&mv; }
	__forceinline operator uint32_t() const noexcept { return *(uint32_t*)this; }

	__forceinline MVU(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		assert(pcMove.apc() != apcNull);
		*(uint32_t*)this = 0;	// initialize everything else to 0
		upcMove = pcMove;
		usqFrom = sqFrom;
		usqTo = sqTo;
	}

	__forceinline void SetPcMove(PC pcMove) noexcept { upcMove = pcMove; }
	__forceinline APC apcMove(void) const noexcept { return PC(upcMove).apc(); }
	__forceinline CPC cpcMove(void) const noexcept { return PC(upcMove).cpc(); }
	__forceinline PC pcMove(void) const noexcept { return PC(upcMove); }
	__forceinline void SetCapture(APC apc) noexcept { uapcCapt = static_cast<uint16_t>(apc); }

	__forceinline void SetCsEp(int cs, SQ sqEnPassant) noexcept
	{
		ucs = cs;
		ufEnPassant = !sqEnPassant.fIsNil();
		if (ufEnPassant)
			ufileEnPassant = sqEnPassant.file();
	}

	__forceinline int csPrev(void) const noexcept { return ucs; }
	__forceinline int fileEpPrev(void) const noexcept { return ufileEnPassant; }
	__forceinline bool fEpPrev(void) const noexcept { return ufEnPassant; }
	__forceinline APC apcCapture(void) const noexcept { return (APC)uapcCapt; }
	__forceinline bool fIsCapture(void) const noexcept { return apcCapture() != apcNull; }
	__forceinline bool fIsCastle(void) const noexcept { return apcMove() == apcKing && abs(sqFrom().file() - sqTo().file()) > 1; }
	/* beware! - comparison operators only work on the MV part of the MVE */
	__forceinline bool operator==(const MVU& mv) const noexcept { return *(uint16_t*)this == (uint16_t)mv; }
	__forceinline bool operator!=(const MVU& mv) const noexcept { return *(uint16_t*)this != (uint16_t)mv; }
	__forceinline bool operator==(const MV& mv) const noexcept { return *(uint16_t*)this == (uint16_t)mv; }
	__forceinline bool operator!=(const MV& mv) const noexcept { return *(uint16_t*)this != (uint16_t)mv; }
};

static_assert(sizeof(MVU) == sizeof(uint32_t));

const MVU mvuNil = MVU();
const MV mvNil = MV();
const MVU mvuAll = MVU(sqH8, sqH8, PC(cpcWhite, apcError));
const MV mvAll = MV(sqH8, sqH8);
wstring to_wstring(MV mv);


/*
 *
 *	EV type
 *
 *	A board evaluation. This is a signed integer, in centi-pawns, or 1/100 of a
 *	pawn. We support "infinite" evaluations, mate-in-X moves evaluations, various
 *	special evaluations representing things like aborted searches, etc.
 *
 *	For you bit-packers out there, this'll fit in 15-bits.
 *
 */


typedef int16_t EV;

const int dMax = 127;							/* maximum search depth */
const EV evPawn = 100;							/* evals are in centi-pawns */
const EV evInf = 160 * evPawn + dMax;			/* largest possible evaluation */
const EV evSureWin = 40 * evPawn;				/* we have sure win when up this amount of material */
const EV evMate = evInf - 1;					/* checkmates are given evals of evalMate minus moves to mate */
const EV evMateMin = evMate - dMax;
const EV evTempo = evPawn/3;					/* evaluation of a single move advantage */
const EV evDraw = 0;							/* evaluation of a draw */
const EV evTimedOut = evInf + 1;
const EV evCanceled = evTimedOut + 1;
const EV evMax = evCanceled + 1;
const EV evBias = evInf;						/* used to bias evaluations for saving as an unsigned */
static_assert(evMax <= 16384);					/* there is code that asssumes EV stores in 15 bits */

inline EV EvMate(int d) { return evMate - d; }
inline bool FEvIsMate(EV ev) { return ev >= evMateMin; }
inline bool FEvIsInterrupt(EV ev) { return abs(ev) > evInf; }	/* timeout or cancel */
inline int DFromEvMate(EV ev) { return evMate - ev; }
wstring SzFromEv(EV ev);
inline wstring to_wstring(EV ev) { return SzFromEv(ev); }


/*
 *
 *	TSC enumeration
 * 
 *	Score types, used for move ordering in the AI search. The order of score types 
 *	determines the order of moves searched in the alpha-beta search, and is therefore 
 *	crucial to efficiency of the the alpha-beta pruning. The order of moves is
 *	designed to try to make cut nodes happen as early as possible in the ordering.
 *
 */


enum TSC : int {
	tscNil = 255,
	tscPrincipalVar = 0,
	tscEvCapture = 1,
	tscXTable = 2,
	tscEvOther = 3
};

__forceinline TSC& operator++(TSC& tsc)
{
	tsc = static_cast<TSC>(tsc + 1);
	return tsc;
}

__forceinline TSC operator++(TSC& tsc, int)
{
	TSC tscT = tsc;
	tsc = static_cast<TSC>(tsc + 1);
	return tscT;
}

wstring to_wstring(TSC tsc);


/*
 *
 *	MVE structure
 *
 *	A move along with move evaluation information, which is used for generating AI move 
 *	lists and alpha-beta pruning. All movegens generate moves into this sized element 
 *	(instead of just a simple move list) to minimize the amount of copying needed during 
 *	AI search.
 *
 *	Non-AI customers of this structure can safely ignore everything except the MVU
 * 
 */


class MVE : public MVU
{
public:
	EV ev;
	uint16_t utsc;	// score type, used by ai search to enumerate good moves first for alpha-beta

#pragma warning(suppress:26495)	
	__forceinline MVE(void) noexcept { *(uint64_t*)this = (uint32_t)mvuNil; }
#pragma warning(suppress:26495)	
	__forceinline MVE(MVU mvu) noexcept { *(uint64_t*)this = (uint32_t)mvu; }
	__forceinline MVE(MVU mvu, EV ev) noexcept : MVU(mvu), ev(ev), utsc(0) { }
#pragma warning(suppress:26495)	 
	__forceinline MVE(uint64_t mve) noexcept { *(uint64_t*)this = mve; }
#pragma warning(suppress:26495)	
	__forceinline MVE(SQ sqFrom, SQ sqTo, PC pcMove) noexcept {
		*(uint64_t*)this = 0;
		usqFrom = sqFrom;
		usqTo = sqTo;
		upcMove = pcMove;
	}
#pragma warning(suppress:26495)	
	__forceinline MVE(MV mv) noexcept {
		*(uint64_t*)this = 0;
		*(MV*)this = mv;
	}

	__forceinline operator uint64_t() const noexcept { return *(uint64_t*)this; }
	__forceinline operator MVU() const noexcept { return *(uint32_t*)this; }

	/* comparison operations work on the eval */

	__forceinline bool operator>(const MVE& mve) const noexcept { return utsc > mve.utsc || (utsc == mve.utsc && ev > mve.ev); }
	__forceinline bool operator<(const MVE& mve) const noexcept { return utsc < mve.utsc || (utsc == mve.utsc && ev < mve.ev); }
	__forceinline bool operator>=(const MVE& mve) const noexcept { return !(*this < mve); }
	__forceinline bool operator<=(const MVE& mve) const noexcept { return !(*this > mve); }

	__forceinline void SetTsc(TSC tsc) noexcept { utsc = static_cast<uint16_t>(tsc); }
	__forceinline TSC tsc() const noexcept { return static_cast<TSC>(utsc); }
	__forceinline void SetMvu(MVU mvu) noexcept { *(MVU*)this = mvu; }
};

static_assert(sizeof(MVE) == sizeof(uint64_t));

const MVE mveNil;


/*
 *
 *	VMVE class
 *
 *	Generated move list. Has simple array semantics. Actually stores an MVE, which
 *	is just handy extra room for an evaluation during AI search.
 *
 */


template<int cmvePreMax> class T_VMVE
{
private:
	uint64_t amve[cmvePreMax];
	int cmveCur;
#ifdef OVERFLOW_VMVE
	vector<uint64_t>* pvmveOverflow;
#endif

public:


	T_VMVE() : 
#ifdef OVERFLOW_VMVE
		pvmveOverflow(nullptr), 
#endif
		cmveCur(0)
	{
		amve[0] = 0;
	}


	~T_VMVE()
	{
#ifdef OVERFLOW_VMVE
		if (pvmveOverflow)
			delete pvmveOverflow;
#endif
	}


	T_VMVE(const T_VMVE& vmve) noexcept :
#ifdef OVERFLOW_VMVE
		pvmveOverflow(nullptr), 
#endif
		cmveCur(vmve.cmveCur)
	{
		assert(vmve.FValid());
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
#ifdef OVERFLOW_VMVE
		if (vmve.pvmveOverflow)
			pvmveOverflow = new vector<uint64_t>(*vmve.pvmveOverflow);
#endif
	}


	/*	VMVE move constructor
	 *
	 *	Moves data from one GMV to another. Note that the source GMV is trashed.
	 */
	T_VMVE(T_VMVE&& vmve) noexcept : 
#ifdef OVERFLOW_VMVE
		pvmveOverflow(vmve.pvmveOverflow),
#endif
		cmveCur(vmve.cmveCur)
	{
		assert(vmve.FValid());
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
		vmve.cmveCur = 0;
#ifdef OVERFLOW_VMVE
		vmve.pvmveOverflow = nullptr;
#endif
	}


	T_VMVE& operator=(const T_VMVE& vmve)
	{
		assert(vmve.FValid());
		cmveCur = vmve.cmveCur;
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
#ifdef OVERFLOW_VMVE
		if (vmve.pvmveOverflow) {
			if (pvmveOverflow)
				*pvmveOverflow = *vmve.pvmveOverflow;
			else
				pvmveOverflow = new vector<uint64_t>(*vmve.pvmveOverflow);
		}
		else {
			if (pvmveOverflow) {
				delete pvmveOverflow;
				pvmveOverflow = nullptr;
			}
		}
#endif
		return *this;
	}


	T_VMVE& operator=(T_VMVE&& vmve) noexcept
	{
		if (this == &vmve)
			return *this;
		cmveCur = vmve.cmveCur;
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
#ifdef OVERFLOW_VMVE
		if (pvmveOverflow != nullptr) {
			delete pvmveOverflow;
			pvmveOverflow = nullptr;
		}
		pvmveOverflow = vmve.pvmveOverflow;
		vmve.pvmveOverflow = nullptr;
#endif
		return *this;
	}


#ifndef NDEBUG
	bool FValid(void) const noexcept
	{
#ifdef OVERFLOW_VMVE
		return (cmveCur <= cmvePreMax && pvmveOverflow == nullptr) ||
			(cmveCur > cmvePreMax && pvmveOverflow != nullptr && pvmveOverflow->size() + cmvePreMax == cmveCur);
#else
		return cmveCur <= cmvePreMax;
#endif
	}
#endif


	__forceinline int cmve(void) const noexcept
	{
		assert(FValid());
		return cmveCur;
	}


	__forceinline MVE& operator[](int imve) noexcept
	{
		assert(FValid());
		assert(imve >= 0 && imve < cmveCur);
#ifdef OVERFLOW_VMVE
		if (imve >= cmvePreMax) {
			assert(pvmveOverflow != nullptr);
			return *(MVE*)&(*pvmveOverflow)[imve - cmvePreMax];
		}
#endif
		return *(MVE*)&amve[imve];
	}


	__forceinline const MVE& operator[](int imve) const noexcept
	{
		assert(FValid());
		assert(imve >= 0 && imve < cmveCur);
#ifdef OVERFLOW_VMVE
		if (imve >= cmvePreMax) {
			assert(pvmveOverflow != nullptr);
			return *(MVE*)&(*pvmveOverflow)[imve - cmvePreMax];
		}
#endif
		return *(MVE*)&amve[imve];
	}

	/* iterator and const iterator for the arrays */

	class it {
		T_VMVE* pvmve;
		int imve;
	public:
		using difference_type = int;
		using value_type = MVE;
		using pointer = MVE*;
		using reference = MVE&;
		using iterator_category = random_access_iterator_tag;

		__forceinline it(T_VMVE* pvmve, difference_type imve) noexcept : pvmve(pvmve), imve(imve) { }
		__forceinline it(const it& it) noexcept : pvmve(it.pvmve), imve(it.imve) { }
		__forceinline it& operator=(const it& it) noexcept { pvmve = it.pvmve; imve = it.imve; return *this; }
		__forceinline reference operator[](difference_type dimve) const noexcept { return (*pvmve)[imve + dimve]; }
		__forceinline reference operator*() const noexcept { return (*pvmve)[imve]; }
		__forceinline pointer operator->() const noexcept { return &(*pvmve)[imve]; }
		__forceinline it& operator++() noexcept { imve++; return *this; }
		__forceinline it operator++(int) noexcept { int imveT = imve; imve++; return it(pvmve, imveT); }
		__forceinline it& operator--() noexcept { imve--; return *this; }
		__forceinline it operator--(int) noexcept { int imveT = imve; imve--; return it(pvmve, imveT); }
		__forceinline it operator+(difference_type dimve) const noexcept { return it(pvmve, imve + dimve); }
		__forceinline it operator-(difference_type dimve) const noexcept { return it(pvmve, imve - dimve); }
		__forceinline it& operator+=(difference_type dimve) noexcept { imve += dimve; return *this; }
		__forceinline it& operator-=(difference_type dimve) noexcept { imve -= dimve; return *this; }
		friend __forceinline it operator+(difference_type dimve, it const& it) { return it(it.pvmve, dimve + it.imve); }
		__forceinline difference_type operator-(const it& it) const noexcept { return imve - it.imve; }
		__forceinline bool operator==(const it& it) const noexcept { return imve == it.imve; }
		__forceinline bool operator!=(const it& it) const noexcept { return imve != it.imve; }
		__forceinline bool operator<(const it& it) const noexcept { return imve < it.imve; }
	};

	class itc {
		const T_VMVE* pvmve;
		int imve;
	public:
		using difference_type = int;
		using value_type = MVE;
		using pointer = const MVE*;
		using reference = const MVE&;
		using iterator_category = random_access_iterator_tag;;

		__forceinline itc(const T_VMVE* pvmve, int imve) noexcept : pvmve(pvmve), imve(imve) { }
		__forceinline itc(const itc& it) noexcept : pvmve(it.pvmve), imve(it.imve) { }
		//		__forceinline itc operator=(const itc& it) noexcept { pvmve = it.pvmve; imve = it.imve; return *this; }
		__forceinline reference operator[](difference_type dimve) const noexcept { return (*pvmve)[imve + dimve]; }
		__forceinline reference operator*() const noexcept { return (*pvmve)[imve]; }
		__forceinline pointer operator->() const noexcept { return &(*pvmve)[imve]; }
		__forceinline itc operator++() noexcept { imve++; return *this; }
		__forceinline itc operator++(int) noexcept { itc it = *this; imve++; return it; }
		__forceinline itc operator--() noexcept { imve--; return *this; }
		__forceinline itc operator--(int) noexcept { itc it = *this; imve--; return it; }
		__forceinline itc operator+(difference_type dimve) const noexcept { return itc(pvmve, imve + dimve); }
		__forceinline itc operator-(difference_type dimve) const noexcept { return itc(pvmve, imve - dimve); }
		__forceinline itc operator+=(difference_type dimve) noexcept { imve += dimve; return *this; }
		__forceinline itc operator-=(difference_type dimve) noexcept { imve -= dimve; return *this; }
		friend inline itc operator+(difference_type dimve, const itc& it) { return itc(it.pvmve, dimve + it.imve); }
		__forceinline difference_type operator-(const itc& it) const noexcept { return imve - it.imve; }
		__forceinline bool operator==(const itc& it) const noexcept { return imve == it.imve; }
		__forceinline bool operator!=(const itc& it) const noexcept { return imve != it.imve; }
		__forceinline bool operator<(const itc& cit) const noexcept { return imve < it.imve; }
	};

	__forceinline int size(void) const noexcept { return cmve(); }
	__forceinline it begin(void) noexcept { return it(this, 0); }
	__forceinline it end(void) noexcept { return it(this, size()); }
	__forceinline itc begin(void) const noexcept { return itc(this, 0); }
	__forceinline itc end(void) const noexcept { return itc(this, size()); }


#ifdef OVERFLOW_VMVE
	__declspec(noinline) void push_back_overflow(MVE mve) noexcept
	{
		if (pvmveOverflow == nullptr) {
			assert(cmveCur == cmvePreMax);
			pvmveOverflow = new vector<uint64_t>;
		}
		pvmveOverflow->push_back(mve);
		cmveCur++;
	}
#endif


	__forceinline void push_back(MVE mve) noexcept
	{
		assert(FValid());
#ifdef OVERFLOW_VMVE
		if (cmveCur >= cmvePreMax) {
			push_back_overflow((uint64_t)mve);
			return;
		}
#endif
		amve[cmveCur++] = (uint64_t)mve;
	}


	__forceinline void push_back(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		push_back(MVE(sqFrom, sqTo, pcMove));
	}


#ifdef OVERFLOW_VMVE
	__declspec(noinline) void insert_overflow(int imve, const MVE& mve) noexcept
	{
		if (pvmveOverflow == nullptr)
			pvmveOverflow = new vector<uint64_t>;
		if (imve >= cmvePreMax)
			pvmveOverflow->insert(pvmveOverflow->begin() + (imve - cmvePreMax), mve);
		else {
			pvmveOverflow->insert(pvmveOverflow->begin(), amve[cmvePreMax - 1]);
			memmove(&amve[imve + 1], &amve[imve], sizeof(uint64_t) * (cmvePreMax - imve - 1));
		}
		cmveCur++;
	}
#endif


	void insert(int imve, const MVE& mve) noexcept
	{
		assert(FValid());
#ifdef OVERFLOW_VMVE
		if (cmveCur >= cmvePreMax) {
			insert_overflow(imve, mve);
			return;
		}
#endif
		memmove(&amve[imve + 1], &amve[imve], sizeof(uint64_t) * (cmveCur - imve));
		amve[imve] = mve;
		cmveCur++;
	}


	void resize(int cmveNew) noexcept
	{
		assert(FValid());
#ifdef OVERFLOW_VMVE
		if (cmveNew > cmvePreMax) {
			if (pvmveOverflow == nullptr)
				pvmveOverflow = new vector<uint64_t>;
			pvmveOverflow->resize(cmveNew - cmvePreMax);
		}
		else {
			if (pvmveOverflow != nullptr) {
				delete pvmveOverflow;
				pvmveOverflow = nullptr;
			}
		}
#endif
		cmveCur = cmveNew;
		assert(FValid());
	}


	void reserve(int cmve) noexcept
	{
#ifdef OVERFLOW_VMVE
		if (cmve <= cmvePreMax)
			return;
		if (pvmveOverflow == nullptr)
			pvmveOverflow = new vector<uint64_t>;
		pvmveOverflow->reserve(cmve - cmvePreMax);
#endif
	}


	__forceinline void clear(void) noexcept
	{
#ifdef OVERFLOW_VMVE
		if (pvmveOverflow) {
			delete pvmveOverflow;
			pvmveOverflow = nullptr;
		}
#endif
		cmveCur = 0;
	}
};


/*	
 *	VMVE is a maximum 256-move list, which is enough to hold the worst-case movegen
 *	number of moves.
 */

typedef T_VMVE<256> VMVE;