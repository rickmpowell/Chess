/*
 *
 *	bd.h
 *
 *	Board definitions
 *
 */
#pragma once
#include "framework.h"
#include "bb.h"



/*
 *
 *	RULE
 *
 *	Game rules
 *
 */


class RULE
{
	DWORD tmGame;	// if zero, untimed game
	DWORD dtmMove;
	unsigned cmvRepeatDraw; // if zero, no automatic draw detection
public:
	RULE(void) : tmGame(60 * 60 * 1000), dtmMove(0 * 1000), cmvRepeatDraw(3) { }
	RULE(DWORD tmGame, DWORD dtmMove, unsigned cmvRepeatDraw) : tmGame(tmGame), dtmMove(dtmMove),
		cmvRepeatDraw(cmvRepeatDraw) {
	}
	DWORD TmGame(void) const {
		return tmGame;
	}
	DWORD DtmMove(void) const {
		return dtmMove;
	}
	int CmvRepeatDraw(void) const {
		return cmvRepeatDraw;
	}
	void SetTmGame(DWORD tm) {
		tmGame = tm;
	}
};


/*
 *
 *	CPC enumeration
 * 
 *	Color of a piece. White or black, generally
 * 
 */


enum CPC {
	NoColor = -1,
	White = 0,
	Black = 1,
	ColorMax = 2
};

inline CPC operator~(CPC cpc)
{
	return static_cast<CPC>(static_cast<int>(cpc) ^ 1);
}

inline CPC operator++(CPC& cpc)
{
	cpc = static_cast<CPC>(static_cast<int>(cpc) + 1);
	return cpc;
}


/*
 *
 *	APC enumeration
 *
 *	Actual piece types
 *
 */


enum APC {
	Error = -1,
	Null = 0,
	Pawn = 1,
	Knight = 2,
	Bishop = 3,
	Rook = 4,
	Queen = 5,
	King = 6,
	ActMax = 7,
	Bishop2 = 7,	// only used for draw detection computation to keep track of bishop square color
	ActMax2 = 8
};

inline APC& operator++(APC& apc)
{
	apc = static_cast<APC>(static_cast<int>(apc) + 1);
	return apc;
}

inline APC& operator+=(APC& apc, int dapc)
{
	apc = static_cast<APC>(static_cast<int>(apc) + dapc);
	return apc;
}


/*
 *
 *	PC class
 * 
 *	Simple class for typing piece/color combination
 */


class PC
{
	uint8_t grf;
public:
	
	PC(uint8_t grf) noexcept : grf(grf)
	{
	}

	PC(CPC cpc, APC apc) noexcept : grf((cpc << 3) | apc)
	{
	}

	APC apc(void) const noexcept
	{
		return (APC)(grf & 7);
	}

	CPC cpc(void) const noexcept
	{
		return (CPC)((grf >> 3) & 1);
	}

	inline operator uint8_t() const noexcept
	{
		return grf;
	}

	inline PC& operator++()
	{
		grf++;
		return *this;
	}

	inline PC operator++(int)
	{
		uint8_t grfT = grf++;
		return PC(grfT);
	}
};

const uint8_t pcMax = 2*8;
const PC pcWhitePawn(CPC::White, APC::Pawn);
const PC pcBlackPawn(CPC::Black, APC::Pawn);
const PC pcWhiteKnight(CPC::White, APC::Knight);
const PC pcBlackKnight(CPC::Black, APC::Knight);
const PC pcWhiteBishop(CPC::White, APC::Bishop);
const PC pcBlackBishop(CPC::Black, APC::Bishop);
const PC pcWhiteRook(CPC::White, APC::Rook);
const PC pcBlackRook(CPC::Black, APC::Rook);
const PC pcWhiteQueen(CPC::White, APC::Queen);
const PC pcBlackQueen(CPC::Black, APC::Queen);
const PC pcWhiteKing(CPC::White, APC::King);
const PC pcBlackKing(CPC::Black, APC::King);


/*	
 * 
 *	MV type
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
 *  |    pc move|       to        |      from       |
 *  +--+--------+--------+--------+--------+--------+
 *  +-----+--------+--------+--+--------+-----------+
 *  | XXX |apc prom|ep file |ep|apc capt| cs state  |
 *  +-----+--------+--------+--+--------+-----------+
 *   31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 */

class MV {
private:
	uint32_t sqFromGrf : 6,
		sqToGrf : 6,	
		pcMoveGrf : 4,	// the piece that is moving
		csGrf : 4,		// saved castle state
		apcCaptGrf : 3,	// for captures, the piece we take
		fEnPassantGrf : 1,	// en passant state
		fileEnPassantGrf : 3,	
		apcPromoteGrf : 3,	// for promotion, the piece we promote to
		padding : 2;

public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline MV(void) noexcept
	{
		assert(sizeof(MV) == sizeof(uint32_t));
		*(uint32_t*)this = 0;
	}

	inline MV(SQ sqFrom, SQ sqTo, PC pcMove) noexcept
	{
		assert(pcMove.apc() != APC::Null);
		*(uint32_t*)this = 0;	// initialize everything else to 0
		sqFromGrf = sqFrom;
		sqToGrf = sqTo;
		pcMoveGrf = pcMove;
	}

