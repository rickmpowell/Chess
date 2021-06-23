/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */


#include "bd.h"



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
	cs = ((csKing | csQueen) << (int)CPC::White) | ((csKing | csQueen) << (int)CPC::Black); 
	sqEnPassant = sqNil;
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
			mpapcbb[cpc][apc] = 0;
	}
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
	return cs == bd.cs && sqEnPassant == bd.sqEnPassant;
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
void BD::InitFENPieces(const WCHAR*& szFEN)
{
	SetEmpty();

	/* parse the line */

	int rank = rankMax - 1;
	SQ sq = SQ(rank, 0);
	const WCHAR* pch;
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
 *	board mappings are correct. Makes an attempt to set unmoved bits
 *	correctly.
 * 
 *	Promoted pawns are the real can of worms with this process.
 */
void BD::AddPieceFEN(SQ sq, TPC tpc, CPC cpc, APC apc)
{
	if (mptpcsq[cpc][tpc] != sqNil) {
		switch (apc) {
		/* TODO: two kings is an error */
		case APC::King: assert(false); return;
		default:
			/* try piece on other side; otherwise it's a promoted pawn */
			if (mptpcsq[cpc][tpc = TpcOpposite(tpc)] == sqNil)
				break;
			[[fallthrough]];
		case APC::Pawn:
		case APC::Queen:
			tpc = TpcUnusedPawn(cpc);
			break;
		}
	}

	mpsqipc[sq] = IPC(tpc, cpc, apc);
	mptpcsq[cpc][tpc] = sq;
	SetBB((*this)(sq), sq);
}


void BD::SkipToSpace(const WCHAR*& sz)
{
	while (*sz && *sz != L' ')
		sz++;
}


void BD::SkipToNonSpace(const WCHAR*& sz)
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
		case 'K': SetCastle(CPC::White, csKing); break;
		case 'Q': SetCastle(CPC::White, csQueen); break;
		case 'k': SetCastle(CPC::Black, csKing); break;
		case 'q': SetCastle(CPC::Black, csQueen); break;
		case '-': break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
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
	APC apcFrom = ipcFrom.apc();

	/* store undo information in the mv */

	mv.SetCsEp(CsPackColor(cs, ipcFrom.cpc()), sqEnPassant);
	SQ sqTake = sqTo;
	if (apcFrom == APC::Pawn) {
		if (sqTake == sqEnPassant)
			sqTake = SQ(sqTo.rank() ^ 1, sqEnPassant.file());
	}

	/* captures. if we're taking a rook, we can't castle to that rook */

	IPC ipcTake = (*this)(sqTake);
	if (!ipcTake.fIsEmpty()) {
		mv.SetCapture(ApcFromSq(sqTake), TpcFromSq(sqTake));
		SqFromIpc(ipcTake) = sqNil;
		ClearBB(ipcTake, sqTake);
		switch (ipcTake.tpc()) {
		case tpcKingRook: 
			ClearCastle(ipcTake.cpc(), csKing); 
			break;
		case tpcQueenRook: 
			ClearCastle(ipcTake.cpc(), csQueen); 
			break;
		default: 
			break;
		}
	}

	/* move the pieces */

	mpsqipc[sqFrom] = ipcEmpty;
	ClearBB(ipcFrom, sqFrom);
	mpsqipc[sqTo] = ipcFrom;
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
			mpsqipc[sqTake] = ipcEmpty;
		}
		else if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			IPC ipcPromote = IPC(ipcFrom.tpc(), ipcFrom.cpc(), mv.apcPromote());
			mpsqipc[sqTo] = ipcPromote;
			ClearBB(ipcFrom, sqTo);
			SetBB(ipcPromote, sqTo);
		} 
		break;

	case APC::King:
		ClearCastle(ipcFrom.cpc(), csKing|csQueen);
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
			mpsqipc[sqRookFrom] = ipcEmpty;
			ClearBB(ipcRook, sqRookFrom);
			mpsqipc[sqRookTo] = ipcRook;
			SqFromIpc(ipcRook) = sqRookTo;
			SetBB(ipcRook, sqRookTo);
		}
		break;

	case APC::Rook:
		ClearCastle(ipcFrom.cpc(), ipcFrom.tpc() == tpcQueenRook ? csQueen : csKing);
		break;

	default:
		break;
	}

	sqEnPassant = sqNil;
