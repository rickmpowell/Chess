/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */

#include "bd.h"


mt19937_64 rgen(7724322UL);

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
	rghabdPiece[0][0] = 0;	// shut up the compiler warning about uninitialized array
	for (SQ sq = 0; sq < sqMax; sq++)
		for (PC pc = 0; pc < pcMax; pc++)
			rghabdPiece[sq][pc] = HabdRandom(rgen);

	rghabdCastle[0] = 0;
	for (int ics = 1; ics < CArray(rghabdCastle); ics++)
		rghabdCastle[ics] = HabdRandom(rgen);

	for (int iep = 0; iep < CArray(rghabdEnPassant); iep++)
		rghabdEnPassant[iep] = HabdRandom(rgen); 

	habdMove = HabdRandom(rgen);
}


HABD GENHABD::HabdRandom(mt19937_64& rgen)
{
	HABD habd = rgen();
	return habd;
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
		if (pc.apc() != apcNull)
			habd ^= rghabdPiece[sq][pc];
		}

	/* castle state */

	habd ^= rghabdCastle[bd.csCur];

	/* en passant state */

	if (!bd.sqEnPassant.fIsNil())
		habd ^= rghabdEnPassant[bd.sqEnPassant.file()];

	/* current side to move */

	if (bd.cpcToMove == cpcBlack)
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

			/* passed pawn alleys */

			if (in_range(rank, 1, rankMax - 2)) {
				BB bbNorth = mpsqdirbbSlide[sq][dirNorth];
				BB bbSouth = mpsqdirbbSlide[sq][dirSouth];
				mpsqbbPassedPawnAlley[sq-8][cpcWhite] = bbNorth | BbEastOne(bbNorth) | BbWestOne(bbNorth);
				mpsqbbPassedPawnAlley[sq-8][cpcBlack] = bbSouth | BbEastOne(bbSouth) | BbWestOne(bbSouth);
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


/* game phase, whiich is a running sum of the types of pieces on the board,
   which is handy for AI eval */
static const GPH mpapcgph[apcMax] = { gphNone, gphNone, gphMinor, gphMinor,
										   gphRook, gphQueen, gphNone };

BD::BD(void)
{
	SetEmpty();
	Validate();
}


void BD::SetEmpty(void) noexcept
{
	for (PC pc = 0; pc < pcMax; ++pc)
		mppcbb[pc] = bbNone;
	mpcpcbb[cpcWhite] = bbNone;
	mpcpcbb[cpcBlack] = bbNone;
	bbUnoccupied = bbAll;

	csCur = 0;
	cpcToMove = cpcWhite;
	sqEnPassant = SQ();
	
	habd = 0L;
	gph = gphMax;
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
		case 'p': AddPieceFEN(sq++, PC(cpcBlack, apcPawn)); break;
		case 'r': AddPieceFEN(sq++, PC(cpcBlack, apcRook)); break;
		case 'n': AddPieceFEN(sq++, PC(cpcBlack, apcKnight)); break;
		case 'b': AddPieceFEN(sq++, PC(cpcBlack, apcBishop)); break;
		case 'q': AddPieceFEN(sq++, PC(cpcBlack, apcQueen)); break;
		case 'k': AddPieceFEN(sq++, PC(cpcBlack, apcKing)); break;
		case 'P': AddPieceFEN(sq++, PC(cpcWhite, apcPawn)); break;
		case 'R': AddPieceFEN(sq++, PC(cpcWhite, apcRook)); break;
		case 'N': AddPieceFEN(sq++, PC(cpcWhite, apcKnight)); break;
		case 'B': AddPieceFEN(sq++, PC(cpcWhite, apcBishop)); break;
		case 'Q': AddPieceFEN(sq++, PC(cpcWhite, apcQueen)); break;
		case 'K': AddPieceFEN(sq++, PC(cpcWhite, apcKing)); break;
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
	AddApcToGph(pc.apc());
}


void BD::InitFENSideToMove(const wchar_t*& sz)
{
	SkipToNonSpace(sz);
	cpcToMove = cpcWhite;
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
	ClearCastle(cpcWhite, csKing | csQueen);
	ClearCastle(cpcBlack, csKing | csQueen);
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'K': SetCastle(cpcWhite, csKing); break;
		case 'Q': SetCastle(cpcWhite, csQueen); break;
		case 'k': SetCastle(cpcBlack, csKing); break;
		case 'q': SetCastle(cpcBlack, csQueen); break;
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
	if (pcFrom.apc() == apcPawn && sqTake == sqEnPassant)
		sqTake = SQ(sqTo.rank() ^ 1, sqEnPassant.file());
	if (!FIsEmpty(sqTake)) {
		APC apcTake = ApcFromSq(sqTake);
		mv.SetCapture(apcTake);
		ClearBB(PC(~cpcFrom, apcTake), sqTake);
		RemoveApcFromGph(apcTake);
		if (apcTake == apcRook) {
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
	case apcPawn:
		/* save double-pawn moves for potential en passant and return */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02) {
			SetEnPassant(SQ(sqTo.rank() ^ 1, sqTo.file()));
			goto Done;
		}
		if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			ClearBB(PC(cpcFrom, apcPawn), sqTo);
			SetBB(PC(cpcFrom, mv.apcPromote()), sqTo);
			RemoveApcFromGph(apcPawn);
			AddApcToGph(mv.apcPromote());
		} 
		break;

	case apcKing:
		ClearCastle(cpcFrom, csKing|csQueen);
		if (sqTo.file() - sqFrom.file() > 1) { // king side
			ClearBB(PC(cpcFrom, apcRook), sqTo + 1);
			SetBB(PC(cpcFrom, apcRook), sqTo - 1);
		}
		else if (sqTo.file() - sqFrom.file() < -1) { // queen side 
			ClearBB(PC(cpcFrom, apcRook), sqTo - 2);
			SetBB(PC(cpcFrom, apcRook), sqTo + 1);
		}
		break;

	case apcRook:
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

	if (mv.apcPromote() == apcNull)
		ClearBB(PC(cpcMove, mv.apcMove()), mv.sqTo());
	else {
		ClearBB(PC(cpcMove, mv.apcPromote()), mv.sqTo());
		RemoveApcFromGph(mv.apcPromote());
		AddApcToGph(apcPawn);
	}
	SetBB(mv.pcMove(), mv.sqFrom());

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	if (mv.apcCapture() != apcNull) {
		SQ sqTake = mv.sqTo();
		if (sqTake == sqEnPassant)
			sqTake = SQ(RankTakeEpFromCpc(cpcMove), sqEnPassant.file());
		SetBB(PC(~cpcMove, mv.apcCapture()), sqTake);
		AddApcToGph(mv.apcCapture());
	}

	/* undoing a castle means we need to undo the rook, too */

	if (mv.apcMove() == apcKing) {
		int dfile = mv.sqTo().file() - mv.sqFrom().file();
		if (dfile < -1) { /* queen side */
			ClearBB(PC(cpcMove, apcRook), mv.sqTo() + 1);
			SetBB(PC(cpcMove, apcRook), mv.sqTo() - 2);
		}
		else if (dfile > 1) { /* king side */
			ClearBB(PC(cpcMove, apcRook), mv.sqTo() - 1);
			SetBB(PC(cpcMove, apcRook), mv.sqTo() + 1);
		}
	}

	ToggleToMove();

	Validate();
}


/*	BDG::GenVemv
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the
 *	king in check.
 */
void BD::GenVemv(VEMV& vemv, GG gg)
{
	GenVemv(vemv, cpcToMove, gg);
}


/*	BD::GenVemv
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenVemv(VEMV& vemv, CPC cpcMove, GG gg)
{
	Validate();
	GenVemvColor(vemv, cpcMove);
	if (gg == ggLegal)
		RemoveInCheckMoves(vemv, cpcMove);
}


void BD::RemoveInCheckMoves(VEMV& vemv, CPC cpcMove)
{
 	unsigned imvDest = 0;
	for (EMV emv : vemv) {
		MakeMvSq(emv.mv);
		if (!FInCheck(cpcMove))
			vemv[imvDest++] = emv.mv;
		UndoMvSq(emv.mv);
	}
	vemv.Resize(imvDest);
}


/*	BD::FMvIsQuiescent
 *
 *	TODO: should move this into the AI
 */
bool BD::FMvIsQuiescent(MV mv) const noexcept
{
	return !mv.fIsCapture() && mv.apcPromote() == apcNull;
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check.
 */
bool BD::FInCheck(CPC cpc) const noexcept
{
	BB bbKing = mppcbb[PC(cpc, apcKing)].sqLow();

	CPC cpcBy = ~cpc;
	BB bbQueen = mppcbb[PC(cpcBy, apcQueen)];
	if (FBbAttackedByBishop(mppcbb[PC(cpcBy, apcBishop)] | bbQueen, bbKing, cpcBy))
		return true;
	if (FBbAttackedByRook(mppcbb[PC(cpcBy, apcRook)] | bbQueen, bbKing, cpcBy))
		return true;
	if (BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)], cpcBy) & bbKing)
		return true;
	if (BbPawnAttacked(mppcbb[PC(cpcBy, apcPawn)], cpcBy) & bbKing)
		return true;
	if (BbKingAttacked(mppcbb[PC(cpcBy, apcKing)], cpcBy) & bbKing)
		return true;
	return false;
}


