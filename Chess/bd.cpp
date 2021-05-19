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

const float BD::mpapcvpc[] = { 0.0f, 1.0f, 2.75f, 3.0f, 5.0f, 8.5f, 0.0f, -1.0f };

BD::BD(void)
{
	SetEmpty();
	cs = ((csKing | csQueen) << cpcWhite) | ((csKing | csQueen) << cpcBlack); 
	sqEnPassant = sqNil;
	Validate();
}


void BD::SetEmpty(void)
{
	assert(sizeof(mpsqtpc[0]) == 1);
	memset(mpsqtpc, tpcEmpty, sizeof(mpsqtpc));
	for (int file = 8; file < 16; file++)
		for (int rank = 0; rank < 8; rank++)
			mpsqtpc[SQ(rank, file)] = tpcNil;

	for (int color = 0; color < 2; color++) {
//		rggrfAttacked[color] = 0L;
		for (int tpc = 0; tpc < tpcPieceMax; tpc++)
			mptpcsq[color][tpc] = sqNil;
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
		if (mptpcsq[cpcWhite][tpc] != bd.mptpcsq[cpcWhite][tpc] ||
				mptpcsq[cpcBlack][tpc] != bd.mptpcsq[cpcBlack][tpc])
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
	int tpcPawn = tpcPawnFirst;
	SQ sq = SQ(rank, 0);
	const WCHAR* pch;
	for (pch = szFEN; *pch != L' ' && *pch != L'\0'; pch++) {
		switch (*pch) {
		case 'p': AddPieceFEN(sq++, tpcPawnFirst, cpcBlack, APC::Pawn); break;
		case 'r': AddPieceFEN(sq++, tpcQueenRook, cpcBlack, APC::Rook); break;
		case 'n': AddPieceFEN(sq++, tpcQueenKnight, cpcBlack, APC::Knight); break;
		case 'b': AddPieceFEN(sq++, tpcQueenBishop, cpcBlack, APC::Bishop); break;
		case 'q': AddPieceFEN(sq++, tpcQueen, cpcBlack, APC::Queen); break;
		case 'k': AddPieceFEN(sq++, tpcKing, cpcBlack, APC::King); break;
		case 'P': AddPieceFEN(sq++, tpcPawnFirst, cpcWhite, APC::Pawn); break;
		case 'R': AddPieceFEN(sq++, tpcQueenRook, cpcWhite, APC::Rook); break;
		case 'N': AddPieceFEN(sq++, tpcQueenKnight, cpcWhite, APC::Knight); break;
		case 'B': AddPieceFEN(sq++, tpcQueenBishop, cpcWhite, APC::Bishop); break;
		case 'Q': AddPieceFEN(sq++, tpcQueen, cpcWhite, APC::Queen); break;
		case 'K': AddPieceFEN(sq++, tpcKing, cpcWhite, APC::King); break;
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
			/* TODO: illegal character in the string */
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
			if (mptpcsq[cpc][tpc ^= 0x07] == sqNil)
				break;
			/* fall through */
		case APC::Pawn:
		case APC::Queen:
			tpc = TpcUnusedPawn(cpc);
			break;
		}
	}

	mpsqtpc[sq] = Tpc(tpc, cpc, apc);
	mptpcsq[cpc][tpc] = sq;
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


/*	BD::TpcUnusedPawn
 *
 *	Finds an unused pawn slot in the mptpcsq array for the cpc bcolor.
 */
int BD::TpcUnusedPawn(CPC cpc) const
{
	int tpc;
	for (tpc = tpcPawnFirst; mptpcsq[cpc][tpc] != sqNil; tpc++)
		assert(tpc < tpcPawnLim);
	return tpc;
}


/*	BD::MakeMv
 *
 *	Makes a move on the board. Assumes the move is well-formed.
 *	Handles pawn promotion, castling, and en passant captures.
 *	The previous move must be passed in to generate en passant
 *	moves.
 */
void BD::MakeMv(MV mv)
{
	Validate();
	MakeMvSq(mv);
	Validate();
}


/*	BD::MakeMvSq
 *
 *	Makes a partial move on the board, only updating the square and
 *	tpc arrays.
 */
void BD::MakeMvSq(MV mv)
{
	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	BYTE tpcFrom = mpsqtpc[sqFrom];
	BYTE tpcTo = mpsqtpc[sqTo];

	/* if we're taking a rook, we can't castle to that rook */

	if (tpcTo != tpcEmpty) {
		SqFromTpc(tpcTo) = sqNil;
		switch (tpcTo & tpcPiece) {
		case tpcKingRook: ClearCastle(CpcFromTpc(tpcTo), csKing); break;
		case tpcQueenRook: ClearCastle(CpcFromTpc(tpcTo), csQueen); break;
		default: break;
		}
	}

	/* move the pieces */

	mpsqtpc[sqFrom] = tpcEmpty;
	mpsqtpc[sqTo] = tpcFrom;
	SqFromTpc(tpcFrom) = sqTo;

	switch (ApcFromTpc(tpcFrom)) {
	case APC::Pawn:
		/* save double-pawn moves for potential en passant and return */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02) {
			sqEnPassant = SQ((sqFrom.rank() + sqTo.rank()) / 2, sqTo.file());
			return;
		}
		
		if (sqTo == sqEnPassant) {
			/* take en passant */
			SQ sqTake = SQ(sqTo.rank() ^ 1, sqTo.file());
			TPC tpcTake = mpsqtpc[sqTake];
			mpsqtpc[sqTake] = tpcEmpty;
			SqFromTpc(tpcTake) = sqNil;
		}
		else if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion on last rank */
			mpsqtpc[sqTo] = Tpc(tpcFrom & tpcPiece, CpcFromTpc(tpcFrom), mv.ApcPromote());
		} 
		break;

	case APC::King:
		ClearCastle(CpcFromTpc(tpcFrom), csKing|csQueen);
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
			BYTE tpcRook = mpsqtpc[sqRookFrom];
			mpsqtpc[sqRookFrom] = tpcEmpty;
			mpsqtpc[sqRookTo] = tpcRook;
			SqFromTpc(tpcRook) = sqRookTo;
		}
		break;

	case APC::Rook:
		ClearCastle(CpcFromTpc(tpcFrom), (tpcFrom & tpcPiece) == tpcQueenRook ? csQueen : csKing);
		break;

	default:
		break;
	}

	sqEnPassant = sqNil;
}


