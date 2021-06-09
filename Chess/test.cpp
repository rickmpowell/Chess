/*
 *
 *	test.cpp
 * 
 *	A test suite to make sure we've got a good working game and
 *	move generation
 * 
 */

#include "ga.h"


/*	GA::Test
 *
 *	This is the top-level test script.
 */
void GA::Test(SPMV spmv)
{
	InitLog(3);
	Log(LGT::Open, LGF::Normal, L"Test", L"Start");
	this->spmv = spmv;
	Log(LGT::Open, LGF::Normal, L"New Game", L"");
	NewGame(new RULE(0, 0, 0));
	ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	Log(LGT::Close, LGF::Normal, L"New Game", L"Passed");
	Log(LGT::Open, LGF::Normal, L"Undo", L"");
	UndoTest();
	Log(LGT::Close, LGF::Normal, L"Undo", L"Passed");
	Log(LGT::Open, LGF::Normal, L"Play", L"Players");
	PlayPGNFiles(L"..\\Chess\\Test\\Players");
	Log(LGT::Close, LGF::Normal, L"Play", L"Players");
	this->spmv = SPMV::Animate;
	Log(LGT::Close, LGF::Normal, L"Test", L"Passed");
}


void GA::PlayPGNFiles(const WCHAR szPath[])
{
	WIN32_FIND_DATA ffd;
	wstring szSpec(szPath);
	szSpec += L"\\*.pgn";
	HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
	if (hfind == INVALID_HANDLE_VALUE)
		throw 1;
	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		wstring szSpec(szPath);
		szSpec += L"\\";
		szSpec += ffd.cFileName;
		if (PlayPGNFile(szSpec.c_str()) < 0)
			break;
	} while (FindNextFile(hfind, &ffd) != 0);
	FindClose(hfind);
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
		case L'r': assert(bdg.ApcFromSq(sq) == APC::Rook); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'n': assert(bdg.ApcFromSq(sq) == APC::Knight); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'b': assert(bdg.ApcFromSq(sq) == APC::Bishop); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'q': assert(bdg.ApcFromSq(sq) == APC::Queen); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'k': assert(bdg.ApcFromSq(sq) == APC::King); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'p': assert(bdg.ApcFromSq(sq) == APC::Pawn); assert(bdg.CpcFromSq(sq) == CPC::Black); sq++; break;
		case L'R': assert(bdg.ApcFromSq(sq) == APC::Rook); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
		case L'N': assert(bdg.ApcFromSq(sq) == APC::Knight); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
		case L'B': assert(bdg.ApcFromSq(sq) == APC::Bishop); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
		case L'Q': assert(bdg.ApcFromSq(sq) == APC::Queen); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
		case L'K': assert(bdg.ApcFromSq(sq) == APC::King); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
		case L'P': assert(bdg.ApcFromSq(sq) == APC::Pawn); assert(bdg.CpcFromSq(sq) == CPC::White); sq++; break;
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
				assert(bdg.FIsEmpty(sq));
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
		case L'b': assert(bdg.cpcToMove == CPC::Black); break;
		case L'w':
		case L'-': assert(bdg.cpcToMove == CPC::White); break;
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
		case L'k': cs |= csKing << CPC::White; break;
		case L'q': cs |= csQueen << CPC::White; break;
		case L'K': cs |= csKing << CPC::Black; break;
		case L'Q': cs |= csQueen << CPC::Black; break;
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
		assert(bdg.sqEnPassant.fIsNil());
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


/*	GA::PlayPGNFile
 *
 *	Plays the games in the PGN file given by szFile
 */
int GA::PlayPGNFile(const WCHAR szFile[])
{
	ifstream is(szFile, ifstream::in);

	wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
	Log(LGT::Open, LGF::Normal, szFileBase, L"");
	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			Log(LGT::Temp, LGF::Normal, szFileBase, wstring(L"Game ") + to_wstring(igame+1));
			if (DeserializeGame(istkpgn) != 1)
				break;
		}
		Log(LGT::Close, LGF::Normal, szFileBase, L"Passed");
	}
	catch (int err)
	{
		if (err == 1) {
			WCHAR sz[100];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(NULL, sz, L"PGN File Error", MB_OK);
		}
		Log(LGT::Close, LGF::Normal, szFileBase, L"Failed");
		return err;
	}
	return 0;
}


void GA::UndoTest(void)
{
	WIN32_FIND_DATA ffd;
	wstring szSpec(L"..\\Chess\\Test\\Players\\*.pgn");
	HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
	if (hfind == INVALID_HANDLE_VALUE)
		throw 1;
	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		wstring szSpec = wstring(L"..\\Chess\\Test\\Players\\") + ffd.cFileName;
		if (PlayUndoPGNFile(szSpec.c_str()) < 0)
			break;
		break;
	} while (FindNextFile(hfind, &ffd) != 0);
	FindClose(hfind);
}


int GA::PlayUndoPGNFile(const WCHAR* szFile)
{
	ifstream is(szFile, ifstream::in);

	wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
	Log(LGT::Open, LGF::Normal, szFileBase, L"");
	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			Log(LGT::Temp, LGF::Normal, szFileBase, wstring(L"Game ") + to_wstring(igame + 1));
			if (DeserializeGame(istkpgn) != 1)
				break;
			UndoFullGame();
			ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
		Log(LGT::Close, LGF::Normal, szFileBase, L"Passed");
	}
	catch (int err) {
		if (err == 1) {
			WCHAR sz[58];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(NULL, sz, L"PGN File Error", MB_OK);
		}
		Log(LGT::Close, LGF::Normal, szFileBase, L"Failed");
		return err;
	}
	return 0;
}


void GA::UndoFullGame(void)
{
	while (bdg.imvCur >= 0)
		UndoMv();
}