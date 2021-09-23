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
	fileQueenKnight = 1,	/* b */
	fileQueenBishop = 2,	/* c */
	fileQueen = 3,			/* d */
	fileKing = 4,			/* e */
	fileKingBishop = 5,		/* f */
	fileKingKnight = 6,		/* g */
	fileKingRook = 7,		/* h */
	fileMax = 8
};

const int rankMax = 8;


class SHF
{
	uint8_t grf;
public:
	inline SHF(void) noexcept : grf(0xc0)
	{
	}

	inline SHF(uint8_t grf) noexcept : grf(grf)
	{
	}

	inline SHF(int rank, int file) noexcept : grf((rank << 3) | file)
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
};


/*
 *
 *	SQ type
 *
 *	A square is file and rank encoded into a single byte. This implementation is a 0x88 type
 *	square, which uses the extra bits to let us know if certain squares are "off-board".
 *	The off-board bits get set when offsetting causes overflow of the rank and/or file
 * 
 *	+---+---+---+---+---+---+---+---+
 *  | ov|    rank   | ov|    file   |
 *  +---+-----------+---+-----------+
 * 
 */


class SQ {
private:
	friend class MV;
	uint8_t grf;
public:

	inline SQ(void) noexcept : grf(0xff) 
	{ 
	}

	inline SQ(uint8_t grf) noexcept : grf(grf) 
	{ 
	}
	
	inline SQ(int rank, int file) noexcept
	{ 
		grf = (rank << 4) | file; 
	}
	
	inline int file(void) const noexcept
	{ 
		return grf & 0x07; 
	}
	
	inline int rank(void) const noexcept
	{
		return (grf >> 4) & 0x07; 
	}
	
	inline bool fIsNil(void) const noexcept
	{
		return grf == 0xff; 
	}
	
	inline bool fIsOffBoard(void) const noexcept
	{
		return grf & 0x88; 
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
	
	inline operator uint8_t() const noexcept
	{
		return grf; 
	}

	inline SQ operator++(int) noexcept
	{ 
		BYTE grfT = grf++; 
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
	
	inline SHF shf(void) const noexcept
	{
		return SHF(rank(), file());
	}
	
	inline uint64_t fgrf(void) const noexcept
	{ 
		return 1ULL << shf(); 
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

const SQ sqNil;




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
 * 
 *	Uses SIMD (single instruction, multiple data) techniques.
 * 
 *	This is a divide-and-conquer SWAR (SIMD in a register) approach to the bit-counting 
 *	problem. 
 */
inline int popcount(uint64_t grf) noexcept
{
	/* TODO: use intrinsic */
	const uint64_t k1 = 0x5555555555555555LL;
	const uint64_t k2 = 0x3333333333333333LL;
	const uint64_t k4 = 0x0f0f0f0f0f0f0f0fLL;
	const uint64_t kf = 0x0101010101010101LL;
	uint64_t grfT = grf - ((grf >> 1) & k1);
	grfT = (grfT & k2) + ((grfT >> 2) & k2);
	grfT = (grfT + (grfT >> 4)) & k4;
	return (int)((grfT * kf) >> 56);
}


/*	bitscan
 *
 *	Returns the position of the lowest set bit in the 64-bit word, where 0 is the
 *	least significant bit and 63 is the most.
 * 
 *	Does not work on 0.
 * 
 *	Uses De Bruijn multiplication, as devised by Lauter (1997).
 */

#ifndef NO_INTRINSICS
const int rgbsFwd64[64] = {
	0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6
};

const uint64_t debruijn64 = 0x03f79d71b4cb0a89LL;
#endif

inline int bitscan(uint64_t grf) noexcept
{
	assert(grf);
#ifndef NO_INTRINSICS
	unsigned long shf;
	_BitScanForward64(&shf, grf);
	return shf;
#else
	int ibs = (int)(((grf & -grf) * debruijn64) >> 58);
	return rgbsFwd64[ibs];
#endif
}

#ifndef NO_INTRINSICS
const int rgbsRev64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};
#endif

inline int bitscanRev(uint64_t grf) noexcept
{
	assert(grf);
#ifndef NO_INTRINSICS
	unsigned long shf;
	_BitScanReverse64(&shf, grf);
	return shf;
#else
	grf |= grf >> 1;
	grf |= grf >> 2;
	grf |= grf >> 4;
	grf |= grf >> 8;
	grf |= grf >> 16;
	grf |= grf >> 32;
	return rgbsRev64[(grf * debruijn64) >> 58];
#endif
}


inline SQ SqFromShf(SHF shf)
{
	return SQ((uint8_t)shf >> 3, (uint8_t)shf & 7);
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

	inline BB(SHF shf) noexcept : grf(shf.fgrf())
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

	inline BB operator+(SQ sq) const noexcept
	{
		return BB(grf | sq.fgrf());
	}

	inline BB& operator+=(SQ sq) noexcept
	{
		grf |= sq.fgrf();
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

	inline BB operator|(SQ sq) const noexcept
	{
		return *this + sq;
	}

	inline BB& operator|=(SQ sq) noexcept
	{
		return *this += sq;
	}

	inline BB operator|(BB bb) const noexcept
	{
		return *this + bb;
	}

	inline BB& operator|=(BB bb) noexcept
	{
		return *this += bb;
	}

	inline BB operator-(SQ sq) const noexcept
	{
		return BB(grf & ~sq.fgrf());
	}

	inline BB& operator-=(SQ sq) noexcept
	{
		grf &= ~sq.fgrf();
		return *this;
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

	inline BB operator&(SQ sq) const noexcept
	{
		return BB(grf & sq.fgrf());
	}

	inline BB operator&=(SQ sq) noexcept
	{
		grf &= sq.fgrf();
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

	inline BB operator^(SQ sq) const noexcept
	{
		return BB(grf ^ sq.fgrf());
	}

	inline BB& operator^=(SQ sq) noexcept
	{
		grf ^= sq.fgrf();
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

	inline SHF shfLow(void) const noexcept
	{
		assert(grf);
		return SHF(bitscan(grf));
	}

	inline SHF shfHigh(void) const noexcept
	{
		assert(grf);
		return SHF(bitscanRev(grf));
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
	BB mpshfdirbbSlide[64][8];
	BB mpshfbbKing[64];
	BB mpshfbbKnight[64];
public:
	MPBB(void);
	
	inline BB BbSlideTo(SHF shf, DIR dir) noexcept
	{
		assert(shf < 64);
		assert((int)dir < 8);
		return mpshfdirbbSlide[shf][(unsigned)dir];
	}

	inline BB BbKingTo(SHF shf) noexcept
	{
		return mpshfbbKing[shf];
	}

	inline BB BbKnightTo(uint8_t shf) noexcept
	{
		return mpshfbbKnight[shf];
	}
};