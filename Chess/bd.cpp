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
	sqEnPassant = sqNil;
	
	habd = 0L;
	gph = gphMax;
}


/*	BD::operator==
 *
 *	Compare two board states for equality. Boards are equal if all the pieces
 *	are in the same place, the castle states are the same, and the en passant
 *	state is the same.
 * 
 *	This is mainly used to detect 3-repeat draw situations. 
 */
bool BD::operator==(const BD& bd) const noexcept
{
	return habd == bd.habd;
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
	for (pch = szFEN; *pch && !isspace(*pch); pch++) {
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
	while (*sz && !isspace(*sz))
		sz++;
}


void BD::SkipToNonSpace(const wchar_t*& sz)
{
	while (*sz && isspace(*sz))
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
	SQ sq;
	for (; *sz && !isspace(*sz); sz++) {
		if (in_range(*sz, L'a', L'h'))
			sq.SetFile(*sz - L'a');
		else if (in_range(*sz, L'1', L'8'))
			sq.SetRank(*sz - '1');
		else if (*sz == '-')
			sq = sqNil;
	}
	SetEnPassant(sq);
}


/*	BD::MakeMvSq
 *
 *	Makes a partial move on the board, only updating the square,
 *	tpc arrays, and bitboards. On return, the MV is modified to include
 *	information needed to undo the move (i.e., saves state for captures,
 *	castling, en passant, and pawn promotion).
 */
void BD::MakeMvSq(MV& mv) noexcept
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

	SetEnPassant(sqNil);
Done:
	ToggleToMove();
	Validate();
}


/*	BD::UndoMvSq
 *
 *	Undo the move made on the board
 */
void BD::UndoMvSq(MV mv) noexcept
{
	if (mv.fIsNil()) {
		UndoMvNullSq(mv);
		return;
	}

	assert(FIsEmpty(mv.sqFrom()));
	CPC cpcMove = mv.cpcMove();

	/* restore castle and en passant state. */

	SetCastle(mv.csPrev());
	if (mv.fEpPrev())
		SetEnPassant(SQ(RankToEpFromCpc(cpcMove), mv.fileEpPrev()));
	else
		SetEnPassant(sqNil);

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


/*	BD::MakeMvNullSq
 *
 *	Makes a null move, i.e., skips the current movers turn. This is not a legal
 *	move, of course, but it is used by some AI heuristics, so we cobble up enough
 *	of the idea to get it to work. Keeps all board state, bitboars, and checksums.
 * 
 *	The returned mv includes the information necessary to undo the move. We ignore
 *	the move on entry.
 */
void BD::MakeMvNullSq(MV& mv) noexcept
{
	mv = mvNil;
	mv.SetPcMove(PC(cpcToMove, apcNull));
	mv.SetCsEp(csCur, sqEnPassant);
	SetEnPassant(sqNil);
	ToggleToMove();
	Validate();
}


/*	BD::UndoMvNullSq
 * 
 *	Undoes a null move made by MakeMvNullSq.
 */
void BD::UndoMvNullSq(MV mv) noexcept
{
	assert(mv.fIsNil());
	if (mv.fEpPrev())
		SetEnPassant(SQ(RankToEpFromCpc(mv.cpcMove()), mv.fileEpPrev()));
	else
		SetEnPassant(sqNil);
	ToggleToMove();
	Validate();
}


/*	BDG::GenVmve
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the
 *	king in check.
 */
void BD::GenVmve(VMVE& vmve, GG gg) noexcept
{
	GenVmve(vmve, cpcToMove, gg);
}


/*	BD::GenVmve
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenVmve(VMVE& vmve, CPC cpcMove, GG gg) noexcept
{
	Validate();
	GenVmveColor(vmve, cpcMove);
	if (gg == ggLegal)
		RemoveInCheckMoves(vmve, cpcMove);
}


/*	BD::RemoveInCheckMoves
 *
 *	Removes the invalid check moves from a pseudo move list.
 */
void BD::RemoveInCheckMoves(VMVE& vmve, CPC cpcMove) noexcept
{
 	unsigned imvDest = 0;
	for (MVE mve : vmve) {
		MakeMvSq(mve);
		if (!FInCheck(cpcMove))
			vmve[imvDest++] = mve;
		UndoMvSq(mve);
	}
	vmve.resize(imvDest);
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
	CPC cpcBy = ~cpc;

	/* for sliding pieces, reverse the attack to originate in the king, which is
	   more efficient becuase we know there is only one king */
	BB bbKing = mppcbb[PC(cpc, apcKing)];
	BB bbQueen = mppcbb[PC(cpcBy, apcQueen)];
	if (BbBishop1Attacked(bbKing) & (bbQueen | mppcbb[PC(cpcBy, apcBishop)]))
		return true;
	if (BbRook1Attacked(bbKing) & (bbQueen | mppcbb[PC(cpcBy, apcRook)]))
		return true;
	BB bbAttacks = BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)]) |
		BbPawnAttacked(mppcbb[PC(cpcBy, apcPawn)], cpcBy) |
		BbKingAttacked(mppcbb[PC(cpcBy, apcKing)]);
	return bbAttacks & bbKing;
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
BB BD::BbKnightAttacked(BB bbKnights) const noexcept
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


