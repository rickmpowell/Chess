/*
 *
 *	bb.h
 * 
 *	Bitboard and square definitions. A bitboad is a 64-bit set where each bit represents a 
 *	square on the chess board. A full implementation of a chess board can be represented
 *	using multiple bitboards for each piece type/color (e.g., bbBlackRook, bbWhiteKing...)
 * 
 *	Bitboards can also be used to represent squares that a piece can move to, also known as
 *	attack bitboards. Clever bit manipulation can be used to speed up chess move generation.
 * 
 *	We also implement a 0x88 square type here, which is used to index into an 8x16 byte
 *	board array.
 * 
 *	The data types in this file are critical to performance of the program, so almost all 
 *	this code is inline and there are some aggressive optimizations.
 * 
 */

#pragma once
#include "framework.h"


enum {
	fileQueenRook = 0,		/* a */
	fileA = 0,
	fileQueenKnight = 1,	/* b */
	fileB = 1,
	fileQueenBishop = 2,	/* c */
	fileC = 2,
	fileQueen = 3,			/* d */
	fileD = 3,
	fileKing = 4,			/* e */
	fileE = 4,
	fileKingBishop = 5,		/* f */
	fileF = 5,
	fileKingKnight = 6,		/* g */
	fileG = 6,
	fileKingRook = 7,		/* h */
	fileH = 7,
	fileMax = 8
};

enum {
	rankWhiteBack = 0,
	rank1 = 0,
	rankWhitePawn = 1,
	rank2 = 1,
	rank3 = 2,
	rank4 = 3,
	rank5 = 4,
	rank6 = 5,
	rankBlackPawn = 6,
	rank7 = 6,
	rankBlackBack = 7,
	rank8 = 7
};

const int rankMax = 8;


class SQ
{
	uint8_t usq;
public:
	__forceinline SQ(void) noexcept : usq(0xc0) { }
	__forceinline SQ(uint8_t usq) noexcept : usq(usq) { }
	__forceinline SQ(int rank, int file) noexcept : usq((rank << 3) | file) { }
	__forceinline int rank(void) const noexcept { assert(!fIsNil()); return (usq >> 3) & 7; }
	__forceinline int file(void) const noexcept { assert(!fIsNil()); return usq & 7; }
	__forceinline void SetRank(int rank) noexcept { usq = (usq & 7) | (rank << 3); }
	__forceinline void SetFile(int file) noexcept { usq = (usq & 0x38) | file; }
	__forceinline uint64_t fgrf(void) const noexcept { assert(usq < 64); return 1ULL << usq; }
	__forceinline operator uint8_t() const noexcept { return usq; }
	__forceinline bool fIsNil(void) const noexcept { return usq == 0xc0; }

	__forceinline SQ& operator+=(int dsq) noexcept
	{
		usq += dsq;
		return *this;
	}

	__forceinline SQ operator+(int dsq) const noexcept
	{
		return SQ(usq + dsq);
	}

	__forceinline SQ operator++(int) noexcept
	{
		uint8_t usqT = usq++;
		return SQ(usqT);
	}

	__forceinline SQ operator-(int dsq) const noexcept
	{
		return SQ((uint8_t)(usq - dsq));
	}

	__forceinline int operator-(const SQ& sq) const noexcept
	{
		return (int)usq - (int)sq.usq;
	}

	__forceinline SQ sqFlip(void) noexcept
	{
		return SQ(rankMax - 1 - rank(), file());
	}

	/* be careful with validity checks, because enumerating squares will 
	 * often continue on to sqMax, which is 0x40, so invalid sq's are
	 * sometimes legitimate */
	bool fValid(void) const noexcept
	{
		return (usq & 0xc0) == 0 || (usq == 0xc0);
	}

	inline operator string() const noexcept
	{
		if (fIsNil())
			return "--";
		char sz[3] = "a1";
		sz[0] += file();
		sz[1] += rank();
		return string(sz);
	}

};


const uint8_t sqMax = 64;
const SQ sqNil;	

const SQ sqA1(rank1, fileA);
const SQ sqA8(rank8, fileA);

