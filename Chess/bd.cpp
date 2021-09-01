/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */

#include "bd.h"


mt19937 rgen(0);

/*
 *
 *	genhabd
 * 
 *	The zobrist-hash generator.
 *
 */

GENHABD genhabd;


/*	GENHABD::GENHABD
 *
 *	Generates the random bit arrays used to compute the hash.
 */
GENHABD::GENHABD(void)
{
	uniform_int_distribution<uint32_t> grfDist(0L, 0xffffffffUL);
	for (int isq = 0; isq < 8 * 8; isq++)
		for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc)
			for (APC apc = APC::Null; apc <= APC::ActMax; ++apc)
				rghabdPiece[isq][apc][cpc] = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);

	rghabdCastle[0] = 0;
	for (int ics = 1; ics < CArray(rghabdCastle); ics++)
		rghabdCastle[ics] = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);

	for (int iep = 0; iep < CArray(rghabdEnPassant); iep++)
		rghabdEnPassant[iep] = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);

	habdMove = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);
}


/*	GENHABD::HabdFromBd
 *
 *	Creates the initial hash value for a new board position.
 */
HABD GENHABD::HabdFromBd(const BD& bd) const
{
	/* pieces */

	HABD habd = 0ULL;
	for (int rank = 0; rank < rankMax; rank++)
		for (int file = 0; file < fileMax; file++) {
			APC apc = bd(rank, file).apc();
			if (apc != APC::Null)
				habd ^= rghabdPiece[rank * 8 + file][apc][bd(rank, file).cpc()];
		}

	/* castle state */

	habd ^= rghabdCastle[bd.csCur];

#ifdef LATER
	/* en passant state */

	if (!bd.sqEnPassant.fIsNil())
		habd ^= rghabdEnPassant[bd.sqEnPassant.file()];
#endif

	/* current side to move */

	if (bd.cpcToMove == CPC::Black)
		habd ^= habdMove;

	return habd;
}


/*
 *
 *	mpsqdirbb
 *
 *	We keep bitboards for every square and every direction of squares that are
 *	attacked from that square.
 * 
 */


MPSQDIRBB mpsqdirbb;

MPSQDIRBB::MPSQDIRBB(void)
{
	/* build all the attack bitboards for each square and each direction */

	for (int rank = 0; rank < rankMax; rank++)
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			for (int drank = -1; drank <= 1; drank++)
				for (int dfile = -1; dfile <= 1; dfile++) {
					int dsq = 16 * drank + dfile;
					if (dsq == 0)
						continue;
					DIR dir = DirFromDrankDfile(drank, dfile);
					for (SQ sqNext = sq + dsq; !sqNext.fIsOffBoard(); sqNext += dsq)
						mpsqdirbb[sq][(int)dir] |= BB(sqNext.fgrf());
				}
		}
}



/*
 *
 *	BD class implementation
 * 
 *	The minimal board implementation, that implements non-move history
 *	board. That means this class is not able to perform certain chess
 *	stuff by itself, like detect en passant or some draw situations.
 *
 */

const float BD::mpapcvpc[] = { 0.0f, 100.0f, 300.0f, 300.0f, 500.0f, 900.0f, 200.0f, -1.0f };

BD::BD(void)
{
	assert(sizeof(IPC) == sizeof(uint8_t));
	assert(sizeof(SQ) == sizeof(uint8_t));
	SetEmpty();
	Validate();
}


void BD::SetEmpty(void)
{
	assert(sizeof(mpsqipc[0]) == 1);
	memset(mpsqipc, ipcEmpty, sizeof(mpsqipc));
	for (int file = 8; file < 16; file++)
		for (int rank = 0; rank < 8; rank++)
			mpsqipc[SQ(rank, file)] = ipcNil;

	for (int cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
		for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc)
			mptpcsq[cpc][tpc] = sqNil;
		for (APC apc = APC::Null; apc < APC::ActMax; ++apc)
			mpapcbb[cpc][apc] = bbNone;
		mpcpcbb[cpc] = bbNone;
	}

	bbUnoccupied = bbAll;

	csCur = 0;
	cpcToMove = CPC::White;
	sqEnPassant = sqNil;
	
	habd = 0L;
}


/*	BD::operator==
 *
 *	Compare two board states for equality. Boards are equal if all the pieces
 *	are in the same place, the castle states are the same, and the en passant
 *	state is the same.
 * 
 *	This is mainly used to detect 3-repeat draw situations. Note that we don't
 *	have the current move color in the BD, so this isn't' exactly the draw-state
 *	comparison. Caller has to make sure the move color is the same.
 */
bool BD::operator==(const BD& bd) const
{
	for (int tpc = 0; tpc < tpcPieceMax; tpc++)
		if (mptpcsq[(int)CPC::White][tpc] != bd.mptpcsq[(int)CPC::White][tpc] ||
				mptpcsq[(int)CPC::Black][tpc] != bd.mptpcsq[(int)CPC::Black][tpc])
			return false;
	return csCur == bd.csCur && sqEnPassant == bd.sqEnPassant && cpcToMove == bd.cpcToMove;
}


bool BD::operator!=(const BD& bd) const
{
	return !(*this == bd);
}


/*	BD::SzInitFENPieces
 *
 *	Reads the pieces out a Forsyth-Edwards Notation string. The piece
 *	table should start at szFEN. Returns the character after the last
 *	piece character, which will probably be the space character.
 */
void BD::InitFENPieces(const wchar_t*& szFEN)
{
	SetEmpty();

	/* parse the line */

	int rank = rankMax - 1;
	SQ sq = SQ(rank, 0);
	const wchar_t* pch;
	for (pch = szFEN; *pch != L' ' && *pch != L'\0'; pch++) {
		switch (*pch) {
		case 'p': AddPieceFEN(sq++, tpcPawnFirst, CPC::Black, APC::Pawn); break;
		case 'r': AddPieceFEN(sq++, tpcQueenRook, CPC::Black, APC::Rook); break;
		case 'n': AddPieceFEN(sq++, tpcQueenKnight, CPC::Black, APC::Knight); break;
		case 'b': AddPieceFEN(sq++, tpcQueenBishop, CPC::Black, APC::Bishop); break;
		case 'q': AddPieceFEN(sq++, tpcQueen, CPC::Black, APC::Queen); break;
		case 'k': AddPieceFEN(sq++, tpcKing, CPC::Black, APC::King); break;
		case 'P': AddPieceFEN(sq++, tpcPawnFirst, CPC::White, APC::Pawn); break;
		case 'R': AddPieceFEN(sq++, tpcQueenRook, CPC::White, APC::Rook); break;
		case 'N': AddPieceFEN(sq++, tpcQueenKnight, CPC::White, APC::Knight); break;
		case 'B': AddPieceFEN(sq++, tpcQueenBishop, CPC::White, APC::Bishop); break;
		case 'Q': AddPieceFEN(sq++, tpcQueen, CPC::White, APC::Queen); break;
		case 'K': AddPieceFEN(sq++, tpcKing, CPC::White, APC::King); break;
		case '/': sq = SQ(--rank, 0);  break;
		case '1':
		case '2': 
		case '3': 
		case '4': 
		case '5': 
		case '6':
		case '7':
		case '8':
			sq += *pch - L'0'; 
			break;
		default:
			/* TODO: illegal character in the string - report error */
			assert(false);
			return;
		}
	}
	szFEN = pch;
}