	inline operator uint32_t() const noexcept
	{
		return *(uint32_t*)this;
	}

	inline SQ sqFrom(void) const noexcept
	{
		return sqFromGrf;
	}

	inline SQ sqTo(void) const noexcept
	{
		return sqToGrf;
	}
	
	inline APC apcMove(void) const noexcept
	{
		return PC(pcMoveGrf).apc();
	}

	inline CPC cpcMove(void) const noexcept
	{
		return PC(pcMoveGrf).cpc();
	}

	inline PC pcMove(void) const noexcept
	{
		return PC(pcMoveGrf);
	}

	inline APC apcPromote(void) const noexcept
	{
		return (APC)apcPromoteGrf;
	}

	inline bool fIsNil(void) const noexcept
	{
		return sqFromGrf == 0 && sqToGrf == 0;
	}

	inline MV& SetApcPromote(APC apc) noexcept
	{
		apcPromoteGrf = apc;
		return *this;
	}

	inline void SetCapture(APC apc) noexcept
	{
		apcCaptGrf = apc;
	}

	inline void SetCsEp(int cs, SQ sqEnPassant) noexcept
	{
		csGrf = cs;
		fEnPassantGrf = !sqEnPassant.fIsNil();
		if (fEnPassantGrf)
			fileEnPassantGrf = sqEnPassant.file();
	}

	inline int csPrev(void) const noexcept
	{
		return csGrf;
	}

	inline int fileEpPrev(void) const noexcept
	{
		return fileEnPassantGrf;
	}

	inline bool fEpPrev(void) const noexcept
	{
		return fEnPassantGrf;
	}

	inline APC apcCapture(void) const noexcept
	{
		return (APC)apcCaptGrf;
	}

	inline bool fIsCapture(void) const noexcept
	{
		return apcCapture() != APC::Null; 
	}

	inline bool operator==(const MV& mv) const noexcept
	{
		return *(uint32_t*)this == (uint32_t)mv;
	}

	inline bool operator!=(const MV& mv) const noexcept
	{
		return *(uint32_t*)this != (uint32_t)mv;
	}
};


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

const int plyMax = 127;
const EV evInf = 160 * 100;	/* largest possible evaluation, in centi-pawns */
const EV evMate = evInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EV evMateMin = evMate - plyMax;
const EV evTempo = 33;	/* evaluation of a single move advantage */
const EV evDraw = 0;	/* evaluation of a draw */
const EV evAbort = evInf + 1;

inline EV EvMate(int ply) { return evMate - ply; }
inline bool FEvIsMate(EV ev) { return ev >= evMateMin; }
inline bool FEvIsAbort(EV ev) { return ev == evAbort; }
inline int PlyFromEvMate(EV ev) { return evMate - ev; }
wstring SzFromEv(EV ev);


/*
 *
 *	EMV structure
 *
 *	Just a little holder of move evaluation information which is used for
 *	generating AI move lists and alpha-beta pruning. We generate moves into
 *	this sized element (instead of just a simple move list) to speed up search.
 *
 */


class EMV
{
public:
	MV mv;
	EV ev;

	EMV(MV mv = MV()) noexcept : mv(mv), ev(0) { }
#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	EMV(uint64_t emv) noexcept { *(uint64_t*)this = emv; }
	inline operator uint64_t() const noexcept { return *(uint64_t*)this; }
	inline bool operator>(const EMV& emv) const noexcept 
	{
		return ev > emv.ev; 
	}
	inline bool operator>=(const EMV& emv) const noexcept 
	{ 
		return ev >= emv.ev; 
	}
	inline bool operator<(const EMV& emv) const noexcept 
	{ 
		return ev < emv.ev; 
	}
	inline bool operator<=(const EMV& emv) const noexcept 
	{ 
		return ev <= emv.ev; 
	}
};

static_assert(sizeof(EMV) == sizeof(uint64_t));


/*
 *
 *	GEMV class
 * 
 *	Generated move list. Has simple array semantics. Actually stores an EMV, which
 *	is just handy extra room for an evaluation during search.
 * 
 */


const size_t cemvPreMax = 60;

class GEMV
{
private:
	uint64_t aemv[cemvPreMax];
	int cemvCur;
	vector<uint64_t>* pvemvOverflow;

public:
	GEMV() : cemvCur(0), pvemvOverflow(nullptr)
	{
		aemv[0] = 0;
	}

	~GEMV()
	{
		if (pvemvOverflow)
			delete pvemvOverflow;
	}