/*	BD::UndoMv
 *
 *	Undo the move made on the board
 */
void BD::UndoMv(MV mv)
{
	Validate();

	/* unpack some useful information out of the move */

	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	assert(mpsqtpc[sqFrom] == tpcEmpty);
	TPC tpcMove = mpsqtpc[sqTo];
	CPC cpcMove = CpcFromTpc(tpcMove);

	/* restore castle and en passant state. Castle state can only be added
	   on an undo, so it's OK to or those bits back in. Note that the en 
	   passant state only saves the file of the en passant captureable pawn 
	   (due to lack of available bits in the MV). */

	cs |= CsUnpackColor(mv.CsPrev(), cpcMove);
	if (mv.FEpPrev())
		sqEnPassant = SQ(cpcMove == cpcWhite ? 5 : 2, mv.FileEpPrev());
	else
		sqEnPassant = sqNil;

	/* put piece back in source square, undoing any pawn promotion that might
	   have happened */

	if (mv.ApcPromote() != APC::Null)
		tpcMove = TpcSetApc(tpcMove, APC::Pawn);
	mpsqtpc[sqFrom] = tpcMove;
	SqFromTpc(tpcMove) = sqFrom;
	APC apcMove = ApcFromTpc(tpcMove);	// get the type of moved piece after we've undone promotion

	/* if move was a capture, put the captured piece back on the board; otherwise
	   the destination square becomes empty */

	APC apcCapt = mv.ApcCapture();
	if (apcCapt == APC::Null)
		mpsqtpc[sqTo] = tpcEmpty;
	else {
		TPC tpcTake = Tpc(mv.TpcCapture(), cpcMove ^ 1, apcCapt);
		SQ sqTake = sqTo;
		if (sqTake == sqEnPassant) {
			/* capture into the en passant square must be en passant capture
			   pawn x pawn */
			assert(ApcFromSq(sqTo) == APC::Pawn && apcCapt == APC::Pawn);
			sqTake = SQ(sqEnPassant.rank() + cpcMove*2 - 1, sqEnPassant.file());
			mpsqtpc[sqTo] = tpcEmpty;
		}
		mpsqtpc[sqTake] = tpcTake;
		SqFromTpc(tpcTake) = sqTake;
	}

	/* undoing a castle means we need to undo the rook, too */

	if (apcMove == APC::King) {
		int dfile = sqTo.file() - sqFrom.file();
		if (dfile < -1) { /* queen side castle */
			TPC tpcRook = mpsqtpc[sqTo + 1];
			mpsqtpc[sqTo - 2] = tpcRook;
			mpsqtpc[sqTo + 1] = tpcEmpty;
			SqFromTpc(tpcRook) = sqTo - 2;
		}
		else if (dfile > 1) { /* king side castle */
			TPC tpcRook = mpsqtpc[sqTo - 1];
			mpsqtpc[sqTo + 1] = tpcRook;
			mpsqtpc[sqTo - 1] = tpcEmpty;
			SqFromTpc(tpcRook) = sqTo + 1;
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
void BD::GenRgmv(vector<MV>& rgmv, CPC cpcMove, RMCHK rmchk) const
{
	Validate();
	GenRgmvColor(rgmv, cpcMove, false);
	if (rmchk == RMCHK::Remove)
		RemoveInCheckMoves(rgmv, cpcMove);
}


void BD::GenRgmvQuiescent(vector<MV>& rgmv, CPC cpcMove, RMCHK rmchk) const
{
	GenRgmvColor(rgmv, cpcMove, true);
	if (rmchk == RMCHK::Remove) {
		RemoveInCheckMoves(rgmv, cpcMove);
		RemoveQuiescentMoves(rgmv, cpcMove);
	}
}


void BD::RemoveInCheckMoves(vector<MV>& rgmv, CPC cpcMove) const
{
 	unsigned imvDest = 0;
	for (unsigned imv = 0; imv < rgmv.size(); imv++) {
		BD bd(*this);
		bd.MakeMvSq(rgmv[imv]);
		if (!bd.FInCheck(cpcMove))
			rgmv[imvDest++] = rgmv[imv];
	}
	rgmv.resize(imvDest);
}


/*	BD::RemoveQuiescentMoves
 *
 *	Removes quiet moves from the given move list. For now quiet moves are
 *	anything that is not a capture or check.
 */
void BD::RemoveQuiescentMoves(vector<MV>& rgmv, CPC cpcMove) const
{
	if (FInCheck(cpcMove))	/* don't prune if we're in check */
		return;
	int imvTo = 0;
	for (unsigned imv = 0; imv < rgmv.size(); imv++) {
		MV mv = rgmv[imv];
		if (mpsqtpc[mv.SqTo()] != tpcEmpty && 
				VpcFromSq(mv.SqFrom()) + 0.5 < VpcFromSq(mv.SqTo()))
			rgmv[imvTo++] = mv;
		/* TODO: need to test for checks */
	}
	rgmv.resize(imvTo);
}


/*	BD::GenRgmvColor
 *
 *	Generates moves for the given color pieces. Does not check if the king is left in
 *	check, so caller must weed those moves out.
 */
void BD::GenRgmvColor(vector<MV>& rgmv, CPC cpcMove, bool fForAttack) const
{
	rgmv.clear();

	/* go through each piece */

	for (int tpc = 0; tpc < tpcPieceMax; tpc++) {
		SQ sqFrom = mptpcsq[cpcMove][tpc];
		if (sqFrom == sqNil)
			continue;
		assert(mpsqtpc[sqFrom] != tpcEmpty);
		assert(CpcFromTpc(mpsqtpc[sqFrom]) == cpcMove);
	
		switch (ApcFromSq(sqFrom)) {
		case APC::Pawn:
			GenRgmvPawn(rgmv, sqFrom);
			if (sqEnPassant != sqNil)
				GenRgmvEnPassant(rgmv, sqFrom);
			break;
		case APC::Knight:
			GenRgmvKnight(rgmv, sqFrom);
			break;
		case APC::Bishop:
			GenRgmvBishop(rgmv, sqFrom);
			break;
		case APC::Rook:
			GenRgmvRook(rgmv, sqFrom);
			break;
		case APC::Queen:
			GenRgmvQueen(rgmv, sqFrom);
			break;
		case APC::King:
			GenRgmvKing(rgmv, sqFrom);
			if (!fForAttack)
				GenRgmvCastle(rgmv, sqFrom);
			break;
		default:
			assert(false);
			break;
		}
	}
}


/*	BD::AddRgmvMv
 *
 *	Adds the move to the move vector
 */
void BD::AddRgmvMv(vector<MV>& rgmv, MV mv) const
{
	rgmv.push_back(mv);
}


bool BD::FDsqAttack(SQ sqKing, SQ sq, int dsq) const
{
	SQ sqScan = sq;
	do {
		sqScan += dsq;
		if (sqScan == sqKing)
			return true;
	} while (mpsqtpc[sqScan] == tpcEmpty);
	return false;
}


bool BD::FDiagAttack(SQ sqKing, SQ sq, int dsq) const
{
	return FDsqAttack(sqKing, sq, sqKing > sq ? dsq : -dsq);
}


bool BD::FRankAttack(SQ sqKing, SQ sq) const
{
	return FDsqAttack(sqKing, sq, sqKing > sq ? 1 : -1);
}


bool BD::FFileAttack(SQ sqKing, SQ sq) const
{
	return FDsqAttack(sqKing, sq, sqKing > sq ? 16 : -16);
}


/*	BD::FInCheck
 *
 *	Returns true if the given color's king is in check. Does not require ComputeAttacked,
 *	and does a complete scan of the opponent pieces to find checks.
 */

bool BD::FInCheck(CPC cpc) const
{
	SQ sqKing = mptpcsq[cpc][tpcKing];
	return FSqAttacked(CpcOpposite(cpc), sqKing);
}


bool BD::FSqAttacked(CPC cpcBy, SQ sqKing) const
{
	for (TPC tpc = 0; tpc < tpcPieceMax; tpc++) {
		SQ sq = mptpcsq[cpcBy][tpc];
		if (sq.FIsNil())
			continue;
		int dsq;
		switch (ApcFromSq(sq)) {
		case APC::Pawn:
			dsq = sq - sqKing;
			if (cpcBy == cpcWhite)
				dsq = -dsq;
			if (dsq == 17 || dsq == 15)
				return true;
			break;
		case APC::Knight:
			dsq = sq - sqKing;
			if (dsq < 0)
				dsq = -dsq;
			if (dsq == 33 || dsq == 31 || dsq == 18 || dsq == 14)
				return true;
			break;
		case APC::Queen:
			if (sq.rank() == sqKing.rank()) {
				if (FRankAttack(sqKing, sq))
					return true;
			}
			else if (sq.file() == sqKing.file()) {
				if (FFileAttack(sqKing, sq))
					return true;
			}
			/* fall through */
		case APC::Bishop:
			dsq = sq - sqKing;
			if (dsq % 17 == 0) {
				if (FDiagAttack(sqKing, sq, 17))
					return true;
			}
			else if (dsq % 15 == 0) {
				if (FDiagAttack(sqKing, sq, 15))
					return true;
			}
			break;
		case APC::Rook:
			if (sq.rank() == sqKing.rank()) {
				if (FRankAttack(sqKing, sq))
					return true;
			}
			else if (sq.file() == sqKing.file()) {
				if (FFileAttack(sqKing, sq))
					return true;
			}
			break;
		case APC::King:
			dsq = sq - sqKing;
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


/*	BD::GenRgmvPawn
 *
 *	Generates legal moves (without check tests) for a pawn
 *	in the sqFrom square.
 */
void BD::GenRgmvPawn(vector<MV>& rgmv, SQ sqFrom) const
{
	/* pushing pawns */

	CPC cpcFrom = CpcFromTpc(mpsqtpc[sqFrom]);
	int dsq = DsqPawnFromCpc(cpcFrom);
	SQ sqTo = sqFrom + dsq;
	if (mpsqtpc[sqTo] == tpcEmpty) {
		MV mv = MV(sqFrom, sqTo);
		if (sqTo.rank() == RankPromoteFromCpc(cpcFrom))
			AddRgmvMvPromotions(rgmv, mv);
		else {
			AddRgmvMv(rgmv, mv);	// move forward one square
			if (sqFrom.rank() == RankInitPawnFromCpc(cpcFrom)) {
				sqTo += dsq;	// move foreward two squares as first move
				if (mpsqtpc[sqTo] == tpcEmpty)
					AddRgmvMv(rgmv, MV(sqFrom, sqTo));
			}
		}
	}

	/* captures */

	GenRgmvPawnCapture(rgmv, sqFrom, dsq - 1);
	GenRgmvPawnCapture(rgmv, sqFrom, dsq + 1);
}


/*	BD::AddRgmvMvPromotions
 *
 *	When a pawn is pushed to the last rank, adds all the promotion possibilities
 *	to the move list, which includes promotions to queen, rook, knight, and
 *	bishop.
 */
void BD::AddRgmvMvPromotions(vector<MV>& rgmv, MV mv) const
{
	AddRgmvMv(rgmv, mv.SetApcPromote(APC::Queen));
	AddRgmvMv(rgmv, mv.SetApcPromote(APC::Rook));
	AddRgmvMv(rgmv, mv.SetApcPromote(APC::Bishop));
	AddRgmvMv(rgmv, mv.SetApcPromote(APC::Knight));
}


/*	BD::GenRgmvPawnCapture
 *
 *	Generates pawn capture moves of pawns on sqFrom in the dsq direction
 */
void BD::GenRgmvPawnCapture(vector<MV>& rgmv, SQ sqFrom, int dsq) const
{
	assert(ApcFromSq(sqFrom) == APC::Pawn);
	SQ sqTo = sqFrom + dsq;
	if (sqTo.FIsOffBoard())
		return;
	TPC tpcFrom = mpsqtpc[sqFrom];
	CPC cpcFrom = CpcFromTpc(tpcFrom);
	if (mpsqtpc[sqTo] != tpcEmpty && ((mpsqtpc[sqTo] ^ tpcFrom) & tpcColor)) {
		MV mv(sqFrom, sqTo);
		if (sqTo.rank() == RankPromoteFromCpc(cpcFrom))
			AddRgmvMvPromotions(rgmv, mv);
		else
			AddRgmvMv(rgmv, mv);
	}
}


/*	BD::FGenRgmvDsq
 *
 *	Checks the square in the direction given by dsq for a valid
 *	destination square. Adds the move to rgmv if it is valid.
 *	Returns true if the destination square was empty; returns
 *	false if it's not a legal square or there is a piece in the
 *	square.
 */
bool BD::FGenRgmvDsq(vector<MV>& rgmv, SQ sqFrom, SQ sq, TPC tpcFrom, int dsq) const
{
	SQ sqTo = sq + dsq;
	if (sqTo.FIsOffBoard())
		return false;

	/* have we run into one of our own pieces? */
	if (mpsqtpc[sqTo] != tpcEmpty && ((mpsqtpc[sqTo] ^ tpcFrom) & tpcColor) == 0)
		return false;

	/* add the move to the list */
	AddRgmvMv(rgmv, MV(sqFrom, sqTo));

	/* return false if we've reach an enemy piece - this capture
	   is the end of a potential slide move */
	return mpsqtpc[sqTo] == tpcEmpty;
}


/*	BD::GenRgmvKnight
 *
 *	Generates legal moves for the knight at sqFrom. Does not check that
 *	the king ends up in check.
 */
void BD::GenRgmvKnight(vector<MV>& rgmv, SQ sqFrom) const
{
	int rgdsq[8] = { 33, 31, 18, 14, -14, -18, -31, -33 };
	for (int idsq = 0; idsq < 8; idsq++)
		FGenRgmvDsq(rgmv, sqFrom, sqFrom, mpsqtpc[sqFrom], rgdsq[idsq]);
}



/*	BD::GenRgmvBishop
 *
 *	Generates moves for the bishop on square sqFrom
 */
void BD::GenRgmvBishop(vector<MV>& rgmv, SQ sqFrom) const
{
	GenRgmvSlide(rgmv, sqFrom, 17);
	GenRgmvSlide(rgmv, sqFrom, 15);
	GenRgmvSlide(rgmv, sqFrom, -15); 
	GenRgmvSlide(rgmv, sqFrom, -17);
}


void BD::GenRgmvRook(vector<MV>& rgmv, SQ sqFrom) const
{
	GenRgmvSlide(rgmv, sqFrom, 16);
	GenRgmvSlide(rgmv, sqFrom, 1);
	GenRgmvSlide(rgmv, sqFrom, -1); 
	GenRgmvSlide(rgmv, sqFrom, -16);
}


/*	BD::GenRgmvQueen
 *
 *	Generates  moves for the queen at sqFrom
 */
void BD::GenRgmvQueen(vector<MV>& rgmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::Queen);
	GenRgmvBishop(rgmv, sqFrom);
	GenRgmvRook(rgmv, sqFrom);
}


/*	BD::GenRgmvKing
 *
 *	Generates moves for the king at sqFrom. Note: like all the move generators,
 *	this does not check that the king is moving into check.
 */
void BD::GenRgmvKing(vector<MV>& rgmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == APC::King);
	static int rgdsq[8] = { 17, 16, 15, 1, -1, -15, -16, -17 };
	for (int idsq = 0; idsq < 8; idsq++)
		FGenRgmvDsq(rgmv, sqFrom, sqFrom, mpsqtpc[sqFrom], rgdsq[idsq]);
}


/*	BD::GenRgmvSlide
 *
 *	Generates all straight-line moves in the direction dsq starting at sqFrom.
 *	Stops when the piece runs into a piece of its own color, or a capture of
 *	an enemy piece, or it reaches the end of the board.
 */
void BD::GenRgmvSlide(vector<MV>& rgmv, SQ sqFrom, int dsq) const
{
	BYTE tpcFrom = mpsqtpc[sqFrom];
	for (int sq = sqFrom; FGenRgmvDsq(rgmv, sqFrom, sq, tpcFrom, dsq); sq += dsq)
		;
}


/*	BD::GenRgmvCastle
 *
 *	Generates the legal castle moves for the king at sq.
 *
 *	Note that this function only adds moves that are fully legal, and it
 *	does not require a separate check test afterwards.
 */
void BD::GenRgmvCastle(vector<MV>& rgmv, SQ sqKing) const
{
	BYTE tpcKing = mpsqtpc[sqKing];
	if (FCanCastle(CpcFromTpc(tpcKing), csKing))
		GenRgmvCastleSide(rgmv, sqKing, fileKingRook, 1);
	if (FCanCastle(CpcFromTpc(tpcKing), csQueen))
		GenRgmvCastleSide(rgmv, sqKing, fileQueenRook, -1);
}


/*	GenRgmvCastleSide
 *
 *	Checks for valid castle move on the given side. Move stored in rgmv.
 *	sqKing is the square the king is on, and we assume the caller checked
 *	that the king has not already moved. This routine checks that the rook
 *	has not already moved.
 * 
 *	Caller is responsible for verifying the king is not in check, or that
 *	the intermediate squares do not pass through check.
 */
void BD::GenRgmvCastleSide(vector<MV>& rgmv, SQ sqKing, int fileRook, int dsq) const
{
	/* all spaces between king and rook must be empty */
	int sq = sqKing;
	int sqRook = SQ(sqKing.rank(), fileRook);
	for (sq += dsq; sq != sqRook; sq += dsq) {
		if (mpsqtpc[sq] != tpcEmpty)
			return;
	}
	CPC cpcOpp = CpcOpposite(CpcFromSq(sqKing));
	/* TODO: it's probably possible to do a faster range test to see if some pieces
	 * attack this full range, rather than go through each piece 4 times */
	if (FSqAttacked(cpcOpp, sqKing) || FSqAttacked(cpcOpp, sqKing + dsq) ||
		FSqAttacked(cpcOpp, sqKing + 2 * dsq) || FSqAttacked(cpcOpp, sqKing + 3 * dsq))
		return;
	assert(ApcFromSq(sq) == APC::Rook);
	AddRgmvMv(rgmv, MV(sqKing, sqKing + 2 * dsq));
}


/*	BD::GenRgmvEnPassant
 *
 *	Generates en passant pawn moves from the pawn at sqFrom
 */
void BD::GenRgmvEnPassant(vector<MV>& rgmv, SQ sqFrom) const
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
		AddRgmvMv(rgmv, MV(sqFrom, sqEnPassant));
}


float BD::VpcFromSq(SQ sq) const
{
	assert(!sq.FIsOffBoard());
	APC apc = ApcFromTpc(mpsqtpc[sq]);
	float vpc = mpapcvpc[(int)apc];
	if (apc == APC::Pawn) {
		if (CpcFromSq(sq) == cpcWhite) {
			if (sq.rank() >= 5)
				vpc += 0.1f * (sq.rank() - 4);
		}
		else {
			if (sq.rank() <= 2)
				vpc += 0.1f * (3 - sq.rank());
		}
	}
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
		if (!sq.FIsNil())
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
	/* check the two mapping arrays are consistent, and make sure apc is legal
	   for the tpc in place */

	for (int rank = 0; rank < rankMax; rank++) {
		int file;
		for (file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			BYTE tpc = mpsqtpc[sq];
			if (tpc == tpcEmpty)
				continue;

			assert(SqFromTpc(tpc) == sq);

			int tpcPc = tpc & tpcPiece;
			APC apc = ApcFromTpc(tpc);
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
			assert(mpsqtpc[SQ(rank, file)] == tpcNil);
		}
	}

	/* check for valid castle situations */
	/* check for valid en passant situations */
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
BDG::BDG(void) : gs(GS::Playing), cpcToMove(cpcWhite), imvCur(-1), imvPawnOrTakeLast(-1)
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
	rgmvGame.clear();
	imvCur = -1;
	imvPawnOrTakeLast = -1;
	SetGs(GS::Playing);
}


void BDG::InitFEN(const WCHAR* szFEN)
{
	const WCHAR* sz = szFEN;
	InitFENPieces(sz);
	InitFENSideToMove(sz);
	InitFENCastle(sz);
	InitFENEnPassant(sz);
	InitFENHalfmoveClock(sz);
	InitFENFullmoveCounter(sz);
}


void BDG::InitFENSideToMove(const WCHAR*& sz)
{
	SkipToNonSpace(sz);
	cpcToMove = cpcWhite;
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case 'w': cpcToMove = cpcWhite ; break;
		case 'b': cpcToMove = cpcBlack; break;
		default: goto Done;
		}
	}
Done:
	SkipToSpace(sz);
}


/* TODO: move to BD */
void BDG::InitFENCastle(const WCHAR*& sz)
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


/* TODO: move to BD */
void BDG::InitFENEnPassant(const WCHAR*& sz)
{
	SkipToNonSpace(sz);
	sqEnPassant = sqNil;
	int rank=-1, file=-1;
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


void BDG::InitFENHalfmoveClock(const WCHAR*& sz)
{
	SkipToNonSpace(sz);
	SkipToSpace(sz);
}


void BDG::InitFENFullmoveCounter(const WCHAR*& sz)
{
	SkipToNonSpace(sz);
	SkipToSpace(sz);
}


/*	BDG::GenRgmv
 *
 *	Generates the legal moves for the current player who has the move.
 *	Optionally doesn't bother to remove moves that would leave the 
 *	king in check.
 */
void BDG::GenRgmv(vector<MV>& rgmv, RMCHK rmchk) const
{
	BD::GenRgmv(rgmv, cpcToMove, rmchk);
}


void BDG::GenRgmvQuiescent(vector<MV>& rgmv, RMCHK rmchk) const
{
	BD::GenRgmvQuiescent(rgmv, cpcToMove, rmchk);
}


/*	BDG::MakeMv
 *
 *	Make a move on the board.
 */
void BDG::MakeMv(MV mv)
{
	/* store undo information in the mv, and keep track of last pawn move or
	   capture move */
	
	mv.SetCsEp(CsPackColor(cs, cpcToMove), sqEnPassant);
	APC apcFrom = ApcFromSq(mv.SqFrom());
	SQ sqTake = mv.SqTo();
	if (apcFrom == APC::Pawn) {
		imvPawnOrTakeLast = imvCur + 1;
		if (sqTake == sqEnPassant)
			sqTake = SQ(cpcToMove == cpcWhite ? 4 : 3, sqEnPassant.file());
	}
	if (mpsqtpc[sqTake] != tpcEmpty) {
		imvPawnOrTakeLast = imvCur + 1;
		mv.SetCapture(ApcFromSq(sqTake), mpsqtpc[sqTake] & tpcPiece);
	}

	/* make the move and save the move in the move list */

	BD::MakeMv(mv);
	if (++imvCur == rgmvGame.size())
		rgmvGame.push_back(mv);
	else if (mv != rgmvGame[imvCur]) {
		rgmvGame[imvCur] = mv;
		/* all moves after this in the move list are now invalid */
		for (int imv = imvCur + 1; imv < rgmvGame.size(); imv++)
			rgmvGame[imv] = MV();
	}

	/* other player's move */

	cpcToMove = CpcOpposite(cpcToMove);
}


void BDG::UndoMv(void)
{
	if (imvCur < 0)
		return;
	BD::UndoMv(rgmvGame[imvCur--]);
	cpcToMove = CpcOpposite(cpcToMove);
}


void BDG::RedoMv(void)
{
	if (imvCur+1 >= rgmvGame.size() || rgmvGame[imvCur+1].FIsNil())
		return;
	imvCur++;
	BD::MakeMv(rgmvGame[imvCur]);
	cpcToMove = CpcOpposite(cpcToMove);
}


/*	BDG::GsTestGameOver
 *
 *	Tests for the game in an end state. Returns the new state. Takes the legal move
 *	list for the current to-move player and the rule struture.
 */
GS BDG::GsTestGameOver(const vector<MV>& rgmv, const RULE& rule) const
{
	if (rgmv.size() == 0) {
		if (FInCheck(cpcToMove))
			return cpcToMove == cpcWhite ? GS::WhiteCheckMated : GS::BlackCheckMated;
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


void BDG::SetGameOver(const vector<MV>& rgmv, const RULE& rule)
{
	SetGs(GsTestGameOver(rgmv, rule));
}


/*	BDG::FDrawDead
 *
 *	Returns true if we're in a board state where no one can force checkmate on the
 *	other player.
 */
bool BDG::FDrawDead(void) const
{
	/* keep total piece count for the color at apcNull, and keep seperate counts for the 
	   different color square bishops */
	int mpapccapc[2][(int)APC::Max2];	
	memset(mpapccapc, 0, sizeof(mpapccapc));

	for (CPC cpc = cpcWhite; cpc <= cpcBlack; cpc++) {
		for (int tpc = 0; tpc < tpcPieceMax; tpc++) {
			SQ sq = mptpcsq[cpc][tpc];
			if (sq == sqNil)
				continue;
			APC apc = ApcFromSq(sq);
			if (apc == APC::Bishop)
				apc += (sq & 1) * ((int)APC::Bishop2 - (int)APC::Bishop);
			else if (apc == APC::Rook || apc == APC::Queen || apc == APC::Pawn)	// rook, queen, or pawn, not dead
				return false;
			mpapccapc[cpc][(int)apc]++;
			if (++mpapccapc[cpc][(int)APC::Null] > 2)	// if total pieces more than 2, not dead
				return false;; 
		}
	}

	assert(mpapccapc[cpcWhite][(int)APC::King] == 1 && mpapccapc[cpcBlack][(int)APC::King] == 1);
	assert(mpapccapc[cpcWhite][(int)APC::Null] <= 2 && mpapccapc[cpcBlack][(int)APC::Null] <= 2);

	/* this picks off K vs. K, K vs. K-N, and K vs. K-B */

	if (mpapccapc[cpcWhite][(int)APC::Null] == 1 || mpapccapc[cpcBlack][(int)APC::Null] == 1)
		return true;

	/* The other draw case is K-B vs. K-B with bishops on same color */

	assert(mpapccapc[cpcWhite][(int)APC::Null] == 2 && mpapccapc[cpcBlack][(int)APC::Null] == 2);
	if ((mpapccapc[cpcWhite][(int)APC::Bishop] && mpapccapc[cpcBlack][(int)APC::Bishop]) ||
			(mpapccapc[cpcWhite][(int)APC::Bishop2] && mpapccapc[cpcBlack][(int)APC::Bishop2]))
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
	bd.UndoMv(rgmvGame[imvCur]);
	bd.UndoMv(rgmvGame[imvCur - 1]);
	for (int imv = imvCur - 2; imv >= imvPawnOrTakeLast + 2; imv -= 2) {
		bd.UndoMv(rgmvGame[imv]);
		bd.UndoMv(rgmvGame[imv - 1]);
		if (bd == *this) {
			if (++cbdSame >= cbdDraw)
				return true;
			imv -= 2;
			/* let's go ahead and back up two more moves here, since we can't possibly match */
			if (imv < imvPawnOrTakeLast + 2)
				break;
			bd.UndoMv(rgmvGame[imv]);
			bd.UndoMv(rgmvGame[imv - 1]);
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

