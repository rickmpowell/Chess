/*
 *
 *	bd.cpp
 * 
 *	Chess board
 * 
 */


#include "Chess.h"


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
	assert(sizeof(mpsqtpc[0]) == 1);
	memset(mpsqtpc, tpcEmpty, sizeof(mpsqtpc));
	for (int color = 0; color < 2; color++) {
		rggrfAttacked[color] = 0L;
		for (int tpc = 0; tpc < tpcPieceMax; tpc++)
			mptpcsq[color][tpc] = sqNil;
	}
	cs = ((csKing | csQueen) << cpcWhite) | ((csKing | csQueen) << cpcBlack); 
	sqEnPassant = sqNil;
	Validate();
}


BD::BD(const BD& bd)
{
	memcpy(mpsqtpc, bd.mpsqtpc, sizeof(mpsqtpc));
	memcpy(rggrfAttacked, bd.rggrfAttacked, sizeof(rggrfAttacked));
	memcpy(mptpcsq, bd.mptpcsq, sizeof(mptpcsq));
	cs = bd.cs;
	sqEnPassant = bd.sqEnPassant;
	Validate();
}


/*	BD::SzInitFENPieces
 *
 *	Reads the pieces out a Forsyth-Edwards Notation string. The piece
 *	table should start at szFEN. Returns the character after the last
 *	piece character, which will probably be the space character.
 */
