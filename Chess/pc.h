/*
 *
 *	pc.h
 * 
 *	Piece definitions
 * 
 */
#pragma once
#include "bb.h"


/*
 *
 *	APC enumeration
 *
 *	Actual piece types
 *
 */


enum APC : int {
	apcError = -1,
	apcNull = 0,
	apcPawn = 1,
	apcKnight = 2,
	apcBishop = 3,
	apcRook = 4,
	apcQueen = 5,
	apcKing = 6,
	apcMax = 7,
	Bishop2 = 7	// only used for draw detection computation to keep track of bishop square color
};

inline APC& operator++(APC& apc)
{
	apc = static_cast<APC>(apc + 1);
	return apc;
}

inline APC operator++(APC& apc, int)
{
	APC apcT = apc;
	apc = static_cast<APC>(apc + 1);
	return apcT;
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
 *
 */


class PC
{
	uint8_t upc;
public:
	PC(uint8_t upc) noexcept : upc(upc) { }
	PC(CPC cpc, APC apc) noexcept : upc((static_cast<uint8_t>(cpc) << 3) | static_cast<uint8_t>(apc)) { }
	APC apc(void) const noexcept { return static_cast<APC>(upc & 7); }
	CPC cpc(void) const noexcept { return static_cast<CPC>((upc >> 3) & 1); }
	inline operator uint8_t() const noexcept { return upc; }
	inline PC& operator++() { upc++; return *this; }
	inline PC operator++(int) { uint8_t upcT = upc++; return PC(upcT); }
};

const uint8_t pcMax = 2 * 8;
const PC pcWhitePawn(cpcWhite, apcPawn);
const PC pcBlackPawn(cpcBlack, apcPawn);
const PC pcWhiteKnight(cpcWhite, apcKnight);
const PC pcBlackKnight(cpcBlack, apcKnight);
const PC pcWhiteBishop(cpcWhite, apcBishop);
const PC pcBlackBishop(cpcBlack, apcBishop);
const PC pcWhiteRook(cpcWhite, apcRook);
const PC pcBlackRook(cpcBlack, apcRook);
const PC pcWhiteQueen(cpcWhite, apcQueen);
const PC pcBlackQueen(cpcBlack, apcQueen);
const PC pcWhiteKing(cpcWhite, apcKing);
const PC pcBlackKing(cpcBlack, apcKing);
