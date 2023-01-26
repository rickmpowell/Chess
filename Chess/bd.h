/*
 *
 *	bd.h
 *
 *	Board definitions. This includes types for pieces, moves, game rules, and
 *	move generation. 
 *
 */

#pragma once
#include "framework.h"
#include "bb.h"
#include "rule.h"
#include "pc.h"
#include "mv.h"


/*
 *
 *	GS emumeration
 * 
 *	Game state. Either we're playing, or the game is over by several possible
 *	situations.
 *
 */


enum GS {
	
	gsPlaying = 0,
	
	gsWhiteCheckMated,
	gsBlackCheckMated,
	gsWhiteTimedOut,
	gsBlackTimedOut,
	gsWhiteResigned,
	gsBlackResigned,
	
	gsStaleMate,
	gsDrawDead,
	gsDrawAgree,
	gsDraw3Repeat,
	gsDraw50Move,
	
	gsCanceled,
	gsNotStarted,
	gsPaused
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
 *	GG enumeration
 * 
 *	Move generation options. Pseudo moves does not remove moves that leave
 *	the king in check. Detecting checks is actually quite time consuming, so 
 *	we have the option to delay the check detection as long as possible.
 * 
 */


enum GG : int {	
	ggLegal,
	ggPseudo
};


/*
 *
 *	GPH enumeration
 * 
 *	Game phase is a rudimentary approximation of how far along we are in the
 *	game progression. It's basically a measure of the non-pawn pieces on the
 *	board. When most pieces are still on the board, we're in the opening, while
 *	if we're down to a few minor pieces, we're in the end game.
 *
 */


enum GPH : int {	// game phase
	gphQueen = 4,
	gphRook = 2,
	gphMinor = 1,
	gphNone = 0,

	gphMax = 24,		// when all pieces are on the board
	gphMidMin = 2,		// opening is over when two minor pieces are gone
	gphMidMid = 4,	// a mid-point of the middle game for when strategies should start transitioning
	gphMidMax = 18,		// end game is when we're down to rook/minor piece vs. rook/minor piece

	gphNil = -1
};

static_assert(gphMax == 2 * gphQueen + 4 * gphRook + 8 * gphMinor);

inline bool FInOpening(GPH gph)
{
	return gph <= gphMidMin;
}

inline bool FInEndGame(GPH gph)
{
	return gph >= gphMidMax;
}


/*
 *
 *	BD class
 * 
 *	The board. Base board class does not include move history and therefore can not
 *	detect draw conditions. But it does track en passant and castle legality, along
 *	with the side that has the move.
 * 
 */


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

	BB mpcpcbb[cpcMax]; // squares occupied by pieces of the color 
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

	void MakeMvuSq(MVU& mvu) noexcept;
	void UndoMvuSq(MVU mvu) noexcept;
	void MakeMvuNullSq(MVU& mvu) noexcept;
	void UndoMvuNullSq(MVU mvu) noexcept;

	/* move generation */

