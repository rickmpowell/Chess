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
 *	There are three basic move types: MVM (a mini-move), a MV (contains undo
 *	information), and an EMV (an evaluated move).
 *	
 */
#pragma once
#include "pc.h"
#include "bb.h"


/*
 *
 *	MV and MVM type
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
 *	The first word of the move is the mini-move (MVM), which is the
 *	minimal amount of information needed, along with the board state,
 *	to specify a move. The redundant information could be filled in
 *	from the BD before the move is made.
 */

class MVM {
protected:
	uint16_t usqFrom : 6,
		usqTo : 6,
		uapcPromote : 3,
		unused1 : 1;
public:
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MVM(void) noexcept { *(uint16_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MVM(uint16_t u) noexcept { *(uint16_t*)this = u; }
	inline MVM(SQ sqFrom, SQ sqTo) noexcept { *(uint16_t*)this = 0;	usqFrom = sqFrom; usqTo = sqTo; }
	inline operator uint16_t() const noexcept { return *(uint16_t*)this; }

	inline SQ sqFrom(void) const noexcept { return usqFrom; }
	inline SQ sqTo(void) const noexcept { return usqTo; }
	inline APC apcPromote(void) const noexcept { return static_cast<APC>(uapcPromote); }
	inline void SetApcPromote(APC apc) noexcept { uapcPromote = static_cast<uint16_t>(apc); }
	inline bool fIsNil(void) const noexcept { return usqFrom == 0 && usqTo == 0; }
};

static_assert(sizeof(MVM) == sizeof(uint16_t));


class MV : public MVM {
private:
	uint16_t
		upcMove : 4,	// the piece that is moving
		uapcCapt : 3,	// for captures, the piece we take
		ufEnPassant : 1,	// en passant state
		ufileEnPassant : 3,
		ucs : 4,		// saved castle state
		unused2 : 1;

public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MV(void) noexcept { *(uint32_t*)this = 0; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MV(uint32_t u) noexcept { *(uint32_t*)this = u; }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MV(MVM mvm) noexcept { *(uint32_t*)this = 0; *(uint16_t*)this = *(uint16_t*)&mvm; }
	inline operator uint32_t() const noexcept { return *(uint32_t*)this; }
	//inline operator MVM() const noexcept { return *(MVM*)this; }

	inline MV(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		assert(pcMove.apc() != apcNull);
		*(uint32_t*)this = 0;	// initialize everything else to 0
		upcMove = pcMove;
		usqFrom = sqFrom;
		usqTo = sqTo;
	}

	inline MV& SetPcMove(PC pcMove) { upcMove = pcMove; return *this; }
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
	inline bool operator==(const MV& mv) const noexcept { return *(uint32_t*)this == (uint32_t)mv; }
	inline bool operator!=(const MV& mv) const noexcept { return *(uint32_t*)this != (uint32_t)mv; }
	inline bool operator==(const MVM& mvm) const noexcept { return *(uint16_t*)this == (uint16_t)mvm; }
	inline bool operator!=(const MVM& mvm) const noexcept { return *(uint16_t*)this != (uint16_t)mvm; }
};

static_assert(sizeof(MV) == sizeof(uint32_t));

const MV mvNil = MV();
const MVM mvmNil = MVM();
const MV mvAll = MV(sqH8, sqH8, PC(cpcWhite, apcError));
const MVM mvmAll = MVM(sqH8, sqH8);
wstring SzFromMvm(MVM mvm);


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
 *	EMV structure
 *
 *	A move along with move evaluation information, which is used for generating AI move 
 *	lists and alpha-beta pruning. All movegens generate moves into this sized element 
 *	(instead of just a simple move list) to minimize the amount of copying needed during 
 *	AI search.
 *
 *	Non-AI customers of this structure can safely ignore everything except the MV
 * 
 */


class EMV : public MV
{
public:
	EV ev;
	uint16_t utsc;	// score type, used by ai search to enumerate good moves first for alpha-beta

	EMV(MV mv = MV()) noexcept : MV(mv), ev(0), utsc(0) { }
	EMV(MV mv, EV ev) noexcept : MV(mv), ev(ev), utsc(0) { }
#pragma warning(suppress:26495)	 
	EMV(uint64_t emv) noexcept { *(uint64_t*)this = emv; }
	inline operator uint64_t() const noexcept { return *(uint64_t*)this; }

	/* comparison operations work on the eval */

	inline bool operator>(const EMV& emv) const noexcept { return utsc > emv.utsc || (utsc == emv.utsc && ev > emv.ev); }
	inline bool operator<(const EMV& emv) const noexcept { return utsc < emv.utsc || (utsc == emv.utsc && ev < emv.ev); }
	inline bool operator>=(const EMV& emv) const noexcept { return !(*this < emv); }
	inline bool operator<=(const EMV& emv) const noexcept { return !(*this > emv); }

	inline void SetTsc(TSC tsc) noexcept { utsc = static_cast<uint16_t>(tsc); }
	inline TSC tsc() const noexcept { return static_cast<TSC>(utsc); }
	inline void SetMv(MV mv) noexcept { *(MV*)this = mv; }
};

static_assert(sizeof(EMV) == sizeof(uint64_t));


/*
 *
 *	VEMV class
 *
 *	Generated move list. Has simple array semantics. Actually stores an EMV, which
 *	is just handy extra room for an evaluation during AI search.
 *
 */


const size_t cemvPreMax = 60;

class VEMV
{
private:
	uint64_t aemv[cemvPreMax];
	int cemvCur;
	vector<uint64_t>* pvemvOverflow;

public:
	VEMV() : cemvCur(0), pvemvOverflow(nullptr)
	{
		aemv[0] = 0;
	}

	~VEMV()
	{
		if (pvemvOverflow)
			delete pvemvOverflow;
	}

	VEMV(const VEMV& vemv) : cemvCur(vemv.cemvCur), pvemvOverflow(nullptr)
	{
		assert(vemv.FValid());
		memcpy(aemv, vemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (vemv.pvemvOverflow)
			pvemvOverflow = new vector<uint64_t>(*vemv.pvemvOverflow);
	}


	/*	VEMV move constructor
	 *
	 *	Moves data from one GMV to another. Note that the source GMV is trashed.
	 */
	VEMV(VEMV&& vemv) noexcept : cemvCur(vemv.cemvCur), pvemvOverflow(vemv.pvemvOverflow)
	{
		assert(vemv.FValid());
		memcpy(aemv, vemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		vemv.cemvCur = 0;
		vemv.pvemvOverflow = nullptr;
	}


	VEMV& operator=(const VEMV& vemv)
	{
		assert(vemv.FValid());
		cemvCur = vemv.cemvCur;
		memcpy(aemv, vemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (vemv.pvemvOverflow) {
			if (pvemvOverflow)
				*pvemvOverflow = *vemv.pvemvOverflow;
			else
				pvemvOverflow = new vector<uint64_t>(*vemv.pvemvOverflow);
		}
		else {
			if (pvemvOverflow) {
				delete pvemvOverflow;
				pvemvOverflow = nullptr;
			}
		}
		assert(FValid());
		return *this;
	}

	VEMV& operator=(VEMV&& vemv) noexcept
	{
		if (this == &vemv)
			return *this;
		cemvCur = vemv.cemvCur;
		memcpy(aemv, vemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (pvemvOverflow != nullptr) {
			delete pvemvOverflow;
			pvemvOverflow = nullptr;
		}
		pvemvOverflow = vemv.pvemvOverflow;
		vemv.pvemvOverflow = nullptr;
		return *this;
	}

#ifndef NDEBUG
	bool FValid(void) const noexcept
	{
		return (cemvCur <= cemvPreMax && pvemvOverflow == nullptr) ||
			(cemvCur > cemvPreMax && pvemvOverflow != nullptr && pvemvOverflow->size() + cemvPreMax == cemvCur);
	}
#endif

	inline int cemv(void) const noexcept
	{
		assert(FValid());
		return cemvCur;
	}


	inline EMV& operator[](int iemv) noexcept
	{
		assert(FValid());
		assert(iemv >= 0 && iemv < cemvCur);
		if (iemv < cemvPreMax)
			return *(EMV*)&aemv[iemv];
		else {
			assert(pvemvOverflow != nullptr);
			return *(EMV*)&(*pvemvOverflow)[iemv - cemvPreMax];
		}
	}

	inline const EMV& operator[](int iemv) const noexcept
	{
		assert(FValid());
		assert(iemv >= 0 && iemv < cemvCur);
		if (iemv < cemvPreMax)
			return *(EMV*)&aemv[iemv];
		else {
			assert(pvemvOverflow != nullptr);
			return *(EMV*)&(*pvemvOverflow)[iemv - cemvPreMax];
		}
	}

	/* iterator and const iterator for the arrays */

	class iterator {
		VEMV* pvemv;
		int iemv;
	public:
		using difference_type = int;
		using value_type = EMV;
		using pointer = EMV*;
		using reference = EMV&;
		using iterator_category = random_access_iterator_tag;

		inline iterator(VEMV* pvemv, difference_type iemv) noexcept : pvemv(pvemv), iemv(iemv) { }
		inline iterator(const iterator& it) noexcept : pvemv(it.pvemv), iemv(it.iemv) { }
		inline iterator& operator=(const iterator& it) noexcept { pvemv = it.pvemv; iemv = it.iemv; return *this; }
		inline reference operator[](difference_type diemv) const noexcept { return (*pvemv)[iemv + diemv]; }
		inline reference operator*() const noexcept { return (*pvemv)[iemv]; }
		inline pointer operator->() const noexcept { return &(*pvemv)[iemv]; }
		inline iterator& operator++() noexcept { iemv++; return *this; }
		inline iterator operator++(int) noexcept { int iemvT = iemv; iemv++; return iterator(pvemv, iemvT); }
		inline iterator& operator--() noexcept { iemv--; return *this; }
		inline iterator operator--(int) noexcept { int iemvT = iemv; iemv--; return iterator(pvemv, iemvT); }
		inline iterator operator+(difference_type diemv) const noexcept { return iterator(pvemv, iemv + diemv); }
		inline iterator operator-(difference_type diemv) const noexcept { return iterator(pvemv, iemv - diemv); }
		inline iterator& operator+=(difference_type diemv) noexcept { iemv += diemv; return *this; }
		inline iterator& operator-=(difference_type diemv) noexcept { iemv -= diemv; return *this; }
		friend inline iterator operator+(difference_type diemv, iterator const& it) { return iterator(it.pvemv, diemv + it.iemv); }
		inline difference_type operator-(const iterator& it) const noexcept { return iemv - it.iemv; }
		inline bool operator==(const iterator& it) const noexcept { return iemv == it.iemv; }
		inline bool operator!=(const iterator& it) const noexcept { return iemv != it.iemv; }
		inline bool operator<(const iterator& it) const noexcept { return iemv < it.iemv; }
	};

	class citerator {
		const VEMV* pvemv;
		int iemv;
	public:
		using difference_type = int;
		using value_type = EMV;
		using pointer = const EMV*;
		using reference = const EMV&;
		typedef random_access_iterator_tag iterator_category;

		inline citerator(const VEMV* pvemv, int iemv) noexcept : pvemv(pvemv), iemv(iemv) { }
		inline citerator(const citerator& it) noexcept : pvemv(it.pvemv), iemv(it.iemv) { }
		//		inline citerator operator=(const citerator& cit) noexcept { pvemv = cit.pvemv; iemv = cit.iemv; return *this; }

		inline reference operator[](difference_type diemv) const noexcept { return (*pvemv)[iemv + diemv]; }
		inline reference operator*() const noexcept { return (*pvemv)[iemv]; }
		inline pointer operator->() const noexcept { return &(*pvemv)[iemv]; }

		inline citerator operator++() noexcept { iemv++; return *this; }
		inline citerator operator++(int) noexcept { citerator cit = *this; iemv++; return cit; }
		inline citerator operator--() noexcept { iemv--; return *this; }
		inline citerator operator--(int) noexcept { citerator it = *this; iemv--; return it; }

		inline citerator operator+(difference_type diemv) const noexcept { return citerator(pvemv, iemv + diemv); }
		inline citerator operator-(difference_type diemv) const noexcept { return citerator(pvemv, iemv - diemv); }
		inline citerator operator+=(difference_type diemv) noexcept { iemv += diemv; return *this; }
		inline citerator operator-=(difference_type diemv) noexcept { iemv -= diemv; return *this; }
		friend inline citerator operator+(difference_type diemv, const citerator& cit) { return citerator(cit.pvemv, diemv + cit.iemv); }
		inline difference_type operator-(const citerator& cit) const noexcept { return iemv - cit.iemv; }

		inline bool operator==(const citerator& cit) const noexcept { return iemv == cit.iemv; }
		inline bool operator!=(const citerator& cit) const noexcept { return iemv != cit.iemv; }
		inline bool operator<(const citerator& cit) const noexcept { return iemv < cit.iemv; }
	};

	inline int size(void) const noexcept { return cemv(); }
	inline iterator begin(void) noexcept { return iterator(this, 0); }
	inline iterator end(void) noexcept { return iterator(this, size()); }
	inline citerator begin(void) const noexcept { return citerator(this, 0); }
	inline citerator end(void) const noexcept { return citerator(this, size()); }

	void push_back_overflow(EMV emv)
	{
		if (pvemvOverflow == nullptr) {
			assert(cemvCur == cemvPreMax);
			pvemvOverflow = new vector<uint64_t>;
		}
		pvemvOverflow->push_back(emv);
		cemvCur++;
	}


	inline void push_back(MV mv)
	{
		assert(FValid());
		if (cemvCur < cemvPreMax)
			aemv[cemvCur++] = EMV(mv);
		else
			push_back_overflow(EMV(mv));
		assert(FValid());
	}


	inline void push_back(SQ sqFrom, SQ sqTo, PC pcMove)
	{
		push_back(MV(sqFrom, sqTo, pcMove));
	}

	void insert_overflow(int iemv, const EMV& emv) noexcept
	{
		if (pvemvOverflow == nullptr)
			pvemvOverflow = new vector<uint64_t>;
		if (iemv >= cemvPreMax)
			pvemvOverflow->insert(pvemvOverflow->begin() + (iemv - cemvPreMax), emv);
		else {
			pvemvOverflow->insert(pvemvOverflow->begin(), aemv[cemvPreMax - 1]);
			memmove(&aemv[iemv + 1], &aemv[iemv], sizeof(uint64_t) * (cemvPreMax - iemv - 1));
		}
		cemvCur++;
	}

	inline void insert(int iemv, const EMV& emv) noexcept
	{
		assert(FValid());
		if (cemvCur >= cemvPreMax)
			insert_overflow(iemv, emv);
		else {
			memmove(&aemv[iemv + 1], &aemv[iemv], sizeof(uint64_t) * (cemvCur - iemv));
			aemv[iemv] = emv;
			cemvCur++;
		}
	}

	void resize(int cemvNew)
	{
		assert(FValid());
		if (cemvNew > cemvPreMax) {
			if (pvemvOverflow == nullptr)
				pvemvOverflow = new vector<uint64_t>;
			pvemvOverflow->resize(cemvNew - cemvPreMax);
		}
		else {
			if (pvemvOverflow != nullptr) {
				delete pvemvOverflow;
				pvemvOverflow = nullptr;
			}
		}
		cemvCur = cemvNew;
		assert(FValid());
	}


	void reserve(int cemv)
	{
		if (cemv <= cemvPreMax)
			return;
		if (pvemvOverflow == nullptr)
			pvemvOverflow = new vector<uint64_t>;
		pvemvOverflow->reserve(cemv - cemvPreMax);
	}


	void clear(void) noexcept
	{
		if (pvemvOverflow) {
			delete pvemvOverflow;
			pvemvOverflow = nullptr;
		}
		cemvCur = 0;
	}
};
