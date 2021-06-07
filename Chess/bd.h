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


enum TPC {
	tpcPieceMin = 0,
	tpcQueenRook = 0,
	tpcQueenKnight = 1,
	tpcQueenBishop = 2,
	tpcQueen = 3,
	tpcKing = 4,
	tpcKingBishop = 5,
	tpcKingKnight = 6,
	tpcKingRook = 7,
	tpcPawnFirst = 8,
	tpcPawnQR = 8,
	tpcPawnQN = 9,
	tpcPawnQB = 10,
	tpcPawnQ = 11,
	tpcPawnK = 12,
	tpcPawnKB = 13,
	tpcPawnKN = 14,
	tpcPawnKR = 15,
	tpcPawnLim = 16,
	tpcPieceMax = 16,
};


inline TPC operator++(TPC& tpc)
{
	tpc = (TPC)(((int)tpc) + 1);
	return tpc;
}

inline TPC TpcOpposite(TPC tpc)
{
	return (TPC)((int)tpc ^ 0x07);
}

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

class IPC
{
	uint8_t tpcGrf : 4,
		apcGrf : 3,
		cpcGrf : 1;

public:
	inline IPC(void) : tpcGrf(tpcPieceMin), cpcGrf(CPC::Black), apcGrf(APC::ActMax)
	{
	}

	inline IPC(TPC tpc, CPC cpc, APC apc) : tpcGrf(tpc), cpcGrf(cpc), apcGrf(apc)
	{
	}

	inline CPC cpc(void) const
	{
		return (CPC)cpcGrf;
	}

	inline TPC tpc(void) const
	{
		return (TPC)tpcGrf;
	}

	inline APC apc(void) const
	{
		return (APC)apcGrf;
	}

	inline IPC& SetApc(APC apc)
	{
		this->apcGrf = apc;
		return *this;
	}

	inline bool fIsNil(void) const
	{
		return apcGrf == APC::ActMax;
	}

	inline bool fIsEmpty(void) const
	{
		return apcGrf == APC::Null;
	}

	inline bool operator!=(IPC ipc) const
	{
		return apcGrf != ipc.apcGrf || tpcGrf != ipc.tpcGrf || cpcGrf != ipc.cpcGrf;
	}

	inline bool operator==(IPC ipc) const
	{
		return apcGrf == ipc.apcGrf && tpcGrf == ipc.tpcGrf && cpcGrf == ipc.cpcGrf;
	}

	inline operator uint8_t() const
	{
		return *(uint8_t*)this;
	}
};

static const IPC ipcNil((TPC)0, CPC::Black, APC::ActMax);
static const IPC ipcEmpty((TPC)0, CPC::Black, APC::Null);


inline IPC IpcSetApc(IPC ipc, APC apc) 
{
	assert(apc >= APC::Pawn && apc <= APC::King);
	return ipc.SetApc(apc); 
}


/*	
 * 
 *	MV type
 *
 *	A move is a from and to square, with a little extra info for
 *	weird moves, along with enough information to undo the move.
 * 
 *	Low 8 bits is the source square, the next 8 bits are the 
 *	destination square. The rest of the bottom word is currently
 *	unused, reserved for a change in board representation.
 *	
 *	The high word is mostly used for undo information. On captures
 *	the tpc of the captured piece is in the bottom 4 bits, and
 *	the next 3 bits are the 1pc of the captured piece. If the capture
 *	apc is apcNull, then the move was not a capture. The next bit
 *	is 1 if en passant capture ws possible in previous position. 
 *	The next three bits are the file of the en passant piece if en
 *	passant is true. The next two bits are the previous castle bits.
 *	The top 3 bits are the apc of the new piece on pawn promotions. 
 * 
 *	There are opportunities to compress the upper word of the move.
 * 
 *   15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *  +-----------+-----------+-----------+-----------+
 *  |           to          |         from          |
 *  +-----------+-----------+-----------+-----------+
 *  +--------+-----+--------+--+--------+-----------+
 *  | promote|  cs | ep file|ep| cap apc|  cap tpc  |
 *  +--------+-----+--------+--+--------+-----------+
 *   31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 */