BB BD::BbBishop1Attacked(BB bbBishop) const noexcept
{
	SQ sq = bbBishop.sqLow();
	return BbFwdSlideAttacks(dirNorthEast, sq) |
		BbFwdSlideAttacks(dirNorthWest, sq) |
		BbRevSlideAttacks(dirSouthEast, sq) |
		BbRevSlideAttacks(dirSouthWest, sq);
}


BB BD::BbBishopAttacked(BB bbBishops) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbBishops; bbBishops.ClearLow())
		bbAttacks |= BbBishop1Attacked(bbBishops);
	return bbAttacks;
}


BB BD::BbRook1Attacked(BB bbRook) const noexcept
{
	SQ sq = bbRook.sqLow();
	return BbFwdSlideAttacks(dirNorth, sq) |
		BbFwdSlideAttacks(dirEast, sq) |
		BbRevSlideAttacks(dirSouth, sq) |
		BbRevSlideAttacks(dirWest, sq);
}


BB BD::BbRookAttacked(BB bbRooks) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbRooks; bbRooks.ClearLow())
		bbAttacks |= BbRook1Attacked(bbRooks);
	return bbAttacks;
}


BB BD::BbQueen1Attacked(BB bbQueen) const noexcept
{
	SQ sq = bbQueen.sqLow();
	return BbFwdSlideAttacks(dirNorth, sq) |
		BbFwdSlideAttacks(dirEast, sq) |
		BbRevSlideAttacks(dirSouth, sq) |
		BbRevSlideAttacks(dirWest, sq) |
		BbFwdSlideAttacks(dirNorthEast, sq) |
		BbFwdSlideAttacks(dirNorthWest, sq) |
		BbRevSlideAttacks(dirSouthEast, sq) |
		BbRevSlideAttacks(dirSouthWest, sq);
}


BB BD::BbQueenAttacked(BB bbQueens) const noexcept
{
	BB bbAttacks = 0;
	for ( ; bbQueens; bbQueens.ClearLow())
		bbAttacks |= BbQueen1Attacked(bbQueens);
	return bbAttacks;
}


BB BD::BbKingAttacked(BB bbKing) const noexcept
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
	if (BbKnightAttacked(mppcbb[PC(cpcBy, apcKnight)]) & bbAttacked)
		return apcKnight;
	if (BbBishopAttacked(mppcbb[PC(cpcBy, apcBishop)]) & bbAttacked)
		return apcBishop;
	if (BbRookAttacked(mppcbb[PC(cpcBy, apcRook)]) & bbAttacked)
		return apcRook;
	if (BbQueenAttacked(mppcbb[PC(cpcBy, apcQueen)]) & bbAttacked)
		return apcQueen;
	if (BbKingAttacked(mppcbb[PC(cpcBy, apcKing)]) & bbAttacked)
		return apcKing;
	return apcNull;
}