/*	BD::AddPieceFEN
 *
 *	Adds a piece from the FEN piece stream to the board. Makes sure both
 *	board mappings are correct. 
 * 
 *	Promoted  pawns are the real can of worms with this process.
 */
void BD::AddPieceFEN(SQ sq, TPC tpc, CPC cpc, APC apc)
{
	/* if piece is already on the board, we need to go to work to find a place for 
	 * this piece */

	if (!SqFromTpc(tpc, cpc).fIsNil()) {
		switch (apc) {
			/* TODO: two kings is an error */
		case APC::King:
			assert(false);
			return;
		default:
		{
			/* try king-side/queen-side */
			TPC tpcOpp = TpcOpposite(tpc);
			if (SqFromTpc(tpcOpp, cpc).fIsNil()) {
				tpc = tpcOpp;
				break;
			}
		}
		[[fallthrough]];
		case APC::Pawn:
		case APC::Queen:
			/* this must be a promoted pawn, so find an unused pawn location */
			tpc = TpcUnusedPawn(cpc);
			break;
		}
	}
	else {
		switch (apc) {
		default:
		case APC::Pawn:
			/* if the pawn tpc that corresponds to this file is available, try to put 
			   it there; this doesn't actually matter for any real purpose, which is why 
			   I haven't implemented it */
			break;
		case APC::Bishop:
		case APC::Knight:
		case APC::Rook:
			/* try to put the piece in the correct king-side/queen-side slot; this
			   is only important for clearing castle state */
			TPC tpcPref = tpc;
			if (sq.rank() == RankBackFromCpc(cpc))
				tpcPref = sq.file() <= fileQueen ? TpcQueenSide(tpc) : TpcKingSide(tpc);
			if (SqFromTpc(tpcPref, cpc).fIsNil())
				tpc = tpcPref;
			break;
		}
	}

	(*this)(sq) = IPC(tpc, cpc, apc);
	SqFromTpc(tpc, cpc) = sq;
	SetBB((*this)(sq), sq);
}


void BD::InitFENSideToMove(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	cpcToMove = CPC::White;
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'w': break;
		case 'b': ToggleToMove(); break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
}


void BD::SkipToSpace(const wchar_t*& sz)
{
	while (*sz && *sz != L' ')
		sz++;
}


void BD::SkipToNonSpace(const wchar_t*& sz)
{
	while (*sz && *sz == L' ')
		sz++;
}


void BD::InitFENCastle(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	ClearCastle(CPC::White, csKing | csQueen);
	ClearCastle(CPC::Black, csKing | csQueen);
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'K': SetCastle(CPC::White, csKing); EnsureCastleRook(CPC::White, tpcKingRook); break;
		case 'Q': SetCastle(CPC::White, csQueen); EnsureCastleRook(CPC::White, tpcQueenRook); break;
		case 'k': SetCastle(CPC::Black, csKing); EnsureCastleRook(CPC::Black, tpcKingRook);  break;
		case 'q': SetCastle(CPC::Black, csQueen); EnsureCastleRook(CPC::Black, tpcQueenRook);  break;
		case '-': break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
}


/*	BD::EnsureCastleRook
 *
 *	If castling is legal, then our move generation assumes the tpc of the rook
 *	that can be castled with must be in its original slot. When loading files,
 *	it's possible to get rooks in the wrong queen-side/king-side slots.
 */
void BD::EnsureCastleRook(CPC cpc, TPC tpcCorner)
{
	/* check if rook is already correct */

	SQ sqRook = SqFromTpc(tpcCorner, cpc);
	SQ sqCorner = SQ(RankBackFromCpc(cpc), tpcCorner);
	if (sqRook == sqCorner)
		return;
	
	assert(ApcFromSq(sqCorner) == APC::Rook);
	
	/* swap the rooks so the correct one is in castle position */

	TPC tpcRook = (*this)(sqCorner).tpc();
	IPC ipcT = (*this)(sqRook);
	(*this)(sqRook) = (*this)(sqCorner);
	(*this)(sqCorner) = ipcT;
	SqFromTpc(tpcCorner, cpc) = sqCorner;
	SqFromTpc(tpcRook, cpc) = sqRook; 
}


void BD::InitFENEnPassant(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	sqEnPassant = sqNil;
	int rank = -1, file = -1;
	for (; *sz && *sz != L' '; sz++) {
		if (*sz >= L'a' && *sz <= L'h')
			file = *sz - L'a';
		else if (*sz >= L'1' && *sz <= L'8')
			rank = *sz - '1';
		else if (*sz == '-')
			rank = file = -1;
	}
	if (rank != -1 && file != -1)
		sqEnPassant = SQ(rank, file);
	SkipToSpace(sz);
}


/*	BD::TpcUnusedPawn
 *
 *	Finds an unused pawn slot in the mptpcsq array for the cpc color.
 */
TPC BD::TpcUnusedPawn(CPC cpc) const
{
	TPC tpc;
	for (tpc = tpcPawnFirst; mptpcsq[cpc][tpc] != sqNil; ++tpc)
		assert(tpc < tpcPawnLim);
	return tpc;
}


/*	BD::MakeMvSq
 *
 *	Makes a partial move on the board, only updating the square,
 *	tpc arrays, and bitboards. On return, the MV is modified to include
 *	information needed to undo the move (i.e., saves state for captures,
 *	castling, en passant, and pawn promotion).
 */