Done:
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

	/* restore castle and en passant state. Castle state can only be added
	   on an undo, so it's OK to or those bits back in. Note that the en 
	   passant state only saves the file of the en passant captureable pawn 
	   (due to lack of available bits in the MV). */

	cs |= CsUnpackColor(mv.csPrev(), cpcMove);
	if (mv.fEpPrev())
		sqEnPassant = SQ(cpcMove == CPC::White ? 5 : 2, mv.fileEpPrev());
	else
		sqEnPassant = sqNil;

	/* put piece back in source square, undoing any pawn promotion that might
	   have happened */

	if (mv.apcPromote() != APC::Null) {
		ClearBB(ipcMove, sqTo);
		ipcMove = IpcSetApc(ipcMove, APC::Pawn);
	}
	mpsqipc[sqFrom] = ipcMove;
	SetBB(ipcMove, sqFrom);
	ClearBB(ipcMove, sqTo);
	SqFromIpc(ipcMove) = sqFrom;
	APC apcMove = ipcMove.apc();	// get the type of moved piece after we've undone promotion

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	APC apcCapt = mv.apcCapture();
	if (apcCapt == APC::Null) 
		mpsqipc[sqTo] = ipcEmpty;
	else {
		IPC ipcTake = IPC(mv.tpcCapture(), ~cpcMove, apcCapt);
		SQ sqTake = sqTo;
		if (sqTake == sqEnPassant) {
			/* capture into the en passant square must be en passant capture
			   pawn x pawn */
			assert(ApcFromSq(sqTo) == APC::Pawn && apcCapt == APC::Pawn);
			sqTake = SQ(sqEnPassant.rank() + cpcMove*2 - 1, sqEnPassant.file());
			mpsqipc[sqTo] = ipcEmpty;
		}
		mpsqipc[sqTake] = ipcTake;
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
			mpsqipc[sqTo - 2] = ipcRook;
			mpsqipc[sqTo + 1] = ipcEmpty;
			SqFromIpc(ipcRook) = sqTo - 2;
		}
		else if (dfile > 1) { /* king side castle */
			IPC ipcRook = (*this)(sqTo - 1);
			ClearBB(ipcRook, sqTo - 1);
			SetBB(ipcRook, sqTo + 1);
			mpsqipc[sqTo + 1] = ipcRook;
			mpsqipc[sqTo - 1] = ipcEmpty;
			SqFromIpc(ipcRook) = sqTo + 1;
		}
	}

	Validate();
}


/*	BD::GenRgmv
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenRgmv(GMV& gmv, CPC cpcMove, RMCHK rmchk) const
{
	Validate();
	GenRgmvColor(gmv, cpcMove, false);
	if (rmchk == RMCHK::Remove)
		RemoveInCheckMoves(gmv, cpcMove);
}


void BD::GenRgmvQuiescent(GMV& gmv, CPC cpcMove, RMCHK rmchk) const
{
	GenRgmvColor(gmv, cpcMove, true);
	if (rmchk == RMCHK::Remove) {
		RemoveInCheckMoves(gmv, cpcMove);
		RemoveQuiescentMoves(gmv, cpcMove);
	}
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


bool BD::FMvIsQuiescent(MV mv, CPC cpcMove) const
{
	return FIsEmpty(mv.sqTo()) ||
		VpcFromSq(mv.sqFrom()) + 0.5 >= VpcFromSq(mv.sqTo());
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check. Does not require ComputeAttacked,
 *	and does a complete scan of the opponent pieces to find checks.
 */