/*	BD::GenVmveColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenVmveColor(VMVE& vmve, CPC cpcMove) const noexcept
{
	vmve.clear();

	GenVmvePawnMvs(vmve, mppcbb[PC(cpcMove, apcPawn)], cpcMove);

	/* generate knight moves */

	for (BB bb = mppcbb[PC(cpcMove, apcKnight)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = mpbb.BbKnightTo(sqFrom);
		GenVmveBbMvs(vmve, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcKnight));
	}

	/* generate bishop moves */

	for (BB bb = mppcbb[PC(cpcMove, apcBishop)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbBishop1Attacked(bb); 
		GenVmveBbMvs(vmve, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcBishop));
	}

	/* generate rook moves */

	for (BB bb = mppcbb[PC(cpcMove, apcRook)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbRook1Attacked(bb);
		GenVmveBbMvs(vmve, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcRook));
	}

	/* generate queen moves */

	for (BB bb = mppcbb[PC(cpcMove, apcQueen)]; bb; bb.ClearLow()) {
		SQ sqFrom = bb.sqLow();
		BB bbTo = BbQueen1Attacked(bb);
		GenVmveBbMvs(vmve, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcQueen));
	}

	/* generate king moves */

	BB bbKing = mppcbb[PC(cpcMove, apcKing)];
	assert(bbKing.csq() == 1);
	SQ sqFrom = bbKing.sqLow();
	BB bbTo = mpbb.BbKingTo(sqFrom);
	GenVmveBbMvs(vmve, sqFrom, bbTo - mpcpcbb[cpcMove], PC(cpcMove, apcKing));
	GenVmveCastle(vmve, sqFrom, cpcMove);
}


/*	BD::GenVmveBbMvs
 *
 *	Generates all moves with destination squares in bbTo and square offset to
 *	the soruce square dsq 
 */
void BD::GenVmveBbMvs(VMVE& vmve, BB bbTo, int dsq, PC pcMove) const noexcept
{
	for (; bbTo; bbTo.ClearLow()) {
		SQ sqTo = bbTo.sqLow();
		vmve.push_back(sqTo + dsq, sqTo, pcMove);
	}
}


/*	BD::GenVmveBbMvs
 *
 *	Generates all moves with the source square sqFrom and the destination squares
 *	in bbTo.
 */
void BD::GenVmveBbMvs(VMVE& vmve, SQ sqFrom, BB bbTo, PC pcMove) const noexcept
{
	for (; bbTo; bbTo.ClearLow())
		vmve.push_back(sqFrom, bbTo.sqLow(), pcMove);
}


void BD::GenVmveBbPawnMvs(VMVE& vmve, BB bbTo, BB bbRankPromotion, int dsq, CPC cpcMove) const noexcept
{
	if (bbTo & bbRankPromotion) {
		for (BB bbT = bbTo & bbRankPromotion; bbT; bbT.ClearLow()) {
			SQ sqTo = bbT.sqLow();
			AddVmveMvPromotions(vmve, MV(sqTo + dsq, sqTo, PC(cpcMove, apcPawn)));
		}
		bbTo -= bbRankPromotion;
	}
	GenVmveBbMvs(vmve, bbTo, dsq, PC(cpcMove, apcPawn));
}


void BD::GenVmvePawnMvs(VMVE& vmve, BB bbPawns, CPC cpcMove) const noexcept
{
	if (cpcMove == cpcWhite) {
		BB bbTo = (bbPawns << 8) & bbUnoccupied;
		GenVmveBbPawnMvs(vmve, bbTo, bbRank8, -8, cpcMove);
		bbTo = ((bbTo & bbRank3) << 8) & bbUnoccupied;
		GenVmveBbMvs(vmve, bbTo, -16, PC(cpcMove, apcPawn));
		bbTo = ((bbPawns - bbFileA) << 7) & mpcpcbb[cpcBlack];
		GenVmveBbPawnMvs(vmve, bbTo, bbRank8, -7, cpcMove);
		bbTo = ((bbPawns - bbFileH) << 9) & mpcpcbb[cpcBlack];
		GenVmveBbPawnMvs(vmve, bbTo, bbRank8, -9, cpcMove);
	}
	else {
		BB bbTo = (bbPawns >> 8) & bbUnoccupied;
		GenVmveBbPawnMvs(vmve, bbTo, bbRank1, 8, cpcMove);
		bbTo = ((bbTo & bbRank6) >> 8) & bbUnoccupied;
		GenVmveBbMvs(vmve, bbTo, 16, PC(cpcMove, apcPawn));
		bbTo = ((bbPawns - bbFileA) >> 9) & mpcpcbb[cpcWhite];
		GenVmveBbPawnMvs(vmve, bbTo, bbRank1, 9, cpcMove);
		bbTo = ((bbPawns - bbFileH) >> 7) & mpcpcbb[cpcWhite];
		GenVmveBbPawnMvs(vmve, bbTo, bbRank1, 7, cpcMove);
	}
	
	if (!sqEnPassant.fIsNil()) {
		BB bbEnPassant(sqEnPassant), bbFrom;
		if (cpcMove == cpcWhite)
			bbFrom = ((bbEnPassant - bbFileA) >> 9) | ((bbEnPassant - bbFileH) >> 7);
		else
			bbFrom = ((bbEnPassant - bbFileA) << 7) | ((bbEnPassant - bbFileH) << 9);
		for (bbFrom &= bbPawns; bbFrom; bbFrom.ClearLow()) {
			SQ sqFrom = bbFrom.sqLow();
			vmve.push_back(sqFrom, sqEnPassant, PC(cpcMove, apcPawn));
		}
	}
}