void BD::MakeMvSq(MV& mv)
{
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	IPC ipcFrom = (*this)(sqFrom);
	CPC cpcFrom = ipcFrom.cpc();
	APC apcFrom = ipcFrom.apc();

	/* store undo information in the mv */

	mv.SetCsEp(CsPackColor(csCur, cpcFrom), sqEnPassant);
	SQ sqTake = sqTo;
	if (apcFrom == APC::Pawn) {
		if (sqTake == sqEnPassant)
			sqTake = SQ(sqTo.rank() ^ 1, sqEnPassant.file());
	}

	/* captures. if we're taking a rook, we can't castle to that rook */

	IPC ipcTake = (*this)(sqTake);
	if (!ipcTake.fIsEmpty()) {
		TPC tpcTake = TpcFromSq(sqTake);
		APC apcTake = ApcFromSq(sqTake);
		if ((tpcTake == tpcQueenRook && (csCur & (csQueen << ~cpcFrom))) ||
				(tpcTake == tpcKingRook && (csCur & (csKing << ~cpcFrom))))
			apcTake = APC::RookCastleable;
		mv.SetCapture(apcTake, tpcTake);

		SqFromIpc(ipcTake) = sqNil;
		ClearBB(ipcTake, sqTake);
		
		switch (ipcTake.tpc()) {
		case tpcKingRook: 
			ClearCastle(~cpcFrom, csKing); 
			break;
		case tpcQueenRook: 
			ClearCastle(~cpcFrom, csQueen); 
			break;
		default: 
			break;
		}
	}

	/* move the pieces */

	(*this)(sqFrom) = ipcEmpty;
	ClearBB(ipcFrom, sqFrom);
	(*this)(sqTo) = ipcFrom;
	SqFromIpc(ipcFrom) = sqTo;
	SetBB(ipcFrom, sqTo);

	switch (apcFrom) {
	case APC::Pawn:
		/* save double-pawn moves for potential en passant and return */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02) {
			sqEnPassant = SQ((sqFrom.rank() + sqTo.rank()) / 2, sqTo.file());
			goto Done;
		}
		
		if (sqTo == sqEnPassant) {
			/* take en passant - this is the case where we need to clear the taken piece
			   instead of just replacing it */
			(*this)(sqTake) = ipcEmpty;
		}
		else if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			IPC ipcPromote = IPC(ipcFrom.tpc(), cpcFrom, mv.apcPromote());
			(*this)(sqTo) = ipcPromote;
			ClearBB(ipcFrom, sqTo);
			SetBB(ipcPromote, sqTo);
		} 
		break;

	case APC::King:
		ClearCastle(cpcFrom, csKing|csQueen);
		if (sqFrom.file() - sqTo.file() > 1 || sqFrom.file() - sqTo.file() < -1) {
			/* castle moves */
			int fileDst = sqTo.file();
			assert(fileDst == fileKingKnight || fileDst == fileQueenBishop);
			SQ sqRookFrom, sqRookTo;
			if (fileDst == fileKingKnight) {	// king side
				sqRookFrom = sqTo + 1;
				sqRookTo = sqTo - 1;
			}
			else {	// must be queen side
				sqRookFrom = sqTo - 2;
				sqRookTo = sqTo + 1;
			}
			IPC ipcRook = (*this)(sqRookFrom);
			(*this)(sqRookFrom) = ipcEmpty;
			ClearBB(ipcRook, sqRookFrom);
			(*this)(sqRookTo) = ipcRook;
			SqFromIpc(ipcRook) = sqRookTo;
			SetBB(ipcRook, sqRookTo);
		}
		break;

	case APC::Rook:
		/* be careful! promoted rooks don't affect castle rights */
		if (ipcFrom.tpc() == tpcQueenRook)
			ClearCastle(cpcFrom, csQueen);
		else if (ipcFrom.tpc() == tpcKingRook)
			ClearCastle(cpcFrom, csKing);
		break;

	default:
		break;
	}

	sqEnPassant = sqNil;
Done:
	ToggleToMove();
	Validate();
}


/*	BD::UndoMvSq
 *
 *	Undo the move made on the board
 */
void BD::UndoMvSq(MV mv)
{
	/* unpack some useful information out of the move */

	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	assert(FIsEmpty(sqFrom));
	IPC ipcMove = (*this)(sqTo);
	CPC cpcMove = ipcMove.cpc();

	/* restore castle state. Castle state can only be added on an undo, so it's 
	   OK to or those bits back in. Note that we use the tpc to determine which
	   side the castle was on, which is a little hacky and is prone to break,
	   so be careful in any code that initializes boards pieces to make sure the 
	   right tpc is used for rooks that are still in castleable state */

	SetCastle(CsUnpackColor(mv.csPrev(), cpcMove));
	APC apcCapt = mv.apcCapture();
	if (mv.apcCapture() == APC::RookCastleable) {
		assert(mv.tpcCapture() == tpcKingRook || mv.tpcCapture() == tpcQueenRook);
		SetCastle(~cpcMove, mv.tpcCapture() == tpcKingRook ? csKing : csQueen);
		apcCapt = APC::Rook;
	}

	/* Restore en passant state. Note that the en passant state only saves the file 
	   of the en passant captureable pawn (due to lack of available bits in the MV),
	   but we can generate the rank knowing the side that has the move */

	if (mv.fEpPrev())
		sqEnPassant = SQ(cpcMove == CPC::White ? 5 : 2, mv.fileEpPrev());
	else
		sqEnPassant = sqNil;

	/* put piece back in source square, undoing any pawn promotion that might
	   have happened */

	ClearBB(ipcMove, sqTo);
	if (mv.apcPromote() != APC::Null)
		ipcMove = IpcSetApc(ipcMove, APC::Pawn);
	(*this)(sqFrom) = ipcMove;
	SetBB(ipcMove, sqFrom);
	SqFromIpc(ipcMove) = sqFrom;
	APC apcMove = ipcMove.apc();	// get the type of moved piece after we've undone promotion

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	if (apcCapt == APC::Null) 
		(*this)(sqTo) = ipcEmpty;
	else {
		IPC ipcTake = IPC(mv.tpcCapture(), ~cpcMove, apcCapt);
		SQ sqTake = sqTo;
		if (sqTake == sqEnPassant) {
			/* capture into the en passant square must be pawn x pawn */
			assert(ApcFromSq(sqTo) == APC::Pawn && apcCapt == APC::Pawn);
			sqTake = SQ(sqEnPassant.rank() + cpcMove*2 - 1, sqEnPassant.file());
			(*this)(sqTo) = ipcEmpty;
		}
		(*this)(sqTake) = ipcTake;
		SqFromIpc(ipcTake) = sqTake;
		SetBB(ipcTake, sqTake);
	}

	/* undoing a castle means we need to undo the rook, too */

	if (apcMove == APC::King) {
		int dfile = sqTo.file() - sqFrom.file();
		if (dfile < -1) { /* queen side castle */
			IPC ipcRook = (*this)(sqTo + 1);
			ClearBB(ipcRook, sqTo + 1);
			SetBB(ipcRook, sqTo - 2);
			(*this)(sqTo - 2) = ipcRook;
			(*this)(sqTo + 1) = ipcEmpty;
			SqFromIpc(ipcRook) = sqTo - 2;
		}
		else if (dfile > 1) { /* king side castle */
			IPC ipcRook = (*this)(sqTo - 1);
			ClearBB(ipcRook, sqTo - 1);
			SetBB(ipcRook, sqTo + 1);
			(*this)(sqTo + 1) = ipcRook;
			(*this)(sqTo - 1) = ipcEmpty;
			SqFromIpc(ipcRook) = sqTo + 1;
		}
	}

	ToggleToMove();

	Validate();
}