const SQ sqB1(rank1, fileB);
const SQ sqB2(rank2, fileB);
const SQ sqB3(rank3, fileB);
const SQ sqB4(rank4, fileB);
const SQ sqB5(rank5, fileB);
const SQ sqB6(rank6, fileB);
const SQ sqB7(rank7, fileB);
const SQ sqB8(rank8, fileB);

const SQ sqC1(rank1, fileC);
const SQ sqC2(rank2, fileC);
const SQ sqC3(rank3, fileC);
const SQ sqC4(rank4, fileC);
const SQ sqC5(rank5, fileC);
const SQ sqC6(rank6, fileC);
const SQ sqC7(rank7, fileC);
const SQ sqC8(rank8, fileC);

const SQ sqD1(rank1, fileD);
const SQ sqD2(rank2, fileD);
const SQ sqD3(rank3, fileD);
const SQ sqD4(rank4, fileD);
const SQ sqD5(rank5, fileD);
const SQ sqD6(rank6, fileD);
const SQ sqD7(rank7, fileD);
const SQ sqD8(rank8, fileD);

const SQ sqE1(rank1, fileE);
const SQ sqE2(rank2, fileE);
const SQ sqE3(rank3, fileE);
const SQ sqE4(rank4, fileE);
const SQ sqE5(rank5, fileE);
const SQ sqE6(rank6, fileE);
const SQ sqE7(rank7, fileE);
const SQ sqE8(rank8, fileE);

const SQ sqF1(rank1, fileF);
const SQ sqF2(rank2, fileF);
const SQ sqF3(rank3, fileF);
const SQ sqF4(rank4, fileF);
const SQ sqF5(rank5, fileF);
const SQ sqF6(rank6, fileF);
const SQ sqF7(rank7, fileF);
const SQ sqF8(rank8, fileF);

const SQ sqG1(rank1, fileG);
const SQ sqG2(rank2, fileG);
const SQ sqG3(rank3, fileG);
const SQ sqG4(rank4, fileG);
const SQ sqG5(rank5, fileG);
const SQ sqG6(rank6, fileG);
const SQ sqG7(rank7, fileG);
const SQ sqG8(rank8, fileG);

const SQ sqH1(rank1, fileH);
const SQ sqH2(rank2, fileH);
const SQ sqH3(rank3, fileH);
const SQ sqH4(rank4, fileH);
const SQ sqH5(rank5, fileH);
const SQ sqH6(rank6, fileH);
const SQ sqH7(rank7, fileH);
const SQ sqH8(rank8, fileH);


/*
 *
 *	CPC enumeration
 *
 *	Color of a piece. White or black, generally
 *
 */


enum CPC : int {
	cpcNil = -1,
	cpcWhite = 0,
	cpcBlack = 1,
	cpcMax = 2
};

__forceinline CPC operator~(CPC cpc)
{
	return static_cast<CPC>(cpc ^ 1);
}

__forceinline CPC& operator++(CPC& cpc)
{
	cpc = static_cast<CPC>(cpc + 1);
	return cpc;
}

__forceinline CPC operator++(CPC& cpc, int)
{
	CPC cpcT = cpc;
	cpc = static_cast<CPC>(cpc + 1);
	return cpcT;
}

inline wstring to_wstring(CPC cpc)
{
	switch (cpc) {
	case cpcWhite:
		return L"White";
	case cpcBlack:
		return L"Black";
	default:
		return L"(nil)";
	}
}

/*
 *
 *	BB type
 *
 *	Bitboards. We implement a bunch of operators on these things for doing common
 *	bitboard operators.
 * 
 *	~bb		all the squares not in bb
 *	bb1+bb2	the union of the two bb
 *	bb1|bbw	same as +
 *	bb1-bb2	removes the squares in bb2 from bb1
 *	bb1&bb2	the intersection of the two bb
 *	bb1^bb2	the squares that are different between bb1 and bb2
 *	bb<<num	
 *	bb>>num
 *
 *	We also operate on squares
 * 
 *	bb+sq	adds the square to the bitboard
 *	bb|sq	same as +
 *	bb-sq	removes the square from the bitboared
 *	bb&sq	sets only the bitboard square that matches sq
 *	bb^sq	toggles the sq square in the bitboard
 */


class BB
{
public:
	uint64_t grf;