/*	BD::BbPawnAttacked
 *
 *	Returns bitboard of all squares all pawns attack.
 */
BB BD::BbPawnAttacked(BB bbPawns, CPC cpcBy) const noexcept
{
	bbPawns = cpcBy == cpcWhite ? BbNorthOne(bbPawns) : BbSouthOne(bbPawns);
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
	assert(dir == dirEast || dir == dirNorthWest || dir == dirNorth || dir == dirNorthEast);
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
	assert(dir == dirWest || dir == dirSouthWest || dir == dirSouth || dir == dirSouthEast);
	BB bbAttacks = mpbb.BbSlideTo(sqFrom, dir);
	BB bbBlockers = (bbAttacks - bbUnoccupied) | BB(1);
	return bbAttacks ^ mpbb.BbSlideTo(bbBlockers.sqHigh(), dir);
}


bool BD::FBbAttackedByBishop(BB bbBishops, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbBishops; bbBishops.ClearLow()) {
		SQ sq = bbBishops.sqLow();
		if (BbFwdSlideAttacks(dirNorthEast, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(dirNorthWest, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouthEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouthWest, sq) & bb)
			return true;
	}
	return false;
}


bool BD::FBbAttackedByRook(BB bbRooks, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbRooks; bbRooks.ClearLow()) {
		SQ sq = bbRooks.sqLow();
		if (BbFwdSlideAttacks(dirNorth, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(dirEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouth, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirWest, sq) & bb)
			return true;
	}
	return false;
}