bool BD::FInCheck(CPC cpc) const
{
	return FSqAttacked(~cpc, (*this)(tpcKing, cpc));
}


/*	BD::FSqAttacked
 *
 *	Returns true if sqAttacked is attacked by some piece of the color cpcBy. The piece
 *	on sqAttacked is not considered to be attacking sqAttacked.
 */
bool BD::FSqAttacked(CPC cpcBy, SQ sqAttacked) const
{
	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sq = (*this)(tpc, cpcBy);
		if (sq.fIsNil())
			continue;
		int dsq = sq - sqAttacked;
		if (dsq == 0)
			continue;
		APC apc = ApcFromSq(sq);
		switch (apc) {
		case APC::Pawn:
			if (cpcBy == CPC::White)
				dsq = -dsq;
			if (dsq == 17 || dsq == 15)
				return true;
			break;
		case APC::Knight:
			if (dsq < 0)
				dsq = -dsq;
			if (dsq == 33 || dsq == 31 || dsq == 18 || dsq == 14)
				return true;
			break;
		case APC::Queen:
			if (sq.rank() == sqAttacked.rank()) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 1 : -1))
					return true;
			}
			else if (sq.file() == sqAttacked.file()) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 16 : -16))
					return true;
			}
			[[fallthrough]];
		case APC::Bishop:
			if (dsq % 17 == 0) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 17 : -17))
					return true;
			}
			else if (dsq % 15 == 0) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 15 : -15))
					return true;
			}
			break;
		case APC::Rook:
			if (sq.rank() == sqAttacked.rank()) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 1 : -1))
					return true;
			}
			else if (sq.file() == sqAttacked.file()) {
				if (FDsqAttack(sqAttacked, sq, sqAttacked > sq ? 16 : -16))
					return true;
			}
			break;
		case APC::King:
			if (dsq < 0)
				dsq = -dsq;
			if (dsq == 17 || dsq == 16 || dsq == 15 || dsq == 1)
				return true;
			break;
		default:
			break;
		}
	}
	return false;
}


/*	BD::GenRgmvColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenRgmvColor(GMV& gmv, CPC cpcMove, bool fForAttack) const
{
	gmv.Clear();

	/* go through each piece */

	for (TPC tpc = tpcPieceMin; tpc < tpcPieceMax; ++tpc) {
		SQ sqFrom = mptpcsq[cpcMove][tpc];
		if (sqFrom == sqNil)
			continue;
		assert(!FIsEmpty(sqFrom));
		assert((*this)(sqFrom).cpc() == cpcMove);
	
		switch (ApcFromSq(sqFrom)) {
		case APC::Pawn:
			GenRgmvPawn(gmv, sqFrom);
			if (sqEnPassant != sqNil)
				GenRgmvEnPassant(gmv, sqFrom);
			break;
		case APC::Knight:
			GenRgmvKnight(gmv, sqFrom);
			break;
		case APC::Bishop:
			GenRgmvBishop(gmv, sqFrom);
			break;
		case APC::Rook:
			GenRgmvRook(gmv, sqFrom);
			break;
		case APC::Queen:
			GenRgmvQueen(gmv, sqFrom);
			break;
		case APC::King:
			GenRgmvKing(gmv, sqFrom);
			if (!fForAttack)
				GenRgmvCastle(gmv, sqFrom);
			break;
		default:
			assert(false);
			break;
		}
	}
}


/*	BD::GenRgmvPawn
 *
 *	Generates legal moves (without check tests) for a pawn
 *	in the sqFrom square.
 */