/*	BDG::GenGmv
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the
 *	king in check.
 */
void BD::GenGmv(GMV& gmv, RMCHK rmchk) const
{
	GenGmv(gmv, cpcToMove, rmchk);
}


/*	BD::GenGmv
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenGmv(GMV& gmv, CPC cpcMove, RMCHK rmchk) const
{
	Validate();
	GenGmvColor(gmv, cpcMove);
	if (rmchk == RMCHK::Remove)
		RemoveInCheckMoves(gmv, cpcMove);
}


void BD::RemoveInCheckMoves(GMV& gmv, CPC cpcMove) const
{
 	unsigned imvDest = 0;
	BD bd(*this);
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mv = gmv[imv];
		bd.MakeMvSq(mv);
		if (!bd.FInCheck(cpcMove))
			gmv[imvDest++] = mv;
		bd.UndoMvSq(mv);
	}
	gmv.Resize(imvDest);
}


/*	BD::RemoveQuiescentMoves
 *
 *	Removes quiet moves from the given move list. For now quiet moves are
 *	anything that is not a capture or check.
 */
void BD::RemoveQuiescentMoves(GMV& gmv, CPC cpcMove) const
{
	if (FInCheck(cpcMove))	/* don't prune if we're in check */
		return;
	int imvTo = 0;
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mv = gmv[imv];
		if (!FMvIsQuiescent(mv, cpcMove))
			gmv[imvTo++] = mv;
		/* TODO: need to test for checks */
	}
	gmv.Resize(imvTo);
}


bool BD::FMvIsQuiescent(MV mv) const
{
	return FMvIsQuiescent(mv, cpcToMove);
}


bool BD::FMvIsQuiescent(MV mv, CPC cpcMove) const
{
	return FIsEmpty(mv.sqTo()) ||
		VpcFromSq(mv.sqFrom()) + 50.0f >= VpcFromSq(mv.sqTo());
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check. Does not require ComputeAttacked,
 *	and does a complete scan of the opponent pieces to find checks.
 */
bool BD::FInCheck(CPC cpc) const
{
	return FSqAttacked((*this)(tpcKing, cpc), ~cpc);
}


/*	BD::BbPawnAttacked
 *
 *	Returns bitboard of all squares pawns attack.
 */
BB BD::BbPawnAttacked(CPC cpcBy) const
{
	BB bbPawn = mpapcbb[cpcBy][APC::Pawn];
	bbPawn = cpcBy == CPC::White ? BbNorthOne(bbPawn) : BbSouthOne(bbPawn);
	BB bbPawnAttacked = BbEastOne(bbPawn) | BbWestOne(bbPawn);
	return bbPawnAttacked;
}


BB BD::BbKnightAttacked(CPC cpcBy) const
{
	BB bbKnights = mpapcbb[cpcBy][APC::Knight];
	BB bb1 = BbWestOne(bbKnights) | BbEastOne(bbKnights);
	BB bb2 = BbWestTwo(bbKnights) | BbEastTwo(bbKnights);
	return BbNorthTwo(bb1) | BbSouthTwo(bb1) | BbNorthOne(bb2) | BbSouthOne(bb2);
}


BB BD::BbFwdSlideAttacks(DIR dir, SQ sqFrom) const
{
	assert(dir == DIR::East || dir == DIR::NorthWest || dir == DIR::North || dir == DIR::NorthEast);
	BB bbAttacks = mpsqdirbb(sqFrom, dir);
	/* set a bit that can't be hit to so sqLow never has to deal with an empty bitboard */
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(0x8000000000000000ULL);
	return bbAttacks ^ mpsqdirbb(bbBlockers.sqLow(), dir);
}


BB BD::BbRevSlideAttacks(DIR dir, SQ sqFrom) const
{
	assert(dir == DIR::West || dir == DIR::SouthWest || dir == DIR::South || dir == DIR::SouthEast);
	BB bbAttacks = mpsqdirbb(sqFrom, dir);
	/* set a bit that can't be hit to so sqHigh never has to deal with an empty bitboard */
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(0x0000000000000001ULL);
	return bbAttacks ^ mpsqdirbb(bbBlockers.sqHigh(), dir);
}


BB BD::BbBishopAttacked(CPC cpcBy) const
{
	BB bbAttacks;
	for (BB bbBishops = mpapcbb[cpcBy][APC::Bishop]; bbBishops; bbBishops.ClearLow()) {
		SQ sq = bbBishops.sqLow();
		bbAttacks |= BbFwdSlideAttacks(DIR::NorthEast, sq);
		bbAttacks |= BbFwdSlideAttacks(DIR::NorthWest, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::SouthEast, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::SouthWest, sq);
	}
	return bbAttacks;
}


BB BD::BbRookAttacked(CPC cpcBy) const
{
	BB bbAttacks;
	for (BB bbRooks = mpapcbb[cpcBy][APC::Rook]; bbRooks; bbRooks.ClearLow()) {
		SQ sq = bbRooks.sqLow();
		bbAttacks |= BbFwdSlideAttacks(DIR::North, sq);
		bbAttacks |= BbFwdSlideAttacks(DIR::East, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::South, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::West, sq);
	}
	return bbAttacks;
}


BB BD::BbQueenAttacked(CPC cpcBy) const
{
	BB bbAttacks;
	for (BB bbQueens = mpapcbb[cpcBy][APC::Queen]; bbQueens; bbQueens.ClearLow()) {
		SQ sq = bbQueens.sqLow();
		bbAttacks |= BbFwdSlideAttacks(DIR::North, sq);
		bbAttacks |= BbFwdSlideAttacks(DIR::East, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::South, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::West, sq);
		bbAttacks |= BbFwdSlideAttacks(DIR::NorthEast, sq);
		bbAttacks |= BbFwdSlideAttacks(DIR::NorthWest, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::SouthEast, sq);
		bbAttacks |= BbRevSlideAttacks(DIR::SouthWest, sq);
	}
	return bbAttacks;
}


BB BD::BbKingAttacked(CPC cpcBy) const
{
	BB bbKing = mpapcbb[cpcBy][APC::King];
	BB bbKingAttacked = BbEastOne(bbKing) | BbWestOne(bbKing);
	bbKingAttacked |= BbNorthOne(bbKingAttacked|bbKing) | BbSouthOne(bbKingAttacked|bbKing);
	return bbKingAttacked;
}


/*	BD::BbAttackedAll
 *
 *	Returns bitboard of all squares attacked by the color
 */
BB BD::BbAttackedAll(CPC cpcBy) const
{
	return BbQueenAttacked(cpcBy) | BbRookAttacked(cpcBy) | BbBishopAttacked(cpcBy) | 
		BbKnightAttacked(cpcBy) | BbPawnAttacked(cpcBy) | BbKingAttacked(cpcBy);
}


/*	BD::FBbAttacked
 *
 *	Returns true if the given bitboard is attacked by someone of the color cpcBy.
 */
bool BD::FBbAttacked(BB bbAttacked, CPC cpcBy) const
{
	if (BbQueenAttacked(cpcBy) & bbAttacked)
		return true;
	if (BbRookAttacked(cpcBy) & bbAttacked)
		return true;
	if (BbBishopAttacked(cpcBy) & bbAttacked)
		return true;
	if (BbPawnAttacked(cpcBy) & bbAttacked)
		return true;
	if (BbKnightAttacked(cpcBy) & bbAttacked)
		return true;
	if (BbKingAttacked(cpcBy) & bbAttacked)
		return true;
	return false;
}


/*	BD::GenGmvColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenGmvColor(GMV& gmv, CPC cpcMove) const
{
	gmv.Clear();

	/* generate pawn moves */

	for (BB bb = mpapcbb[cpcMove][APC::Pawn]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
		assert(ApcFromSq(sqFrom) == APC::Pawn);
		GenGmvPawn(gmv, sqFrom);
		if (sqEnPassant != sqNil)
			GenGmvEnPassant(gmv, sqFrom);
	}

	/* generate knight moves */

	for (BB bb = mpapcbb[cpcMove][APC::Knight]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
		assert(ApcFromSq(sqFrom) == APC::Knight);
		GenGmvKnight(gmv, sqFrom);
	}

	/* generate bishop moves */

	for (BB bb = mpapcbb[cpcMove][APC::Bishop]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
		assert(ApcFromSq(sqFrom) == APC::Bishop);
		GenGmvBishop(gmv, sqFrom);
	}

	/* generate rook moves */

	for (BB bb = mpapcbb[cpcMove][APC::Rook]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
		assert(ApcFromSq(sqFrom) == APC::Rook);
		GenGmvRook(gmv, sqFrom);
	}

	/* generate queen moves */

	for (BB bb = mpapcbb[cpcMove][APC::Queen]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
		assert(ApcFromSq(sqFrom) == APC::Queen);
		GenGmvQueen(gmv, sqFrom);
	}

	/* generate king moves - only one king per side */

	BB bb = mpapcbb[cpcMove][APC::King];
	assert(bb);
	SQ sqFrom = bb.sqLow();
	assert(!FIsEmpty(sqFrom));
	assert((*this)(sqFrom).cpc() == cpcMove);
	assert(ApcFromSq(sqFrom) == APC::King);
	GenGmvKing(gmv, sqFrom);
	GenGmvCastle(gmv, sqFrom);
}