/*	BD::AddVmveMvPromotions
 *
 *	When a pawn is pushed to the last rank, adds all the promotion possibilities
 *	to the move list, which includes promotions to queen, rook, knight, and
 *	bishop.
 */
void BD::AddVmveMvPromotions(VMVE& vmve, MV mv) const noexcept
{
	mv.SetApcPromote(apcQueen);
	vmve.push_back(mv);
	mv.SetApcPromote(apcRook);
	vmve.push_back(mv);
	mv.SetApcPromote(apcBishop);
	vmve.push_back(mv);
	mv.SetApcPromote(apcKnight);
	vmve.push_back(mv);
}


/*	BD::GenVmveCastle
 *
 *	Generates the legal castle moves for the king at sq. Checks that the king is in
 *	check and intermediate squares are not under attack, checks for intermediate 
 *	squares are empty, but does not check the final king destination for in check.
 */
void BD::GenVmveCastle(VMVE& vmve, SQ sqKing, CPC cpcMove) const noexcept
{
	BB bbKing = BB(sqKing);
	if (FCanCastle(cpcMove, csKing) && !(((bbKing << 1) | (bbKing << 2)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing << 1), ~cpcMove) == apcNull)
			vmve.push_back(sqKing, sqKing + 2, PC(cpcMove, apcKing));
	}
	if (FCanCastle(cpcMove, csQueen) && !(((bbKing >> 1) | (bbKing >> 2) | (bbKing >> 3)) - bbUnoccupied)) {
		if (ApcBbAttacked(bbKing | (bbKing >> 1), ~cpcMove) == apcNull) 
			vmve.push_back(sqKing, sqKing - 2, PC(cpcMove, apcKing));
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
void BD::Validate(void) const noexcept
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

	assert(sqEnPassant.fValid());
	if (!sqEnPassant.fIsNil()) {
		if (sqEnPassant.rank() == rank3) {
			assert(cpcToMove == cpcBlack);
			assert(mppcbb[pcWhitePawn].fSet(SQ(rank4, sqEnPassant.file())));
		}
		else if (sqEnPassant.rank() == rank6) {
			assert(cpcToMove == cpcWhite);
			assert(mppcbb[pcBlackPawn].fSet(SQ(rank5, sqEnPassant.file())));
		}
		else {
			assert(false);
		}
	}

	/* game phase should be accurate */

	assert(gph == GphCompute());

	/* make sure hash is kept accurate */

	assert(habd == genhabd.HabdFromBd(*this));

}


void BD::ValidateBB(PC pcVal, SQ sq) const noexcept
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
BDG::BDG(void) noexcept : gs(gsPlaying), imvCurLast(-1), imvPawnOrTakeLast(-1)
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


BDG::BDG(const BDG& bdg) noexcept : BD(bdg), gs(bdg.gs), vmvGame(bdg.vmvGame), 
		imvPawnOrTakeLast(bdg.imvPawnOrTakeLast), imvCurLast(bdg.imvCurLast)
{
}


const wchar_t BDG::szFENInit[] = L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void BDG::InitGame(const wchar_t* szFEN)
{
	if (szFEN == nullptr)
		szFEN = szFENInit;
	const wchar_t* sz = szFEN;

	vmvGame.clear();
	imvCurLast = -1;
	imvPawnOrTakeLast = -1;
	SetGs(gsNotStarted);

	InitFENPieces(sz);
	InitFENSideToMove(sz);
	InitFENCastle(sz);
	InitFENEnPassant(sz);
	InitFENHalfmoveClock(sz);
	InitFENFullmoveCounter(sz);


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
	sz += to_wstring(imvCurLast - imvPawnOrTakeLast);

	/* fullmove number */

	sz += L' ';
	sz += to_wstring(1 + imvCurLast/2);

	return sz;
}