bool BD::FBbAttackedByQueen(BB bbQueens, BB bb, CPC cpcBy) const noexcept
{
	for ( ; bbQueens; bbQueens.ClearLow()) {
		SQ sq = bbQueens.sqLow();
		if (BbFwdSlideAttacks(dirNorth, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(dirEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouth, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirWest, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(dirNorthEast, sq) & bb)
			return true;
		if (BbFwdSlideAttacks(dirNorthWest, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouthEast, sq) & bb)
			return true;
		if (BbRevSlideAttacks(dirSouthWest, sq) & bb)
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
 *	by someone of the color cpcBy. Returns apcNull if no pieces are attacking.
 */
APC BD::ApcBbAttacked(BB bbAttacked, CPC cpcBy) const noexcept
{
	if (BbPawnAttacked(mppcbb[PC(cpcBy, apcPawn)], cpcBy) & bbAttacked)
		return apcPawn;
	if (BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)], cpcBy) & bbAttacked)
		return apcKnight;
	if (FBbAttackedByBishop(mppcbb[PC(cpcBy, apcBishop)], bbAttacked, cpcBy))
		return apcBishop;
	if (FBbAttackedByRook(mppcbb[PC(cpcBy, apcRook)], bbAttacked, cpcBy))
		return apcRook;
	if (FBbAttackedByQueen(mppcbb[PC(cpcBy, apcQueen)], bbAttacked, cpcBy))
		return apcQueen;
	if (BbKingAttacked(mppcbb[PC(cpcBy, apcKing)], cpcBy) & bbAttacked)
		return apcKing;
	return apcNull;
}


/*	BD::GenVemvColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenVemvColor(VEMV& vemv, CPC cpcMove) const
{
	vemv.Clear();

	GenVemvPawnMvs(vemv, mppcbb[PC(cpcMove, apcPawn)], cpcMove);

	/* generate knight moves */

	for (BB bb = mppcbb[PC(cpcMove, apcKnight)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = mpbb.BbKnightTo(sqFrom);
		GenVemvBbMvs(vemv, sqFrom, bbTo - mpcpcbb[(int)cpcMove], PC(cpcMove, apcKnight));
	}

	/* generate bishop moves */

	for (BB bb = mppcbb[PC(cpcMove, apcBishop)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(dirNorthEast, sqFrom) |
				BbRevSlideAttacks(dirSouthEast, sqFrom) |
				BbRevSlideAttacks(dirSouthWest, sqFrom) |
				BbFwdSlideAttacks(dirNorthWest, sqFrom);
		GenVemvBbMvs(vemv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcBishop));
	}

	/* generate rook moves */

	for (BB bb = mppcbb[PC(cpcMove, apcRook)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(dirNorth, sqFrom) |
				BbFwdSlideAttacks(dirEast, sqFrom) |
				BbRevSlideAttacks(dirSouth, sqFrom) |
				BbRevSlideAttacks(dirWest, sqFrom);
		GenVemvBbMvs(vemv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcRook));
	}

	/* generate queen moves */

	for (BB bb = mppcbb[PC(cpcMove, apcQueen)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbFwdSlideAttacks(dirNorth, sqFrom) |
				BbFwdSlideAttacks(dirNorthEast, sqFrom) |
				BbFwdSlideAttacks(dirEast, sqFrom) |
				BbRevSlideAttacks(dirSouthEast, sqFrom) |
				BbRevSlideAttacks(dirSouth, sqFrom) |
				BbRevSlideAttacks(dirSouthWest, sqFrom) |
				BbRevSlideAttacks(dirWest, sqFrom) |
				BbFwdSlideAttacks(dirNorthWest, sqFrom);
		GenVemvBbMvs(vemv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcQueen));
	}

	/* generate king moves */

	BB bbKing = mppcbb[PC(cpcMove, apcKing)];
	assert(bbKing.csq() == 1);
	SQ sqFrom = bbKing.sqLow();
	BB bbTo = mpbb.BbKingTo(sqFrom);
	GenVemvBbMvs(vemv, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcKing));
	GenVemvCastle(vemv, sqFrom, cpcMove);
}