/*	BD::GenGmvPawn
 *
 *	Generates legal moves (without check tests) for a pawn
 *	in the sqFrom square.
 */
void BD::GenGmvPawn(GMV& gmv, SQ sqFrom) const
{
	/* pushing pawns */

	CPC cpcFrom = CpcFromSq(sqFrom);
	int dsq = DsqPawnFromCpc(cpcFrom);
	SQ sqTo = sqFrom + dsq;
	if (FIsEmpty(sqTo)) {
		MV mv(sqFrom, sqTo);
		if (sqTo.rank() == RankPromoteFromCpc(cpcFrom))
			AddGmvMvPromotions(gmv, mv);
		else {
			gmv.AppendMv(mv);	// move forward one square
			if (sqFrom.rank() == RankInitPawnFromCpc(cpcFrom)) {
				sqTo += dsq;	// move foreward two squares as first move
				if (FIsEmpty(sqTo))
					gmv.AppendMv(MV(sqFrom, sqTo));
			}
		}
	}

	/* captures */

	GenGmvPawnCapture(gmv, sqFrom, dsq - 1);
	GenGmvPawnCapture(gmv, sqFrom, dsq + 1);
}


/*	BD::AddGmvMvPromotions
 *
 *	When a pawn is pushed to the last rank, adds all the promotion possibilities
 *	to the move list, which includes promotions to queen, rook, knight, and
 *	bishop.
 */
void BD::AddGmvMvPromotions(GMV& gmv, MV mv) const
{
	gmv.AppendMv(mv.SetApcPromote(APC::Queen));
	gmv.AppendMv(mv.SetApcPromote(APC::Rook));
	gmv.AppendMv(mv.SetApcPromote(APC::Bishop));
	gmv.AppendMv(mv.SetApcPromote(APC::Knight));
}


/*	BD::GenGmvPawnCapture
 *
 *	Generates pawn capture moves of pawns on sqFrom in the dsq direction
 */
void BD::GenGmvPawnCapture(GMV& gmv, SQ sqFrom, int dsq) const
{
	assert(ApcFromSq(sqFrom) == APC::Pawn);
	SQ sqTo = sqFrom + dsq;
	if (sqTo.fIsOffBoard())
		return;
	IPC ipcFrom = (*this)(sqFrom);
	CPC cpcFrom = ipcFrom.cpc();
	if (!FIsEmpty(sqTo) && CpcFromSq(sqTo) != cpcFrom) {
		MV mv(sqFrom, sqTo);
		if (sqTo.rank() == RankPromoteFromCpc(cpcFrom))
			AddGmvMvPromotions(gmv, mv);
		else
			gmv.AppendMv(mv);
	}
}



/*	BD::GenGmvKnight
 *
 *	Generates legal moves for the knight at sqFrom. Does not check that
 *	the king ends up in check.
 */
void BD::GenGmvKnight(GMV& gmv, SQ sqFrom) const
{
	IPC ipcKnight = (*this)(sqFrom);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, 33);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, 31);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, 18);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, 14);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, -14);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, -18);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, -31);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKnight, -33);
}


