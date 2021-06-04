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
	BYTE grf;
public:
	
	inline SQ(void) : grf(0xff) 
	{ 
	}
	
	inline SQ(BYTE grf) : grf(grf) 
	{ 
	}
	
	inline SQ(int rank, int file) 
	{ 
		grf = (rank << 4) | file; 
	}
	
	inline SQ(const SQ& sq) 
	{ 
		grf = sq.grf; 
	}
	
	inline int file(void) const 
	{ 
		return grf & 0x07; 
	}
	
	inline int rank(void) const 
	{
		return (grf >> 4) & 0x07; 
	}
	
	inline bool fIsNil(void) const 
	{
		return grf == 0xff; 
	}
	
	inline bool fIsOffBoard(void) const 
	{
		return grf & 0x88; 
	}
	
	inline SQ& operator+=(int dsq) 
	{
		grf += dsq; 
		return *this; 
	}
	
	inline SQ operator+(int dsq) const 
	{ 
		return SQ(grf + dsq); 
	}
	
	inline operator int() const 
	{
		return grf; 
	}

	inline SQ operator++(int) 
	{ 
		BYTE grfT = grf++; 
		return SQ(grfT); 
	}
	
	inline SQ operator-(int dsq) const 
	{
		return SQ((BYTE)(grf - dsq)); 
	}
	
	inline int operator-(const SQ& sq) const 
	{
		return (int)grf - (int)sq.grf; 
	}
	
	inline SQ sqFlip(void) 
	{
		return SQ(rankMax - 1 - rank(), file()); 
	}
	
	inline int shgrf(void) const 
	{
		return (grf & 7) | ((grf >> 1) & 0x38); 
	}
	
	inline __int64 fgrf(void) const 
	{ 
		return 1LL << shgrf(); 
	}
};

const SQ sqNil = SQ();




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
inline int popcount(__int64 grf)
{
	/* TODO: use intrinsic */
	const __int64 k1 = 0x5555555555555555LL;
	const __int64 k2 = 0x3333333333333333LL;
	const __int64 k4 = 0x0f0f0f0f0f0f0f0fLL;
	const __int64 kf = 0x0101010101010101LL;
	__int64 grfT = grf - ((grf >> 1) & k1);
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
const int rgbs64[64] = {
	0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6
};

const __int64 debruijn64 = 0x03f79d71b4cb0a89LL;

inline int bitscan(__int64 grf)
{
	/* TODO: use intrinsic */
	assert(grf);
	int ibs = (int)(((grf & -grf) * debruijn64) >> 58);
	return rgbs64[ibs];
}


/*
 *
 *	BB type
 *
 *	Bitboards.
 *
 */


class BB
{
	__int64 grf;
public:
	inline BB(void) : grf(0)
	{
	}

	inline BB(__int64 grf) : grf(grf)
	{
	}

	inline BB(SQ sq) : grf(sq.fgrf())
	{
	}

	inline BB& clear(void)
	{
		grf = 0;
		return *this;
	}

	inline BB operator+(SQ sq) const
	{
		return BB(grf | sq.fgrf());
	}

	inline BB& operator+=(SQ sq)
	{
		grf |= sq.fgrf();
		return *this;
	}

	inline BB operator+(BB bb) const
	{
		return BB(grf | bb.grf);
	}

	inline BB& operator+=(BB bb)
	{
		grf |= bb.grf;
		return *this;
	}

	inline BB operator-(SQ sq) const
	{
		return BB(grf & ~sq.fgrf());
	}

	inline BB& operator-=(SQ sq)
	{
		grf &= ~sq.fgrf();
		return *this;
	}

	inline BB operator-(BB bb) const
	{
		return BB(grf & ~bb.grf);
	}

	inline BB& operator-=(BB bb)
	{
		grf &= ~bb.grf;
		return *this;
	}

	inline BB operator^(SQ sq) const
	{
		return BB(grf ^ sq.fgrf());
	}

	inline BB& operator^=(SQ sq)
	{
		grf ^= sq.fgrf();
		return *this;
	}

	inline BB operator^(BB bb) const
	{
		return BB(grf ^ bb.grf);
	}

	inline BB& operator^=(BB bb)
	{
		grf ^= bb.grf;
		return *this;
	}

	inline BB operator~(void) const
	{
		return BB(~grf);
	}

	inline BB operator<<(int dsq) const
	{
		return BB(grf << dsq);
	}

	inline BB& operator<<=(int dsq)
	{
		grf <<= dsq;
		return *this;
	}

	inline BB operator>>(int dsq) const
	{
		return BB(grf >> dsq);
	}

	inline BB& operator>>=(int dsq)
	{
		grf >>= dsq;
		return *this;
	}

	operator bool() const
	{
		return grf != 0;
	}

	bool operator!() const
	{
		return grf == 0;
	}

	inline int csq(void) const
	{
		return popcount(grf);
	}

	inline SQ sqLow(void) const
	{
		int bit = bitscan(grf);
		return SQ(bit >> 3, bit & 7);
	}

	inline void ClearLow(void)
	{
		grf &= grf - 1;
	}

	inline bool fSet(SQ sq) const
	{
		return (grf & sq.fgrf()) != 0;
	}
};


/*
 *
 *	some constant bitboards
 * 
 */

const BB            bbFileA(0b0000000100000001000000010000000100000001000000010000000100000001LL);
const BB            bbFileB(0b0000001000000010000000100000001000000010000000100000001000000010LL);
const BB            bbFileC(0b0000010000000100000001000000010000000100000001000000010000000100LL);
const BB            bbFileD(0b0000100000001000000010000000100000001000000010000000100000001000LL);
const BB            bbFileE(0b0001000000010000000100000001000000010000000100000001000000010000LL);
const BB            bbFileF(0b0010000000100000001000000010000000100000001000000010000000100000LL);
const BB            bbFileG(0b0100000001000000010000000100000001000000010000000100000001000000LL);
const BB            bbFileH(0b1000000010000000100000001000000010000000100000001000000010000000LL);

const BB            bbRank1(0b0000000000000000000000000000000000000000000000000000000011111111LL);
const BB            bbRank2(0b0000000000000000000000000000000000000000000000001111111100000000LL);
const BB            bbRank3(0b0000000000000000000000000000000000000000111111110000000000000000LL);
const BB            bbRank4(0b0000000000000000000000000000000011111111000000000000000000000000LL);
const BB            bbRank5(0b0000000000000000000000001111111100000000000000000000000000000000LL);
const BB            bbRank6(0b0000000000000000111111110000000000000000000000000000000000000000LL);
const BB            bbRank7(0b0000000011111111000000000000000000000000000000000000000000000000LL);
const BB            bbRank8(0b1111111100000000000000000000000000000000000000000000000000000000LL);

const BB           bbCenter(0b0000000000000000000000000001100000011000000000000000000000000000LL);
const BB         bbCenterEx(0b0000000000000000001111000011110000111100001111000000000000000000LL);

const BB  bbWhiteKingCastle(0b0000000000000000000000000000000000000000000000000000000000001110LL);
const BB bbWhiteQueenCastle(0b0000000000000000000000000000000000000000000000000000000000111000LL);
const BB  bbBlackKingCastle(0b0000111000000000000000000000000000000000000000000000000000000000LL);
const BB bbBlackQueenCastle(0b0011100000000000000000000000000000000000000000000000000000000000LL);