/*	BD::GenVemvBbMvs
 *
 *	Generates all moves with destination squares in bbTo and square offset to
 *	the soruce square dsq 
 */
void BD::GenVemvBbMvs(VEMV& vemv, BB bbTo, int dsq, PC pcMove) const
{
	for (; bbTo; bbTo.ClearLow()) {
		SQ sqTo = bbTo.sqLow();
		vemv.AppendMv(sqTo + dsq, sqTo, pcMove);
	}
}


/*	BD::GenVemvBbMvs
 *
 *	Generates all moves with the source square sqFrom and the destination squares
 *	in bbTo.
 */
void BD::GenVemvBbMvs(VEMV& vemv, SQ sqFrom, BB bbTo, PC pcMove) const
{
	for (; bbTo; bbTo.ClearLow())
		vemv.AppendMv(sqFrom, bbTo.sqLow(), pcMove);
}


void BD::GenVemvBbPawnMvs(VEMV& vemv, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const
{
	if (bbTo & bbRankPromotion) {
		for (BB bbT = bbTo & bbRankPromotion; bbT; bbT.ClearLow()) {
			SQ sqTo = bbT.sqLow();
			AddVemvMvPromotions(vemv, MV(sqTo + dsq, sqTo, PC(cpcMove, apcPawn)));
		}
		bbTo -= bbRankPromotion;
	}
	GenVemvBbMvs(vemv, bbTo, dsq, PC(cpcMove, apcPawn));
}


void BD::GenVemvPawnMvs(VEMV& vemv, BB bbPawns, CPC cpcMove) const
{
	if (cpcMove == cpcWhite) {
		BB bbTo = (bbPawns << 8) & bbUnoccupied;
		GenVemvBbPawnMvs(vemv, bbTo, bbRank8, -8, cpcMove);
		bbTo = ((bbTo & bbRank3) << 8) & bbUnoccupied;
		GenVemvBbMvs(vemv, bbTo, -16, PC(cpcMove, apcPawn));
		bbTo = ((bbPawns - bbFileA) << 7) & mpcpcbb[cpcBlack];
		GenVemvBbPawnMvs(vemv, bbTo, bbRank8, -7, cpcMove);
		bbTo = ((bbPawns - bbFileH) << 9) & mpcpcbb[cpcBlack];
		GenVemvBbPawnMvs(vemv, bbTo, bbRank8, -9, cpcMove);
	}
	else {
		BB bbTo = (bbPawns >> 8) & bbUnoccupied;
		GenVemvBbPawnMvs(vemv, bbTo, bbRank1, 8, cpcMove);
		bbTo = ((bbTo & bbRank6) >> 8) & bbUnoccupied;
		GenVemvBbMvs(vemv, bbTo, 16, PC(cpcMove, apcPawn));
		bbTo = ((bbPawns - bbFileA) >> 9) & mpcpcbb[cpcWhite];
		GenVemvBbPawnMvs(vemv, bbTo, bbRank1, 9, cpcMove);
		bbTo = ((bbPawns - bbFileH) >> 7) & mpcpcbb[cpcWhite];
		GenVemvBbPawnMvs(vemv, bbTo, bbRank1, 7, cpcMove);
	}
	
	if (!sqEnPassant.fIsNil()) {
		BB bbEnPassant(sqEnPassant), bbFrom;
		if (cpcMove == cpcWhite)
			bbFrom = ((bbEnPassant - bbFileA) >> 9) | ((bbEnPassant - bbFileH) >> 7);
		else
			bbFrom = ((bbEnPassant - bbFileA) << 7) | ((bbEnPassant - bbFileH) << 9);
		for (bbFrom &= bbPawns; bbFrom; bbFrom.ClearLow()) {
			SQ sqFrom = bbFrom.sqLow();
			vemv.AppendMv(sqFrom, sqEnPassant, PC(cpcMove, apcPawn));
		}
	}
}


/*	BD::AddVemvMvPromotions
 *
 *	When a pawn is pushed to the last rank, adds all the promotion possibilities
 *	to the move list, which includes promotions to queen, rook, knight, and
 *	bishop.
 */
void BD::AddVemvMvPromotions(VEMV& vemv, MV mv) const
{
	vemv.AppendMv(mv.SetApcPromote(apcQueen));
	vemv.AppendMv(mv.SetApcPromote(apcRook));
	vemv.AppendMv(mv.SetApcPromote(apcBishop));
	vemv.AppendMv(mv.SetApcPromote(apcKnight));
}


/*	BD::GenVemvCastle
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, checks for intermediate 
 *	squares are empty, but does not check the final king destination for in check.
 */
void BD::GenVemvCastle(VEMV& vemv, SQ sqKing, CPC cpcMove) const
{
	BB bbKing = BB(sqKing);
	if (FCanCastle(cpcMove, csKing) && !(((bbKing << 1) | (bbKing << 2)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing << 1), ~cpcMove) == apcNull)
			vemv.AppendMv(sqKing, sqKing + 2, PC(cpcMove, apcKing));
	}
	if (FCanCastle(cpcMove, csQueen) && !(((bbKing >> 1) | (bbKing >> 2) | (bbKing >> 3)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing >> 1), ~cpcMove) == apcNull) 
			vemv.AppendMv(sqKing, sqKing - 2, PC(cpcMove, apcKing));
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
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		for (BB bb = mppcbb[PC(cpc, apc)]; bb; bb.ClearLow())
			ev += EvFromSq(bb.sqLow());
	}
	return ev;
}