void BD::InitFENPieces(const WCHAR*& szFEN)
{
	/* mark everything empty */
	
	SQ sq;
	for (sq = 0; sq < sqMax; sq++)
		mpsqtpc[sq] = tpcEmpty;
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; cpc++)
		for (int tpc = 0; tpc < tpcPieceMax; tpc++)
			mptpcsq[cpc][tpc] = sqNil;

	/* parse the line */

	int rank = rankMax - 1;
	int tpcPawn = tpcPawnFirst;
	sq = SQ(rank, 0);
	const WCHAR* pch;
	for (pch = szFEN; *pch != L' ' && *pch != L'\0'; pch++) {
		switch (*pch) {
		case 'p': AddPieceFEN(sq++, tpcPawnFirst, cpcBlack, apcPawn); break;
		case 'r': AddPieceFEN(sq++, tpcQueenRook, cpcBlack, apcRook); break;
		case 'n': AddPieceFEN(sq++, tpcQueenKnight, cpcBlack, apcKnight); break;
		case 'b': AddPieceFEN(sq++, tpcQueenBishop, cpcBlack, apcBishop); break;
		case 'q': AddPieceFEN(sq++, tpcQueen, cpcBlack, apcQueen); break;
		case 'k': AddPieceFEN(sq++, tpcKing, cpcBlack, apcKing); break;
		case 'P': AddPieceFEN(sq++, tpcPawnFirst, cpcWhite, apcPawn); break;
		case 'R': AddPieceFEN(sq++, tpcQueenRook, cpcWhite, apcRook); break;
		case 'N': AddPieceFEN(sq++, tpcQueenKnight, cpcWhite, apcKnight); break;
		case 'B': AddPieceFEN(sq++, tpcQueenBishop, cpcWhite, apcBishop); break;
		case 'Q': AddPieceFEN(sq++, tpcQueen, cpcWhite, apcQueen); break;
		case 'K': AddPieceFEN(sq++, tpcKing, cpcWhite, apcKing); break;
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
		case apcKing: assert(false); return;
		default:
			/* try piece on other side; otherwise it's a promoted pawn */
			if (mptpcsq[cpc][tpc ^= 0x07] == sqNil)
				break;
			/* fall through */
		case apcPawn:
		case apcQueen:
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


/*	BD::ComputeAttacked
 *
 *	Computes the attacked square bit mask for the given color
 */
void BD::ComputeAttacked(CPC cpc)
{
	vector<MV> rgmv;
	GenRgmvColor(rgmv, cpc);
	UINT64 grfAttacked = 0;
	for (MV mv : rgmv)
		grfAttacked |= 1LL << mv.SqTo();
	rggrfAttacked[cpc] = grfAttacked;
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
	ComputeAttacked(cpcWhite);
	ComputeAttacked(cpcBlack);
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
	if (mpsqtpc[sqTo] != tpcEmpty)
		SqFromTpc(mpsqtpc[sqTo]) = sqNil;
	mpsqtpc[sqFrom] = tpcEmpty;
	mpsqtpc[sqTo] = tpcFrom;
	SqFromTpc(tpcFrom) = sqTo;
	switch (ApcFromTpc(tpcFrom)) {
	case apcPawn:
		/* save double-pawn moves for potential en passant */
		if (((sqFrom.rank() ^ sqTo.rank()) & 0x03) == 0x02)
			sqEnPassant = SQ((sqFrom.rank() + sqTo.rank()) / 2, sqTo.file());
		else {
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
			sqEnPassant = sqNil;
		}
		break;
	case apcKing:
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
	case apcRook:
		ClearCastle(CpcFromTpc(tpcFrom), (tpcFrom & tpcPiece) == tpcQueenRook ? csQueen : csKing);
		break;
	default:
		break;
	}
}


/*	BD::UndoLastMv
 *
 *	Undo the last move made on the board
 */
void BD::UndoLastMv(MV mv)
{
	assert(false);
}


/*	BD::GenRgmv
 *
 *	Generates verified moves for the given board position,
 *	where verified means we make sure all moves do not leave our own 
 *	king in check
 */
void BD::GenRgmv(vector<MV>& rgmv, CPC cpcMove) const
{
	Validate();
	GenRgmvColor(rgmv, cpcMove);
	RemoveInCheckMoves(rgmv, cpcMove);
}


void BD::RemoveInCheckMoves(vector<MV>& rgmv, CPC cpcMove) const
{
 	unsigned imvDest = 0;
	int cpcOpp = cpcMove ^ 1;
	for (unsigned imv = 0; imv < rgmv.size(); imv++) {
		BD bd = *this;
		bd.MakeMvSq(rgmv[imv]);
		bd.ComputeAttacked(cpcOpp);
		if (!(bd.FSqAttacked(bd.mptpcsq[cpcMove][tpcKing], cpcOpp)))
			rgmv[imvDest++] = rgmv[imv];
	}
	rgmv.resize(imvDest);
}


/*	BD::FSqAttacked
 *
 *	Checks that the square sq is attacked by a piece of color tpcBy. Returns true if
 *	the square is attacked.
 */
bool BD::FSqAttacked(SQ sq, CPC cpcBy) const
{
	UINT64 grf = 1LL << sq;
	return (rggrfAttacked[cpcBy] & grf) != 0;
}


/*	BD::GenRgmvColor
 *
 *	Generates moves for the given color pieces. The previous move is
 *	provided, for checking en passant moves.
 */
void BD::GenRgmvColor(vector<MV>& rgmv, CPC cpcMove) const
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
		case apcPawn:
			GenRgmvPawn(rgmv, sqFrom);
			if (sqEnPassant != sqNil)
				GenRgmvEnPassant(rgmv, sqFrom);
			break;
		case apcKnight:
			GenRgmvKnight(rgmv, sqFrom);
			break;
		case apcBishop:
			GenRgmvBishop(rgmv, sqFrom);
			break;
		case apcRook:
			GenRgmvRook(rgmv, sqFrom);
			break;
		case apcQueen:
			GenRgmvQueen(rgmv, sqFrom);
			break;
		case apcKing:
			GenRgmvKing(rgmv, sqFrom);
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


/*	BD::FInCheck
 *
 *	Returns true if the king at square sqKing is under attack by an opponent
 *	piece.
 */
bool BD::FInCheck(SQ sqKing) const
{
	assert((mpsqtpc[sqKing] & tpcPiece) == tpcKing);
	return ((1LL << sqKing) & rggrfAttacked[!CpcFromTpc(mpsqtpc[sqKing])]);
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
	int dsq = cpcFrom  == cpcWhite ? 8 : -8;
	SQ sqTo = sqFrom + dsq;
	if (mpsqtpc[sqTo] == tpcEmpty) {
		MV mv = MV(sqFrom, sqTo);
		if (sqTo.rank() == 0 || sqTo.rank() == 7) {
			/* pawn promotion */

			for (BYTE apc = apcQueen; apc >= apcKnight; apc--) {
				mv.SetApcPromote(apc);
				AddRgmvMv(rgmv, mv);
			}
		}
		else {
			AddRgmvMv(rgmv, mv);	// move forward one square
			if ((cpcFrom==cpcWhite && sqFrom.rank()==1) || 
					(cpcFrom==cpcBlack && sqFrom.rank()==6)) {
				sqTo += dsq;	// move foreward two squares as first move
				if (mpsqtpc[sqTo] == tpcEmpty)
					AddRgmvMv(rgmv, MV(sqFrom, sqTo));
			}
		}
	}

	/* captures */

	if (sqFrom.file() != 0)
		GenRgmvPawnCapture(rgmv, sqFrom, dsq - 1);
	if (sqFrom.file() != 7)
		GenRgmvPawnCapture(rgmv, sqFrom, dsq + 1);
}


/*	BD::GenRgmvPawnCapture
 *
 *	Generates pawn capture moves of pawns on sqFrom in the dsq direction
 */
void BD::GenRgmvPawnCapture(vector<MV>& rgmv, SQ sqFrom, int dsq) const
{
	assert(ApcFromSq(sqFrom) == apcPawn);
	int sqTo = sqFrom + dsq;
	if (mpsqtpc[sqTo] != tpcEmpty && ((mpsqtpc[sqTo] ^ mpsqtpc[sqFrom]) & tpcColor))
		AddRgmvMv(rgmv, MV(sqFrom, sqTo));
}


/*	BD::GenRgmvKnight
 *
 *	Generates legal moves for the knight at sqFrom. Does not check that
 *	the king ends up in check.
 */
void BD::GenRgmvKnight(vector<MV>& rgmv, SQ sqFrom) const
{
	int rgdsq[8] = { 17, 15, 10, 6, -6, -10, -15, -17 };
	for (int idsq = 0; idsq < 8; idsq++)
		FGenRgmvDsq(rgmv, sqFrom, sqFrom, mpsqtpc[sqFrom], rgdsq[idsq]);
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
	
	/* did we move off the bottom or top edge of the board? */
	if (!sqTo.FIsValid())
		return false;

	/* have we run into one of our own pieces? */
	if (mpsqtpc[sqTo] != tpcEmpty && ((mpsqtpc[sqTo] ^ tpcFrom) & tpcColor) == 0)
		return false;
	
	/* knights can potentially move 2 files away, all other pieces 
	   only move at most one file at a time during the slide; any 
	   other value is a wrap-around on the side, which is not legal */
	int dfile = sq.file() - sqTo.file();
	if (dfile < -2 || dfile > 2)
		return false;
	
	/* add the move to the list */
	AddRgmvMv(rgmv, MV(sqFrom, sqTo));

	/* return false if we've reach an enemy piece - this capture
	   is the end of a potential slide move */
	return mpsqtpc[sqTo] == tpcEmpty;
}


/*	BD::GenRgmvBishop
 *
 *	Generates moves for the bishop on square sqFrom
 */
void BD::GenRgmvBishop(vector<MV>& rgmv, SQ sqFrom) const
{
	GenRgmvSlide(rgmv, sqFrom, 9);
	GenRgmvSlide(rgmv, sqFrom, 7);
	GenRgmvSlide(rgmv, sqFrom, -7); 
	GenRgmvSlide(rgmv, sqFrom, -9);
}


void BD::GenRgmvRook(vector<MV>& rgmv, SQ sqFrom) const
{
	GenRgmvSlide(rgmv, sqFrom, 8);
	GenRgmvSlide(rgmv, sqFrom, 1);
	GenRgmvSlide(rgmv, sqFrom, -1); 
	GenRgmvSlide(rgmv, sqFrom, -8);
}


/*	BD::GenRgmvQueen
 *
 *	Generates  moves for the queen at sqFrom
 */
void BD::GenRgmvQueen(vector<MV>& rgmv, SQ sqFrom) const
{
	assert(ApcFromSq(sqFrom) == apcQueen);
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
	assert(ApcFromSq(sqFrom) == apcKing);
	static int rgdsq[8] = { 9, 8, 7, 1, -1, -7, -8, -9 };
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
	const UINT64 grfQueenSideCastleAttacked = ((UINT64)((1 << fileQueenBishop) | (1 << fileQueen) | (1 << fileKing))) << (sqKing - fileKing);
	const UINT64 grfKingSideCastleAttacked = grfQueenSideCastleAttacked << 2;
	UINT64 grfAttacked = rggrfAttacked[!CpcFromTpc(tpcKing)];
	if (FCanCastle(CpcFromTpc(tpcKing), csKing)) {
		if (!(grfKingSideCastleAttacked & grfAttacked))
			GenRgmvCastleSide(rgmv, sqKing, fileKingRook, 1);
	}
	if (FCanCastle(CpcFromTpc(tpcKing), csQueen)) {
		if (!(grfQueenSideCastleAttacked & grfAttacked))
			GenRgmvCastleSide(rgmv, sqKing, fileQueenRook, -1);
	}
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
	/* still must be legal to castle on this side */
	if (FCanCastle(CpcFromSq(sqKing), fileRook == fileQueenRook ? csQueen : csKing)) {
		assert(ApcFromSq(sq) == apcRook);
		AddRgmvMv(rgmv, MV(sqKing, sqKing + 2 * dsq));
	}
}


/*	BD::GenRgmvEnPassant
 *
 *	Generates en passant pawn moves from the pawn at sqFrom
 */
void BD::GenRgmvEnPassant(vector<MV>& rgmv, SQ sqFrom) const
{
	assert(sqEnPassant != sqNil);
	assert(ApcFromSq(sqFrom) == apcPawn);

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


/*	BD::Validate
 *
 *	Checks the board state for internal consistency
 */
#ifndef NDEBUG
void BD::Validate(void) const
{
	/* check the two mapping arrays are consistent, and make sure apc is legal
	   for the tpc in place */

	for (int sq = 0; sq < sqMax; sq++) {
		BYTE tpc = mpsqtpc[sq];
		if (tpc == tpcEmpty)
			continue;

		assert(mptpcsq[CpcFromTpc(tpc)][tpc & tpcPiece] == sq);

		int tpcPc = tpc & tpcPiece;
		int apc = ApcFromTpc(tpc);
		switch (tpcPc) {
		case tpcQueenRook:
		case tpcKingRook:
			assert(apc == apcRook);
			break;
		case tpcQueenKnight:
		case tpcKingKnight:
			assert(apc == apcKnight);
			break;
		case tpcQueenBishop:
		case tpcKingBishop:
			assert(apc == apcBishop);
			break;
		case tpcQueen:
			assert(apc == apcQueen);
			break;
		case tpcKing:
			assert(apc == apcKing);
			break;
		default:
			assert(tpcPc >= tpcPawnFirst && tpcPc < tpcPawnLim);
			assert(apc == apcPawn || apc == apcQueen || apc == apcRook || apc == apcBishop || apc == apcKnight);
			break;
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
BDG::BDG(void) : gs(GS::Playing), cpcToMove(cpcWhite)
{
}


BDG::BDG(const BDG& bdg) : BD(bdg), gs(bdg.gs), cpcToMove(bdg.cpcToMove), 
		rgmvGame(bdg.rgmvGame)
{
}


BDG::BDG(const WCHAR* szFEN)
{
	InitFEN(szFEN);
}


void BDG::NewGame(void)
{
	gs = GS::Playing;
	InitFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	rgmvGame.clear();
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


void BDG::GenRgmv(vector<MV>& rgmv) const
{
	BD::GenRgmv(rgmv, cpcToMove);
}


void BDG::MakeMv(MV mv)
{
	BD::MakeMv(mv);
	cpcToMove ^= 1;
	rgmvGame.push_back(mv);
}


void BDG::UndoLastMv(void)
{
	/* TODO: implement this */
	BD::UndoLastMv(MV());
	cpcToMove ^= 1;
	rgmvGame.pop_back();
}


void BDG::TestGameOver(const vector<MV>& rgmv)
{
	if (rgmv.size() == 0) {
		if (FSqAttacked(mptpcsq[cpcToMove][tpcKing], cpcToMove ^ 1))
			gs = GS::CheckMate;
		else
			gs = GS::StaleMate;
	}
	else {
		/* check for draw circumstances */
		/* king v king, king-knight v king, king-bishop v king, king-bishop v king-bishop (opp color) */
		/* identical board position 3 times (including legal moves the same, cf. en passant) */
		/* both players make 50 moves with no captures or pawn moves */
	}
}



WCHAR mpapcch[] = { L' ', L'P', L'N', L'B', L'R', L'Q', L'K', L'X' };

wstring BDG::SzDecodeMv(MV mv) const
{
	vector<MV> rgmv;
	GenRgmv(rgmv);

	/* if destination square is unique, just include the destination square */
	SQ sqFrom = mv.SqFrom();
	APC apc = ApcFromSq(sqFrom);
	SQ sqTo = mv.SqTo();
	SQ sqCapture = sqTo;

	WCHAR sz[16];
	WCHAR* pch = sz;

	switch (apc) {
	case apcPawn: 
		if (sqTo == sqEnPassant)
			sqCapture = SQ(sqTo.rank() ^ 1, sqTo.file());
		break;

	case apcKing:
		if (sqFrom.file() == fileKing) {
			if (sqTo.file() == fileKingKnight)
				goto FinishCastle;
			if (sqTo.file() == fileQueenBishop) {
				*pch++ = L'O';
				*pch++ = L'\x2013';
FinishCastle:
				*pch++ = L'O';
				*pch++ = L'\x2013';
				*pch = L'O';
				goto FinishMove;
			}
		}
		break;
	case apcKnight: 
		break;
	case apcBishop: 
		break;
	case apcRook: 
		break;
	case apcQueen: 
		break;
	}

	*pch++ = mpapcch[apc];
	for (MV mvOther : rgmv) {
		if (sqTo != mvOther.SqTo() || sqFrom == mvOther.SqFrom())
			continue;
		if (ApcFromSq(mvOther.SqFrom()) == apc) {
			/* there are two matching pieces that can move to the
			   destination square */
			*pch++ = L'a' + sqFrom.file();
			*pch++ = L'1' + sqFrom.rank();
			break;
		}
	}
	/* if we fall out, there is no ambiguity with the apc moving to the
	   destination square */
	*pch++ = mpsqtpc[sqCapture] != tpcEmpty ? L'\x00d7' : L'\x2013';
	*pch++ = L'a' + sqTo.file();
	*pch++ = L'1' + sqTo.rank();

	if (sqTo == sqEnPassant) {
		*pch++ = L' ';
		*pch++ = L'e';
		*pch++ = L'.';
		*pch++ = L'p';
		*pch++ = L'.';
	}
	{
		APC apcPromote = mv.ApcPromote();
		if (apcPromote != apcNull) {
			*pch++ = L'=';
			*pch++ = mpapcch[apcPromote];
		}
	}

FinishMove:
	/* TODO checks and end of game situations */
	*pch++ = 0;
	return wstring(sz);
}


/*
 *
 *	SPABD class
 * 
 *	Screen panel that displays the board 
 * 
 */


ID2D1SolidColorBrush* SPABD::pbrLight;
ID2D1SolidColorBrush* SPABD::pbrDark;
ID2D1SolidColorBrush* SPABD::pbrBlack;
ID2D1SolidColorBrush* SPABD::pbrAnnotation;
ID2D1SolidColorBrush* SPABD::pbrHilite;
IDWriteTextFormat* SPABD::ptfLabel;
IDWriteTextFormat* SPABD::ptfGameState;
ID2D1Bitmap* SPABD::pbmpPieces;
ID2D1PathGeometry* SPABD::pgeomCross;
ID2D1PathGeometry* SPABD::pgeomArrowHead;

const float dxyfCrossFull = 20.0f;
const float dxyfCrossCenter = 4.0f;
PTF rgptfCross[] = {
	{-dxyfCrossCenter, -dxyfCrossFull},
	{dxyfCrossCenter, -dxyfCrossFull},
	{dxyfCrossCenter, -dxyfCrossCenter},
	{dxyfCrossFull, -dxyfCrossCenter},
	{dxyfCrossFull, dxyfCrossCenter},
	{dxyfCrossCenter, dxyfCrossCenter},
	{dxyfCrossCenter, dxyfCrossFull},
	{-dxyfCrossCenter, dxyfCrossFull},
	{-dxyfCrossCenter, dxyfCrossCenter},
	{-dxyfCrossFull, dxyfCrossCenter},
	{-dxyfCrossFull, -dxyfCrossCenter},
	{-dxyfCrossCenter, -dxyfCrossCenter},
	{-dxyfCrossCenter, -dxyfCrossFull} };

PTF rgptfArrowHead[] = {
	{0, 0},
	{dxyfCrossFull*0.5f, dxyfCrossFull*0.86f},
	{-dxyfCrossFull*0.5f, dxyfCrossFull*0.86f},
	{0, 0}
};



/*	SPABD::CreateRsrc
 *
 *	Creates the drawing resources necessary to draw the board.
 */
void SPABD::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (pbrLight)
		return;

	/* brushes */
	
	prt->CreateSolidColorBrush(ColorF(0.5f, 0.6f, 0.4f), &pbrDark);
	prt->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 0.95f), &pbrLight);
	prt->CreateSolidColorBrush(ColorF(ColorF::Black), &pbrBlack);
	prt->CreateSolidColorBrush(ColorF(ColorF::Red), &pbrHilite);
	prt->CreateSolidColorBrush(ColorF(1.f, 0.15f, 0.0f), &pbrAnnotation);
	
	/* fonts */
	
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptfLabel);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		32.0f, L"",
		&ptfGameState);

	/* bitmap */

	HRESULT hr;
	HRSRC hres = ::FindResource(NULL, MAKEINTRESOURCE(idbPieces), L"IMAGE");
	if (hres == NULL)
		throw 1;
	ULONG cbRes = ::SizeofResource(NULL, hres);
	HGLOBAL hresLoad = ::LoadResource(NULL, hres);
	if (hresLoad == NULL)
		throw 1;
	BYTE* pbRes = (BYTE*)::LockResource(hresLoad);
	if (pbRes == NULL)
		throw 1;
	IWICStream* pstm = NULL;
	hr = pfactwic->CreateStream(&pstm);
	hr = pstm->InitializeFromMemory(pbRes, cbRes);
	IWICBitmapDecoder* pdec = NULL;
	hr = pfactwic->CreateDecoderFromStream(pstm, NULL, WICDecodeMetadataCacheOnLoad, &pdec);
	IWICBitmapFrameDecode* pframe = NULL;
	hr = pdec->GetFrame(0, &pframe);
	IWICFormatConverter* pconv = NULL;
	hr = pfactwic->CreateFormatConverter(&pconv);
	hr = pconv->Initialize(pframe, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
	hr = prt->CreateBitmapFromWicBitmap(pconv, NULL, &pbmpPieces);
	
	SafeRelease(&pframe);
	SafeRelease(&pconv);
	SafeRelease(&pdec);
	SafeRelease(&pstm);
	
	/* geometries */
	
	/* capture X, which is created as a cross that is rotated later */
	CreateGeom(pfactd2d, rgptfCross, CArray(rgptfCross), &pgeomCross);
	/* arrow head */
	CreateGeom(pfactd2d, rgptfArrowHead, CArray(rgptfArrowHead), &pgeomArrowHead);
}


void SPABD::CreateGeom(ID2D1Factory* pfactd2d, PTF rgptf[], int cptf, ID2D1PathGeometry** ppgeom)
{
	/* capture X, which is created as a cross that is rotated later */
	pfactd2d->CreatePathGeometry(ppgeom);
	ID2D1GeometrySink* psink;
	(*ppgeom)->Open(&psink);
	psink->BeginFigure(rgptf[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&rgptf[1], cptf - 1);
	psink->EndFigure(D2D1_FIGURE_END_CLOSED);
	psink->Close();
	SafeRelease(&psink);
}


/*	SPABD::DiscardRsrc
 *
 *	Cleans up the resources created by CreateRsrc
 */
void SPABD::DiscardRsrc(void)
{
	SafeRelease(&pbrLight);
	SafeRelease(&pbrDark);
	SafeRelease(&pbrBlack);
	SafeRelease(&pbrAnnotation);
	SafeRelease(&pbrHilite);
	SafeRelease(&ptfLabel);
	SafeRelease(&ptfGameState);
	SafeRelease(&pbmpPieces);
	SafeRelease(&pgeomCross);
	SafeRelease(&pgeomArrowHead);
}


/*	SPABD::SPABD
 *
 *	Constructor for the board screen panel.
 */
SPABD::SPABD(GA& ga) : SPA(ga), phtDragInit(NULL), phtCur(NULL), sqHover(sqNil),
	tpcPointOfView(tpcWhite),
	dxyfSquare(72.0f), dxyfBorder(2.0f), dxyfMargin(45.0f),
	dyfLabel(18.0f)	// TODO: this is a font attribute
{
}


/*	SPABD::~SPABD
 *
 *	Destructor for the board screen panel
 */
SPABD::~SPABD(void)
{
}


/*	SPABD::Layout
 *
 *	Layout work for the board screen panel.
 */
void SPABD::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	SPA::Layout(ptf, pspa, ll);
	rcfSquares = rcfBounds;
	float dxyf = dxyfMargin + 3.0f * dxyfBorder;
	rcfSquares.Inflate(-dxyf, -dxyf);
}


/*	SPABD::DxWidth
 *
 *	Returns the width of the board screen panel. Used for screen layout.
 */
float SPABD::DxWidth(void) const
{
	return 2.0f * dxyfMargin + 6.0f * dxyfBorder + 8.0f * dxyfSquare;
}


/*	SPABD::DyHeight
 *
 *	Returns the height of the board screen panel. Used for screen layout.
 */
float SPABD::DyHeight(void) const
{
	return DxWidth();
}


/*	SPABD::NewGame
 *
 *	Starts a new game on the screen board
 */
void SPABD::NewGame(void)
{
	ga.bdg.GenRgmv(rgmvDrag);
}


/*	SPABD::MakeMv
 *
 *	Makes a move on the board in the screen panel
 */
void SPABD::MakeMv(MV mv)
{
	ga.bdg.MakeMv(mv);
	ga.bdg.GenRgmv(rgmvDrag);
	ga.bdg.TestGameOver(rgmvDrag);
}


/*	SPABD::Draw
 *
 *	Draws the board panel on the screen. This particular implementation
 *	does a green and cream chess board with square labels written in the
 *	board margin.
 * 
 *	Note that mouse tracking feedback is drawn by our generic Draw. It 
 *	turns out Direct2D drawing is fast enough to redraw the entire board
 *	in response to user input, which simplifies drawing quite a bit. We
 *	just handle all dragging drawing in here.
 */
void SPABD::Draw(ID2D1RenderTarget* prt)
{
	assert(prt != NULL);
	DrawMargins(prt);
	DrawLabels(prt);
	DrawSquares(prt);
	DrawPieces(prt);
	DrawHover(prt);
	DrawHilites(prt);
	DrawAnnotations(prt);
	DrawGameState(prt);
}


/*	SPABD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid.
 */
void SPABD::DrawMargins(ID2D1RenderTarget* prt)
{
	assert(prt != NULL);
	RCF rcf = rcfBounds;
	prt->FillRectangle(&rcf, pbrLight);
	rcf.Inflate(PTF(-dxyfMargin, -dxyfMargin));
	prt->FillRectangle(&rcf, pbrDark);
	rcf.Inflate(PTF(-2.0f * dxyfBorder, -2.0f * dxyfBorder));
	prt->FillRectangle(&rcf, pbrLight);
}


/*	SPABD::DrawSquares
 *
 *	Draws the squares of the chessboard. Assumes the light color
 *	has already been used to fill the board area, so all we need to
 *	do is draw the dark colors on top 
 */
void SPABD::DrawSquares(ID2D1RenderTarget* prt)
{
	for (SQ sq = 0; sq < sqMax; sq++) {
		if ((sq.rank()+sq.file()) % 2 == 0) {
			RCF rcf = RcfFromSq(sq);
			prt->FillRectangle(&rcf, pbrDark);
		}
	}
}


/*	SPABD::DrawLabels
 *
 *	Draws the square labels in the margin area of the green and cream
 *	board. 
 */
void SPABD::DrawLabels(ID2D1RenderTarget* prt)
{
	DrawFileLabels(prt);
	DrawRankLabels(prt);
}


/*	SPABD::DrawFileLabels
 *
 *	Draws the file letter labels along the bottom of the board.
 */
void SPABD::DrawFileLabels(ID2D1RenderTarget* prt)
{
	TCHAR chLabel;
	chLabel = 'a';
	RCF rcf(rcfSquares.left, rcfSquares.bottom + 3.0f * dxyfBorder, 0, rcfBounds.bottom);
	ptfLabel->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (int file = 0; file < fileMax; file++) {
		rcf.right = rcf.left + dxyfSquare;
		prt->DrawText(&chLabel, 1, ptfLabel,
			RectF(rcf.left, (rcf.top + rcf.bottom - dyfLabel) / 2, rcf.right, rcf.bottom),
			pbrDark, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
		rcf.left = rcf.right;
		chLabel++;
	}
}


/*	SPABD::DrawRankLabels
 *
 *	Draws the numerical rank labels along the side of the board
 */
void SPABD::DrawRankLabels(ID2D1RenderTarget* prt) 
{
	RCF rcf(rcfBounds.left, rcfSquares.top, rcfBounds.left+dxyfMargin, 0);
	TCHAR chLabel = tpcPointOfView == tpcBlack ? '1' : '8';
	for (int rank = 0; rank < rankMax; rank++) {
		rcf.bottom = rcf.top + dxyfSquare;
		prt->DrawText(&chLabel, 1, ptfLabel,
			RectF(rcf.left, (rcf.top + rcf.bottom - dyfLabel) / 2, rcf.right, rcf.bottom),
			pbrDark, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
		rcf.top = rcf.bottom;
		if (tpcPointOfView == tpcBlack)
			chLabel++;
		else
			chLabel--;
	}
}


/*	SPABD::DrawGameState
 *
 *	When the game is over, this displays the game state on the board
 */
void SPABD::DrawGameState(ID2D1RenderTarget* prt)
{
	if (ga.bdg.gs == GS::Playing)
		return;
	const WCHAR* szState = L"";
	if (ga.bdg.gs == GS::CheckMate)
		szState = L"Check Mate";
	else if (ga.bdg.gs == GS::StaleMate)
		szState = L"Stale Mate";
	else {
		assert(false);
	}
	prt->DrawText(szState, lstrlenW(szState), ptfGameState,
		RectF(rcfBounds.left, rcfBounds.bottom - 32.0f, rcfBounds.right, rcfBounds.bottom),
		pbrDark, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
}


/*	SPABD::RcfFromSq
 *
 *	Returns the rectangle of the given square on the screen
 */
RCF SPABD::RcfFromSq(SQ sq) const
{
	assert(sq >= 0 && sq < sqMax);
	int rank = sq.rank();
	int file = sq.file();
	RCF rcf;
	rcf.left = rcfSquares.left + dxyfSquare * file;
	rcf.right = rcf.left + dxyfSquare;
	rcf.top = rcfSquares.top + dxyfSquare * (rankMax - rank - 1);
	rcf.bottom = rcf.top + dxyfSquare;
	return rcf;
}


/*	SPABD::DrawHover
 *
 *	Draws the move hints that we display while the user is hovering
 *	the mouse over a moveable piece.
 * 
 *	We draw a circle over every square you can move to.
 */
void SPABD::DrawHover(ID2D1RenderTarget* prt)
{
	assert(prt != NULL);
	if (sqHover == sqNil)
		return;
	pbrBlack->SetOpacity(0.4f);
	for (MV mv : rgmvDrag) {
		if (mv.SqFrom() != sqHover)
			continue;
		RCF rcf = RcfFromSq(mv.SqTo());
		if (ga.bdg.mpsqtpc[mv.SqTo()] == tpcEmpty) {
			D2D1_ELLIPSE ellipse;
			ellipse.point.x = (rcf.right + rcf.left) / 2;
			ellipse.point.y = (rcf.top + rcf.bottom) / 2;
			ellipse.radiusX = dxyfSquare / 5;
			ellipse.radiusY = dxyfSquare / 5;
			prt->FillEllipse(&ellipse, pbrBlack);
		}
		else {
			prt->SetTransform(
				Matrix3x2F::Rotation(45.0f, PTF(0.0f, 0.0f)) * 
				Matrix3x2F::Scale(SizeF(dxyfSquare/(2.0f*dxyfCrossFull), dxyfSquare/(2.0f*dxyfCrossFull)), PTF(0.0, 0.0)) *
				Matrix3x2F::Translation(SizeF((rcf.right+rcf.left)/2, (rcf.top+rcf.bottom)/2)));
			prt->FillGeometry(pgeomCross, pbrBlack);
			prt->SetTransform(Matrix3x2F::Identity());
		}
	}
	pbrBlack->SetOpacity(1.0f);
}


/*	SPABD::DrawPieces
 *
 *	Draws pieces on the board. Includes support for the trackingn display 
 *	while we're dragging pieces.
 * 
 *	Direct2D is fast enough for us to do full screen redraws on just about
 *	any user-initiated change, so there isn't much point in optimizing.
 */
void SPABD::DrawPieces(ID2D1RenderTarget* prt)
{
	assert(prt != NULL);
	for (SQ sq = 0; sq < sqMax; sq++) {
		float opacity = (phtDragInit && phtDragInit->sq == sq) ? 0.2f : 1.0f;
		DrawPc(prt, RcfFromSq(sq), opacity, ga.bdg.mpsqtpc[sq]);
	}
	if (phtDragInit && phtCur)
		DrawDragPc(prt, rcfDragPc);
}


/*	SPABD::DrawDragPc
 *
 *	Draws the piece we're currently dragging, as specified by
 *	phtDragInit, on the screen, in the location at rcf. rcf should
 *	track the mouse location.
 */
void SPABD::DrawDragPc(ID2D1RenderTarget* prt, const RCF& rcf)
{
	assert(phtDragInit);
	DrawPc(prt, rcf, 1.0f, ga.bdg.mpsqtpc[phtDragInit->sq]);
}


/*	SPABD::RcfGetDrag
 *
 *	Gets the current screen rectangle the piece we're dragging
 */
RCF SPABD::RcfGetDrag(void)
{
	RCF rcf(0, 0, dxyfSquare, dxyfSquare);
	float xfSquareInit = rcfSquares.left + 
		dxyfSquare * phtDragInit->sq.file();
	float yfSquareInit = rcfSquares.top + 
		dxyfSquare * (rankMax - phtDragInit->sq.rank() - 1);
	float dxfInit = phtDragInit->ptf.x - xfSquareInit;
	float dyfInit = phtDragInit->ptf.y - yfSquareInit;
	rcf.Offset(phtCur->ptf.x - dxfInit, phtCur->ptf.y - dyfInit);
	return rcf;
}


/*	SPABD::DrawPc
 *
 *	Draws the chess piece on the square at rcf.
 */
void SPABD::DrawPc(ID2D1RenderTarget* prt, const RCF& rcf, float opacity, BYTE tpc)
{
	/* the piece png is oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	D2D1_SIZE_F ptf = pbmpPieces->GetSize();
	float dxfPiece = ptf.width / 6.0f;
	float dyfPiece = ptf.height / 2.0f;
	float xfPiece = mpapcxBitmap[ApcFromTpc(tpc)] * dxfPiece;
	float yfPiece = CpcFromTpc(tpc) * dyfPiece;
	prt->DrawBitmap(pbmpPieces, rcf, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			RectF(xfPiece, yfPiece, xfPiece + dxfPiece, yfPiece + dyfPiece));
}


void SPABD::DrawHilites(ID2D1RenderTarget* prt)
{
}


void SPABD::DrawAnnotations(ID2D1RenderTarget* prt)
{
	pbrAnnotation->SetOpacity(0.5f);
	for (ANO ano : rgano) {
		if (ano.sqTo.FIsNil())
			DrawSquareAnnotation(prt, ano.sqFrom);
		else
			DrawArrowAnnotation(prt, ano.sqFrom, ano.sqTo);
	}
	pbrAnnotation->SetOpacity(1.0f);
}


void SPABD::DrawSquareAnnotation(ID2D1RenderTarget* prt, SQ sq)
{
}


void SPABD::DrawArrowAnnotation(ID2D1RenderTarget* prt, SQ sqFrom, SQ sqTo)
{
}


/*	SPABD::PhtHitTest
 *
 *	Hit tests the given mouse coordinate and returns a HT implementation
 *	if it belongs to this panel. Returns NULL if there is no hit.
 *
 *	If we return non-null, we are guaranteed to return a HTBD hit test 
 *	structure from here, which means other mouse tracking notifications
 *	can safely cast to a HTBD to get access to board hit testing info.
 */
HT* SPABD::PhtHitTest(const PTF& ptf)
{
	if (!rcfBounds.FContainsPtf(ptf)) {
		if (phtDragInit)
			return new HTBD(ptf, HTT::Miss, this, sqMax);
		return NULL;
	}
	if (!rcfSquares.FContainsPtf(ptf))
		return new HTBD(ptf, HTT::Static, this, sqMax);
	int rank = (int)((ptf.y - rcfSquares.top) / dxyfSquare);
	int file = (int)((ptf.x - rcfSquares.left) / dxyfSquare);
	if (tpcPointOfView == tpcWhite)
		rank = rankMax - rank - 1;
	SQ sq = SQ(rank, file);
	if (ga.bdg.mpsqtpc[sq] == tpcEmpty)
		return new HTBD(ptf, HTT::EmptyPc, this, sq);
	if (CpcFromTpc(ga.bdg.mpsqtpc[sq]) != ga.bdg.cpcToMove)
		return new HTBD(ptf, HTT::OpponentPc, this, sq);

	if (FMoveablePc(sq))
		return new HTBD(ptf, HTT::MoveablePc, this, sq);
	else
		return new HTBD(ptf, HTT::UnmoveablePc, this, sq);
}


/*	SPABD::FMoveablePc
 *
 *	Returns true if the square contains a piece that has a
 *	legal move
 */
bool SPABD::FMoveablePc(SQ sq)
{
	assert(CpcFromTpc(ga.bdg.mpsqtpc[sq]) == ga.bdg.cpcToMove);
	for (MV mv : rgmvDrag)
		if (mv.SqFrom() == sq)
			return true;
	return false;
}


void SPABD::StartLeftDrag(HT* pht)
{
	sqHover = sqNil;
	if (pht->htt != HTT::MoveablePc) {
		MessageBeep(0);
		return;
	}
	assert(phtDragInit == NULL);
	assert(phtCur == NULL);
	
	phtDragInit = (HTBD*)pht->PhtClone();
	phtCur = (HTBD*)pht->PhtClone();
	
	rcfDragPc = RcfGetDrag();
	Redraw();
}


void SPABD::EndLeftDrag(HT* pht)
{
	if (phtDragInit == NULL)
		return;
	HTBD* phtbd = (HTBD*)pht;
	if (phtbd && phtbd->sq < sqMax) {
		/* find the to/from square in the move list that matches
		 * this move. We do this to pick up the correct special
		 * bit for moves like en passant and castle */
		for (MV mv : rgmvDrag) {
			if (mv.SqFrom() == phtDragInit->sq && mv.SqTo() == phtbd->sq) {
				ga.MakeMv(mv);
				break;
			}
		}
	}
	delete phtDragInit;
	phtDragInit = NULL;
	if (phtCur)
		delete phtCur;
	phtCur = NULL;

	InvalOutsideRcf(rcfDragPc); 
	Redraw();
}


/*	SPABD::LeftDrag
 *
 *	Notification while the mouse button is down and dragging around. The
 *	hit test for the captured mouse position is in pht. 
 * 
 *	We use this for users to drag pieces around while they are trying to
 *	move.
 */
void SPABD::LeftDrag(HT* pht)
{
	if (phtDragInit == NULL || pht == NULL || pht->htt == HTT::Miss) {
		EndLeftDrag(pht);
		return;
	}
	if (phtCur)
		delete phtCur;
	phtCur = (HTBD*)pht->PhtClone();
	
	InvalOutsideRcf(rcfDragPc);
	rcfDragPc = RcfGetDrag();
	Redraw();
}


/*	SPABD::MouseHover
 *
 *	Notification that tells us the mouse is hovering over the board.
 *	The mouse button will not be down. the pht is the this test
 *	class for the mouse location. It is guaranteed to be a HTBD
 *	for hit testing over the SPABD.
 */
void SPABD::MouseHover(HT* pht)
{
	if (pht == NULL)
		return;
	HTBD* phtbd = (HTBD*)pht;
	if (phtbd->htt != HTT::MoveablePc)
		HiliteLegalMoves(sqNil);
	else
		HiliteLegalMoves(phtbd->sq);
}


/*	SPABD::HiliteLegalMoves
 *
 *	When the mouse is hovering over square sq, causes the screen to
 *	be redrawn with squares that the piece can move to with a
 *	hilight. If sq is sqNil, no hilights are drawn.
 */
void SPABD::HiliteLegalMoves(SQ sq)
{
	if (sq == sqHover)
		return;
	sqHover = sq;
	Redraw();
}





/*	SPABD::InvalOutsideRcf
 *
 *	While we're tracking piece dragging, it's possible for a piece
 *	to be drawn outside the bounding box of the board. Any drawing
 *	inside the board is taken care of by calling Draw directly, so
 *	we handle these outside parts by just invalidating the area so
 *	they'll get picked off eventually by normal update paints.
 */
void SPABD::InvalOutsideRcf(const RCF& rcf)
{
	InvalRectF(rcf.left, rcf.top, rcf.right, rcfBounds.top);
	InvalRectF(rcf.left, rcfBounds.bottom, rcf.right, rcf.bottom);
	InvalRectF(rcf.left, rcf.top, rcfBounds.left, rcf.bottom);
	InvalRectF(rcfBounds.right, rcf.top, rcf.right, rcf.bottom);
}


/*	SPABD::InvalRectF
 *
 *	Helper function to invalidate an area in the containing
 *	window. Used to force redraws of areas outside the board
 *	on the screen. Does nothing if the rectangle is empty.
 */
void SPABD::InvalRectF(float left, float top, float right, float bottom)
{
	if (left >= right || top >= bottom)
		return;
	RECT rc;
	::SetRect(&rc, (int)left, (int)top, (int)right, (int)bottom);
	::InvalidateRect(ga.app.hwnd, &rc, false);
}
