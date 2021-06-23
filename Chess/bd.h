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
	inline IPC(void) : tpcGrf(tpcPieceMin), apcGrf(APC::ActMax), cpcGrf(CPC::Black)
	{
	}

	inline IPC(TPC tpc, CPC cpc, APC apc) : tpcGrf(tpc), apcGrf(apc), cpcGrf(cpc)
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

	inline MV(SQ sqFrom, SQ sqTo, APC apcPromote = APC::Null) {
		sqFromGrf = sqFrom;
		sqToGrf = sqTo;
		tpcCaptGrf = 0;
		apcCaptGrf = APC::Null;
		fEnPassantGrf = false;
		csGrf = 0;
		apcPromoteGrf = apcPromote;
	}
	
	inline operator uint32_t() const
	{
		return *(uint32_t*)this;
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
		return sqFromGrf == 255 && sqToGrf == 255 && apcPromoteGrf == APC::ActMax;
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
		return *(uint32_t*)this == (uint32_t)mv;
	}

	inline bool operator!=(const MV& mv) const 
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
#pragma warning(push)
#pragma warning (disable:26495)
	GMV() : cmvCur(0), pvmvOverflow(nullptr) 
	{
	}
#pragma warning(pop)

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

	
	inline int cmv(void) const {
		assert(FValid());
		return cmvCur;
	}


	inline MV& operator[](int imv)
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

	inline MV operator[](int imv) const
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

	inline void AppendMv(SQ sqFrom, SQ sqTo)
	{
		assert(FValid());
		if (cmvCur < cmvPreMax) 
			amv[cmvCur++] = MV(sqFrom, sqTo);
		else
			AppendMvOverflow(MV(sqFrom, sqTo));
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

	void Clear(void)
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
	uint8_t mpsqipc[64+64];	/* the board itself (maps square to piece) */
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
	bool operator==(const BD& bd) const;
	bool operator!=(const BD& bd) const;

	/* making moves */

	void MakeMvSq(MV& mv);
	void UndoMvSq(MV mv);

	/* move generation */
	
	void GenRgmv(GMV& gmv, CPC cpcMove, RMCHK rmchk) const;
	void GenRgmvQuiescent(GMV& gmv, CPC cpcMove, RMCHK rmchk) const;
	void GenRgmvColor(GMV& gmv, CPC cpcMove, bool fForAttack) const;
	void GenRgmvPawn(GMV& gmv, SQ sqFrom) const;
	void GenRgmvKnight(GMV& gmv, SQ sqFrom) const;
	void GenRgmvBishop(GMV& gmv, SQ sqFrom) const;
	void GenRgmvRook(GMV& gmv, SQ sqFrom) const;
	void GenRgmvQueen(GMV& gmv, SQ sqFrom) const;
	void GenRgmvKing(GMV& gmv, SQ sqFrom) const;
	void GenRgmvCastle(GMV& gmv, SQ sqFrom) const;
	void GenRgmvPawnCapture(GMV& gmv, SQ sqFrom, int dsq) const;
	void AddRgmvMvPromotions(GMV& gmv, MV mv) const;
	void GenRgmvEnPassant(GMV& gmv, SQ sqFrom) const;


	/*	BD::FGenRgmvDsq
	 *
	 *	Checks the square in the direction given by dsq for a valid
	 *	destination square. Adds the move to gmv if it is valid.
	 *	Returns true if the destination square was empty; returns
	 *	false if it's not a legal square or there is a piece in the
	 *	square.
	 */
	inline bool FGenRgmvDsq(GMV& gmv, SQ sqFrom, SQ sq, IPC ipcFrom, int dsq) const
	{
		SQ sqTo = sq + dsq;
		if (sqTo.fIsOffBoard())
			return false;

		/* have we run into one of our own pieces? */
		if (!FIsEmpty(sqTo) && CpcFromSq(sqTo) == ipcFrom.cpc())
			return false;

		/* add the move to the list */
		gmv.AppendMv(MV(sqFrom, sqTo));

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
	inline void GenRgmvSlide(GMV& gmv, SQ sqFrom, int dsq) const
	{
		IPC ipcFrom = (*this)(sqFrom);
		for (int sq = sqFrom; FGenRgmvDsq(gmv, sqFrom, sq, ipcFrom, dsq); sq += dsq)
			;
	}
	
	/*
	 *	checking squares for attack 
	 */

	void RemoveInCheckMoves(GMV& gmv, CPC cpc) const;
	void RemoveQuiescentMoves(GMV& gmv, CPC cpcMove) const;
	bool FMvIsQuiescent(MV mv, CPC cpc) const;
	bool FInCheck(CPC cpc) const;
	bool FSqAttacked(CPC cpc, SQ sqAttacked) const;
	
	/*	BD::FDsqAttack
	 *
	 *	Checks that sqAttacked is being attacked from sq, with dsq being the increment
	 *	between the two squares. Assumes the two squares are reachable using the dsq
	 *	increment.
	 */
	inline bool FDsqAttack(SQ sqAttacked, SQ sq, int dsq) const
	{
		do {
			sq += dsq;
			assert(!sq.fIsOffBoard());
			if (sq == sqAttacked)
				return true;
		} while (FIsEmpty(sq));
		return false;
	}

	/*
	 *	move, square, and piece convenience functions. most of these need to be highly
	 *	optimized - beware of bit twiddling tricks!
	 */
	
	inline bool FMvEnPassant(MV mv) const
	{
		return mv.sqTo() == sqEnPassant && ApcFromSq(mv.sqFrom()) == APC::Pawn;
	}

	inline bool FMvIsCapture(MV mv) const
	{
		return !FIsEmpty(mv.sqTo()) || FMvEnPassant(mv);
	}

	inline IPC& operator()(int rank, int file) 
	{ 
		return *(IPC*)&mpsqipc[rank * 16 + file]; 
	}
	
	inline IPC& operator()(SQ sq) 
	{
		return *(IPC*)&mpsqipc[sq]; 
	}

	inline const IPC& operator()(SQ sq) const 
	{
		return *(IPC*)&mpsqipc[sq];
	}

	inline SQ& SqFromTpc(TPC tpc, CPC cpc) 
	{
		return *(SQ*)&mptpcsq[cpc][tpc]; 
	}
	
	inline SQ& SqFromIpc(IPC ipc) 
	{
		return SqFromTpc(ipc.tpc(), ipc.cpc()); 
	}
	
	inline SQ SqFromTpc(TPC tpc, CPC cpc) const 
	{
		return *(SQ*)&mptpcsq[cpc][tpc]; 
	}

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
	
	inline CPC CpcFromSq(SQ sq) const 
	{ 
		return (*this)(sq).cpc(); 
	}
	
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

	/*
	 *	bitboard manipulation
	 */

	inline void SetBB(IPC ipc, SQ sq)
	{
		mpapcbb[ipc.cpc()][ipc.apc()] += sq;
	}

	inline void ClearBB(IPC ipc, SQ sq)
	{
		mpapcbb[ipc.cpc()][ipc.apc()] -= sq;
	}

	/*
	 *	getting piece value of pieces/squares
	 */

	float VpcFromSq(SQ sq) const;
	float VpcTotalFromCpc(CPC cpc) const;

	/*
	 *	reading FEN strings 
	 */

	void InitFENPieces(const wchar_t*& szFEN);
	void AddPieceFEN(SQ sq, TPC tpc, CPC cpc, APC apc);
	void SkipToNonSpace(const wchar_t*& sz);
	void SkipToSpace(const wchar_t*& sz);
	void InitFENCastle(const wchar_t*& sz);
	void InitFENEnPassant(const wchar_t*& sz);
	TPC TpcUnusedPawn(CPC cpc) const;

	/*
	 *	debug and validation
	 */

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
	vector<MV> vmvGame;	// the game moves that resulted in bd board state
	int imvCur;
	int imvPawnOrTakeLast;	/* index of last pawn or capture move (used directly for 50-move 
							   draw detection), but it is also a bound on how far back we need 
							   to search for 3-move repetition draws */

public:
	BDG(void);
	BDG(const WCHAR* szFEN);
	void NewGame(void);

	/* 
	 *	move generation 
	 */

	void GenRgmv(GMV& gmv, RMCHK rmchk) const;
	void GenRgmvQuiescent(GMV& gmv, RMCHK rmchk) const;
	bool FMvIsQuiescent(MV mv) const;
	
	/* 
	 *	making moves 
	 */

	void MakeMv(MV mv);
	void UndoMv(void);
	void RedoMv(void);
	
	/* 
	 *	game over tests
	 */

	GS GsTestGameOver(const GMV& gmv, const RULE& rule) const;
	void SetGameOver(const GMV& gmv, const RULE& rule);
	bool FDrawDead(void) const;
	bool FDraw3Repeat(int cbdDraw) const;
	bool FDraw50Move(int cmvDraw) const;
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

	int ParseMv(const char*& pch, MV& mv) const;
	int ParsePieceMv(const GMV& gmv, TKMV tkmv, const char*& pch, MV& mv) const;
	int ParseSquareMv(const GMV& gmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseMvSuffixes(MV& mv, const char*& pch) const;
	int ParseFileMv(const GMV& gmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseRankMv(const GMV& gmv, SQ sq, const char*& pch, MV& mv) const;
	bool FMvMatchPieceTo(const GMV& gmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, MV& mv) const;
	bool FMvMatchFromTo(const GMV& gmv, SQ sqFrom, SQ sqTo, MV& mv) const;
	TKMV TkmvScan(const char*& pch, SQ& sq) const;

	/*
	 *	importing FEN strings 
	 */

	void InitFEN(const wchar_t* szFen);
	void InitFENSideToMove(const wchar_t*& sz);
	void InitFENHalfmoveClock(const wchar_t*& sz);
	void InitFENFullmoveCounter(const wchar_t*& sz);

	/* 
	 *	exporting FEN strings 
	 */

	wstring SzFEN(void) const;
	operator wstring() const { return SzFEN(); }

};


/*
 *
 *	HABDG class
 * 
 *	Game board hash
 *
 */

class HABDG
{
	uint32_t rggrfRandom[8 * 8][APC::ActMax];
public:
	HABDG(void)
	{
		uniform_int_distribution<uint32_t> grfDist(0L, 0xffffffffL);
		for (int isq = 0; isq < 8 * 8; isq++)
			for (APC apc = APC::Null; apc < APC::ActMax; ++apc)
				rggrfRandom[isq][apc] = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);
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