GPH BD::GphCompute(void) const noexcept
{
	GPH gph = gphMax;
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		gph = static_cast<GPH>(static_cast<int>(gph) -
							   mppcbb[PC(cpcWhite, apc)].csq() * static_cast<int>(mpapcgph[apc]) -
							   mppcbb[PC(cpcBlack, apc)].csq() * static_cast<int>(mpapcgph[apc]));
	}
	return gph;
}


void BD::RecomputeGph(void) noexcept
{
	gph = GphCompute();
}


void BD::AddApcToGph(APC apc) noexcept
{
	gph = static_cast<GPH>(static_cast<int>(gph) - static_cast<int>(mpapcgph[apc]));
}


void BD::RemoveApcFromGph(APC apc) noexcept
{
	gph = static_cast<GPH>(static_cast<int>(gph) + static_cast<int>(mpapcgph[apc]));
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
	assert(mpcpcbb[cpcWhite].csq() >= 0 && mpcpcbb[cpcWhite].csq() <= 16);
	assert(mpcpcbb[cpcBlack].csq() >= 0 && mpcpcbb[cpcBlack].csq() <= 16);
	/* no more than 8 pawns */
	assert(mppcbb[pcWhitePawn].csq() <= 8);
	assert(mppcbb[pcBlackPawn].csq() <= 8);
	/* one king */
	//assert(mpapcbb[pcWhiteKing].csq() == 1);
	//assert(mpapcbb[pcBlackKing].csq() == 1);
	/* union of black, white, and empty bitboards should be all squares */
	assert((mpcpcbb[cpcWhite] | mpcpcbb[cpcBlack] | bbUnoccupied) == bbAll);
	/* white, black, and empty are disjoint */
	assert((mpcpcbb[cpcWhite] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[cpcBlack] & bbUnoccupied) == bbNone);
	assert((mpcpcbb[cpcWhite] & mpcpcbb[cpcBlack]) == bbNone);

	for (int rank = 0; rank < rankMax; rank++) {
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			if (!bbUnoccupied.fSet(sq))
				ValidateBB(PcFromSq(sq), sq);
		}
	}

	/* check for valid castle situations */

	if (csCur & csWhiteKing) {
		assert(mppcbb[pcWhiteKing].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[pcWhiteRook].fSet(SQ(rankWhiteBack, fileKingRook)));
	}
	if (csCur & csBlackKing) {
		assert(mppcbb[pcBlackKing].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[pcBlackRook].fSet(SQ(rankBlackBack, fileKingRook)));
	}
	if (csCur & csWhiteQueen) {
		assert(mppcbb[pcWhiteKing].fSet(SQ(rankWhiteBack, fileKing)));
		assert(mppcbb[pcWhiteRook].fSet(SQ(rankWhiteBack, fileQueenRook)));
	}
	if (csCur & csBlackQueen) {
		assert(mppcbb[pcBlackKing].fSet(SQ(rankBlackBack, fileKing)));
		assert(mppcbb[pcBlackRook].fSet(SQ(rankBlackBack, fileQueenRook)));
	}

	/* check for valid en passant situations */

	if (!sqEnPassant.fIsNil()) {
	}

	/* game phase should be accurate */

	assert(gph == GphCompute());

	/* make sure hash is kept accurate */

	assert(habd == genhabd.HabdFromBd(*this));

}