/*	BD::GenGmvBishop
 *
 *	Generates moves for the bishop on square sqFrom
 */
void BD::GenGmvBishop(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Bishop); 
	GenGmvSlide(gmv, sqFrom, 17);
	GenGmvSlide(gmv, sqFrom, 15);
	GenGmvSlide(gmv, sqFrom, -15);
	GenGmvSlide(gmv, sqFrom, -17);
}


void BD::GenGmvRook(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Rook); 
	GenGmvSlide(gmv, sqFrom, 16);
	GenGmvSlide(gmv, sqFrom, 1);
	GenGmvSlide(gmv, sqFrom, -1);
	GenGmvSlide(gmv, sqFrom, -16);
}


/*	BD::GenGmvQueen
 *
 *	Generates  moves for the queen at sqFrom
 */
void BD::GenGmvQueen(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Queen);
	GenGmvSlide(gmv, sqFrom, 17);
	GenGmvSlide(gmv, sqFrom, 16);
	GenGmvSlide(gmv, sqFrom, 15);
	GenGmvSlide(gmv, sqFrom, 1);
	GenGmvSlide(gmv, sqFrom, -1);
	GenGmvSlide(gmv, sqFrom, -15);
	GenGmvSlide(gmv, sqFrom, -16);
	GenGmvSlide(gmv, sqFrom, -17);
}


/*	BD::GenGmvKing
 *
 *	Generates moves for the king at sqFrom. Note: like all the move generators,
 *	this does not check that the king is moving into check.
 */
void BD::GenGmvKing(GMV& gmv, SQ sqFrom) const
{
	IPC ipcKing = (*this)(sqFrom);
	assert(ApcFromSq(sqFrom) == APC::King);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, 17);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, 16);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, 15);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, 1);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, -1);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, -15);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, -16);
	FGenGmvDsq(gmv, sqFrom, sqFrom, ipcKing, -17);
}


/*	BD::GenGmvCastle
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, checks for intermediate 
 *	squares are empty, but does not check the final king destination for in check.
 */
void BD::GenGmvCastle(GMV& gmv, SQ sqKing) const
{
	CPC cpcKing = (*this)(sqKing).cpc();
	BB bbKing = BB(sqKing);
	BB bbAttacked;
	if (FCanCastle(cpcKing, csKing) && !(((bbKing << 1) | (bbKing << 2)) - bbUnoccupied)) {
		bbAttacked = BbAttackedAll(~cpcKing);
		if (!(bbAttacked & (bbKing | (bbKing << 1))))
			gmv.AppendMv(sqKing, sqKing + 2);
	}
	if (FCanCastle(cpcKing, csQueen) && !(((bbKing >> 1) | (bbKing >> 2) | (bbKing >> 3)) - bbUnoccupied)) {
		if (!bbAttacked)
			bbAttacked = BbAttackedAll(~cpcKing);
		if (!(bbAttacked & (bbKing | (bbKing>>1)))) 
			gmv.AppendMv(sqKing, sqKing - 2);
	}
}


/*	BD::GenGmvEnPassant
 *
 *	Generates en passant pawn moves from the pawn at sqFrom
 */
void BD::GenGmvEnPassant(GMV& gmv, SQ sqFrom) const
{
	assert(sqEnPassant != sqNil);
	assert(ApcFromSq(sqFrom) == APC::Pawn);

	/* make sure the piece we're checking is on the correct rank */

	int rankEnPassant = sqEnPassant.rank();
	assert(rankEnPassant == 2 || rankEnPassant == 5);
	if (sqFrom.rank() != (rankEnPassant ^ 1))
		return;

	/* if the file is adjacent to the source square file, then we
	 * have a legit en passant capture */

	int fileEnPassant = sqEnPassant.file();
	int fileFrom = sqFrom.file();
	if (fileEnPassant == fileFrom + 1 || fileEnPassant == fileFrom - 1)
		gmv.AppendMv(sqFrom, sqEnPassant);
}


float BD::VpcFromSq(SQ sq) const
{
	assert(!sq.fIsOffBoard());
	APC apc = ApcFromSq(sq);
	float vpc = mpapcvpc[apc];
	return vpc;
}


/*	BD::VpcTotalFromCpc
 *
 *	Piece value of the entire board for the given color
 */
float BD::VpcTotalFromCpc(CPC cpc) const
{
	float vpc = 0;
	for (int tpc = 0; tpc < tpcPieceMax; tpc++) {
		SQ sq = mptpcsq[cpc][tpc];
		if (!sq.fIsNil())
			vpc += VpcFromSq(sq);
	}
	return vpc;
}


/*	BD::Validate
 *
 *	Checks the board state for internal consistency
 */
#ifndef NDEBUG
void BD::Validate(void) const
{
	if (!fValidate)
		return;

	/* check the two mapping arrays are consistent, and make sure apc is legal
	   for the tpc in place */

	for (int rank = 0; rank < rankMax; rank++) {
		int file;
		for (file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			IPC ipc = (*this)(sq);
			if (ipc == ipcEmpty)
				continue;

			assert(SqFromIpc(ipc) == sq);
			ValidateBB(ipc, sq);

			TPC tpcPc = ipc.tpc();
			APC apc = ipc.apc();
			switch (tpcPc) {
			case tpcQueenRook:
			case tpcKingRook:
				assert(apc == APC::Rook);
				break;
			case tpcQueenKnight:
			case tpcKingKnight:
				assert(apc == APC::Knight);
				break;
			case tpcQueenBishop:
			case tpcKingBishop:
				assert(apc == APC::Bishop);
				break;
			case tpcQueen:
				assert(apc == APC::Queen);
				break;
			case tpcKing:
				assert(apc == APC::King);
				break;
			default:
				assert(tpcPc >= tpcPawnFirst && tpcPc < tpcPawnLim);
				assert(apc == APC::Pawn || apc == APC::Queen || apc == APC::Rook || apc == APC::Bishop || apc == APC::Knight);
				break;
			}
		}
		for (file = 8; file < 16; file++) {
			assert(mpsqipc[SQ(rank, file)] == ipcNil);
		}
	}

	/* union of black, white, and empty bitboards should be all squares */
	assert((mpcpcbb[CPC::White] | mpcpcbb[CPC::Black] | bbUnoccupied) == bbAll);
	/* white, black, and empty are disjoint */
	assert((mpcpcbb[CPC::White] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[CPC::Black] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[CPC::White] & mpcpcbb[CPC::Black]) == bbNone);

	/* check for valid castle situations */

	if (csCur & csWhiteKing) {
		assert((*this)(SQ(0, fileKing)).apc() == APC::King);
		assert((*this)(SQ(0, fileKing)).cpc() == CPC::White);
		assert((*this)(SQ(0, fileKingRook)).apc() == APC::Rook);
		assert((*this)(SQ(0, fileKingRook)).cpc() == CPC::White);
	}
	if (csCur & csBlackKing) {
		assert((*this)(SQ(7, fileKing)).apc() == APC::King);
		assert((*this)(SQ(7, fileKing)).cpc() == CPC::Black);
		assert((*this)(SQ(7, fileKingRook)).apc() == APC::Rook);
		assert((*this)(SQ(7, fileKingRook)).cpc() == CPC::Black);
	}
	if (csCur & csWhiteQueen) {
		assert((*this)(SQ(0, fileKing)).apc() == APC::King);
		assert((*this)(SQ(0, fileKing)).cpc() == CPC::White);
		assert((*this)(SQ(0, fileQueenRook)).apc() == APC::Rook);
		assert((*this)(SQ(0, fileQueenRook)).cpc() == CPC::White);
	}
	if (csCur & csBlackQueen) {
		assert((*this)(SQ(7, fileKing)).apc() == APC::King);
		assert((*this)(SQ(7, fileKing)).cpc() == CPC::Black);
		assert((*this)(SQ(7, fileQueenRook)).apc() == APC::Rook);
		assert((*this)(SQ(7, fileQueenRook)).cpc() == CPC::Black);
	}

	/* check for valid en passant situations */

	if (!sqEnPassant.fIsNil()) {
	}

	/* make sure hash is kept accurate */

	assert(habd == genhabd.HabdFromBd(*this));

}