/*	BDG::MakeMv
 *
 *	Make a move on the board, and keeps the move list for the game. Caller is
 *	responsible for testing for game over.
 */
void BDG::MakeMv(MV& mv) noexcept
{
	assert(!mv.fIsNil());

	/* make the move and save the move in the move list */

	MakeMvSq(mv);
	if (++imvCurLast == vmvGame.size())
		vmvGame.push_back(mv);
	else if (mv != vmvGame[imvCurLast]) {
		vmvGame[imvCurLast] = mv;
		/* all moves after this in the move list are now invalid */
		for (size_t imv = (size_t)imvCurLast + 1; imv < vmvGame.size(); imv++)
			vmvGame[imv] = MV();
	}

	/* keep track of last pawn move or capture, which is used to detect draws */

	if (mv.apcMove() == apcPawn || mv.fIsCapture())
		imvPawnOrTakeLast = imvCurLast;
}


void BDG::MakeMvNull(void) noexcept
{
	MV mv;
	MakeMvNullSq(mv);
	if (++imvCurLast == vmvGame.size())
		vmvGame.push_back(mv);
	else 
		vmvGame[imvCurLast] = mv;
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
	if (FInCheck(~cpc)) {
		/* TODO: check for mates */
		sz += L'+';
	}
	return sz;
}


/*	BDG::UndoMv
 *
 *	Undoes the last made move at imvCur. Caller is responsible for resetting game
 *	over state
 */
void BDG::UndoMv(void) noexcept
{
	if (imvCurLast < 0)
		return;
	if (imvCurLast == imvPawnOrTakeLast) {
		/* scan backwards looking for pawn moves or captures */
		for (imvPawnOrTakeLast = imvCurLast-1; imvPawnOrTakeLast >= 0; imvPawnOrTakeLast--)
			if (vmvGame[imvPawnOrTakeLast].apcMove() == apcPawn || 
					vmvGame[imvPawnOrTakeLast].fIsCapture())
				break;
	}
	UndoMvSq(vmvGame[imvCurLast--]);
	assert(imvCurLast >= -1);
}


/*	BDG::RedoMv
 *
 *	Redoes that last undone move, which will be at imvCur+1. Caller is responsible
 *	for testing for game-over state afterwards.
 */
void BDG::RedoMv(void) noexcept
{
	if (imvCurLast > (int)vmvGame.size() || vmvGame[imvCurLast+1].fIsNil())
		return;
	imvCurLast++;
	MV mv = vmvGame[imvCurLast];
	if (mv.apcMove() == apcPawn || mv.fIsCapture())
		imvPawnOrTakeLast = imvCurLast;
	MakeMvSq(mv);
}


/*	BDG::GsTestGameOver
 *
 *	Tests for the game in an end state. Returns the new state. Takes the legal move
 *	list for the current to-move player and the rule struture.
 */
GS BDG::GsTestGameOver(int cmvToMove, int cmvRepeatDraw) const noexcept
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
		if (FDraw3Repeat(cmvRepeatDraw))
			return gsDraw3Repeat;
		if (FDraw50Move(50))
			return gsDraw50Move;
	}
	return gsPlaying;
}


void BDG::SetGameOver(const VMVE& vmve, const RULE& rule) noexcept
{
	SetGs(GsTestGameOver(vmve.cmve(), rule.CmvRepeatDraw()));
}


/*	BDG::FDrawDead
 *
 *	Returns true if we're in a board state where no one can force checkmate on the
 *	other player.
 */
bool BDG::FDrawDead(void) const noexcept
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
 * 
 *	cbdDraw = 0 means don't check for repeated position draws, always return false.
 */
