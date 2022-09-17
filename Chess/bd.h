#pragma once

/*
 *
 *	bd.h
 *
 *	Board definitions
 *
 */

#include "framework.h"
#include "bb.h"



typedef int32_t EVAL;


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
 *	GMV class
 * 
 *	Generated move list. Has simple array semantics.
 * 
 */


const size_t cmvPreMax = 60;

class GMV
{
private:
	uint32_t amv[cmvPreMax];
	int cmvCur;
	vector<uint32_t>* pvmvOverflow;

public:
	GMV() : cmvCur(0), pvmvOverflow(nullptr) 
	{
		amv[0] = 0;
	}

	~GMV() 
	{
		if (pvmvOverflow)
			delete pvmvOverflow;
	}

	/*	GMV copy constructor
	 */
	GMV(const GMV& gmv) : cmvCur(gmv.cmvCur), pvmvOverflow(nullptr)
	{
		assert(gmv.FValid());
		memcpy(amv, gmv.amv, min(cmvCur, cmvPreMax) * sizeof(uint32_t));
		if (gmv.pvmvOverflow)
			pvmvOverflow = new vector<uint32_t>(*gmv.pvmvOverflow);
	}
	

	/*	GMV move constructor
	 *
	 *	Moves data from one GMV to another. Note that the source GMV is trashed.
	 */
	GMV(GMV&& gmv) noexcept : cmvCur(gmv.cmvCur), pvmvOverflow(gmv.pvmvOverflow)
	{
		assert(gmv.FValid());
		memcpy(amv, gmv.amv, min(cmvCur, cmvPreMax) * sizeof(uint32_t));
		gmv.cmvCur = 0;
		gmv.pvmvOverflow = nullptr;
	}


	GMV& operator=(const GMV& gmv)
	{
		assert(gmv.FValid());
		cmvCur = gmv.cmvCur;
		memcpy(amv, gmv.amv, min(cmvCur, cmvPreMax) * sizeof(uint32_t));
		if (gmv.pvmvOverflow) {
			if (pvmvOverflow)
				*pvmvOverflow = *gmv.pvmvOverflow;
			else
				pvmvOverflow = new vector<uint32_t>(*gmv.pvmvOverflow);
		}
		else {
			if (pvmvOverflow) {
				delete pvmvOverflow;
				pvmvOverflow = nullptr;
			}
		}
		assert(FValid());
		return *this;
	}

	GMV& operator=(GMV&& gmv) noexcept
	{
		if (this == &gmv)
			return *this;
		cmvCur = gmv.cmvCur;
		memcpy(amv, gmv.amv, min(cmvCur, cmvPreMax) * sizeof(uint32_t));
		if (pvmvOverflow != nullptr) {
			delete pvmvOverflow;
			pvmvOverflow = nullptr;
		}
		pvmvOverflow = gmv.pvmvOverflow;
		gmv.pvmvOverflow = nullptr;
		return *this;
	}


#ifndef NDEBUG
	bool FValid(void) const
	{
		return (cmvCur <= cmvPreMax && pvmvOverflow == nullptr) ||
			(cmvCur > cmvPreMax && pvmvOverflow != nullptr && pvmvOverflow->size() + cmvPreMax == cmvCur);
	}
#endif

	
	inline int cmv(void) const noexcept
	{
		assert(FValid());
		return cmvCur;
	}


	inline MV& operator[](int imv) noexcept
	{
		assert(FValid());
		assert(imv >= 0 && imv < cmvCur);
		if (imv < cmvPreMax)
			return *(MV*)&amv[imv];
		else {
			assert(pvmvOverflow != nullptr);
			return *(MV*)&(*pvmvOverflow)[imv - cmvPreMax];
		}
	}

	inline MV operator[](int imv) const noexcept
	{
		assert(FValid());
		assert(imv >= 0 && imv < cmvCur);
		if (imv < cmvPreMax)
			return *(MV*)&amv[imv];
		else {
			assert(pvmvOverflow != nullptr);
			return *(MV*)&(*pvmvOverflow)[imv - cmvPreMax];
		}
	}

	void AppendMvOverflow(MV mv)
	{
		if (pvmvOverflow == nullptr) {
			assert(cmvCur == cmvPreMax);
			pvmvOverflow = new vector<uint32_t>;
		}
		pvmvOverflow->push_back(mv);
		cmvCur++;
	}

	inline void AppendMv(MV mv)
	{
		assert(FValid()); 
		if (cmvCur < cmvPreMax)
			amv[cmvCur++] = mv;
		else
			AppendMvOverflow(mv);
		assert(FValid());
	}

	inline void AppendMv(SQ sqFrom, SQ sqTo, PC pcMove)
	{
		assert(FValid());
		if (cmvCur < cmvPreMax) {
			amv[cmvCur] = MV(sqFrom, sqTo, pcMove);
			cmvCur++;
		}
		else
			AppendMvOverflow(MV(sqFrom, sqTo, pcMove));
	}