void BD::ValidateBB(PC pcVal, SQ sq) const
{
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
		for (APC apc = apcPawn; apc < apcMax; ++apc) {
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
BDG::BDG(void) : gs(gsPlaying), imvCur(-1), imvPawnOrTakeLast(-1)
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
	SetGs(gsNotStarted);
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
				case apcPawn: ch = L'P'; break;
				case apcKnight: ch = L'N'; break;
				case apcBishop: ch = L'B'; break;
				case apcRook: ch = L'R'; break;
				case apcQueen: ch = L'Q'; break;
				case apcKing: ch = L'K'; break;
				default: assert(false); break;
				}
				if (CpcFromSq(sq) == cpcBlack)
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
	sz += cpcToMove == cpcWhite ? L'w' : L'b';
	
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

	if (mv.apcMove() == apcPawn || mv.fIsCapture())
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
			if (vmvGame[imvPawnOrTakeLast].apcMove() == apcPawn || 
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
	if (mv.apcMove() == apcPawn || mv.fIsCapture())
		imvPawnOrTakeLast = imvCur;
	MakeMvSq(mv);
}


/*	BDG::GsTestGameOver
 *
 *	Tests for the game in an end state. Returns the new state. Takes the legal move
 *	list for the current to-move player and the rule struture.
 */
GS BDG::GsTestGameOver(int cmvToMove, const RULE& rule) const
{
	if (cmvToMove == 0) {
		if (FInCheck(cpcToMove))
			return cpcToMove == cpcWhite ? gsWhiteCheckMated : gsBlackCheckMated;
		else
			return gsStaleMate;
	}
	else {
		/* check for draw circumstances */
		if (FDrawDead())
			return gsDrawDead;
		if (FDraw3Repeat(rule.CmvRepeatDraw()))
			return gsDraw3Repeat;
		if (FDraw50Move(50))
			return gsDraw50Move;
	}
	return gsPlaying;
}


void BDG::SetGameOver(const VEMV& vemv, const RULE& rule)
{
	SetGs(GsTestGameOver(vemv.cemv(), rule));
}


/*	BDG::FDrawDead
 *
 *	Returns true if we're in a board state where no one can force checkmate on the
 *	other player.
 */
bool BDG::FDrawDead(void) const
{
	/* any pawns, rooks, or queens means no draw */

	if (mppcbb[PC(cpcWhite, apcPawn)] || mppcbb[PC(cpcBlack, apcPawn)] ||
			mppcbb[PC(cpcWhite, apcRook)] || mppcbb[PC(cpcBlack, apcRook)] ||
			mppcbb[PC(cpcWhite, apcQueen)] || mppcbb[PC(cpcBlack, apcQueen)])
		return false;
	
	/* only bishops and knights on the board from here on out. everyone has a king */

	/* if either side has more than 2 pieces, no draw */

	if (mpcpcbb[cpcWhite].csq() > 2 || mpcpcbb[cpcBlack].csq() > 2)
		return false;

	/* K vs. K, K-N vs. K, or K-B vs. K is a draw */

	if (mpcpcbb[cpcWhite].csq() == 1 || mpcpcbb[cpcBlack].csq() == 1)
		return true;
	
	/* The other draw case is K-B vs. K-B with bishops on same color */

	if (mppcbb[pcWhiteBishop] && mppcbb[pcBlackBishop]) {
		SQ sqBishopBlack = mppcbb[pcBlackBishop].sqLow();
		SQ sqBishopWhite = mppcbb[pcWhiteBishop].sqLow();
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


/*	SzFromEv
 *
 *	Creates a string from an evaluation. Evaluations are in centi-pawns, so
 *	this basically returns a string representing a number with 2-decimal places.
 * 
 *	Handles special evaluation values, like infinities, aborts, mates, etc.,
 *	so be aware that this doesn't always return just a simple numeric string.
 */
wstring SzFromEv(EV ev)
{
	wchar_t sz[20], * pch = sz;
	if (ev >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		ev = -ev;
	}
	if (ev == evInf) {
		lstrcpy(pch, L"Inf");
		pch += lstrlen(pch);
	}
	else if (FEvIsAbort(abs(ev))) {
		lstrcpy(pch, L"(interrupted)");
		pch += lstrlen(pch);
	}
	else if (FEvIsMate(ev)) {
		*pch++ = L'#';
		pch = PchDecodeInt((PlyFromEvMate(ev) + 1) / 2, pch);
	}
	else if (ev > evInf)
		*pch++ = L'*';
	else {
		pch = PchDecodeInt(ev / 100, pch);
		ev %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + ev / 10;
		*pch++ = L'0' + ev % 10;
	}
	*pch = 0;
	return wstring(sz);
}


wstring SzFromSct(SCT sct)
{
	switch (sct) {
	case SCT::Eval:
		return L"EV";
	case SCT::XTable:
		return L"XT";
	case SCT::PrincipalVar:
		return L"PV";
	default:
		return L"";
	}
}


wstring SzFromMvm(MVM mvm)
{
	wchar_t sz[16], *pch = sz;

	*pch++ = L'a' + mvm.sqFrom().file();
	*pch++ = L'1' + mvm.sqFrom().rank();
	*pch++ = L'a' + mvm.sqTo().file();
	*pch++ = L'1' + mvm.sqTo().rank();
	APC apc;
	if ((apc = mvm.apcPromote()) != apcNull) {
		*pch++ = L'=';
		switch (apc) {
		case apcKnight:
			*pch++ = L'N';
			break;
		case apcBishop:
			*pch++ = L'B';
			break;
		case apcRook:
			*pch++ = L'R';
			break;
		case apcQueen:
			*pch++ = L'Q';
			break;
		}
	}
	*pch = 0;
	return wstring(sz);
}