	/*	GEMV copy constructor
	 */
	GEMV(const GEMV& gemv) : cemvCur(gemv.cemvCur), pvemvOverflow(nullptr)
	{
		assert(gemv.FValid());
		memcpy(aemv, gemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (gemv.pvemvOverflow)
			pvemvOverflow = new vector<uint64_t>(*gemv.pvemvOverflow);
	}


	/*	GEMV move constructor
	 *
	 *	Moves data from one GMV to another. Note that the source GMV is trashed.
	 */
	GEMV(GEMV&& gemv) noexcept : cemvCur(gemv.cemvCur), pvemvOverflow(gemv.pvemvOverflow)
	{
		assert(gemv.FValid());
		memcpy(aemv, gemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		gemv.cemvCur = 0;
		gemv.pvemvOverflow = nullptr;
	}


	GEMV& operator=(const GEMV& gemv)
	{
		assert(gemv.FValid());
		cemvCur = gemv.cemvCur;
		memcpy(aemv, gemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (gemv.pvemvOverflow) {
			if (pvemvOverflow)
				*pvemvOverflow = *gemv.pvemvOverflow;
			else
				pvemvOverflow = new vector<uint64_t>(*gemv.pvemvOverflow);
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

	GEMV& operator=(GEMV&& gemv) noexcept
	{
		if (this == &gemv)
			return *this;
		cemvCur = gemv.cemvCur;
		memcpy(aemv, gemv.aemv, min(cemvCur, cemvPreMax) * sizeof(uint64_t));
		if (pvemvOverflow != nullptr) {
			delete pvemvOverflow;
			pvemvOverflow = nullptr;
		}
		pvemvOverflow = gemv.pvemvOverflow;
		gemv.pvemvOverflow = nullptr;
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
		GEMV* pgemv;
		int iemv;
	public:
		using difference_type = int;
		using value_type = EMV;
		using pointer = EMV*;
		using reference = EMV&;
		using iterator_category = random_access_iterator_tag;

		inline iterator(GEMV* pgemv, difference_type iemv) noexcept : pgemv(pgemv), iemv(iemv)
		{
		}
		inline iterator(const iterator& it) noexcept : pgemv(it.pgemv), iemv(it.iemv) 
		{
		}
		inline iterator& operator=(const iterator& it) noexcept 
		{ 
			pgemv = it.pgemv; iemv = it.iemv; return *this;
		}

		inline reference operator[](difference_type diemv) const noexcept 
		{ 
			return (*pgemv)[iemv + diemv]; 
		}
		inline reference operator*() const noexcept 
		{ 
			return (*pgemv)[iemv]; 
		}
		inline pointer operator->() const noexcept 
		{
			return &(*pgemv)[iemv]; 
		}

		inline iterator& operator++() noexcept 
		{
			iemv++; return *this;
		}
		inline iterator operator++(int) noexcept 
		{
			int iemvT = iemv; iemv++; return iterator(pgemv, iemvT);
		}
		inline iterator& operator--() noexcept 
		{
			iemv--; return *this;
		}
		inline iterator operator--(int) noexcept 
		{ 
			int iemvT = iemv; iemv--; return iterator(pgemv, iemvT);
		}

		inline iterator operator+(difference_type diemv) const noexcept 
		{
			return iterator(pgemv, iemv + diemv); 
		}
		inline iterator operator-(difference_type diemv) const noexcept 
		{ 
			return iterator(pgemv, iemv - diemv); 
		}
		inline iterator& operator+=(difference_type diemv) noexcept 
		{ 
			iemv += diemv; return *this; 
		}
		inline iterator& operator-=(difference_type diemv) noexcept 
		{ 
			iemv -= diemv; return *this; 
		}
		friend inline iterator operator+(difference_type diemv, iterator const& it) 
		{ 
			return iterator(it.pgemv, diemv + it.iemv); 
		}
		inline difference_type operator-(const iterator& it) const noexcept 
		{ 
			return iemv - it.iemv; 
		}

		inline bool operator==(const iterator& it) const noexcept 
		{
			return iemv == it.iemv; 
		}
		inline bool operator!=(const iterator& it) const noexcept 
		{ 
			return iemv != it.iemv; 
		}
		inline bool operator<(const iterator& it) const noexcept 
		{ 
			return iemv < it.iemv; 
		}
	};

	class citerator {
		const GEMV* pgemv;
		int iemv;
	public:
		using difference_type = int;
		using value_type = EMV;
		using pointer = const EMV*;
		using reference = const EMV&;
		typedef random_access_iterator_tag iterator_category;

		inline citerator(const GEMV* pgemv, int iemv) noexcept : pgemv(pgemv), iemv(iemv) { }
		inline citerator(const citerator& it) noexcept : pgemv(it.pgemv), iemv(it.iemv) { }
//		inline citerator operator=(const citerator& cit) noexcept { pgemv = cit.pgemv; iemv = cit.iemv; return *this; }

		inline reference operator[](difference_type diemv) const noexcept { return (*pgemv)[iemv + diemv]; }
		inline reference operator*() const noexcept { return (*pgemv)[iemv]; }
		inline pointer operator->() const noexcept { return &(*pgemv)[iemv]; }

		inline citerator operator++() noexcept { iemv++; return *this; }
		inline citerator operator++(int) noexcept { citerator cit = *this; iemv++; return cit; }
		inline citerator operator--() noexcept { iemv--; return *this; }
		inline citerator operator--(int) noexcept { citerator it = *this; iemv--; return it; }

		inline citerator operator+(difference_type diemv) const noexcept { return citerator(pgemv, iemv + diemv); }
		inline citerator operator-(difference_type diemv) const noexcept { return citerator(pgemv, iemv - diemv); }
		inline citerator operator+=(difference_type diemv) noexcept { iemv += diemv; return *this; }
		inline citerator operator-=(difference_type diemv) noexcept { iemv -= diemv; return *this; }
		friend inline citerator operator+(difference_type diemv, const citerator& cit) { return citerator(cit.pgemv, diemv + cit.iemv); }
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

	void AppendMvOverflow(MV mv)
	{
		if (pvemvOverflow == nullptr) {
			assert(cemvCur == cemvPreMax);
			pvemvOverflow = new vector<uint64_t>;
		}
		pvemvOverflow->push_back(mv);
		cemvCur++;
	}


	inline void AppendMv(MV mv)
	{
		assert(FValid()); 
		if (cemvCur < cemvPreMax)
			aemv[cemvCur++] = mv;
		else
			AppendMvOverflow(mv);
		assert(FValid());
	}


	inline void AppendMv(SQ sqFrom, SQ sqTo, PC pcMove)
	{
		assert(FValid());
		if (cemvCur < cemvPreMax) {
			aemv[cemvCur] = MV(sqFrom, sqTo, pcMove);
			cemvCur++;
		}
		else
			AppendMvOverflow(MV(sqFrom, sqTo, pcMove));
	}

	void InsertOverflow(int iemv, const EMV& emv) noexcept
	{
		if (pvemvOverflow == nullptr)
			pvemvOverflow = new vector<uint64_t>;
		if (iemv >= cemvPreMax)
			pvemvOverflow->insert(pvemvOverflow->begin() + (iemv - cemvPreMax), emv);
		else {
			pvemvOverflow->insert(pvemvOverflow->begin(), aemv[cemvPreMax - 1]);
			memmove(&aemv[iemv+1], &aemv[iemv], sizeof(uint64_t) * (cemvPreMax - iemv - 1));
		}
		cemvCur++;
	}

	inline void Insert(int iemv, const EMV& emv) noexcept
	{
		assert(FValid());
		if (cemvCur >= cemvPreMax)
			InsertOverflow(iemv, emv);
		else {
			memmove(&aemv[iemv+1], &aemv[iemv], sizeof(uint64_t) * (cemvCur - iemv));
			aemv[iemv] = emv;
			cemvCur++;
		}
	}

	void Resize(int cemvNew)
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


	void Reserve(int cemv)
	{
		if (cemv <= cemvPreMax)
			return;
		if (pvemvOverflow == nullptr)
			pvemvOverflow = new vector<uint64_t>;
		pvemvOverflow->reserve(cemv - cemvPreMax);
	}


	void Clear(void) noexcept
	{
		if (pvemvOverflow) {
			delete pvemvOverflow;
			pvemvOverflow = nullptr;
		}
		cemvCur = 0;
	}
};


/*
 *
 *	GS emumeration
 * 
 *	Game state. Either we're playing, or the game is over by several possible
 *	situations.
 *
 */


enum class GS {
	Playing = 0,
	WhiteCheckMated,
	BlackCheckMated,
	WhiteTimedOut,
	BlackTimedOut,
	WhiteResigned,
	BlackResigned,
	StaleMate,
	DrawDead,
	DrawAgree,
	Draw3Repeat,
	Draw50Move,
	Canceled
};


/*
 *
 *	CS enumeration
 * 
 *	Castle state. This is a 4-bit value which keeps track of which castles are still
 *	legal.
 *
 */

enum {
	csKing = 0x01, csQueen = 0x04,
	csWhiteKing = 0x01,
	csBlackKing = 0x02,
	csWhiteQueen = 0x04,
	csBlackQueen = 0x08
};


/*
 *
 *	Some convenience functions that return specific special locations on the board. This
 *	is not exhaustive, and I'm just writing and adding them as I need them. Many of these
 *	need to be very fast.
 * 
 */


/*	RankPromoteFromCpc
 *
 *	Returns the rank that pawns promote on for the given color; i.e., Rank 7 for
 *	white, rank 0 for black.
 */
inline int RankPromoteFromCpc(CPC cpc)
{
	return ~-(int)cpc & 7;
}


/*	RankBackFromCpc
 *
 *	Returns the back rank for the given color, i.e., Rank 0 for white, rank
 *	7 for black.
 */
inline int RankBackFromCpc(CPC cpc)
{
	/* white -> 0, black -> 7 */
	return (-(int)cpc) & 7;
}


/*	RankTakeEpFromCpc
 *
 *	The taken pawn rank for en passant. Will be the rank of the opposite color
 *	pawn taken during the en passant capture
 */
inline int RankTakeEpFromCpc(CPC cpc)
{
	/* white -> 4, black -> 3 */
	return 4 - (int)cpc;
}


/*	RankToEpFromCpc
 *
 *	The destination rank of en passant captures (where the capturing pawn ends
 *	up).
 */
inline int RankToEpFromCpc(CPC cpc)
{
	/* white -> 5, black -> 2 */
	return (5 ^ -(int)cpc) & 7;
}


/*
 *
 *	HABD
 * 
 *	Board hash
 * 
 */


typedef uint64_t HABD;


/*
 *
 *	GENHABD class
 *
 *	Generate game board hashing. This uses Zobrist hashing, which creates a large
 *	bit-array for each possible individual square state on the board. The hash is
 *	the XOR of all the individual states.
 *
 *	The advantage of Zobrist hashing is it's inexpensive to keep up-to-date during
 *	move/undo with two or three simple xor operations.
 *
 */

class BD;

class GENHABD
{
private:
	HABD rghabdPiece[sqMax][pcMax];
	HABD rghabdCastle[16];
	HABD rghabdEnPassant[8];
	HABD habdMove;

public:

	GENHABD(void);
	HABD HabdRandom(mt19937_64& rgen);
	HABD HabdFromBd(const BD& bd) const;

	
	/*	HABD::TogglePiece
	 *
	 *	Toggles the square/ipc in the hash at the given square.
	 */
	inline void TogglePiece(HABD& habd, SQ sq, PC pc) const
	{
		habd ^= rghabdPiece[sq][pc];
	}


	/*	HABD::ToggleToMove
	 *
	 *	Toggles the player to move in the hash.
	 */
	inline void ToggleToMove(HABD& habd) const
	{
		habd ^= habdMove;
	}


	/*	HABD::ToggleCastle
	 *
	 *	Toggles the castle state in the hash
	 */
	inline void ToggleCastle(HABD& habd, int cs) const
	{
		habd ^= rghabdCastle[cs];
	}


	/*	HABD::ToggleEnPassant
	 *
	 *	Toggles the en passant state in the hash
	 */
	inline void ToggleEnPassant(HABD& habd, int file) const
	{
		habd ^= rghabdEnPassant[file];
	}
};

extern GENHABD genhabd;


/*
 *
 *	BD class
 * 
 *	The board. Our board is 8 rows (rank) x 16 column (file) representation, with unused
 *	files 8-15. This representation is convenient because it leaves an extra bit worth of 
 *	space for overflowing the legal range, which we use for representing "off board". 
 * 
 *	Be careful how you enumerate over the squares of the board, since there are invalid 
 *	squares in the naive loop.
 * 
 */


enum class GG {	// GenGmv Option to optionally remove checks
	Legal,
	Pseudo
};


/*
 *
 *	Game phase is a rudimentary approximation of how far along we are in the
 *	game progression. It's basically a measure of the non-pawn pieces on the
 *	board. When most pieces are still on the board, we're in the opening, while
 *	if we're down to a few minor pieces, we're in the end game.
 * 
 */


enum class GPH {	// game phase
	Queen = 4,
	Rook = 2,
	Minor = 1,
	None = 0,

	Max = 24,		// when all pieces are on the board
	MidMin = 2,		// opening is over when two minor pieces are gone
	MidMid = 4,	// a mid-point of the middle game for when strategies should start transitioning
	MidMax = 18,		// end game is when we're down to rook/minor piece vs. rook/minor piece

	Nil = -1
};

static_assert((int)GPH::Max == 2 * (int)GPH::Queen + 4 * (int)GPH::Rook + 8 * (int)GPH::Minor);

inline bool FInOpening(GPH gph)
{
	return gph <= GPH::MidMin;
}

inline bool FInEndGame(GPH gph)
{
	return gph >= GPH::MidMax;
}


class BD
{
public:
	/* minimal board state, defines a position exactly */

	BB mppcbb[pcMax];	/* bitboards for the pieces */
	CPC cpcToMove;	/* side with the move */
	SQ sqEnPassant;	/* non-nil when previous move was a two-square pawn move, destination
					   of en passant capture */
	uint8_t csCur;	/* castle sides */

	/* everything below is derived from previous state */

	BB mpcpcbb[CPC::ColorMax]; // squares occupied by pieces of the color 
	BB bbUnoccupied;	// empty squares
	HABD habd;	/* board hash */
	GPH gph;	/* game phase */

public:
	BD(void);

	BD(const BD& bd) noexcept : bbUnoccupied(bd.bbUnoccupied), cpcToMove(bd.cpcToMove), 
			sqEnPassant(bd.sqEnPassant), csCur(bd.csCur), habd(bd.habd),
			gph(bd.gph)
	{
		memcpy(mppcbb, bd.mppcbb, sizeof(mppcbb));
		memcpy(mpcpcbb, bd.mpcpcbb, sizeof(mpcpcbb));
	}

	void SetEmpty(void) noexcept;
	bool operator==(const BD& bd) const noexcept;
	bool operator!=(const BD& bd) const noexcept;

	/* making moves */

	void MakeMvSq(MV& mv);
	void UndoMvSq(MV mv);

	/* move generation */
	
	void GenGemv(GEMV& gemv, GG gg);
	void GenGemv(GEMV& gemv, CPC cpcMove, GG gg);
	void GenGemvColor(GEMV& gemv, CPC cpcMove) const;
	void GenGemvPawnMvs(GEMV& gemv, BB bbPawns, CPC cpcMove) const;
	void GenGemvCastle(GEMV& gemv, SQ sqFrom, CPC cpcMove) const;
	void AddGemvMvPromotions(GEMV& gemv, MV mv) const;
	void GenGemvBbPawnMvs(GEMV& gemv, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const;
	void GenGemvBbMvs(GEMV& gemv, BB bbTo, int dsq, PC pcMove) const;
	void GenGemvBbMvs(GEMV& gemv, SQ sqFrom, BB bbTo, PC pcMove) const;
	void GenGemvBbPromotionMvs(GEMV& gemv, BB bbTo, int dsq) const;
	
	/*
	 *	checking squares for attack 
	 */

	void RemoveInCheckMoves(GEMV& gemv, CPC cpc);
	bool FMvIsQuiescent(MV mv) const noexcept;
	bool FInCheck(CPC cpc) const noexcept;
	APC ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept;
	bool FBbAttackedByQueen(BB bbQueens, BB bb, CPC cpcBy) const noexcept;
	bool FBbAttackedByRook(BB bbRooks, BB bb, CPC cpcBy) const noexcept;
	bool FBbAttackedByBishop(BB bbBishops, BB bb, CPC cpcBy) const noexcept;

	BB BbFwdSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	BB BbRevSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	BB BbPawnAttacked(BB bbPawns, CPC cpcBy) const noexcept;
	BB BbKingAttacked(BB bbKing, CPC cpcBy) const noexcept;
	BB BbKnightAttacked(BB bbKnights, CPC cpcBy) const noexcept;
	
	/*	BD::ApcSqAttacked
	 *
	 *	Returns the lowest piece type of the pieces attacking sqAttacked. The piece
	 *	on sqAttacked is not considered to be attacking sqAttacked. Returns
	 *	APC::Null if no pieces are attacking the square.
	 */
	inline APC ApcSqAttacked(SQ sqAttacked, CPC cpcBy) const noexcept
	{
		return ApcBbAttacked(BB(sqAttacked), cpcBy);
	}

	/*
	 *	move, square, and piece convenience functions. most of these need to be highly
	 *	optimized - beware of bit twiddling tricks!
	 */
	
	inline bool FMvEnPassant(MV mv) const noexcept
	{
		return mv.sqTo() == sqEnPassant && mv.apcMove() == APC::Pawn;
	}

	inline bool FMvIsCapture(MV mv) const noexcept
	{
		return !FIsEmpty(mv.sqTo()) || FMvEnPassant(mv);
	}

	inline void SetEnPassant(SQ sq) noexcept
	{
		if (!sqEnPassant.fIsNil())
			genhabd.ToggleEnPassant(habd, sqEnPassant.file());
		sqEnPassant = sq;
		if (!sqEnPassant.fIsNil())
			genhabd.ToggleEnPassant(habd, sqEnPassant.file());
	}
	
	inline CPC CpcFromSq(SQ sq) const noexcept
	{ 
		return mpcpcbb[CPC::White].fSet(sq) ? CPC::White : CPC::Black;
	}

	inline PC PcFromSq(SQ sq) const noexcept
	{
		CPC cpc = CpcFromSq(sq);
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
			if (mppcbb[PC(cpc, apc)].fSet(sq))
				return PC(cpc, apc);
		return PC(cpc, APC::Null);
	}


	inline APC ApcFromSq(SQ sq) const noexcept
	{
		return PcFromSq(sq).apc();
	}


	inline bool FIsEmpty(SQ sq) const noexcept
	{
		return bbUnoccupied.fSet(sq);
	}


	inline bool FCanCastle(CPC cpc, int csSide) const noexcept
	{
		return (csCur & (csSide << (int)cpc)) != 0;
	}
	
	inline void SetCastle(CPC cpc, int csSide) noexcept
	{ 
		genhabd.ToggleCastle(habd, csCur);
		csCur |= csSide << (int)cpc;
		genhabd.ToggleCastle(habd, csCur);
	}

	inline void SetCastle(int cs) noexcept
	{
		genhabd.ToggleCastle(habd, csCur);
		csCur |= cs;
		genhabd.ToggleCastle(habd, csCur);
	}
	
	inline void ClearCastle(CPC cpc, int csSide) noexcept
	{ 
		genhabd.ToggleCastle(habd, csCur);
		csCur &= ~(csSide << (int)cpc);
		genhabd.ToggleCastle(habd, csCur);
	}

	/*
	 *	bitboard manipulation
	 */

	/*	BD::SetBB
	 *
	 *	Sets the bitboard of the piece ipc in the square sq. Keeps all bitboards
	 *	in sync, including the piece color bitboard and the unoccupied bitboard.
	 * 
	 *	Note that the unoccupied bitboard is not necessarily kept in a consistent
	 *	state by SetBB, if the bitboard in the opposite color happens to be set.
	 *	This normally doesn't happen, because it would technically be in an illegal 
	 *	state to have a square set by both colors. When making moves, clear before 
	 *	you set and you shouldn't get in any trouble.
	 */
	inline void SetBB(PC pc, SQ sq) noexcept
	{
		BB bb(sq);
		mppcbb[pc] += bb;
		mpcpcbb[pc.cpc()] += bb;
		bbUnoccupied -= bb;
		genhabd.TogglePiece(habd, sq, pc);
	}


	/*	BD::ClearBB
	 *
	 *	Clears the piece ipc in square sq from the bitboards. Keeps track of piece
	 *	boards, color boards, and unoccupied boards.
	 * 
	 *	Note that we clear the unoccupied bit here, even if the opposite color might
	 *	theoretically have a piece in that square. It's up to the calling code to
	 *	make sure this doesn't happen.
	 */
	inline void ClearBB(PC pc, SQ sq) noexcept
	{
		BB bb(sq);
		mppcbb[pc] -= bb;
		mpcpcbb[pc.cpc()] -= bb;
		assert(!mpcpcbb[~pc.cpc()].fSet(sq));
		bbUnoccupied += bb;
		genhabd.TogglePiece(habd, sq, pc);
	}

	void ToggleToMove(void) noexcept
	{
		cpcToMove = ~cpcToMove;
		genhabd.ToggleToMove(habd);
	}

	/*
	 *	getting piece value of pieces/squares
	 */

	EV EvFromSq(SQ sq) const noexcept;
	EV EvTotalFromCpc(CPC cpc) const noexcept;

	/*
	 *	reading FEN strings 
	 */

	void InitFENPieces(const wchar_t*& szFEN);
	void AddPieceFEN(SQ sq, PC pc);
	void InitFENSideToMove(const wchar_t*& sz);
	void InitFENCastle(const wchar_t*& sz);
	void InitFENEnPassant(const wchar_t*& sz);
	void SkipToNonSpace(const wchar_t*& sz);
	void SkipToSpace(const wchar_t*& sz);


	/*	BD::GphCur
	 *
	 *	Returns the current game phase (opening, mid, or end game).
	 *	Note that this is a continuum and we use cut-offs to specify when
	 *	the phases transition.
	 */
	GPH GphCur(void) const noexcept
	{
		return gph;
	}

	GPH GphCompute(void) const noexcept;
	void RecomputeGph(void) noexcept;

	inline void AddApcToGph(APC apc) noexcept;
	inline void RemoveApcFromGph(APC apc) noexcept;


	/*
	 *	debug and validation
	 */

#ifndef NDEBUG
	void Validate(void) const;
	void ValidateBB(PC pc, SQ sq) const;
#else
	inline void Validate(void) const { }
	inline void ValidateBB(PC pck, SQ sq) const { }
#endif
};


/*
 *
 *	TKMV enumeration
 * 
 *	Move tokens for parsing move text
 *
 */


enum class TKMV {
	Error,
	End,
	
	King,
	Queen,
	Rook,
	Bishop,
	Knight,
	Pawn,

	Square,
	File,
	Rank,

	Take,
	To,
	
	Promote,
	
	Check,
	Mate,
	EnPassant,
	
	CastleKing,
	CastleQueen,

	WhiteWins,
	BlackWins,
	Draw,
	InProgress
};


string to_string(TKMV tkmv);


/*
 *
 *	BDG class
 * 
 *	The game board, which is the regular board along with the move history and other
 *	game state.
 * 
 */


class BDG : public BD
{
public:
	GS gs;
	vector<MV> vmvGame;	// the game moves that resulted in bd board state
	int64_t imvCur;
	int64_t imvPawnOrTakeLast;	/* index of last pawn or capture move (used directly for 50-move 
							   draw detection), but it is also a bound on how far back we need 
							   to search for 3-move repetition draws */

public:
	BDG(void);
	BDG(const wchar_t* szFEN);
	
	/* 
	 *	making moves 
	 */

	void MakeMv(MV& mv);
	void UndoMv(void);
	void RedoMv(void);
	
	/* 
	 *	game over tests
	 */

	GS GsTestGameOver(int cmvToMove, const RULE& rule) const;
	void SetGameOver(const GEMV& gemv, const RULE& rule);
	bool FDrawDead(void) const;
	bool FDraw3Repeat(int cbdDraw) const;
	bool FDraw50Move(int64_t cmvDraw) const;
	void SetGs(GS gs); 

	/*
	 *	decoding moves 
	 */

	wstring SzMoveAndDecode(MV mv);
	wstring SzDecodeMv(MV mv, bool fPretty);
	bool FMvApcAmbiguous(const GEMV& gemv, MV mv) const;
	bool FMvApcRankAmbiguous(const GEMV& gemv, MV mv) const;
	bool FMvApcFileAmbiguous(const GEMV& gemv, MV mv) const;
	string SzFlattenMvSz(const wstring& wsz) const;
	wstring SzDecodeMvPost(MV mv) const;

	/* 
	 *	parsing moves 
	 */

	ERR ParseMv(const char*& pch, MV& mv);
	ERR ParsePieceMv(const GEMV& gemv, TKMV tkmv, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseSquareMv(const GEMV& gemv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseMvSuffixes(MV& mv, const char*& pch) const;
	ERR ParseFileMv(const GEMV& gemv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseRankMv(const GEMV& gemv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	MV MvMatchPieceTo(const GEMV& gemv, APC apc, int rankFrom, int fileFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
	MV MvMatchFromTo(const GEMV& gemv, SQ sqFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
	TKMV TkmvScan(const char*& pch, SQ& sq) const;

	/*
	 *	importing FEN strings 
	 */

	void InitGame(const wchar_t* szFen);
	void InitFENHalfmoveClock(const wchar_t*& sz);
	void InitFENFullmoveCounter(const wchar_t*& sz);

	/* 
	 *	exporting FEN strings 
	 */

	wstring SzFEN(void) const;
	operator wstring() const { return SzFEN(); }


	/*	BDG::CpcMoveFromImv
	 *
	 *	Returns the color of the side that moved move imv
	 */
	inline CPC CpcMoveFromImv(int imv) const noexcept
	{
		return (CPC)(imv & 1);
	}
};


/*
 *
 *	Little helper class for making/undo-ing a move
 * 
 */

class MAKEMV
{
	BDG& bdg;
public:
	MAKEMV(BDG& bdg, MV& mv) : bdg(bdg)
	{
		bdg.MakeMv(mv);
	}

	~MAKEMV(void)
	{
		bdg.UndoMv();
	}
};


/*
 *
 *	ANO class
 * 
 *	Class that represents an annotation on the board. Our annotations
 *	are either single square markers, which display as a circle on the
 *	board, or arrows between squares.
 *
 */


class ANO
{
	friend class UIBD;
	SQ sqFrom, sqTo;
public:
	ANO(void) { }
	~ANO(void) { }
	ANO(SQ sq) : sqFrom(sq), sqTo(SQ()) { }
	ANO(SQ sqFrom, SQ sqTo) : sqFrom(sqFrom), sqTo(sqTo) { }
};


/*
 *
 *	TK class
 * 
 *	A simple file scanner token class. These are virtual classes, so
 *	they need to be allocated.
 * 
 */


class TK
{
	int tk;
public:
	TK(int tk);
	virtual ~TK(void);
	operator int() const;
	virtual bool FIsString(void) const;
	virtual bool FIsInteger(void) const;
	virtual const string& sz(void) const;
};


class TKSZ : public TK
{
	string szToken;
public:
	TKSZ(int tk, const string& sz);
	TKSZ(int tk, const char* sz);
	virtual ~TKSZ(void);
	virtual bool FIsString(void) const;
	virtual const string& sz(void) const;
};


class TKW : public TK
{
	int wToken;
public:
	TKW(int tk, int w);
	virtual bool FIsInteger(void) const;
	virtual int w(void) const;
};


/*
 *
 *	ISTK class
 * 
 *	A generic token input stream class. We'll build this up to be a
 *	general purpose scanner eventually.
 * 
 */


class ISTK
{
	friend class EXPARSE;

protected:
	int liCur;
	istream& is;
	TK* ptkPrev;
	wstring szStream;

	char ChNext(void);
	void UngetCh(char ch);
	bool FIsWhite(char ch) const;
	bool FIsEnd(char ch) const;
	bool FIsDigit(char ch) const;
	virtual bool FIsSymbol(char ch, bool fFirst) const;

public:
	ISTK(istream& is);
	virtual ~ISTK(void);
	operator bool();
	virtual TK* PtkNext(void) = 0;
	void UngetTk(TK* ptk);
	int line(void) const;
	wstring SzStream(void) const;
	void SetSzStream(const wstring& sz);
};


/*
 *
 *	ISTKPGN class
 * 
 *	Class for scanning/tokenizing chess PGN files
 * 
 */


enum TKPGN {
	tkpgnWhiteSpace,
	tkpgnBlankLine,
	tkpgnEndLine,
	tkpgnString,
	tkpgnComment,
	tkpgnLineComment,
	tkpgnLParen,
	tkpgnRParen,
	tkpgnLBracket,
	tkpgnRBracket,
	tkpgnLAngleBracket,
	tkpgnRAngleBracket,
	tkpgnPeriod,
	tkpgnStar,
	tkpgnPound,
	tkpgnInteger,
	tkpgnNumAnno,
	tkpgnSymbol,
	tkpgnEnd,

	/* tag keywords */

	tkpgnEvent,
	tkpgnSite,
	tkpgnDate,
	tkpgnRound,
	tkpgnWhite,
	tkpgnBlack,
	tkpgnResult,
	tkpgnTagsStart,
	tkpgnTagsEnd
};


class ISTKPGN : public ISTK
{
protected:
	bool fWhiteSpace;
	int imvCur;

	virtual bool FIsSymbol(char ch, bool fFirst) const;
	char ChSkipWhite(void);

public:
	ISTKPGN(istream& is);
	void WhiteSpace(bool fReturn);
	virtual TK* PtkNext(void);
	void ScanToBlankLine(void);
	void SetImvCur(int imv);
};