void BD::ValidateBB(IPC ipc, SQ sq) const
{
	for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc)
		for (APC apc = APC::Null; apc < APC::ActMax; ++apc) {
			if (cpc == ipc.cpc() && apc == ipc.apc()) {
				assert(mpapcbb[cpc][apc].fSet(sq));
				assert(mpcpcbb[cpc].fSet(sq));
				assert(!bbUnoccupied.fSet(sq));
			}
			else {
				assert(!mpapcbb[cpc][apc].fSet(sq));
			}
		}

}

#endif


/*
 *
 *	BDG  class implementation
 * 
 *	This is the implementation of the full game board class, which is
 *	the board along with move history.
 * 
 */


/*	BDG::BDG
 *	
 *	Constructor for the game board.
 */
BDG::BDG(void) : gs(GS::Playing), imvCur(-1), imvPawnOrTakeLast(-1)
{
}


/*	BDG::BDG
 *
 *	Constructor for initializing a board with a FEN board state string.
 */
BDG::BDG(const wchar_t* szFEN)
{
	InitGame(szFEN);
}


void BDG::InitGame(const wchar_t* szFEN)
{
	const wchar_t* sz = szFEN;
	InitFENPieces(sz);
	InitFENSideToMove(sz);
	InitFENCastle(sz);
	InitFENEnPassant(sz);
	InitFENHalfmoveClock(sz);
	InitFENFullmoveCounter(sz);

	vmvGame.clear();
	imvCur = -1;
	imvPawnOrTakeLast = -1;
	SetGs(GS::Playing);
}



void BDG::InitFENHalfmoveClock(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	SkipToSpace(sz);
}


void BDG::InitFENFullmoveCounter(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	SkipToSpace(sz);
}


/*	BDG::SzFEN
 *
 *	Returns the FEN string of the given board
 */
wstring BDG::SzFEN(void) const
{
	wstring sz;

	/* write the squares and pieces */

	for (int rank = rankMax-1; rank >= 0; rank--) {
		int csqEmpty = 0;
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			if ((*this)(sq).fIsEmpty())
				csqEmpty++;
			else {
				if (csqEmpty > 0) {
					sz += to_wstring(csqEmpty);
					csqEmpty = 0;
				}
				wchar_t ch = L' ';
				switch (ApcFromSq(sq)) {
				case APC::Pawn: ch = L'P'; break;
				case APC::Knight: ch = L'N'; break;
				case APC::Bishop: ch = L'B'; break;
				case APC::Rook: ch = L'R'; break;
				case APC::Queen: ch = L'Q'; break;
				case APC::King: ch = L'K'; break;
				default: assert(false); break;
				}
				if (CpcFromSq(sq) == CPC::Black)
					ch += L'a' - L'A';
				sz += ch;
			}
		}
		if (csqEmpty > 0)
			sz += to_wstring(csqEmpty);
		sz += L'/';
	}

	/* piece to move */

	sz += L' ';
	sz += cpcToMove == CPC::White ? L'w' : L'b';
	
	/* castling */

	sz += L' ';
	if (csCur == 0)
		sz += L'-';
	else {
		if (csCur & csWhiteKing)
			sz += L'K';
		if (csCur & csWhiteQueen)
			sz += L'Q';
		if (csCur & csBlackKing)
			sz += L'k';
		if (csCur & csBlackQueen)
			sz += L'q';
	}

	/* en passant */

	sz += L' ';
	if (sqEnPassant.fIsNil())
		sz += L'-';
	else {
		sz += (L'a' + sqEnPassant.file());
		sz += (L'1' + sqEnPassant.rank());
	}

	/* halfmove clock */

	sz += L' ';
	sz += to_wstring(imvCur - imvPawnOrTakeLast);

	/* fullmove number */

	sz += L' ';
	sz += to_wstring(1 + imvCur/2);

	return sz;
}


/*	BDG::MakeMv
 *
 *	Make a move on the board, and keeps the move list for the game. Caller is
 *	responsible for testing for game over.
 */
void BDG::MakeMv(MV mv)
{
	assert(!mv.fIsNil());

	/* make the move and save the move in the move list */

	MakeMvSq(mv);
	if (++imvCur == vmvGame.size())
		vmvGame.push_back(mv);
	else if (mv != vmvGame[imvCur]) {
		vmvGame[imvCur] = mv;
		/* all moves after this in the move list are now invalid */
		for (size_t imv = imvCur + 1; imv < vmvGame.size(); imv++)
			vmvGame[imv] = MV();
	}

	/* keep track of last pawn move or capture, which is used to detect draws */

	if (ApcFromSq(mv.sqTo()) == APC::Pawn || mv.fIsCapture())
		imvPawnOrTakeLast = imvCur;
}


/*	BDG::SzMoveAndDecode
 *
 *	Makes a move and returns the decoded text of the move. This is the only
 *	way to get postfix marks on the move text, like '+' for check, or '#' for
 *	checkmate.
 */