	__forceinline BB(void) noexcept : grf(0) { }
	__forceinline BB(uint64_t grf) noexcept : grf(grf) { }
	__forceinline BB(SQ sq) noexcept : grf(sq.fgrf()) { }
	__forceinline BB& clear(void) noexcept { grf = 0; return *this; }
	
	/* standard  bit opereations */

	__forceinline BB operator|(BB bb) const noexcept { return BB(grf | bb.grf); }
	__forceinline BB& operator|=(BB bb) noexcept { grf |= bb.grf; return *this; }
	__forceinline BB operator&(BB bb) const noexcept { return BB(grf & bb.grf); }
	__forceinline BB& operator&=(BB bb) noexcept { grf &= bb.grf; return *this; }
	__forceinline BB operator^(BB bb) const noexcept { return BB(grf ^ bb.grf); }
	__forceinline BB& operator^=(BB bb) noexcept { grf ^= bb.grf; return *this; }
	__forceinline BB operator~(void) const noexcept { return BB(~grf); }

	/* addition is same as "or", subtraction clears bits */

	__forceinline BB operator+(BB bb) const noexcept { return BB(grf | bb.grf); }
	__forceinline BB& operator+=(BB bb) noexcept { grf |= bb.grf; return *this; }
	__forceinline BB operator-(BB bb) const noexcept { return BB(grf & ~bb.grf); }
	__forceinline BB& operator-=(BB bb) noexcept { grf &= ~bb.grf; return *this; }

	/* standard shifts */

	__forceinline BB operator<<(int dsq) const noexcept { assert(dsq>=0); return BB(grf << dsq); }
	__forceinline BB& operator<<=(int dsq) noexcept { assert(dsq>=0); grf <<= dsq; return *this; }
	__forceinline BB operator>>(int dsq) const noexcept { assert(dsq>=0); return BB(grf >> dsq); }
	__forceinline BB& operator>>=(int dsq) noexcept { assert(dsq>=0); grf >>= dsq; return *this; }

	/* comparisons */

	__forceinline bool operator==(const BB& bb) const noexcept { return grf == bb.grf; }
	__forceinline bool operator!=(const BB& bb) const noexcept { return grf != bb.grf; }
	__forceinline operator bool() const noexcept { return grf != 0; }
	__forceinline bool operator!() const noexcept { return grf == 0; }

	/* information and extraction, square count, lowest and highest bit, removing 
	   lowest square, and testing for a square */

	__forceinline int csq(void) const noexcept { return (int)__popcnt64(grf); }
	__forceinline SQ sqLow(void) const noexcept { assert(grf); DWORD sq; _BitScanForward64(&sq, grf); return (uint8_t)sq; }
	__forceinline SQ sqHigh(void) const noexcept { assert(grf); DWORD sq; _BitScanReverse64(&sq, grf); return (uint8_t)sq; }
	__forceinline void ClearLow(void) noexcept { grf &= grf - 1; }
	__forceinline bool fSet(SQ sq) const noexcept { return (grf & sq.fgrf()) != 0; }
};


/*
 *	some constant bitboards 
 */


const BB            bbFileA(0b0000000100000001000000010000000100000001000000010000000100000001ULL);
const BB            bbFileB(0b0000001000000010000000100000001000000010000000100000001000000010ULL);
const BB            bbFileC(0b0000010000000100000001000000010000000100000001000000010000000100ULL);
const BB            bbFileD(0b0000100000001000000010000000100000001000000010000000100000001000ULL);
const BB            bbFileE(0b0001000000010000000100000001000000010000000100000001000000010000ULL);
const BB            bbFileF(0b0010000000100000001000000010000000100000001000000010000000100000ULL);
const BB            bbFileG(0b0100000001000000010000000100000001000000010000000100000001000000ULL);
const BB            bbFileH(0b1000000010000000100000001000000010000000100000001000000010000000ULL);
const BB			bbFileAB(bbFileA | bbFileB);
const BB			bbFileABC(bbFileA | bbFileB | bbFileC);
const BB			bbFileGH(bbFileG | bbFileH);

