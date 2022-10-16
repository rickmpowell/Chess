/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */

#include "bd.h"


mt19937 rgen(7724322UL);

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
	rghabdPiece[0][0] = 0;	// shut up the compiler warning about uninitialized array
	for (SQ sq = 0; sq < sqMax; sq++)
		for (PC pc = 0; pc < pcMax; pc++)
			rghabdPiece[sq][pc] = ((uint64_t)grfDist(rgen) << 32) | (uint64_t)grfDist(rgen);

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
	for (SQ sq = 0; sq < sqMax; sq++) {
		PC pc = bd.PcFromSq(sq);
		if (pc.apc() != APC::Null)
			habd ^= rghabdPiece[sq][pc];
		}

	/* castle state */

	habd ^= rghabdCastle[bd.csCur];

	/* en passant state */

	if (!bd.sqEnPassant.fIsNil())
		habd ^= rghabdEnPassant[bd.sqEnPassant.file()];

	/* current side to move */

	if (bd.cpcToMove == CPC::Black)
		habd ^= habdMove;

	return habd;
}


/*
 *
 *	mpbb
 *
 *	We keep bitboards for every square and every direction of squares that are
 *	attacked from that square.
 * 
 */


MPBB mpbb;

MPBB::MPBB(void)
{
	/* precompute all the attack bitboards for sliders, kings, and knights */

	for (int rank = 0; rank < rankMax; rank++)
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);

			/* slides */
			
			for (int drank = -1; drank <= 1; drank++)
				for (int dfile = -1; dfile <= 1; dfile++) {
					if (drank == 0 && dfile == 0)
						continue;
					DIR dir = DirFromDrankDfile(drank, dfile);
					for (int rankMove = rank + drank, fileMove = file + dfile;
							((rankMove | fileMove) & ~7) == 0; rankMove += drank, fileMove += dfile)
						mpsqdirbbSlide[sq][(int)dir] |= BB(SQ(rankMove, fileMove));
				}

			/* knights */

			BB bb(sq);
			BB bb1 = BbWestOne(bb) | BbEastOne(bb);
			BB bb2 = BbWestTwo(bb) | BbEastTwo(bb);
			mpsqbbKnight[sq] = BbNorthTwo(bb1) | BbSouthTwo(bb1) | BbNorthOne(bb2) | BbSouthOne(bb2);

			/* kings */

			bb1 = BbEastOne(bb) | BbWestOne(bb);
			mpsqbbKing[sq] = bb1 | BbNorthOne(bb1 | bb) | BbSouthOne(bb1 | bb);
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


BD::BD(void)
{
	SetEmpty();
	Validate();
}