	void GenVmve(VMVE& vmve, GG gg) noexcept;
	void GenVmve(VMVE& vmve, CPC cpcMove, GG gg) noexcept;
	void GenVmveColor(VMVE& vmve, CPC cpcMove) const noexcept;
	inline void GenVmvePawnMoves(VMVE& vmve, BB bbPawns, CPC cpcMove) const noexcept;
	inline void GenVmveCastle(VMVE& vmve, SQ sqFrom, CPC cpcMove) const noexcept;
	inline void AddVmveMvuPromotions(VMVE& vmve, MVU mvu) const noexcept;
	inline void GenVmveBbPawnMoves(VMVE& vmve, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const noexcept;
	inline void GenVmveBbMoves(VMVE& vmve, BB bbTo, int dsq, PC pcMove) const noexcept;
	inline void GenVmveBbMoves(VMVE& vmve, SQ sqFrom, BB bbTo, PC pcMove) const noexcept;

	/*
	 *	checking squares for attack
	 */

	void RemoveInCheckMoves(VMVE& vmve, CPC cpc) noexcept;
	bool FMvuIsQuiescent(MVU mvu) const noexcept;
	bool FInCheck(CPC cpc) const noexcept;
	APC ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept;

	inline BB BbFwdSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	inline BB BbRevSlideAttacks(DIR dir, SQ sqFrom) const noexcept;
	inline BB BbPawnAttacked(BB bbPawns, CPC cpcBy) const noexcept;
	inline BB BbKingAttacked(BB bbKing) const noexcept;
	inline BB BbKnightAttacked(BB bbKnights) const noexcept;
	inline BB BbBishopAttacked(BB bbBishops) const noexcept;
	inline BB BbBishop1Attacked(BB bbBishops) const noexcept;
	inline BB BbRookAttacked(BB bbRooks) const noexcept;
	inline BB BbRook1Attacked(BB bbRooks) const noexcept;
	inline BB BbQueenAttacked(BB bbQueens) const noexcept;
	inline BB BbQueen1Attacked(BB bbQueen) const noexcept;

	/*	BD::ApcSqAttacked
	 *
	 *	Returns the lowest piece type of the pieces attacking sqAttacked. The piece
	 *	on sqAttacked is not considered to be attacking sqAttacked. Returns
	 *	apcNull if no pieces are attacking the square.
	 */
	inline APC ApcSqAttacked(SQ sqAttacked, CPC cpcBy) const noexcept
	{
		return ApcBbAttacked(BB(sqAttacked), cpcBy);
	}

	/*
	 *	move, square, and piece convenience functions. most of these need to be highly
	 *	optimized - beware of bit twiddling tricks!
	 */

	inline bool FMvuEnPassant(MVU mvu) const noexcept
	{
		return mvu.sqTo() == sqEnPassant && mvu.apcMove() == apcPawn;
	}

	inline bool FMvuIsCapture(MVU mvu) const noexcept
	{
		return !FIsEmpty(mvu.sqTo()) || FMvuEnPassant(mvu);
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
		return mpcpcbb[cpcWhite].fSet(sq) ? cpcWhite : cpcBlack;
	}

	inline PC PcFromSq(SQ sq) const noexcept
	{
		CPC cpc = CpcFromSq(sq);
		for (APC apc = apcPawn; apc < apcMax; ++apc)
			if (mppcbb[PC(cpc, apc)].fSet(sq))
				return PC(cpc, apc);
		return PC(cpc, apcNull);
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

	inline void AssignCastle(int cs) noexcept
	{
		genhabd.ToggleCastle(habd, csCur);
		csCur = cs;
		genhabd.ToggleCastle(habd, csCur);
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

	inline void ClearCastle(int cs) noexcept
	{
		genhabd.ToggleCastle(habd, csCur);
		csCur &= ~cs;
		genhabd.ToggleCastle(habd, csCur);
	}

	void ClearSq(SQ sq) noexcept;
	void SetSq(SQ sq, PC pc) noexcept;
	void GuessEpAndCastle(SQ sq) noexcept;
	void GuessCastle(CPC cpc, int rank) noexcept;


	/*
	 *	bitboard manipulation
	 */


	/*	BD::SetBB
	 *
	 *	Sets the bitboard of the piece ipc in the square sq. Keeps all bitboards
	 *	in sync, including the piece color bitboard and the unoccupied bitboard,
	 *	hash.
	 */
	inline void SetBB(PC pc, SQ sq) noexcept
	{
		BB bb(sq);
		mppcbb[pc] += bb;
		mpcpcbb[(int)pc.cpc()] += bb;
		assert(!mpcpcbb[(int)~pc.cpc()].fSet(sq));
		bbUnoccupied -= bb;
		genhabd.TogglePiece(habd, sq, pc);
	}


	/*	BD::ClearBB
	 *
	 *	Clears the piece pc in square sq from the bitboards. Keeps track of piece
	 *	boards, color boards, and unoccupied boards, and hash.
	 */
	inline void ClearBB(PC pc, SQ sq) noexcept
	{
		BB bb(sq);
		mppcbb[pc] -= bb;
		mpcpcbb[(int)pc.cpc()] -= bb;
		assert(!mpcpcbb[(int)~pc.cpc()].fSet(sq));
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

	/*
	 *	Game phase
	 */

	GPH GphCur(void) const noexcept { return gph; }
	GPH GphCompute(void) const noexcept;
	void RecomputeGph(void) noexcept;
	inline bool FInOpening(void) const noexcept { return ::FInOpening(gph); }
	inline bool FInEndGame(void) const noexcept { return ::FInEndGame(gph); }
	inline void AddApcToGph(APC apc) noexcept;
	inline void RemoveApcFromGph(APC apc) noexcept;

	/*
	 *	debug and validation
	 */

#ifndef NDEBUG
	void Validate(void) const noexcept;
	void ValidateBB(PC pc, SQ sq) const noexcept;
#else
	inline void Validate(void) const noexcept { }
	inline void ValidateBB(PC pc, SQ sq) const noexcept { }
#endif
};


/*
 *
 *	TKMV enumeration
 * 
 *	Move tokens for parsing move text
 *
 */


enum TKMV {
	tkmvError,
	tkmvEnd,
	
	tkmvKing,
	tkmvQueen,
	tkmvRook,
	tkmvBishop,
	tkmvKnight,
	tkmvPawn,

	tkmvSquare,
	tkmvFile,
	tkmvRank,

	tkmvTake,
	tkmvTo,
	
	tkmvPromote,
	
	tkmvCheck,
	tkmvMate,
	tkmvEnPassant,
	
	tkmvCastleKing,
	tkmvCastleQueen,

	tkmvWhiteWins,
	tkmvBlackWins,
	tkmvDraw,
	tkmvInProgress
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
	vector<MVU> vmvuGame;		/* the game moves that resulted in bd board state */
	int imvuCurLast;		/* position of current last made move, -1 before first move; may be 
							   less than vmvGame.size after Undo/Redo */
	int imvuPawnOrTakeLast;	/* index of last pawn or capture move (used for 50-move draw
							       detection and 3-move repetition draws) */

public:
	BDG(void) noexcept;
	BDG(const wchar_t* szFEN);
	BDG(const BDG& bdg) noexcept;

	/*
	 *	Game control
	 */

	static const wchar_t szFENInit[];
	void InitGame(const wchar_t* szFEN);
	void InitGame(void);

	/*
	 *	making moves 
	 */

	void MakeMvu(MVU& mvu) noexcept;
	void UndoMvu(void) noexcept;
	void RedoMvu(void) noexcept;
	void MakeMvuNull(void) noexcept;
	
	/* 
	 *	game over tests
	 */

	GS GsTestGameOver(int cmvToMove, int cmvRepeatDraw) const noexcept;
	void SetGameOver(const VMVE& vmve, const RULE& rule) noexcept;
	bool FDrawDead(void) const noexcept;
	bool FDraw3Repeat(int cbdDraw) const noexcept;
	bool FDraw50Move(int64_t cmvDraw) const noexcept;
	void SetGs(GS gs) noexcept; 

	/*
	 *	decoding moves 
	 */

	wstring SzMoveAndDecode(MVU mvu);
	wstring SzDecodeMvu(MVU mvu, bool fPretty);
	bool FMvuApcAmbiguous(const VMVE& vmve, MVU mvu) const;
	bool FMvuApcRankAmbiguous(const VMVE& vmve, MVU mvu) const;
	bool FMvuApcFileAmbiguous(const VMVE& vmve, MVU mvu) const;
	string SzFlattenMvuSz(const wstring& wsz) const;
	wstring SzDecodeMvuPost(MVU mvu) const;

	/* 
	 *	parsing moves 
	 */

	ERR ParseMvu(const char*& pch, MVU& mvu);
	ERR ParsePieceMvu(const VMVE& vmve, TKMV tkmv, const char* pchInit, const char*& pch, MVU& mvu) const;
	ERR ParseSquareMvu(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVU& mvu) const;
	ERR ParseMvuSuffixes(MVU& mvu, const char*& pch) const;
	ERR ParseFileMvu(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVU& mvu) const;
	ERR ParseRankMvu(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVU& mvu) const;
	MVU MvuMatchPieceTo(const VMVE& vmve, APC apc, int rankFrom, int fileFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
	MVU MvuMatchFromTo(const VMVE& vmve, SQ sqFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const;
	TKMV TkmvScan(const char*& pch, SQ& sq) const;

	/*
	 *	importing FEN strings 
	 */

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
	ANO(SQ sq) : sqFrom(sq), sqTo(sqNil) { }
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
 *	general purpose scanner some day.
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