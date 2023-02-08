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
	inline MV(void) noexcept { *(uint16_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MV(uint16_t u) noexcept { *(uint16_t*)this = u; }
	inline MV(SQ sqFrom, SQ sqTo) noexcept { *(uint16_t*)this = 0;	usqFrom = sqFrom; usqTo = sqTo; }
	inline operator uint16_t() const noexcept { return *(uint16_t*)this; }

	inline SQ sqFrom(void) const noexcept { return usqFrom; }
	inline SQ sqTo(void) const noexcept { return usqTo; }
	inline APC apcPromote(void) const noexcept { return static_cast<APC>(uapcPromote); }
	inline void SetApcPromote(APC apc) noexcept { uapcPromote = static_cast<uint16_t>(apc); }
	inline bool fIsNil(void) const noexcept { return usqFrom == 0 && usqTo == 0; }
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
	inline MVU(void) noexcept { *(uint32_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MVU(uint32_t u) noexcept { *(uint32_t*)this = u; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MVU(MV mv) noexcept { *(uint32_t*)this = 0; *(uint16_t*)this = *(uint16_t*)&mv; }
	inline operator uint32_t() const noexcept { return *(uint32_t*)this; }

	inline MVU(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		assert(pcMove.apc() != apcNull);
		*(uint32_t*)this = 0;	// initialize everything else to 0
		upcMove = pcMove;
		usqFrom = sqFrom;
		usqTo = sqTo;
	}

	inline void SetPcMove(PC pcMove) noexcept { upcMove = pcMove; }
	inline APC apcMove(void) const noexcept { return PC(upcMove).apc(); }
	inline CPC cpcMove(void) const noexcept { return PC(upcMove).cpc(); }
	inline PC pcMove(void) const noexcept { return PC(upcMove); }
	inline void SetCapture(APC apc) noexcept { uapcCapt = static_cast<uint16_t>(apc); }

	inline void SetCsEp(int cs, SQ sqEnPassant) noexcept
	{
		ucs = cs;
		ufEnPassant = !sqEnPassant.fIsNil();
		if (ufEnPassant)
			ufileEnPassant = sqEnPassant.file();
	}

	inline int csPrev(void) const noexcept { return ucs; }
	inline int fileEpPrev(void) const noexcept { return ufileEnPassant; }
	inline bool fEpPrev(void) const noexcept { return ufEnPassant; }
	inline APC apcCapture(void) const noexcept { return (APC)uapcCapt; }
	inline bool fIsCapture(void) const noexcept { return apcCapture() != apcNull; }
	inline bool operator==(const MVU& mv) const noexcept { return *(uint32_t*)this == (uint32_t)mv; }
	inline bool operator!=(const MVU& mv) const noexcept { return *(uint32_t*)this != (uint32_t)mv; }
	inline bool operator==(const MV& mv) const noexcept { return *(uint16_t*)this == (uint16_t)mv; }
	inline bool operator!=(const MV& mv) const noexcept { return *(uint16_t*)this != (uint16_t)mv; }
};

static_assert(sizeof(MVU) == sizeof(uint32_t));

const MVU mvuNil = MVU();
const MV mvNil = MV();
const MVU mvuAll = MVU(sqH8, sqH8, PC(cpcWhite, apcError));
const MV mvAll = MV(sqH8, sqH8);
wstring SzFromMv(MV mv);


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

const int depthMax = 127;						/* maximum search depth */
const EV evPawn = 100;							/* evals are in centi-pawns */
const EV evInf = 160 * evPawn + depthMax;		/* largest possible evaluation */
const EV evSureWin = 40 * evPawn;				/* we have sure win when up this amount of material */
const EV evMate = evInf - 1;					/* checkmates are given evals of evalMate minus moves to mate */
const EV evMateMin = evMate - depthMax;
const EV evTempo = evPawn/3;					/* evaluation of a single move advantage */
const EV evDraw = 0;							/* evaluation of a draw */
const EV evTimedOut = evInf + 1;
const EV evCanceled = evTimedOut + 1;
const EV evMax = evCanceled + 1;
const EV evBias = evInf;						/* used to bias evaluations for saving as an unsigned */
static_assert(evMax <= 16384);					/* there is code that asssumes EV stores in 15 bits */

inline EV EvMate(int depth) { return evMate - depth; }
inline bool FEvIsMate(EV ev) { return ev >= evMateMin; }
inline bool FEvIsInterrupt(EV ev) { return abs(ev) > evInf; }	/* timeout or cancel */
inline int DepthFromEvMate(EV ev) { return evMate - ev; }
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
	tscXTable = 1,
	tscEvCapture = 2,
	tscEvOther = 3
};

inline TSC& operator++(TSC& tsc)
{
	tsc = static_cast<TSC>(tsc + 1);
	return tsc;
}

inline TSC operator++(TSC& tsc, int)
{
	TSC tscT = tsc;
	tsc = static_cast<TSC>(tsc + 1);
	return tscT;
}

wstring SzFromTsc(TSC tsc);


/*
 *
 *	MVE structure
 *
 *	A move along with move evaluation information, which is used for generating AI move 
 *	lists and alpha-beta pruning. All movegens generate moves into this sized element 
 *	(instead of just a simple move list) to minimize the amount of copying needed during 
 *	AI search.
 *
 *	Non-AI customers of this structure can safely ignore everything except the MV
 * 
 */


class MVE : public MVU
{
public:
	EV ev;
	uint16_t utsc;	// score type, used by ai search to enumerate good moves first for alpha-beta

	inline MVE(void) noexcept { *(uint64_t*)this = (uint32_t)mvuNil; }
	MVE(MVU mvu) noexcept { *(uint64_t*)this = (uint32_t)mvu; }
	MVE(MVU mvu, EV ev) noexcept : MVU(mvu), ev(ev), utsc(0) { }
#pragma warning(suppress:26495)	 
	MVE(uint64_t mve) noexcept { *(uint64_t*)this = mve; }
#pragma warning(suppress:26495)	
	MVE(SQ sqFrom, SQ sqTo, PC pcMove) noexcept {
		*(uint64_t*)this = 0;
		usqFrom = sqFrom;
		usqTo = sqTo;
		upcMove = pcMove;
	}

	inline operator uint64_t() const noexcept { return *(uint64_t*)this; }
	inline operator MVU() const noexcept { return *(uint32_t*)this; }

	/* comparison operations work on the eval */

	inline bool operator>(const MVE& mve) const noexcept { return utsc > mve.utsc || (utsc == mve.utsc && ev > mve.ev); }
	inline bool operator<(const MVE& mve) const noexcept { return utsc < mve.utsc || (utsc == mve.utsc && ev < mve.ev); }
	inline bool operator>=(const MVE& mve) const noexcept { return !(*this < mve); }
	inline bool operator<=(const MVE& mve) const noexcept { return !(*this > mve); }

	inline void SetTsc(TSC tsc) noexcept { utsc = static_cast<uint16_t>(tsc); }
	inline TSC tsc() const noexcept { return static_cast<TSC>(utsc); }
	inline void SetMvu(MVU mvu) noexcept { *(MVU*)this = mvu; }
};

static_assert(sizeof(MVE) == sizeof(uint64_t));


/*
 *
 *	VMVE class
 *
 *	Generated move list. Has simple array semantics. Actually stores an MVE, which
 *	is just handy extra room for an evaluation during AI search.
 *
 */


const size_t cmvePreMax = 60;

class VMVE
{
private:
	uint64_t amve[cmvePreMax];
	int cmveCur;
	vector<uint64_t>* pvmveOverflow;

public:


	VMVE() : cmveCur(0), pvmveOverflow(nullptr)
	{
		amve[0] = 0;
	}


	~VMVE()
	{
		if (pvmveOverflow)
			delete pvmveOverflow;
	}


	VMVE(const VMVE& vmve) noexcept : cmveCur(vmve.cmveCur), pvmveOverflow(nullptr)
	{
		assert(vmve.FValid());
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
		if (vmve.pvmveOverflow)
			pvmveOverflow = new vector<uint64_t>(*vmve.pvmveOverflow);
	}


	/*	VMVE move constructor
	 *
	 *	Moves data from one GMV to another. Note that the source GMV is trashed.
	 */
	VMVE(VMVE&& vmve) noexcept : cmveCur(vmve.cmveCur), pvmveOverflow(vmve.pvmveOverflow)
	{
		assert(vmve.FValid());
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
		vmve.cmveCur = 0;
		vmve.pvmveOverflow = nullptr;
	}


	VMVE& operator=(const VMVE& vmve)
	{
		assert(vmve.FValid());
		cmveCur = vmve.cmveCur;
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
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
		assert(FValid());
		return *this;
	}


	VMVE& operator=(VMVE&& vmve) noexcept
	{
		if (this == &vmve)
			return *this;
		cmveCur = vmve.cmveCur;
		memcpy(amve, vmve.amve, min(cmveCur, cmvePreMax) * sizeof(uint64_t));
		if (pvmveOverflow != nullptr) {
			delete pvmveOverflow;
			pvmveOverflow = nullptr;
		}
		pvmveOverflow = vmve.pvmveOverflow;
		vmve.pvmveOverflow = nullptr;
		return *this;
	}


#ifndef NDEBUG
	bool FValid(void) const noexcept
	{
		return (cmveCur <= cmvePreMax && pvmveOverflow == nullptr) ||
			(cmveCur > cmvePreMax && pvmveOverflow != nullptr && pvmveOverflow->size() + cmvePreMax == cmveCur);
	}
#endif


	inline int cmve(void) const noexcept
	{
		assert(FValid());
		return cmveCur;
	}


	inline MVE& operator[](int imve) noexcept
	{
		assert(FValid());
		assert(imve >= 0 && imve < cmveCur);
		if (imve < cmvePreMax)
			return *(MVE*)&amve[imve];
		else {
			assert(pvmveOverflow != nullptr);
			return *(MVE*)&(*pvmveOverflow)[imve - cmvePreMax];
		}
	}


	inline const MVE& operator[](int imve) const noexcept
	{
		assert(FValid());
		assert(imve >= 0 && imve < cmveCur);
		if (imve < cmvePreMax)
			return *(MVE*)&amve[imve];
		else {
			assert(pvmveOverflow != nullptr);
			return *(MVE*)&(*pvmveOverflow)[imve - cmvePreMax];
		}
	}

	/* iterator and const iterator for the arrays */

	class iterator {
		VMVE* pvmve;
		int imve;
	public:
		using difference_type = int;
		using value_type = MVE;
		using pointer = MVE*;
		using reference = MVE&;
		using iterator_category = random_access_iterator_tag;

		inline iterator(VMVE* pvmve, difference_type imve) noexcept : pvmve(pvmve), imve(imve) { }
		inline iterator(const iterator& it) noexcept : pvmve(it.pvmve), imve(it.imve) { }
		inline iterator& operator=(const iterator& it) noexcept { pvmve = it.pvmve; imve = it.imve; return *this; }
		inline reference operator[](difference_type dimve) const noexcept { return (*pvmve)[imve + dimve]; }
		inline reference operator*() const noexcept { return (*pvmve)[imve]; }
		inline pointer operator->() const noexcept { return &(*pvmve)[imve]; }
		inline iterator& operator++() noexcept { imve++; return *this; }
		inline iterator operator++(int) noexcept { int imveT = imve; imve++; return iterator(pvmve, imveT); }
		inline iterator& operator--() noexcept { imve--; return *this; }
		inline iterator operator--(int) noexcept { int imveT = imve; imve--; return iterator(pvmve, imveT); }
		inline iterator operator+(difference_type dimve) const noexcept { return iterator(pvmve, imve + dimve); }
		inline iterator operator-(difference_type dimve) const noexcept { return iterator(pvmve, imve - dimve); }
		inline iterator& operator+=(difference_type dimve) noexcept { imve += dimve; return *this; }
		inline iterator& operator-=(difference_type dimve) noexcept { imve -= dimve; return *this; }
		friend inline iterator operator+(difference_type dimve, iterator const& it) { return iterator(it.pvmve, dimve + it.imve); }
		inline difference_type operator-(const iterator& it) const noexcept { return imve - it.imve; }
		inline bool operator==(const iterator& it) const noexcept { return imve == it.imve; }
		inline bool operator!=(const iterator& it) const noexcept { return imve != it.imve; }
		inline bool operator<(const iterator& it) const noexcept { return imve < it.imve; }
	};

	class citerator {
		const VMVE* pvmve;
		int imve;
	public:
		using difference_type = int;
		using value_type = MVE;
		using pointer = const MVE*;
		using reference = const MVE&;
		typedef random_access_iterator_tag iterator_category;

		inline citerator(const VMVE* pvmve, int imve) noexcept : pvmve(pvmve), imve(imve) { }
		inline citerator(const citerator& it) noexcept : pvmve(it.pvmve), imve(it.imve) { }
		//		inline citerator operator=(const citerator& cit) noexcept { pvmve = cit.pvmve; imve = cit.imve; return *this; }

		inline reference operator[](difference_type dimve) const noexcept { return (*pvmve)[imve + dimve]; }
		inline reference operator*() const noexcept { return (*pvmve)[imve]; }
		inline pointer operator->() const noexcept { return &(*pvmve)[imve]; }

		inline citerator operator++() noexcept { imve++; return *this; }
		inline citerator operator++(int) noexcept { citerator cit = *this; imve++; return cit; }
		inline citerator operator--() noexcept { imve--; return *this; }
		inline citerator operator--(int) noexcept { citerator it = *this; imve--; return it; }

		inline citerator operator+(difference_type dimve) const noexcept { return citerator(pvmve, imve + dimve); }
		inline citerator operator-(difference_type dimve) const noexcept { return citerator(pvmve, imve - dimve); }
		inline citerator operator+=(difference_type dimve) noexcept { imve += dimve; return *this; }
		inline citerator operator-=(difference_type dimve) noexcept { imve -= dimve; return *this; }
		friend inline citerator operator+(difference_type dimve, const citerator& cit) { return citerator(cit.pvmve, dimve + cit.imve); }
		inline difference_type operator-(const citerator& cit) const noexcept { return imve - cit.imve; }

		inline bool operator==(const citerator& cit) const noexcept { return imve == cit.imve; }
		inline bool operator!=(const citerator& cit) const noexcept { return imve != cit.imve; }
		inline bool operator<(const citerator& cit) const noexcept { return imve < cit.imve; }
	};

	inline int size(void) const noexcept { return cmve(); }
	inline iterator begin(void) noexcept { return iterator(this, 0); }
	inline iterator end(void) noexcept { return iterator(this, size()); }
	inline citerator begin(void) const noexcept { return citerator(this, 0); }
	inline citerator end(void) const noexcept { return citerator(this, size()); }


	void push_back_overflow(MVE mve) noexcept
	{
		if (pvmveOverflow == nullptr) {
			assert(cmveCur == cmvePreMax);
			pvmveOverflow = new vector<uint64_t>;
		}
		pvmveOverflow->push_back(mve);
		cmveCur++;
	}


	inline void push_back(MVU mvu) noexcept
	{
		assert(FValid());
		if (cmveCur < cmvePreMax)
			amve[cmveCur++] = (uint32_t)mvu;
		else
			push_back_overflow((uint32_t)mvu);
		assert(FValid());
	}


	inline void push_back(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		push_back(MVU(sqFrom, sqTo, pcMove));
	}


	void insert_overflow(int imve, const MVE& mve) noexcept
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


	inline void insert(int imve, const MVE& mve) noexcept
	{
		assert(FValid());
		if (cmveCur >= cmvePreMax)
			insert_overflow(imve, mve);
		else {
			memmove(&amve[imve + 1], &amve[imve], sizeof(uint64_t) * (cmveCur - imve));
			amve[imve] = mve;
			cmveCur++;
		}
	}


	void resize(int cmveNew) noexcept
	{
		assert(FValid());
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
		cmveCur = cmveNew;
		assert(FValid());
	}


	void reserve(int cmve) noexcept
	{
		if (cmve <= cmvePreMax)
			return;
		if (pvmveOverflow == nullptr)
			pvmveOverflow = new vector<uint64_t>;
		pvmveOverflow->reserve(cmve - cmvePreMax);
	}


	void clear(void) noexcept
	{
		if (pvmveOverflow) {
			delete pvmveOverflow;
			pvmveOverflow = nullptr;
		}
		cmveCur = 0;
	}
};