class MV {
private:
	uint32_t sqFromGrf : 8,
		sqToGrf : 8,
		tpcCaptGrf : 4,
		apcCaptGrf : 3,
		fEnPassantGrf : 1,
		fileEnPassantGrf : 3,
		csGrf : 2,
		apcPromoteGrf : 3;

public:
	inline MV(void) 
	{
		*(uint32_t*)this = 0xffffffff;
	}

	inline MV(const MV& mv)
	{
		*(uint32_t*)this = *(uint32_t*)&mv;
	}

	inline MV(SQ sqFrom, SQ sqTo, APC apcPromote = APC::Null) {
		sqFromGrf = sqFrom;
		sqToGrf = sqTo;
		tpcCaptGrf = 0;
		apcCaptGrf = APC::Null;
		fEnPassantGrf = false;
		csGrf = 0;
		apcPromoteGrf = apcPromote;
	}
	
	inline SQ sqFrom(void) const 
	{
		return (SQ)sqFromGrf; 
	}

	inline SQ sqTo(void) const 
	{
		return (SQ)sqToGrf;
	}
	
	inline APC apcPromote(void) const 
	{
		return (APC)apcPromoteGrf;
	}

	inline bool fIsNil(void) const 
	{
		return sqFromGrf == 15 && sqToGrf == 15 && apcPromoteGrf == APC::ActMax;
	}

	inline MV& SetApcPromote(APC apc)
	{
		apcPromoteGrf = apc;
		return *this;
	}

	inline MV& SetCapture(APC apc, TPC tpc) 
	{
		apcCaptGrf = apc;
		tpcCaptGrf = tpc;
		return *this;  
	}

	inline MV& SetCsEp(int cs, const SQ& sqEnPassant)
	{
		csGrf = cs;
		fileEnPassantGrf = sqEnPassant.file();
		fEnPassantGrf = !sqEnPassant.fIsNil();
		return *this;
	}

	inline int csPrev(void) const 
	{
		return csGrf;
	}

	inline int fileEpPrev(void) const 
	{
		return fileEnPassantGrf;
	}

	inline bool fEpPrev(void) const 
	{
		return fEnPassantGrf;
	}

	inline APC apcCapture(void) const 
	{
		return (APC)apcCaptGrf;
	}

	inline bool fIsCapture(void) const 
	{
		return apcCapture() != APC::Null; 
	}

	inline TPC tpcCapture(void) const 
	{ 
		return (TPC)tpcCaptGrf; 
	}

	inline bool operator==(const MV& mv) const 
	{
		return sqFromGrf == mv.sqFromGrf &&
			   sqToGrf == mv.sqToGrf &&
			   tpcCaptGrf == mv.tpcCaptGrf &&
			   apcCaptGrf == mv.apcCaptGrf &&
			   fEnPassantGrf == mv.fEnPassantGrf &&
			   fileEnPassantGrf == mv.fileEnPassantGrf &&
			   csGrf == mv.csGrf &&
			   apcPromoteGrf == mv.apcPromoteGrf;
	}

	inline bool operator!=(const MV& mv) const 
	{
		return !(*this == mv);
	}