bool BDG::FDraw3Repeat(int cbdDraw) const noexcept
{
	if (cbdDraw == 0)
		return false;
	if (imvCurLast - imvPawnOrTakeLast < (cbdDraw-1) * 2 * 2)
		return false;
	BD bd = *this;
	int cbdSame = 1;
	assert(imvCurLast - 1 >= 0);
	bd.UndoMvSq(vmvGame[imvCurLast]);
	bd.UndoMvSq(vmvGame[imvCurLast - 1]);
	for (int imv = imvCurLast - 2; imv >= imvPawnOrTakeLast + 2; imv -= 2) {
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
bool BDG::FDraw50Move(int64_t cmvDraw) const noexcept
{
	return imvCurLast - imvPawnOrTakeLast >= cmvDraw * 2;
}


/*	BDG::SetGs
 *
 *	Sets the game state.
 */
void BDG::SetGs(GS gs) noexcept
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
	else if (ev == evCanceled) {
		lstrcpy(pch, L"(interrupted)");
		pch += lstrlen(pch);
	}
	else if (ev == evTimedOut) {
		lstrcpy(pch, L"(timeout)");
		pch += lstrlen(pch);
	}
	else if (FEvIsMate(ev)) {
		*pch++ = L'#';
		pch = PchDecodeInt((DepthFromEvMate(ev) + 1) / 2, pch);
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


wstring SzFromTsc(TSC tsc)
{
	switch (tsc) {
	case tscEvOther:
		return L"EV";
	case tscEvCapture:
		return L"CAP";
	case tscXTable:
		return L"XT";
	case tscPrincipalVar:
		return L"PV";
	default:
		return L"";
	}
}


wstring SzFromMvm(MVM mvm)
{
	if (mvm.fIsNil())
		return L"--";
	wchar_t sz[8], *pch = sz;
	*pch++ = L'a' + mvm.sqFrom().file();
	*pch++ = L'1' + mvm.sqFrom().rank();
	*pch++ = L'a' + mvm.sqTo().file();
	*pch++ = L'1' + mvm.sqTo().rank();
	if (mvm.apcPromote() != apcNull) {
		*pch++ = L'=';
		*pch++ = L" PNBRQK"[mvm.apcPromote()];
	}
	*pch = 0;
	return wstring(sz);
}


/*
 *
 *	RULE class
 * 
 *	Our one-shop stop for all the rule variants that we might have. Includes stuff
 *	like time control, repeat position counts, etc.
 * 
 */


RULE::RULE(void) : cmvRepeatDraw(3)
{
	//vtmi.push_back(TMI(0, -1, msecMin*30, msecSec*3));	/* 30min and 3sec is TCEC early time control */
	vtmi.push_back(TMI(0, 40, msecMin * 100, 0));
	vtmi.push_back(TMI(41, 60, msecMin * 50, 0));
	vtmi.push_back(TMI(61, -1, msecMin * 15, msecSec * 30));
}


/*	RULE::RULE
 *
 *	Create a new rule set with simple game/move increments and the number of repeated
 *	positions to trigger a draw.
 */
RULE::RULE(int dsecGame, int dsecMove, unsigned cmvRepeatDraw) : cmvRepeatDraw(cmvRepeatDraw)
{
	vtmi.push_back(TMI(0, -1, msecSec * dsecGame, msecSec * dsecMove));
}


/*	RULE::FUntimed
 *
 *	Returns true if this is an untimed game.
 */
bool RULE::FUntimed(void) const
{
	return vtmi.size() == 0;
}


/*	RULE::DmsecAddInterval
 *
 *	If the current board state is at the transition between timing intervals, returns
 *	the amount of time to add to the CPC player's clock after move number mvn is made.
 *	
 *	WARNING! mvn is 1-based.
 */
DWORD RULE::DmsecAddInterval(CPC cpc, int mvn) const
{
	for (const TMI& tmi : vtmi) {
		if (tmi.mvnFirst == mvn)	
			return tmi.dmsec;
	}

	return 0;
}


/*	RULE::DmsecAddMove
 *
 *	After a player moves, returns the amount of time to modify the player's clock
 *	based on move bonus.
 */
DWORD RULE::DmsecAddMove(CPC cpc, int mvn) const
{
	for (const TMI& tmi : vtmi) {
		if (tmi.mvnLast == -1 || mvn <= tmi.mvnLast)
			return tmi.dmsecMove;
	}
	return 0;
}


/*	RULE::CmvRepeatDraw
 *
 *	Number of repeated positions used for detecting draws. Typically 3.
 */
int RULE::CmvRepeatDraw(void) const
{
	return cmvRepeatDraw;
}


/*	RULE::SetGameTime
 *
 *	Sets the amount of time each player gets on their game clock. -1 for an
 *	untimed game.
 * 
 *	This is not a very good general purpose time control API. Should be replaced
 *	with something more generally useful.
 */
void RULE::SetGameTime(CPC cpc, DWORD dsec)
{
	vtmi.clear();
	if (dsec != -1)
		vtmi.push_back(TMI(1, -1, msecSec*dsec, 0));
}