const BB            bbRank1(0b0000000000000000000000000000000000000000000000000000000011111111ULL);
const BB            bbRank2(0b0000000000000000000000000000000000000000000000001111111100000000ULL);
const BB            bbRank3(0b0000000000000000000000000000000000000000111111110000000000000000ULL);
const BB            bbRank4(0b0000000000000000000000000000000011111111000000000000000000000000ULL);
const BB            bbRank5(0b0000000000000000000000001111111100000000000000000000000000000000ULL);
const BB            bbRank6(0b0000000000000000111111110000000000000000000000000000000000000000ULL);
const BB            bbRank7(0b0000000011111111000000000000000000000000000000000000000000000000ULL);
const BB            bbRank8(0b1111111100000000000000000000000000000000000000000000000000000000ULL);

const BB           bbCenter(0b0000000000000000000000000001100000011000000000000000000000000000ULL);
const BB         bbCenterEx(0b0000000000000000001111000011110000111100001111000000000000000000ULL);
const BB              bbAll(0b1111111111111111111111111111111111111111111111111111111111111111ULL);
const BB             bbNone(0b0000000000000000000000000000000000000000000000000000000000000000ULL);

const BB  bbWhiteKingCastleCheck(0b0000000000000000000000000000000000000000000000000000000000001110ULL);
const BB  bbWhiteKingCastleEmpty(0b0000000000000000000000000000000000000000000000000000000000000110ULL);
const BB bbWhiteQueenCastleCheck(0b0000000000000000000000000000000000000000000000000000000000111000ULL);
const BB bbWhiteQueenCastleEmpty(0b0000000000000000000000000000000000000000000000000000000001110000ULL);
const BB  bbBlackKingCastleCheck(0b0000111000000000000000000000000000000000000000000000000000000000ULL);
const BB  bbBlackKingCastleEmpty(0b0000011000000000000000000000000000000000000000000000000000000000ULL);
const BB bbBlackQueenCastleCheck(0b0011100000000000000000000000000000000000000000000000000000000000ULL);
const BB bbBlackQueenCastleEmpty(0b0111000000000000000000000000000000000000000000000000000000000000ULL);


/*
 *	Some direction operations on bitboards. Unless otherwise noted, these
 *	all handle board edge cases. Note that they work on more than just single
 *	bit bitboards. They return the bitboard shifted in the named direction.
 */

const int dsqEast = 1;
const int dsqWest = -1;
const int dsqNorth = 8;
const int dsqSouth = -8;
const int dsqNorthWest = 7;
const int dsqNorthEast = 9;
const int dsqSouthWest = -9;
const int dsqSouthEast = -7;
static_assert(dsqNorthWest == dsqNorth + dsqWest);
static_assert(dsqNorthEast == dsqNorth + dsqEast);
static_assert(dsqSouthWest == dsqSouth + dsqWest);
static_assert(dsqSouthEast == dsqSouth + dsqEast);

__forceinline BB BbShift(BB bb, int dsq) noexcept { return dsq > 0 ? (bb << dsq) : (bb >> -dsq); }
__forceinline BB BbEast1(BB bb) noexcept { return BbShift(bb - bbFileH, dsqEast); }
__forceinline BB BbEast2(BB bb) noexcept { return BbShift(bb - bbFileGH, 2*dsqEast); }
__forceinline BB BbWest1(BB bb) noexcept { return BbShift(bb - bbFileA, dsqWest); }
__forceinline BB BbWest2(BB bb) noexcept { return BbShift(bb - bbFileAB, 2*dsqWest); }
__forceinline BB BbWest3(BB bb) noexcept { return BbShift(bb - bbFileABC, 3*dsqWest); }
__forceinline BB BbNorth1(BB bb) noexcept { return BbShift(bb, dsqNorth); }
__forceinline BB BbNorth2(BB bb) noexcept { return BbShift(bb, 2*dsqNorth); }
__forceinline BB BbSouth1(BB bb) noexcept { return BbShift(bb, dsqSouth); }
__forceinline BB BbSouth2(BB bb) noexcept { return BbShift(bb, 2*dsqSouth); }

__forceinline BB BbNorthWest1(BB bb) noexcept { return BbShift(bb - bbFileA, dsqNorthWest); }
__forceinline BB BbNorthEast1(BB bb) noexcept { return BbShift(bb - bbFileH, dsqNorthEast); }
__forceinline BB BbSouthWest1(BB bb) noexcept { return BbShift(bb - bbFileA, dsqSouthWest); }
__forceinline BB BbSouthEast1(BB bb) noexcept { return BbShift(bb - bbFileH, dsqSouthEast); }

