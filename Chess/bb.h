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
	uint8_t grf;
public:
	inline SQ(void) noexcept : grf(0xc0)
	{
	}

	inline SQ(uint8_t grf) noexcept : grf(grf)
	{
	}

	inline SQ(int rank, int file) noexcept : grf((rank << 3) | file)
	{
	}

	inline int rank(void) const noexcept
	{
		assert(!fIsNil());
		return (grf >> 3) & 7;
	}

	inline int file(void) const noexcept
	{
		assert(!fIsNil());
		return grf & 7;
	}

	inline uint64_t fgrf(void) const noexcept
	{
		assert(grf < 64);
		return 1ULL << grf;
	}

	inline operator uint8_t() const noexcept
	{
		return grf;
	}

	inline bool fIsNil(void) const noexcept
	{
		return grf == 0xc0;
	}

	inline SQ& operator+=(int dsq) noexcept
	{
		grf += dsq;
		return *this;
	}

	inline SQ operator+(int dsq) const noexcept
	{
		return SQ(grf + dsq);
	}

	inline SQ operator++(int) noexcept
	{
		uint8_t grfT = grf++;
		return SQ(grfT);
	}

	inline SQ operator-(int dsq) const noexcept
	{
		return SQ((uint8_t)(grf - dsq));
	}

	inline int operator-(const SQ& sq) const noexcept
	{
		return (int)grf - (int)sq.grf;
	}

	inline SQ sqFlip(void) noexcept
	{
		return SQ(rankMax - 1 - rank(), file());
	}

	inline operator string() const noexcept
	{
		if (fIsNil())
			return "<nil>";
		char sz[3];
		sz[0] = 'a' + file();
		sz[1] = '1' + rank();
		sz[2] = 0;
		return string(sz);
	}

};


const uint8_t sqMax = 64;

const SQ sqB1(rank1, fileB);

const SQ sqC1(rank1, fileC);
const SQ sqC2(rank2, fileC);
const SQ sqC3(rank3, fileC);

const SQ sqD1(rank1, fileD);
const SQ sqD2(rank2, fileD);
const SQ sqD3(rank3, fileD);
const SQ sqD4(rank4, fileD);
const SQ sqD5(rank5, fileD);
const SQ sqD6(rank6, fileD);
const SQ sqD7(rank7, fileD);
const SQ sqD8(rank8, fileD);

const SQ sqE5(rank5, fileE);
const SQ sqE6(rank6, fileE);
const SQ sqE7(rank7, fileE);
const SQ sqE8(rank8, fileE);

const SQ sqF1(rank1, fileF);
const SQ sqF2(rank2, fileF);
const SQ sqF3(rank3, fileF);

const SQ sqG1(rank1, fileG);
const SQ sqG2(rank2, fileG);
const SQ sqG3(rank3, fileG);
const SQ sqG4(rank4, fileG);
const SQ sqG5(rank5, fileG);


/*
 *
 *	Some magic functions for manipulatng 64-bit numbers. On some architectures, there are
 *	intrinsic functions that might implement these very efficiently.
 * 
 */


/*	popcount
 *
 *	Returns the "population count" of the 64-bit number. Population count is the number
 *	of 1 bits in the 64-bit word.
 */
inline int popcount(uint64_t grf) noexcept
{
	return (int)__popcnt64(grf);
}


/*	bitscan
 *
 *	Returns the position of the lowest set bit in the 64-bit word, where 0 is the
 *	least significant bit and 63 is the most.
 * 
 *	Does not work on 0.
 */

inline int bitscan(uint64_t grf) noexcept
{
	assert(grf);
	unsigned long shf;
	_BitScanForward64(&shf, grf);
	return shf;
}