	void Resize(int cmvNew)
	{
		assert(FValid());
		if (cmvNew > cmvPreMax) {
			if (pvmvOverflow == nullptr)
				pvmvOverflow = new vector<uint32_t>;
			pvmvOverflow->resize(cmvNew - cmvPreMax);
		}
		else {
			if (pvmvOverflow != nullptr) {
				delete pvmvOverflow;
				pvmvOverflow = nullptr;
			}
		}
		cmvCur = cmvNew;
		assert(FValid());
	}

	void Reserve(int cmv)
	{
		if (cmv <= cmvPreMax)
			return;
		if (pvmvOverflow == nullptr)
			pvmvOverflow = new vector<uint32_t>;
		pvmvOverflow->reserve(cmv - cmvPreMax);
	}

	void Clear(void) noexcept
	{
		if (pvmvOverflow) {
			delete pvmvOverflow;
			pvmvOverflow = nullptr;
		}
		cmvCur = 0;
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

class BD
{
public:
	BB mppcbb[pcMax];	// bitboards for the pieces
	BB mpcpcbb[CPC::ColorMax]; // squares occupied by pieces of the color 
	BB bbUnoccupied;	// empty squares
	CPC cpcToMove;	/* side with the move */
	SQ sqEnPassant;	/* non-nil when previous move was a two-square pawn move, destination
					   of en passant capture */
	uint8_t csCur;	/* castle sides */
	HABD habd;	/* board hash */

public:
	BD(void);

	BD(const BD& bd) noexcept : bbUnoccupied(bd.bbUnoccupied), cpcToMove(bd.cpcToMove), sqEnPassant(bd.sqEnPassant), csCur(bd.csCur), habd(bd.habd)
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
	
	void GenGmv(GMV& gmv, GG gg) const;
	void GenGmv(GMV& gmv, CPC cpcMove, GG gg) const;
	void GenGmvColor(GMV& gmv, CPC cpcMove) const;
	void GenGmvPawnMvs(GMV& gmv, BB bbPawns, CPC cpcMove) const;
	void GenGmvCastle(GMV& gmv, SQ sqFrom, CPC cpcMove) const;
	void AddGmvMvPromotions(GMV& gmv, MV mv) const;
	void GenGmvBbPawnMvs(GMV& gmv, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const;
	void GenGmvBbMvs(GMV& gmv, BB bbTo, int dsq, PC pcMove) const;
	void GenGmvBbMvs(GMV& gmv, SQ sqFrom, BB bbTo, PC pcMove) const;
	void GenGmvBbPromotionMvs(GMV& gmv, BB bbTo, int dsq) const;
	
	/*
	 *	checking squares for attack 
	 */

	void RemoveInCheckMoves(GMV& gmv, CPC cpc) const;
	bool FMvIsQuiescent(MV mv) const noexcept;
	bool FInCheck(CPC cpc) const noexcept;
	APC ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept;
	bool FBbAttackedByQueen(BB bb, CPC cpcBy) const noexcept;
	bool FBbAttackedByRook(BB bb, CPC cpcBy) const noexcept;
	bool FBbAttackedByBishop(BB bb, CPC cpcBy) const noexcept;


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

	BB BbFwdSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	BB BbRevSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	BB BbPawnAttacked(CPC cpcBy) const noexcept;
	BB BbKingAttacked(CPC cpcBy) const noexcept;
	BB BbKnightAttacked(CPC cpcBy) const noexcept;

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

	EVAL VpcFromSq(SQ sq) const noexcept;
	EVAL VpcTotalFromCpc(CPC cpc) const noexcept;

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

	GS GsTestGameOver(const GMV& gmv, const RULE& rule) const;
	void SetGameOver(const GMV& gmv, const RULE& rule);
	bool FDrawDead(void) const;
	bool FDraw3Repeat(int cbdDraw) const;
	bool FDraw50Move(int64_t cmvDraw) const;
	void SetGs(GS gs); 

	/*
	 *	decoding moves 
	 */

	wstring SzMoveAndDecode(MV mv);
	wstring SzDecodeMv(MV mv, bool fPretty) const;
	bool FMvApcAmbiguous(const GMV& gmv, MV mv) const;
	bool FMvApcRankAmbiguous(const GMV& gmv, MV mv) const;
	bool FMvApcFileAmbiguous(const GMV& gmv, MV mv) const;
	string SzFlattenMvSz(const wstring& wsz) const;
	wstring SzDecodeMvPost(MV mv) const;

	/* 
	 *	parsing moves 
	 */

	ERR ParseMv(const char*& pch, MV& mv) const;
	ERR ParsePieceMv(const GMV& gmv, TKMV tkmv, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseSquareMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseMvSuffixes(MV& mv, const char*& pch) const;
	ERR ParseFileMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	ERR ParseRankMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const;
	MV MvMatchPieceTo(const GMV& gmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
	MV MvMatchFromTo(const GMV& gmv, SQ sqFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
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