	inline operator uint32_t() const
	{
		return *(uint32_t*)this;
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

enum {
	csKing = 0x01, csQueen = 0x04,
	csWhiteKing = 0x01,
	csBlackKing = 0x02,
	csWhiteQueen = 0x04,
	csBlackQueen = 0x08
};


/*	CsPackColor
 *
 *	Packs castle state (king- and queen- side) for the given color into 2 bits for 
 *	saving in the MV undo 
 */
inline int CsPackColor(int csUnpack, CPC cpc)
{
	int csPack = csUnpack >> (BYTE)cpc;
	return ((csPack>>1)&2) | (csPack&1);
}


/*	CsUnpackColor
 *
 *	Unpack castle state packed by CsPackColor. When undoing, this can be or-ed into 
 *	board's regular castle state, because undoing can never remove castle rights 
 */
inline int CsUnpackColor(int csPack, CPC cpc)
{
	int csUnpack = (csPack&1) | ((csPack&2)<<1);
	return csUnpack << (BYTE)cpc;
}


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


/*	RankInitPawnFromCpc
 *
 *	Initial rank of pawns for the given color. Either 1 or 6.
 */
inline int RankInitPawnFromCpc(CPC cpc)
{
	return RankBackFromCpc(cpc) ^ 1;
}


/*	DsqPawnFromCpc
 *
 *	Number of squares a pawn of the given color moves
 */
inline int DsqPawnFromCpc(CPC cpc)
{
	/* white -> 16, black -> -16 */
	return 16 - ((int)cpc << 5);
}


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

enum class RMCHK {	// GenRgmv Option to optionally remove checks
	Remove,
	NoRemove
};

class BD
{
	static const float mpapcvpc[];
public:
	uint8_t mpsqipc[64 * 2];	// the board itself (maps square to piece)
	BB mpapcbb[CPC::ColorMax][APC::ActMax];	// bitboards
	uint8_t mptpcsq[CPC::ColorMax][tpcPieceMax]; // reverse mapping of mpsqtpc
	SQ sqEnPassant;	/* non-nil when previous move was a two-square pawn move, destination
					   of en passant capture */
	BYTE cs;	/* castle sides */

public:
	BD(void);

	BD(const BD& bd) : sqEnPassant(bd.sqEnPassant), cs(bd.cs)
	{
		memcpy(mpsqipc, bd.mpsqipc, sizeof(mpsqipc));
		memcpy(mpapcbb, bd.mpapcbb, sizeof(mpapcbb));
		memcpy(mptpcsq, bd.mptpcsq, sizeof(mptpcsq));
	}

	void SetEmpty(void);

	void InitFENPieces(const WCHAR*& szFEN);
	void AddPieceFEN(SQ sq, TPC tpc, CPC cpc, APC apc);
	void SkipToNonSpace(const WCHAR*& sz);
	void SkipToSpace(const WCHAR*& sz);
	TPC TpcUnusedPawn(CPC cpc) const;

	void MakeMvSq(MV& mv);
	void UndoMvSq(MV mv);

	void GenRgmv(vector<MV>& rgmv, CPC cpcMove, RMCHK rmchk) const;
	void GenRgmvQuiescent(vector<MV>& rgmv, CPC cpcMove, RMCHK rmchk) const;
	void GenRgmvColor(vector<MV>& rgmv, CPC cpcMove, bool fForAttack) const;
	void GenRgmvPawn(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvKnight(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvBishop(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvRook(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvQueen(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvKing(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvCastle(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvCastleSide(vector<MV>& rgmv, SQ sqKing, int fileRook, int dsq) const;
	void GenRgmvPawnCapture(vector<MV>& rgmv, SQ sqFrom, int dsq) const;
	void AddRgmvMvPromotions(vector<MV>& rgmv, MV mv) const;
	void GenRgmvEnPassant(vector<MV>& rgmv, SQ sqFrom) const;


	/*	BD::AddRgmvMv
	 *
	 *	Adds the move to the move vector
	 */
	inline void AddRgmvMv(vector<MV>& rgmv, MV mv) const
	{
		rgmv.push_back(mv);
	}


	/*	BD::FGenRgmvDsq
	 *
	 *	Checks the square in the direction given by dsq for a valid
	 *	destination square. Adds the move to rgmv if it is valid.
	 *	Returns true if the destination square was empty; returns
	 *	false if it's not a legal square or there is a piece in the
	 *	square.
	 */
	inline bool FGenRgmvDsq(vector<MV>& rgmv, SQ sqFrom, SQ sq, IPC ipcFrom, int dsq) const
	{
		SQ sqTo = sq + dsq;
		if (sqTo.fIsOffBoard())
			return false;

		/* have we run into one of our own pieces? */
		if (!FIsEmpty(sqTo) && CpcFromSq(sqTo) == ipcFrom.cpc())
			return false;

		/* add the move to the list */
		AddRgmvMv(rgmv, MV(sqFrom, sqTo));

		/* return false if we've reached an enemy piece - this capture
		   is the end of a potential slide move */
		return FIsEmpty(sqTo);
	}


	/*	BD::GenRgmvSlide
	 *
	 *	Generates all straight-line moves in the direction dsq starting at sqFrom.
	 *	Stops when the piece runs into a piece of its own color, or a capture of
	 *	an enemy piece, or it reaches the end of the board.
	 */
	inline void GenRgmvSlide(vector<MV>& rgmv, SQ sqFrom, int dsq) const
	{
		IPC ipcFrom = (*this)(sqFrom);
		for (int sq = sqFrom; FGenRgmvDsq(rgmv, sqFrom, sq, ipcFrom, dsq); sq += dsq)
			;
	}
	

	void RemoveInCheckMoves(vector<MV>& rgmv, CPC cpc) const;
	void RemoveQuiescentMoves(vector<MV>& rgmv, CPC cpcMove) const;
	bool FMvIsQuiescent(MV mv, CPC cpc) const;
	bool FInCheck(CPC cpc) const;
	bool FSqAttacked(CPC cpc, SQ sqKing) const;
	
	inline bool FDsqAttack(SQ sqKing, SQ sq, int dsq) const
	{
		SQ sqScan = sq;
		do {
			sqScan += dsq;
			if (sqScan == sqKing)
				return true;
		} while (FIsEmpty(sqScan));
		return false;
	}


	inline bool FDiagAttack(SQ sqKing, SQ sq, int dsq) const
	{
		return FDsqAttack(sqKing, sq, sqKing > sq ? dsq : -dsq);
	}


	inline bool FRankAttack(SQ sqKing, SQ sq) const
	{
		return FDsqAttack(sqKing, sq, sqKing > sq ? 1 : -1);
	}


	inline bool FFileAttack(SQ sqKing, SQ sq) const
	{
		return FDsqAttack(sqKing, sq, sqKing > sq ? 16 : -16);
	}

	inline bool FMvEnPassant(MV mv) const
	{
		return mv.sqTo() == sqEnPassant && ApcFromSq(mv.sqFrom()) == APC::Pawn;
	}

	inline bool FMvIsCapture(MV mv) const
	{
		return !FIsEmpty(mv.sqTo()) || FMvEnPassant(mv);
	}

	inline IPC& operator()(int rank, int file) { return *(IPC*)&mpsqipc[rank * 16 + file]; }
	inline IPC& operator()(SQ sq) { return *(IPC*)&mpsqipc[sq]; }

	inline const IPC& operator()(SQ sq) const {
		return *(IPC*)&mpsqipc[sq];
	}

	inline SQ& SqFromTpc(TPC tpc, CPC cpc) { return *(SQ*)&mptpcsq[cpc][tpc]; }
	inline SQ& SqFromIpc(IPC ipc) { return SqFromTpc(ipc.tpc(), ipc.cpc()); }
	inline SQ SqFromTpc(TPC tpc, CPC cpc) const { return *(SQ*)&mptpcsq[cpc][tpc]; }
	inline SQ SqFromIpc(IPC ipc) const { return SqFromTpc(ipc.tpc(), ipc.cpc()); }

	inline SQ& operator()(TPC tpc, CPC cpc) {
		return SqFromTpc(tpc, cpc);
	}
	inline SQ operator()(TPC tpc, CPC cpc) const {
		return SqFromTpc(tpc, cpc);
	}
	inline SQ& operator()(IPC ipc) {
		return SqFromIpc(ipc);
	}
	inline const SQ operator()(IPC ipc) const {
		return SqFromIpc(ipc);
	}

	inline APC ApcFromSq(SQ sq) const 
	{
		return (*this)(sq).apc();
	}
	
	inline TPC TpcFromSq(SQ sq) const 
	{
		return (*this)(sq).tpc();
	}
	
	inline CPC CpcFromSq(SQ sq) const { return (*this)(sq).cpc(); }
	inline float VpcFromSq(SQ sq) const;
	
	inline bool FIsEmpty(SQ sq) const 
	{
		return (*this)(sq).fIsEmpty();
	}

	inline bool FCanCastle(CPC cpc, int csSide) const 
	{
		return (this->cs & (csSide << (int)cpc)) != 0;
	}
	
	inline void SetCastle(CPC cpc, int csSide) 
	{ 
		this->cs |= csSide << (int)cpc; 
	}
	
	inline void ClearCastle(CPC cpc, int csSide) 
	{ 
		this->cs &= ~(csSide << (int)cpc); 
	}

	inline void SetBB(IPC ipc, SQ sq)
	{
		mpapcbb[ipc.cpc()][ipc.apc()] += sq;
	}

	inline void ClearBB(IPC ipc, SQ sq)
	{
		mpapcbb[ipc.cpc()][ipc.apc()] -= sq;
	}

	float VpcTotalFromCpc(CPC cpc) const;

	bool operator==(const BD& bd) const;
	bool operator!=(const BD& bd) const;

#ifndef NDEBUG
	void Validate(void) const;
	void ValidateBB(IPC ipc, SQ sq) const;
#else
	inline void Validate(void) const { }
	inline void ValidateBB(IPC ipc, SQ sq) const { }
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
	CPC cpcToMove;
	vector<MV> rgmvGame;	// the game moves that resulted in bd board state
	int imvCur;
	int imvPawnOrTakeLast;	/* index of last pawn or capture move (used directly for 50-move 
							   draw detection), but it is also a bound on how far back we need 
							   to search for 3-move repetition draws */

public:
	BDG(void);
	BDG(const WCHAR* szFEN);

	void NewGame(void);

	void GenRgmv(vector<MV>& rgmv, RMCHK rmchk) const;
	void GenRgmvQuiescent(vector<MV>& rgmv, RMCHK rmchk) const;
	bool FMvIsQuiescent(MV mv) const;
	
	void MakeMv(MV mv);
	void UndoMv(void);
	void RedoMv(void);
	
	GS GsTestGameOver(const vector<MV>& rgmv, const RULE& rule) const;
	void SetGameOver(const vector<MV>& rgmv, const RULE& rule);
	bool FDrawDead(void) const;
	bool FDraw3Repeat(int cbdDraw) const;
	bool FDraw50Move(int cmvDraw) const;
	void SetGs(GS gs); 

	wstring SzMoveAndDecode(MV mv);
	wstring SzDecodeMv(MV mv, bool fPretty) const;
	bool FMvApcAmbiguous(const vector<MV>& rgmv, MV mv) const;
	bool FMvApcRankAmbiguous(const vector<MV>& rgmv, MV mv) const;
	bool FMvApcFileAmbiguous(const vector<MV>& rgmv, MV mv) const;
	string SzFlattenMvSz(const wstring& wsz) const;
	wstring SzDecodeMvPost(MV mv) const;

	void InitFEN(const WCHAR* szFen);
	void InitFENSideToMove(const WCHAR*& sz);
	void InitFENCastle(const WCHAR*& sz);
	void InitFENEnPassant(const WCHAR*& sz);
	void InitFENHalfmoveClock(const WCHAR*& sz);
	void InitFENFullmoveCounter(const WCHAR*& sz);

	int ParseMv(const char*& pch, MV& mv) const;
	int ParsePieceMv(const vector<MV>& rgmv, TKMV tkmv, const char*& pch, MV& mv) const;
	int ParseSquareMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseMvSuffixes(MV& mv, const char*& pch) const;
	int ParseFileMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseRankMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	bool FMvMatchPieceTo(const vector<MV>& rgmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, MV& mv) const;
	bool FMvMatchFromTo(const vector<MV>& rgmv, SQ sqFrom, SQ sqTo, MV& mv) const;
	TKMV TkmvScan(const char*& pch, SQ& sq) const;
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
 *	general purpose scanner eventually.
 * 
 */


class ISTK
{
protected:
	int li;
	istream& is;
	TK* ptkPrev;

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
	tkpgnResult
};


class ISTKPGN : public ISTK
{
protected:
	bool fWhiteSpace;

	virtual bool FIsSymbol(char ch, bool fFirst) const;
	char ChSkipWhite(void);

public:
	ISTKPGN(istream& is);
	void WhiteSpace(bool fReturn);
	virtual TK* PtkNext(void);
};