__forceinline BB BbWest1(BB bb, int dsq) noexcept { return BbShift(bb - bbFileA, dsq + dsqWest); }
__forceinline BB BbEast1(BB bb, int dsq) noexcept { return BbShift(bb - bbFileH, dsq + dsqEast); }
__forceinline BB BbVertical(BB bb, int dsq) noexcept { return BbShift(bb, dsq); }

__forceinline BB BbRankBack(CPC cpc) noexcept { return bbRank1 << ((7*8) & (-(int)cpc)); }
__forceinline BB BbRankPawnsInit(CPC cpc) noexcept { return bbRank2 << ((5*8) & (-(int)cpc)); }
__forceinline BB BbRankPawnsFirst(CPC cpc) noexcept { return bbRank3 << ((3*8) & (-(int)cpc)); }
__forceinline BB BbRankPrePromote(CPC cpc) noexcept { return bbRank7 >> ((5*8) & (-(int)cpc)); }
__forceinline BB BbRankPromote(CPC cpc) noexcept { return bbRank8 >> ((7*8) & (-(int)cpc)); }


/*
 *
 *	We create attack vectors for every square in every direction (rank, file, 
 *	and diagonal). I could compute these and hard code them into the app, but 
 *	it's very quick to calculate once at boot time.
 * 
 */


enum DIR {
	dirMin = 0,
	dirSouthWest = 0,	/* reverse directions */
	dirSouth = 1,
	dirSouthEast = 2,
	dirWest = 3,
	dirEast = 4,	/* forward directions */
	dirNorthWest = 5,
	dirNorth = 6,
	dirNorthEast = 7,
	dirMax = 8
};

__forceinline DIR& operator++(DIR& dir)
{
	dir = static_cast<DIR>(dir + 1);
	return dir;
}

__forceinline DIR operator++(DIR& dir, int)
{
	DIR dirT = dir;
	dir = static_cast<DIR>(dir + 1);
	return dirT;
}

/*	DirFromDrankDfile
 *	
 *	Computes the DIR directon from the rank and file directions, which should
 *	only be, -1, 0, or +1.
 */
__forceinline DIR DirFromDrankDfile(int drank, int dfile) noexcept
{
	assert(in_range(drank, -1, 1));
	assert(in_range(dfile, -1, 1));
	int dir = ((drank + 1) * 3 + dfile + 1);
	if (dir >= dirEast + 1)
		dir--;
	return (DIR)dir;
}

__forceinline int DrankFromDir(DIR dir) noexcept
{
	if (dir >= dirEast)
		return ((int)dir + 1) / 3 - 1;
	else
		return (int)dir / 3 - 1;
}

__forceinline int DfileFromDir(DIR dir) noexcept
{
	if (dir >= dirEast)
		return ((int)dir + 1) % 3 - 1;
	else
		return (int)dir % 3 - 1;
}


/*
 *
 *	MPBB
 *
 *	Just a little class to compute constant static attack bitboards for each square 
 *	of the board. 
 *
 */


class MPBB
{
	BB mpsqdirbbSlide[64][8];
	BB mpsqbbKing[64];
	BB mpsqbbKnight[64];
	BB mpsqbbPassedPawnAlley[48][2];
public:
	MPBB(void);
	
	/* given a direction and a square, returns squares we attack in that direction */
	__forceinline BB BbSlideTo(SQ sq, DIR dir) const noexcept { assert(sq < 64); return mpsqdirbbSlide[sq][dir];}
	
	/* quick collection of squares a king can move to */
	__forceinline BB BbKingTo(SQ sq) const noexcept { return mpsqbbKing[sq]; }

	/* quick collection of squares a knight can move to */
	__forceinline BB BbKnightTo(uint8_t sq) const noexcept { return mpsqbbKnight[sq]; }

	/* the 3-wide pawn alley in front of a pawn, used to determine if the pawn is a
	   a passed pawn or not */
	__forceinline BB BbPassedPawnAlley(uint8_t sq, CPC cpc) const noexcept { return mpsqbbPassedPawnAlley[sq-8][static_cast<int>(cpc)]; }
};

/* we compute a global of these for movegen and eval to use */

extern MPBB mpbb;