void BD::GenRgmvPawn(GMV& gmv, SQ sqFrom) const
{
	/* pushing pawns */

	CPC cpcFrom = CpcFromSq(sqFrom);
	int dsq = DsqPawnFromCpc(cpcFrom);
	SQ sqTo = sqFrom + dsq;
	if (FIsEmpty(sqTo)) {
		MV mv(sqFrom, sqTo);
		if (sqTo.rank() == RankPromoteFromCpc(cpcFrom))
			AddRgmvMvPromotions(gmv, mv);
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

	GenRgmvPawnCapture(gmv, sqFrom, dsq - 1);
	GenRgmvPawnCapture(gmv, sqFrom, dsq + 1);
}


/*	BD::AddRgmvMvPromotions
 *
 *	When a pawn is pushed to the last rank, adds all the promotion possibilities
 *	to the move list, which includes promotions to queen, rook, knight, and
 *	bishop.
 */
void BD::AddRgmvMvPromotions(GMV& gmv, MV mv) const
{
	gmv.AppendMv(mv.SetApcPromote(APC::Queen));
	gmv.AppendMv(mv.SetApcPromote(APC::Rook));
	gmv.AppendMv(mv.SetApcPromote(APC::Bishop));
	gmv.AppendMv(mv.SetApcPromote(APC::Knight));
}


/*	BD::GenRgmvPawnCapture
 *
 *	Generates pawn capture moves of pawns on sqFrom in the dsq direction
 */
void BD::GenRgmvPawnCapture(GMV& gmv, SQ sqFrom, int dsq) const
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
			AddRgmvMvPromotions(gmv, mv);
		else
			gmv.AppendMv(mv);
	}
}



/*	BD::GenRgmvKnight
 *
 *	Generates legal moves for the knight at sqFrom. Does not check that
 *	the king ends up in check.
 */
void BD::GenRgmvKnight(GMV& gmv, SQ sqFrom) const
{
	static int rgdsq[] = { 33, 31, 18, 14, -14, -18, -31, -33 };
	for (int idsq = 0; idsq < CArray(rgdsq); idsq++)
		FGenRgmvDsq(gmv, sqFrom, sqFrom, (*this)(sqFrom), rgdsq[idsq]);
}


/*	BD::GenRgmvBishop
 *
 *	Generates moves for the bishop on square sqFrom
 */
void BD::GenRgmvBishop(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Bishop); 
	GenRgmvSlide(gmv, sqFrom, 17);
	GenRgmvSlide(gmv, sqFrom, 15);
	GenRgmvSlide(gmv, sqFrom, -15);
	GenRgmvSlide(gmv, sqFrom, -17);
}


void BD::GenRgmvRook(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Rook); 
	GenRgmvSlide(gmv, sqFrom, 16);
	GenRgmvSlide(gmv, sqFrom, 1);
	GenRgmvSlide(gmv, sqFrom, -1);
	GenRgmvSlide(gmv, sqFrom, -16);
}


/*	BD::GenRgmvQueen
 *
 *	Generates  moves for the queen at sqFrom
 */
void BD::GenRgmvQueen(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Queen);
	GenRgmvSlide(gmv, sqFrom, 17);
	GenRgmvSlide(gmv, sqFrom, 16);
	GenRgmvSlide(gmv, sqFrom, 15);
	GenRgmvSlide(gmv, sqFrom, 1);
	GenRgmvSlide(gmv, sqFrom, -1);
	GenRgmvSlide(gmv, sqFrom, -15);
	GenRgmvSlide(gmv, sqFrom, -16);
	GenRgmvSlide(gmv, sqFrom, -17);
}


/*	BD::GenRgmvKing
 *
 *	Generates moves for the king at sqFrom. Note: like all the move generators,
 *	this does not check that the king is moving into check.
 */
void BD::GenRgmvKing(GMV& gmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::King);
	static int rgdsq[] = { 17, 16, 15, 1, -1, -15, -16, -17 };
	for (int idsq = 0; idsq < CArray(rgdsq); idsq++)
		FGenRgmvDsq(gmv, sqFrom, sqFrom, (*this)(sqFrom), rgdsq[idsq]);
}



/*	BD::GenRgmvCastle
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, but does not check the final 
 *	king destination.
 */