void BD::SetEmpty(void) noexcept
{
	for (PC pc = 0; pc < pcMax; ++pc)
		mppcbb[pc] = bbNone;
	mpcpcbb[CPC::White] = bbNone;
	mpcpcbb[CPC::Black] = bbNone;
	bbUnoccupied = bbAll;

	csCur = 0;
	cpcToMove = CPC::White;
	sqEnPassant = SQ();
	
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
bool BD::operator==(const BD& bd) const noexcept
{
	for (PC pc = 0; pc < pcMax; ++pc)
		if (mppcbb[pc] != bd.mppcbb[pc])
			return false;
	return csCur == bd.csCur && sqEnPassant == bd.sqEnPassant && cpcToMove == bd.cpcToMove;
}


bool BD::operator!=(const BD& bd) const noexcept
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
		case 'p': AddPieceFEN(sq++, PC(CPC::Black, APC::Pawn)); break;
		case 'r': AddPieceFEN(sq++, PC(CPC::Black, APC::Rook)); break;
		case 'n': AddPieceFEN(sq++, PC(CPC::Black, APC::Knight)); break;
		case 'b': AddPieceFEN(sq++, PC(CPC::Black, APC::Bishop)); break;
		case 'q': AddPieceFEN(sq++, PC(CPC::Black, APC::Queen)); break;
		case 'k': AddPieceFEN(sq++, PC(CPC::Black, APC::King)); break;
		case 'P': AddPieceFEN(sq++, PC(CPC::White, APC::Pawn)); break;
		case 'R': AddPieceFEN(sq++, PC(CPC::White, APC::Rook)); break;
		case 'N': AddPieceFEN(sq++, PC(CPC::White, APC::Knight)); break;
		case 'B': AddPieceFEN(sq++, PC(CPC::White, APC::Bishop)); break;
		case 'Q': AddPieceFEN(sq++, PC(CPC::White, APC::Queen)); break;
		case 'K': AddPieceFEN(sq++, PC(CPC::White, APC::King)); break;
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
void BD::AddPieceFEN(SQ sq, PC pc)
{
	SetBB(pc, sq);
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
	SetEnPassant(SQ());
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
		SetEnPassant(SQ(rank, file));
	SkipToSpace(sz);
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
	Validate();
	assert(!mv.fIsNil());
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	PC pcFrom = mv.pcMove();
	CPC cpcFrom = pcFrom.cpc();

	/* store undo information in the mv */

	mv.SetCsEp(csCur, sqEnPassant);

	/* captures. if we're taking a rook, we can't castle to that rook */

	SQ sqTake = sqTo;
	if (pcFrom.apc() == APC::Pawn && sqTake == sqEnPassant)
		sqTake = SQ(sqTo.rank() ^ 1, sqEnPassant.file());
	if (!FIsEmpty(sqTake)) {
		APC apcTake = ApcFromSq(sqTake);
		mv.SetCapture(apcTake);
		ClearBB(PC(~cpcFrom, apcTake), sqTake);
		if (apcTake == APC::Rook) {
			if (sqTake == SQ(RankBackFromCpc(~cpcFrom), fileKingRook))
				ClearCastle(~cpcFrom, csKing);
			else if (sqTake == SQ(RankBackFromCpc(~cpcFrom), fileQueenRook))
				ClearCastle(~cpcFrom, csQueen);
		}
	}

	/* move the pieces */

	ClearBB(pcFrom, sqFrom);
	SetBB(pcFrom, sqTo);

	switch (pcFrom.apc()) {
	case APC::Pawn:
		/* save double-pawn moves for potential en passant and return */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02) {
			SetEnPassant(SQ(sqTo.rank() ^ 1, sqTo.file()));
			goto Done;
		}
		if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			ClearBB(PC(cpcFrom, APC::Pawn), sqTo);
			SetBB(PC(cpcFrom, mv.apcPromote()), sqTo);
		} 
		break;

	case APC::King:
		ClearCastle(cpcFrom, csKing|csQueen);
		if (sqTo.file() - sqFrom.file() > 1) { // king side
			ClearBB(PC(cpcFrom, APC::Rook), sqTo + 1);
			SetBB(PC(cpcFrom, APC::Rook), sqTo - 1);
		}
		else if (sqTo.file() - sqFrom.file() < -1) { // queen side 
			ClearBB(PC(cpcFrom, APC::Rook), sqTo - 2);
			SetBB(PC(cpcFrom, APC::Rook), sqTo + 1);
		}
		break;

	case APC::Rook:
		if (sqFrom == SQ(RankBackFromCpc(cpcFrom), fileQueenRook))
			ClearCastle(cpcFrom, csQueen);
		else if (sqFrom == SQ(RankBackFromCpc(cpcFrom), fileKingRook))
			ClearCastle(cpcFrom, csKing);
		break;

	default:
		break;
	}

	SetEnPassant(SQ());
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
	assert(FIsEmpty(mv.sqFrom()));
	CPC cpcMove = mv.cpcMove();

	/* restore castle and en passant state. */

	SetCastle(mv.csPrev());
	if (mv.fEpPrev())
		SetEnPassant(SQ(RankToEpFromCpc(cpcMove), mv.fileEpPrev()));
	else
		SetEnPassant(SQ());

	/* put piece back in source square, undoing any pawn promotion that might
	   have happened */

	ClearBB(PC(cpcMove, mv.apcPromote() ? mv.apcPromote() : mv.apcMove()), mv.sqTo());
	SetBB(mv.pcMove(), mv.sqFrom());

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	if (mv.apcCapture() != APC::Null) {
		SQ sqTake = mv.sqTo();
		if (sqTake == sqEnPassant)
			sqTake = SQ(RankTakeEpFromCpc(cpcMove), sqEnPassant.file());
		SetBB(PC(~cpcMove, mv.apcCapture()), sqTake);
	}

	/* undoing a castle means we need to undo the rook, too */

	if (mv.apcMove() == APC::King) {
		int dfile = mv.sqTo().file() - mv.sqFrom().file();
		if (dfile < -1) { /* queen side */
			ClearBB(PC(cpcMove, APC::Rook), mv.sqTo() + 1);
			SetBB(PC(cpcMove, APC::Rook), mv.sqTo() - 2);
		}
		else if (dfile > 1) { /* king side */
			ClearBB(PC(cpcMove, APC::Rook), mv.sqTo() - 1);
			SetBB(PC(cpcMove, APC::Rook), mv.sqTo() + 1);
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
void BD::GenGmv(GMV& gmv, GG gg)
{
	GenGmv(gmv, cpcToMove, gg);
}


/*	BD::GenGmv
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenGmv(GMV& gmv, CPC cpcMove, GG gg)
{
	Validate();
	GenGmvColor(gmv, cpcMove);
	if (gg == GG::Legal)
		RemoveInCheckMoves(gmv, cpcMove);
}


void BD::RemoveInCheckMoves(GMV& gmv, CPC cpcMove)
{
 	unsigned imvDest = 0;
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mv = gmv[imv];
		MakeMvSq(mv);
		if (!FInCheck(cpcMove))
			gmv[imvDest++] = mv;
		UndoMvSq(mv);
	}
	gmv.Resize(imvDest);
}


/*	BD::FMvIsQuiescent
 *
 *	TODO: should move this into the AI
 */
bool BD::FMvIsQuiescent(MV mv) const noexcept
{
	return !mv.fIsCapture();
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check.
 */
bool BD::FInCheck(CPC cpc) const noexcept
{
	BB bbKing = mppcbb[PC(cpc, APC::King)].sqLow();

	CPC cpcBy = ~cpc;
	if (BbKnightAttacked(mppcbb[PC(cpcBy, APC::Knight)], cpcBy) & bbKing)
		return true;
	BB bbQueen = mppcbb[PC(cpcBy, APC::Queen)];
	if (FBbAttackedByBishop(mppcbb[PC(cpcBy, APC::Bishop)] | bbQueen, bbKing, cpcBy))
		return true;
	if (FBbAttackedByRook(mppcbb[PC(cpcBy, APC::Rook)] | bbQueen, bbKing, cpcBy))
		return true;
	if (BbPawnAttacked(mppcbb[PC(cpcBy, APC::Pawn)], cpcBy) & bbKing)
		return true;
	if (BbKingAttacked(mppcbb[PC(cpcBy, APC::King)], cpcBy) & bbKing)
		return true;
	return false;
}


/*	BD::BbPawnAttacked
 *
 *	Returns bitboard of all squares all pawns attack.
 */
BB BD::BbPawnAttacked(BB bbPawns, CPC cpcBy) const noexcept
{
	bbPawns = cpcBy == CPC::White ? BbNorthOne(bbPawns) : BbSouthOne(bbPawns);
	BB bbPawnAttacked = BbEastOne(bbPawns) | BbWestOne(bbPawns);
	return bbPawnAttacked;
}


/*	BD::BbKnightAttacked
 *
 *	Returns a bitboard of all squares a knight attacks.
 */
BB BD::BbKnightAttacked(BB bbKnights, CPC cpcBy) const noexcept
{
	BB bb1 = BbWestOne(bbKnights) | BbEastOne(bbKnights);
	BB bb2 = BbWestTwo(bbKnights) | BbEastTwo(bbKnights);
	return BbNorthTwo(bb1) | BbSouthTwo(bb1) | BbNorthOne(bb2) | BbSouthOne(bb2);
}


/*	BD::BbFwdSlideAttacks
 *
 *	Returns a bitboard of squares a sliding piece (rook, bishop, queen) attacks
 *	in the direction given. Only works for forward moves (north, northwest,
 *	northeast, and east).
 * 
 *	A square is attacked if it is empty or is the first square along the attack 
 *	ray that has a piece on it (either self of opponent color). So it includes
 *	not only squares that can be moved to, but also pieces that are defended.
 */
BB BD::BbFwdSlideAttacks(DIR dir, SQ sqFrom) const noexcept
{
	assert(dir == DIR::East || dir == DIR::NorthWest || dir == DIR::North || dir == DIR::NorthEast);
	BB bbAttacks = mpbb.BbSlideTo(sqFrom, dir);
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(1ULL<<63);
	return bbAttacks ^ mpbb.BbSlideTo(bbBlockers.sqLow(), dir);
}


/*	BD::BbRevSlideAttacks
 *
 *	Returns bitboard of squares a sliding piece attacks in the direction given.
 *	Only works for reverse moves (south, southwest, southeast, and west).
 */
BB BD::BbRevSlideAttacks(DIR dir, SQ sqFrom) const noexcept
{
	assert(dir == DIR::West || dir == DIR::SouthWest || dir == DIR::South || dir == DIR::SouthEast);
	BB bbAttacks = mpbb.BbSlideTo(sqFrom, dir);
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(1);
	return bbAttacks ^ mpbb.BbSlideTo(bbBlockers.sqHigh(), dir);
}


bool BD::FBbAttackedByBishop(BB bbBishops, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbBishops; bbBishops.ClearLow()) {
		SQ sq = bbBishops.sqLow();
		if (BbFwdSlideAttacks(DIR::NorthEast, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(DIR::NorthWest, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::SouthEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::SouthWest, sq) & bb)
			return true;
	}
	return false;
}


bool BD::FBbAttackedByRook(BB bbRooks, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbRooks; bbRooks.ClearLow()) {
		SQ sq = bbRooks.sqLow();
		if (BbFwdSlideAttacks(DIR::North, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(DIR::East, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::South, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::West, sq) & bb)
			return true;
	}
	return false;
}


bool BD::FBbAttackedByQueen(BB bbQueens, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbQueens; bbQueens.ClearLow()) {
		SQ sq = bbQueens.sqLow();
		if (BbFwdSlideAttacks(DIR::North, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(DIR::East, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::South, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::West, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(DIR::NorthEast, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(DIR::NorthWest, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::SouthEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(DIR::SouthWest, sq) & bb)
			return true;
	}
	return false;
}


BB BD::BbKingAttacked(BB bbKing, CPC cpcBy) const noexcept
{
	return mpbb.BbKingTo(bbKing.sqLow());
}


/*	BD::ApcBbAttacked
 *
 *	Returns the piece type of the lowest valued piece if the given bitboard is attacked 
 *	by someone of the color cpcBy. Returns APC::Null if no pieces are attacking.
 */
APC BD::ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept
{
	if (BbPawnAttacked(mppcbb[PC(cpcBy, APC::Pawn)], cpcBy) & bbAttacked)
		return APC::Pawn;
	if (BbKnightAttacked(mppcbb[PC(cpcBy, APC::Knight)], cpcBy) & bbAttacked)
		return APC::Knight;
	if (FBbAttackedByBishop(mppcbb[PC(cpcBy, APC::Bishop)], bbAttacked, cpcBy))
		return APC::Bishop;
	if (FBbAttackedByRook(mppcbb[PC(cpcBy, APC::Rook)], bbAttacked, cpcBy))
		return APC::Rook;
	if (FBbAttackedByQueen(mppcbb[PC(cpcBy, APC::Queen)], bbAttacked, cpcBy))
		return APC::Queen;
	if (BbKingAttacked(mppcbb[PC(cpcBy, APC::King)], cpcBy) & bbAttacked)
		return APC::King;
	return APC::Null;
}


/*	BD::GenGmvColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenGmvColor(GMV& gmv, CPC cpcMove) const
{
	gmv.Clear();

	GenGmvPawnMvs(gmv, mppcbb[PC(cpcMove, APC::Pawn)], cpcMove);

	/* generate knight moves */

	for (BB bb = mppcbb[PC(cpcMove, APC::Knight)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = mpbb.BbKnightTo(sqFrom);
		GenGmvBbMvs(gmv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, APC::Knight));
	}

	/* generate bishop moves */

	for (BB bb = mppcbb[PC(cpcMove, APC::Bishop)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(DIR::NorthEast, sqFrom) |
				BbRevSlideAttacks(DIR::SouthEast, sqFrom) |
				BbRevSlideAttacks(DIR::SouthWest, sqFrom) |
				BbFwdSlideAttacks(DIR::NorthWest, sqFrom);
		GenGmvBbMvs(gmv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, APC::Bishop));
	}

	/* generate rook moves */

	for (BB bb = mppcbb[PC(cpcMove, APC::Rook)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(DIR::North, sqFrom) |
				BbFwdSlideAttacks(DIR::East, sqFrom) |
				BbRevSlideAttacks(DIR::South, sqFrom) |
				BbRevSlideAttacks(DIR::West, sqFrom);
		GenGmvBbMvs(gmv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, APC::Rook));
	}

	/* generate queen moves */

	for (BB bb = mppcbb[PC(cpcMove, APC::Queen)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(DIR::North, sqFrom) |
				BbFwdSlideAttacks(DIR::NorthEast, sqFrom) |
				BbFwdSlideAttacks(DIR::East, sqFrom) |
				BbRevSlideAttacks(DIR::SouthEast, sqFrom) |
				BbRevSlideAttacks(DIR::South, sqFrom) |
				BbRevSlideAttacks(DIR::SouthWest, sqFrom) |
				BbRevSlideAttacks(DIR::West, sqFrom) |
				BbFwdSlideAttacks(DIR::NorthWest, sqFrom);
		GenGmvBbMvs(gmv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, APC::Queen));
	}

	/* generate king moves */

	BB bbKing = mppcbb[PC(cpcMove, APC::King)];
	assert(bbKing.csq() == 1);
	SQ sqFrom = bbKing.sqLow();
	BB bbTo = mpbb.BbKingTo(sqFrom);
	GenGmvBbMvs(gmv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, APC::King));
	GenGmvCastle(gmv, sqFrom, cpcMove);
}


/*	BD::GenGmvBbMvs
 *
 *	Generates all moves with destination squares in bbTo and square offset to
 *	the soruce square dsq 
 */
void BD::GenGmvBbMvs(GMV& gmv, BB bbTo, int dsq, PC pcMove) const
{
	for (; bbTo; bbTo.ClearLow()) {
		SQ sqTo = bbTo.sqLow();
		gmv.AppendMv(sqTo + dsq, sqTo, pcMove);
	}
}


/*	BD::GenGmvBbMvs
 *
 *	Generates all moves with the source square sqFrom and the destination squares
 *	in bbTo.
 */
void BD::GenGmvBbMvs(GMV& gmv, SQ sqFrom, BB bbTo, PC pcMove) const
{
	for (; bbTo; bbTo.ClearLow())
		gmv.AppendMv(sqFrom, bbTo.sqLow(), pcMove);
}


void BD::GenGmvBbPawnMvs(GMV& gmv, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const
{
	if (bbTo & bbRankPromotion) {
		for (BB bbT = bbTo & bbRankPromotion; bbT; bbT.ClearLow()) {
			SQ sqTo = bbT.sqLow();
			AddGmvMvPromotions(gmv, MV(sqTo + dsq, sqTo, PC(cpcMove, APC::Pawn)));
		}
		bbTo -= bbRankPromotion;
	}
	GenGmvBbMvs(gmv, bbTo, dsq, PC(cpcMove, APC::Pawn));
}


void BD::GenGmvPawnMvs(GMV& gmv, BB bbPawns, CPC cpcMove) const
{
	if (cpcMove == CPC::White) {
		BB bbTo = (bbPawns << 8) & bbUnoccupied;
		GenGmvBbPawnMvs(gmv, bbTo, bbRank8, -8, cpcMove);
		bbTo = ((bbTo & bbRank3) << 8) & bbUnoccupied;
		GenGmvBbMvs(gmv, bbTo, -16, PC(cpcMove, APC::Pawn));
		bbTo = ((bbPawns - bbFileA) << 7) & mpcpcbb[CPC::Black];
		GenGmvBbPawnMvs(gmv, bbTo, bbRank8, -7, cpcMove);
		bbTo = ((bbPawns - bbFileH) << 9) & mpcpcbb[CPC::Black];
		GenGmvBbPawnMvs(gmv, bbTo, bbRank8, -9, cpcMove);
	}
	else {
		BB bbTo = (bbPawns >> 8) & bbUnoccupied;
		GenGmvBbPawnMvs(gmv, bbTo, bbRank1, 8, cpcMove);
		bbTo = ((bbTo & bbRank6) >> 8) & bbUnoccupied;
		GenGmvBbMvs(gmv, bbTo, 16, PC(cpcMove, APC::Pawn));
		bbTo = ((bbPawns - bbFileA) >> 9) & mpcpcbb[CPC::White];
		GenGmvBbPawnMvs(gmv, bbTo, bbRank1, 9, cpcMove);
		bbTo = ((bbPawns - bbFileH) >> 7) & mpcpcbb[CPC::White];
		GenGmvBbPawnMvs(gmv, bbTo, bbRank1, 7, cpcMove);
	}
	
	if (!sqEnPassant.fIsNil()) {
		BB bbEnPassant(sqEnPassant), bbFrom;
		if (cpcMove == CPC::White)
			bbFrom = ((bbEnPassant - bbFileA) >> 9) | ((bbEnPassant - bbFileH) >> 7);
		else
			bbFrom = ((bbEnPassant - bbFileA) << 7) | ((bbEnPassant - bbFileH) << 9);
		for (bbFrom &= bbPawns; bbFrom; bbFrom.ClearLow()) {
			SQ sqFrom = bbFrom.sqLow();
			gmv.AppendMv(sqFrom, sqEnPassant, PC(cpcMove, APC::Pawn));
		}
	}
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


/*	BD::GenGmvCastle
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, checks for intermediate 
 *	squares are empty, but does not check the final king destination for in check.
 */
void BD::GenGmvCastle(GMV& gmv, SQ sqKing, CPC cpcMove) const
{
	BB bbKing = BB(sqKing);
	if (FCanCastle(cpcMove, csKing) && !(((bbKing << 1) | (bbKing << 2)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing << 1), ~cpcMove) == APC::Null)
			gmv.AppendMv(sqKing, sqKing + 2, PC(cpcMove, APC::King));
	}
	if (FCanCastle(cpcMove, csQueen) && !(((bbKing >> 1) | (bbKing >> 2) | (bbKing >> 3)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing >> 1), ~cpcMove) == APC::Null) 
			gmv.AppendMv(sqKing, sqKing - 2, PC(cpcMove, APC::King));
	}
}


EV BD::EvFromSq(SQ sq) const noexcept
{
	static const EV mpapcev[] = { 0, 100, 300, 300, 500, 900, 10000, -1 };
	return mpapcev[ApcFromSq(sq)];
}


/*	BD::EvTotalFromCpc
 *
 *	Piece value of the entire board for the given color
 */
EV BD::EvTotalFromCpc(CPC cpc) const noexcept
{
	EV ev = 0;
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		for (BB bb = mppcbb[PC(cpc, apc)]; bb; bb.ClearLow())
			ev += EvFromSq(bb.sqLow());
	}
	return ev;
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

	/* there must be between 1 and 16 pieces of each color */
	assert(mpcpcbb[CPC::White].csq() >= 0 && mpcpcbb[CPC::White].csq() <= 16);
	assert(mpcpcbb[CPC::Black].csq() >= 0 && mpcpcbb[CPC::Black].csq() <= 16);
	/* no more than 8 pawns */
	assert(mppcbb[PC(CPC::White, APC::Pawn)].csq() <= 8);
	assert(mppcbb[PC(CPC::Black, APC::Pawn)].csq() <= 8);
	/* one king */
	//assert(mpapcbb[CPC::White][APC::King].csq() == 1);
	//assert(mpapcbb[CPC::Black][APC::King].csq() == 1);
	/* union of black, white, and empty bitboards should be all squares */
	assert((mpcpcbb[CPC::White] | mpcpcbb[CPC::Black] | bbUnoccupied) == bbAll);
	/* white, black, and empty are disjoint */
	assert((mpcpcbb[CPC::White] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[CPC::Black] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[CPC::White] & mpcpcbb[CPC::Black]) == bbNone);

	for (int rank = 0; rank < rankMax; rank++) {
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			if (!bbUnoccupied.fSet(sq))
				ValidateBB(PcFromSq(sq), sq);
		}
	}

	/* check for valid castle situations */

	if (csCur & csWhiteKing) {
		assert(mppcbb[PC(CPC::White, APC::King)].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[PC(CPC::White, APC::Rook)].fSet(SQ(rankWhiteBack, fileKingRook)));
	}
	if (csCur & csBlackKing) {
		assert(mppcbb[PC(CPC::Black, APC::King)].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[PC(CPC::Black, APC::Rook)].fSet(SQ(rankBlackBack, fileKingRook)));
	}
	if (csCur & csWhiteQueen) {
		assert(mppcbb[PC(CPC::White, APC::King)].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[PC(CPC::White, APC::Rook)].fSet(SQ(rankWhiteBack, fileQueenRook)));
	}
	if (csCur & csBlackQueen) {
		assert(mppcbb[PC(CPC::Black, APC::King)].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[PC(CPC::Black, APC::Rook)].fSet(SQ(rankBlackBack, fileQueenRook)));
	}

	/* check for valid en passant situations */

	if (!sqEnPassant.fIsNil()) {
	}

	/* make sure hash is kept accurate */

	assert(habd == genhabd.HabdFromBd(*this));

}


void BD::ValidateBB(PC pcVal, SQ sq) const
{
	for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc)
		for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
			if (PC(cpc, apc) == pcVal) {
				assert(mppcbb[PC(cpc, apc)].fSet(sq));
				assert(mpcpcbb[cpc].fSet(sq));
				assert(!bbUnoccupied.fSet(sq));
			}
			else {
				assert(!mppcbb[PC(cpc, apc)].fSet(sq));
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
			if (FIsEmpty(sq))
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
void BDG::MakeMv(MV& mv)
{
	assert(!mv.fIsNil());

	/* make the move and save the move in the move list */

	MakeMvSq(mv);
	if (++imvCur == vmvGame.size())
		vmvGame.push_back(mv);
	else if (mv != vmvGame[imvCur]) {
		vmvGame[imvCur] = mv;
		/* all moves after this in the move list are now invalid */
		for (size_t imv = (size_t)imvCur + 1; imv < vmvGame.size(); imv++)
			vmvGame[imv] = MV();
	}

	/* keep track of last pawn move or capture, which is used to detect draws */

	if (mv.apcMove() == APC::Pawn || mv.fIsCapture())
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
	if (FInCheck(~cpc))
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
			if (vmvGame[imvPawnOrTakeLast].apcMove() == APC::Pawn || 
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
	if (imvCur > (int)vmvGame.size() || vmvGame[imvCur+1].fIsNil())
		return;
	imvCur++;
	MV mv = vmvGame[imvCur];
	if (mv.apcMove() == APC::Pawn || mv.fIsCapture())
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
	/* any pawns, rooks, or queens means no draw */

	if (mppcbb[PC(CPC::White, APC::Pawn)] || mppcbb[PC(CPC::Black, APC::Pawn)] ||
			mppcbb[PC(CPC::White, APC::Rook)] || mppcbb[PC(CPC::Black, APC::Rook)] ||
			mppcbb[PC(CPC::White, APC::Queen)] || mppcbb[PC(CPC::Black, APC::Queen)])
		return false;
	
	/* only bishops and knights on the board from here on out. everyone has a king */

	/* if either side has more than 2 pieces, no draw */

	if (mpcpcbb[CPC::White].csq() > 2 || mpcpcbb[CPC::Black].csq() > 2)
		return false;

	/* K vs. K, K-N vs. K, or K-B vs. K is a draw */

	if (mpcpcbb[CPC::White].csq() == 1 || mpcpcbb[CPC::Black].csq() == 1)
		return true;
	
	/* The other draw case is K-B vs. K-B with bishops on same color */

	if (mppcbb[PC(CPC::White, APC::Bishop)] && mppcbb[PC(CPC::Black, APC::Bishop)]) {
		SQ sqBishopBlack = mppcbb[PC(CPC::Black, APC::Bishop)].sqLow();
		SQ sqBishopWhite = mppcbb[PC(CPC::White, APC::Bishop)].sqLow();
		if (((sqBishopWhite ^ sqBishopBlack) & 1) == 0)
			return true;
	}

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
	if (imvCur - imvPawnOrTakeLast < ((int64_t)cbdDraw-1) * 2 * 2)
		return false;
	BD bd = *this;
	int cbdSame = 1;
	bd.UndoMvSq(vmvGame[imvCur]);
	bd.UndoMvSq(vmvGame[imvCur - 1]);
	for (int64_t imv = imvCur - 2; imv >= imvPawnOrTakeLast + 2; imv -= 2) {
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
bool BDG::FDraw50Move(int64_t cmvDraw) const
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

