/*
 *
 *	test.cpp
 * 
 *	A test suite to make sure we've got a good working game and
 *	move generation
 * 
 */

#include "Chess.h"


/*	GA::Test
 *
 *	This is the top-level test script.
 */
void GA::Test(void)
{
	NewGame();
	ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}


/*	GA::ValidateFEN
 *
 *	Verifies the board matches the board state in the FEN string. Note that we very
 *	specifically do our own FEN parsing here.
 */
void GA::ValidateFEN(const WCHAR* szFEN) const
{
	const WCHAR* sz = szFEN;
	ValidatePieces(sz);
	ValidateMoveColor(sz);
	ValidateCastle(sz);
	ValidateEnPassant(sz);
}


void GA::ValidatePieces(const WCHAR*& sz) const
{
	SkipWhiteSpace(sz);
	int rank = 7;
	SQ sq = SQ(rank, 0);
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case L'/': rank--; assert(rank >= 0); sq = SQ(rank, 0); break;
		case L'r': assert(bdg.ApcFromSq(sq) == apcRook); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'n': assert(bdg.ApcFromSq(sq) == apcKnight); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'b': assert(bdg.ApcFromSq(sq) == apcBishop); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'q': assert(bdg.ApcFromSq(sq) == apcQueen); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'k': assert(bdg.ApcFromSq(sq) == apcKing); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'p': assert(bdg.ApcFromSq(sq) == apcPawn); assert(bdg.CpcFromSq(sq) == cpcBlack); sq++; break;
		case L'R': assert(bdg.ApcFromSq(sq) == apcRook); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'N': assert(bdg.ApcFromSq(sq) == apcKnight); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'B': assert(bdg.ApcFromSq(sq) == apcBishop); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'Q': assert(bdg.ApcFromSq(sq) == apcQueen); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'K': assert(bdg.ApcFromSq(sq) == apcKing); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'P': assert(bdg.ApcFromSq(sq) == apcPawn); assert(bdg.CpcFromSq(sq) == cpcWhite); sq++; break;
		case L'1':
		case L'2':
		case L'3':
		case L'4':
		case L'5':
		case L'6':
		case L'7':
		case L'8':
		{
			for (int dsq = *sz - L'0'; dsq > 0; dsq--, sq++) {
				assert(bdg.mpsqtpc[sq] == tpcEmpty);
			}
			break;
		}
		default: assert(false); goto Done;
		}
	}
Done:
	SkipToWhiteSpace(sz);
}


void GA::ValidateMoveColor(const WCHAR*& sz) const
{
	SkipWhiteSpace(sz);
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case L'b': assert(bdg.cpcToMove == cpcBlack); break;
		case L'w':
		case L'-': assert(bdg.cpcToMove == cpcWhite); break;
		default: assert(false); goto Done;
		}
	}
Done:
	SkipToWhiteSpace(sz);
}


void GA::ValidateCastle(const WCHAR*& sz) const
{
	SkipWhiteSpace(sz);
	int cs = 0;
	for (; *sz && *sz != L' '; sz++) {
		switch (*sz) {
		case L'k': cs |= csKing << cpcWhite; break;
		case L'q': cs |= csQueen << cpcWhite; break;
		case L'K': cs |= csKing << cpcBlack; break;
		case L'Q': cs |= csQueen << cpcBlack; break;
		case L'-': break;
		default: assert(false); goto Done;
		}
	}
	assert(bdg.cs == cs);
Done:
	SkipToWhiteSpace(sz);
}


void GA::ValidateEnPassant(const WCHAR*& sz) const
{
	SkipWhiteSpace(sz);
	if (*sz == L'-') {
		assert(bdg.sqEnPassant == sqNil);
	}
	else if (*sz >= L'a' && *sz <= L'h') {
		int file = *sz - L'a';
		sz++;
		if (*sz >= L'1' && *sz <= L'8') {
			int rank = *sz - L'1';
			assert(bdg.sqEnPassant == SQ(rank, file));
			sz++;
			if (*sz && *sz != L' ') {
				assert(false);
				goto Done;
			}
		}
	}
	else {
		assert(false);
	}
Done:
	SkipToWhiteSpace(sz);
}


void GA::SkipWhiteSpace(const WCHAR*& sz) const
{
	for (; *sz && *sz == L' '; sz++)
		;
}


void GA::SkipToWhiteSpace(const WCHAR*& sz) const
{
	for (; *sz && *sz != L' '; sz++)
		;
}