inline int bitscanRev(uint64_t grf) noexcept
{
	assert(grf);
	unsigned long shf;
	_BitScanReverse64(&shf, grf);
	return shf;
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

	inline BB(void) noexcept : grf(0)
	{
	}

	inline BB(uint64_t grf) noexcept : grf(grf)
	{
	}

	inline BB(SQ sq) noexcept : grf(sq.fgrf())
	{
	}

	inline BB& clear(void) noexcept
	{
		grf = 0;
		return *this;
	}

	inline BB operator+(BB bb) const noexcept
	{
		return BB(grf | bb.grf);
	}

	inline BB& operator+=(BB bb) noexcept
	{
		grf |= bb.grf;
		return *this;
	}

	inline BB operator|(BB bb) const noexcept
	{
		return *this + bb;
	}

	inline BB& operator|=(BB bb) noexcept
	{
		return *this += bb;
	}

	inline BB operator-(BB bb) const noexcept
	{
		return BB(grf & ~bb.grf);
	}

	inline BB& operator-=(BB bb) noexcept
	{
		grf &= ~bb.grf;
		return *this;
	}

	inline BB operator&(BB bb) const noexcept
	{
		return BB(grf & bb.grf);
	}

	inline BB& operator&=(BB bb) noexcept
	{
		grf &= bb.grf;
		return *this;
	}

	inline BB operator^(BB bb) const noexcept
	{
		return BB(grf ^ bb.grf);
	}

	inline BB& operator^=(BB bb) noexcept
	{
		grf ^= bb.grf;
		return *this;
	}

	inline BB operator~(void) const noexcept
	{
		return BB(~grf);
	}

	inline BB operator<<(int dsq) const noexcept
	{
		return BB(grf << dsq);
	}

	inline BB& operator<<=(int dsq) noexcept
	{
		grf <<= dsq;
		return *this;
	}

	inline BB operator>>(int dsq) const noexcept
	{
		return BB(grf >> dsq);
	}

	inline BB& operator>>=(int dsq) noexcept
	{
		grf >>= dsq;
		return *this;
	}

	inline bool operator==(const BB& bb) const noexcept
	{
		return grf == bb.grf;
	}

	inline bool operator!=(const BB& bb) const noexcept
	{
		return grf != bb.grf;
	}

	operator bool() const noexcept
	{
		return grf != 0;
	}

	bool operator!() const noexcept
	{
		return grf == 0;
	}

	inline int csq(void) const noexcept
	{
		return popcount(grf);
	}

	inline SQ sqLow(void) const noexcept
	{
		assert(grf);
		return SQ(bitscan(grf));
	}

	inline SQ sqHigh(void) const noexcept
	{
		assert(grf);
		return SQ(bitscanRev(grf));
	}

	inline void ClearLow(void) noexcept
	{
		grf &= grf - 1;
	}

	inline bool fSet(SQ sq) const noexcept
	{
		return (grf & sq.fgrf()) != 0;
	}
};


/*
 *
 *	some constant bitboards
 * 
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

inline BB BbEastOne(const BB& bb) noexcept
{
	return (bb - bbFileH) << 1;
}

inline BB BbEastTwo(const BB& bb) noexcept
{
	return (bb - bbFileGH) << 2;
}

inline BB BbWestOne(const BB& bb) noexcept
{
	return (bb - bbFileA) >> 1;
}

inline BB BbWestTwo(const BB& bb) noexcept
{
	return (bb - bbFileAB) >> 2;
}

inline BB BbNorthOne(const BB& bb) noexcept
{
	return bb << 8;
}

inline BB BbNorthTwo(const BB& bb) noexcept
{
	return bb << 16;
}

inline BB BbSouthOne(const BB& bb) noexcept
{
	return bb >> 8;
}

inline BB BbSouthTwo(const BB& bb) noexcept
{
	return bb >> 16;
}

/*
 *
 *	We create attack vectors for every square in every direction (rank, file, and diagonal).
 *	I could compute these and hard code them into the app, but it's very quick to calculate
 *	once at boot time.
 * 
 */

enum class DIR {
	SouthWest = 0,	/* reverse directions */
	South = 1,
	SouthEast = 2,
	West = 3,
	East = 4,	/* forward directions */
	NorthWest = 5,
	North = 6,
	NorthEast = 7,
	Max = 8
};


inline DIR DirFromDrankDfile(int drank, int dfile) noexcept
{
	int dir = ((drank + 1) * 3 + dfile + 1);
	if (dir >= (int)DIR::East + 1)
		dir--;
	return (DIR)dir;
}

inline int DrankFromDir(DIR dir) noexcept
{
	if (dir >= DIR::East)
		return ((int)dir + 1) / 3 - 1;
	else
		return (int)dir / 3 - 1;
}

inline int DfileFromDir(DIR dir) noexcept
{
	if (dir >= DIR::East)
		return ((int)dir + 1) % 3 - 1;
	else
		return (int)dir % 3 - 1;
}


/*
 *
 *	MPBB
 *
 *	Just a little wrapper to compute static attack vectors for each square
 *	of the board. 
 *
 */


class MPBB
{
	BB mpsqdirbbSlide[64][8];
	BB mpsqbbKing[64];
	BB mpsqbbKnight[64];
public:
	MPBB(void);
	
	inline BB BbSlideTo(SQ sq, DIR dir) noexcept
	{
		assert(sq < 64);
		assert((int)dir < 8);
		return mpsqdirbbSlide[sq][(unsigned)dir];
	}

	inline BB BbKingTo(SQ sq) noexcept
	{
		return mpsqbbKing[sq];
	}

	inline BB BbKnightTo(uint8_t sq) noexcept
	{
		return mpsqbbKnight[sq];
	}
};