void BD::GenRgmvCastle(GMV& gmv, SQ sqKing) const
{
	/* this code is a little contorted in order to avoid calling FSqAttacked (which 
	   is an expensive test) as much as possible. */

	CPC cpcKing = (*this)(sqKing).cpc();
	CPC cpcOpp = ~cpcKing;
	bool fMaybeInCheck = true;	
	if (FCanCastle(cpcKing, csKing) && FIsEmpty(sqKing+1) && FIsEmpty(sqKing+2)) {
		if (FSqAttacked(cpcOpp, sqKing))
			return;
		fMaybeInCheck = false;
		if (!FSqAttacked(cpcOpp, sqKing+1))
			gmv.AppendMv(sqKing, sqKing+2);
	}
	if (FCanCastle(cpcKing, csQueen) && FIsEmpty(sqKing-1) && FIsEmpty(sqKing-2) && FIsEmpty(sqKing-3)) {
		if (fMaybeInCheck && FSqAttacked(cpcOpp, sqKing)) 
			return;
		if (!FSqAttacked(cpcOpp, sqKing-1))
			gmv.AppendMv(sqKing, sqKing-2);
	}
}


/*	BD::GenRgmvEnPassant
 *
 *	Generates en passant pawn moves from the pawn at sqFrom
 */
void BD::GenRgmvEnPassant(GMV& gmv, SQ sqFrom) const
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

	/* check for valid castle situations */
	/* check for valid en passant situations */
}

void BD::ValidateBB(IPC ipc, SQ sq) const
{
	for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) 
		for (APC apc = APC::Null; apc < APC::ActMax; ++apc) {
			if (cpc == ipc.cpc() && apc == ipc.apc()) {
				assert(mpapcbb[cpc][apc].fSet(sq));
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
BDG::BDG(void) : gs(GS::Playing), cpcToMove(CPC::White), imvCur(-1), imvPawnOrTakeLast(-1)
{
}


/*	BDG::BDG
 *
 *	Constructor for initializing a board with a FEN board state string.
 */
BDG::BDG(const WCHAR* szFEN)
{
	InitFEN(szFEN);
}


void BDG::NewGame(void)
{
	InitFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	vmvGame.clear();
	imvCur = -1;
	imvPawnOrTakeLast = -1;
	SetGs(GS::Playing);
}


void BDG::InitFEN(const wchar_t* szFEN)
{
	const wchar_t* sz = szFEN;
	InitFENPieces(sz);
	InitFENSideToMove(sz);
	InitFENCastle(sz);
	InitFENEnPassant(sz);
	InitFENHalfmoveClock(sz);
	InitFENFullmoveCounter(sz);
}


void BDG::InitFENSideToMove(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	cpcToMove = CPC::White;
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'w': cpcToMove = CPC::White ; break;
		case 'b': cpcToMove = CPC::Black; break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
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
	if (cs == 0)
		sz += L'-';
	else {
		if (cs & csWhiteKing)
			sz += L'K';
		if (cs & csWhiteQueen)
			sz += L'Q';
		if (cs & csBlackKing)
			sz += L'k';
		if (cs & csBlackQueen)
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


/*	BDG::GenRgmv
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the 
 *	king in check.
 */
void BDG::GenRgmv(GMV& gmv, RMCHK rmchk) const
{
	BD::GenRgmv(gmv, cpcToMove, rmchk);
}


void BDG::GenRgmvQuiescent(GMV& gmv, RMCHK rmchk) const
{
	BD::GenRgmvQuiescent(gmv, cpcToMove, rmchk);
}


bool BDG::FMvIsQuiescent(MV mv) const
{
	return BD::FMvIsQuiescent(mv, cpcToMove);
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

	/* other player's move */

	cpcToMove = ~cpcToMove;
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
	if (FSqAttacked(cpc, mptpcsq[~cpc][tpcKing])) {
		sz += L'+';
	}
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
	cpcToMove = ~cpcToMove;
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
	cpcToMove = ~cpcToMove;
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