wstring BDG::SzMoveAndDecode(MV mv)
{
	wstring sz = SzDecodeMv(mv, true);
	CPC cpc = CpcFromSq(mv.sqFrom());
	MakeMv(mv);
	if (FSqAttacked((*this)(tpcKing, ~cpc), cpc))
		sz += L'+';
	return sz;
}


/*	BDG::UndoMv
 *
 *	Undoes the last made move at imvCur. Caller is responsible for resetting game
 *	over state
 */
void BDG::UndoMv(void)
{
	if (imvCur < 0)
		return;
	if (imvCur == imvPawnOrTakeLast) {
		/* scan backwards looking for pawn moves or captures */
		for (imvPawnOrTakeLast = imvCur-1; imvPawnOrTakeLast >= 0; imvPawnOrTakeLast--)
			if (ApcFromSq(vmvGame[imvPawnOrTakeLast].sqFrom()) == APC::Pawn || 
					vmvGame[imvPawnOrTakeLast].fIsCapture())
				break;
	}
	UndoMvSq(vmvGame[imvCur--]);
	assert(imvCur >= -1);
}


/*	BDG::RedoMv
 *
 *	Redoes that last undone move, which will be at imvCur+1. Caller is responsible
 *	for testing for game-over state afterwards.
 */
void BDG::RedoMv(void)
{
	if (imvCur+1 >= (int)vmvGame.size() || vmvGame[imvCur+1].fIsNil())
		return;
	imvCur++;
	MV mv = vmvGame[imvCur];
	if (ApcFromSq(mv.sqFrom()) == APC::Pawn || mv.fIsCapture())
		imvPawnOrTakeLast = imvCur;
	MakeMvSq(mv);
}


/*	BDG::GsTestGameOver
 *
 *	Tests for the game in an end state. Returns the new state. Takes the legal move
 *	list for the current to-move player and the rule struture.
 */
GS BDG::GsTestGameOver(const GMV& gmv, const RULE& rule) const
{
	if (gmv.cmv() == 0) {
		if (FInCheck(cpcToMove))
			return cpcToMove == CPC::White ? GS::WhiteCheckMated : GS::BlackCheckMated;
		else
			return GS::StaleMate;
	}
	else {
		/* check for draw circumstances */
		if (FDrawDead())
			return GS::DrawDead;
		if (FDraw3Repeat(rule.CmvRepeatDraw()))
			return GS::Draw3Repeat;
		if (FDraw50Move(50))
			return GS::Draw50Move;
	}
	return GS::Playing;
}


void BDG::SetGameOver(const GMV& gmv, const RULE& rule)
{
	SetGs(GsTestGameOver(gmv, rule));
}


/*	BDG::FDrawDead
 *
 *	Returns true if we're in a board state where no one can force checkmate on the
 *	other player.
 */
bool BDG::FDrawDead(void) const
{
	/* keep total piece count for the color at apcNull, and keep seperate counts 
	   for the different color square bishops */

	int mpapccapc[CPC::ColorMax][APC::ActMax2];	
	memset(mpapccapc, 0, sizeof(mpapccapc));

	for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
		for (int tpc = 0; tpc < tpcPieceMax; tpc++) {
			SQ sq = mptpcsq[cpc][tpc];
			if (sq == sqNil)
				continue;
			APC apc = ApcFromSq(sq);
			if (apc == APC::Bishop)
				apc += (sq & 1) * (APC::Bishop2 - APC::Bishop);
			else if (apc == APC::Rook || apc == APC::Queen || apc == APC::Pawn)	// rook, queen, or pawn, not dead
				return false;
			mpapccapc[cpc][apc]++;
			if (++mpapccapc[cpc][APC::Null] > 2)	// if total pieces more than 2, not dead
				return false;; 
		}
	}

	/* if we reach this point, only kings, bishops and knights are on the
	   board, and there are at most 2 pieces per side */

	assert(mpapccapc[CPC::White][APC::King] == 1 && mpapccapc[CPC::Black][APC::King] == 1);
	assert(mpapccapc[CPC::White][APC::Null] <= 2 && mpapccapc[CPC::Black][APC::Null] <= 2);

	/* this picks off K vs. K, K vs. K-N, and K vs. K-B */

	if (mpapccapc[CPC::White][APC::Null] == 1 || 
			mpapccapc[CPC::Black][APC::Null] == 1)
		return true;

	/* The other draw case is K-B vs. K-B with bishops on same color */

	assert(mpapccapc[CPC::White][APC::Null] == 2 && mpapccapc[CPC::Black][APC::Null] == 2);
	if ((mpapccapc[CPC::White][APC::Bishop] && mpapccapc[CPC::Black][APC::Bishop]) ||
			(mpapccapc[CPC::White][APC::Bishop2] && mpapccapc[CPC::Black][APC::Bishop2]))
		return true;

	/*  otherwise forcing checkmate is still possible */

	return false;
}


/*	BDG::FDraw3Repeat
 *
 *	Returns true on the draw condition when we've had the exact same board position
 *	for 3 times in a single game, where exact board position means same player to move,
 *	all pieces in the same place, castle state is the same, and en passant possibility
 *	is the same.
 */
bool BDG::FDraw3Repeat(int cbdDraw) const
{
	if (cbdDraw == 0)
		return false;
	if (imvCur - imvPawnOrTakeLast < (cbdDraw-1) * 2 * 2)
		return false;
	BD bd = *this;
	int cbdSame = 1;
	bd.UndoMvSq(vmvGame[imvCur]);
	bd.UndoMvSq(vmvGame[imvCur - 1]);
	for (int imv = imvCur - 2; imv >= imvPawnOrTakeLast + 2; imv -= 2) {
		bd.UndoMvSq(vmvGame[imv]);
		bd.UndoMvSq(vmvGame[imv - 1]);
		if (bd == *this) {
			if (++cbdSame >= cbdDraw)
				return true;
			imv -= 2;
			/* let's go ahead and back up two more moves here, since we can't possibly 
			   match the next position */
			if (imv < imvPawnOrTakeLast + 2)
				break;
			bd.UndoMvSq(vmvGame[imv]);
			bd.UndoMvSq(vmvGame[imv - 1]);
			assert(bd != *this);
		}
	}
	return false;
}


/*	BDG::FDraw50Move
 *
 *	If we've gone 50 moves (black and white both gone 50 moves each) without a pawn move
 *	or a capture. 
 */
bool BDG::FDraw50Move(int cmvDraw) const
{
	return imvCur - imvPawnOrTakeLast >= cmvDraw * 2;
}


/*	BDG::SetGs
 *
 *	Sets the game state.
 */
void BDG::SetGs(GS gs)
{
	this->gs = gs;
}

