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
	LogOpen(L"Test", L"Start");
	SPMV spmvSav = spmvCur;
	spmvCur = spmv;
	
	LogOpen(L"New Game", L"");
	NewGame(new RULE(0, 0, 0));
	ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	LogClose(L"New Game", L"Passed", LGF::Normal);

	LogOpen(L"Perft", L"");
	PerftTest();
	LogClose(L"Perft", L"Passed", LGF::Normal);

	LogOpen(L"Undo", L"");
	UndoTest();
	LogClose(L"Undo", L"Passed", LGF::Normal);
	
	LogOpen(L"Play", L"Players");
	PlayPGNFiles(L"..\\Chess\\Test\\Players");
	LogClose(L"Play", L"Players", LGF::Normal);
	
	LogClose(L"Test", L"Passed", LGF::Normal);

	spmvCur = spmvSav;
}


void GA::PlayPGNFiles(const wchar_t szPath[])
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
void GA::ValidateFEN(const wchar_t* szFEN) const
{
	const wchar_t* sz = szFEN;
	ValidatePieces(sz);
	ValidateMoveColor(sz);
	ValidateCastle(sz);
	ValidateEnPassant(sz);
}


void GA::ValidatePieces(const wchar_t*& sz) const
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


void GA::ValidateMoveColor(const wchar_t*& sz) const
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


void GA::ValidateCastle(const wchar_t*& sz) const
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


void GA::ValidateEnPassant(const wchar_t*& sz) const
{
	SkipWhiteSpace(sz);
	if (*sz == L'-') {
		assert(bdg.sqEnPassant.fIsNil());
	}
	else if (*sz >= L'a' && *sz <= L'h') {
#ifndef NDEBUG
		int file = *sz - L'a';
#endif
		sz++;
		if (*sz >= L'1' && *sz <= L'8') {
			assert(bdg.sqEnPassant == SQ(*sz - L'1', file));
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


void GA::SkipWhiteSpace(const wchar_t*& sz) const
{
	for (; *sz && *sz == L' '; sz++)
		;
}


void GA::SkipToWhiteSpace(const wchar_t*& sz) const
{
	for (; *sz && *sz != L' '; sz++)
		;
}


/*	GA::PlayPGNFile
 *
 *	Plays the games in the PGN file given by szFile
 */
int GA::PlayPGNFile(const wchar_t szFile[])
{
	ifstream is(szFile, ifstream::in);

	wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
	LogOpen(szFileBase, L"");
	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			LogTemp(wstring(L"Game ") + to_wstring(igame+1));
			if (DeserializeGame(istkpgn) != 1)
				break;
		}
		LogClose(szFileBase, L"Passed", LGF::Normal);
	}
	catch (int err)
	{
		if (err == 1) {
			wchar_t sz[100];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(nullptr, sz, L"PGN File Error", MB_OK);
		}
		LogClose(szFileBase, L"Failed", LGF::Normal);
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


int GA::PlayUndoPGNFile(const wchar_t* szFile)
{
	ifstream is(szFile, ifstream::in);

	wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
	LogOpen(szFileBase, L"");
	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			LogTemp(wstring(L"Game ") + to_wstring(igame + 1));
			if (DeserializeGame(istkpgn) != 1)
				break;
			UndoFullGame();
			ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
		LogClose(szFileBase, L"Passed", LGF::Normal);
	}
	catch (int err) {
		if (err == 1) {
			wchar_t sz[58];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(nullptr, sz, L"PGN File Error", MB_OK);
		}
		LogClose(szFileBase, L"Failed", LGF::Normal);
		return err;
	}
	return 0;
}


void GA::UndoFullGame(void)
{
	while (bdg.imvCur >= 0) {
		BDG bdgInit = bdg;
		UndoMv(spmvCur);
		RedoMv(spmvCur);
		assert(bdg == bdgInit);
		if (bdg != bdgInit)
			throw 1;
		UndoMv(spmvCur);
	}
}


/*	GA::CmvPerft
 *
 *	The standard perft test, which makes all legal moves to a given depth
 *	from the board position.
 */
uint64_t GA::CmvPerft(int depth)
{
	if (depth == 0)
		return 1;
	GMV gmv;
	bdg.GenRgmv(gmv, RMCHK::Remove);
	if (depth == 1)
		return gmv.cmv();
	uint64_t cmv = 0;
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		bdg.MakeMv(gmv[imv]);
		cmv += CmvPerft(depth - 1);
		bdg.UndoMv();
	}
	return cmv;
}

uint64_t GA::CmvPerftDivide(int depthPerft)
{
	if (depthPerft == 0)
		return 1;
	assert(depthPerft >= 1);
	GMV gmv;
	bdg.GenRgmv(gmv, RMCHK::Remove);
	if (depthPerft == 1)
		return gmv.cmv();
	uint64_t cmv = 0;
	BDG bdgInit = bdg;
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		bdg.MakeMv(gmv[imv]);
		LogOpen(TAG(bdg.SzDecodeMvPost(gmv[imv]), ATTR(L"FEN", (wstring)bdg)), L"");
		uint64_t cmvMove = CmvPerft(depthPerft - 1);
		cmv += cmvMove;
		bdg.UndoMv();
		LogClose(bdg.SzDecodeMvPost(gmv[imv]), to_wstring(cmvMove), LGF::Normal);
		//assert(bdg == bdgInit);
	}
	return cmv;

}


/*	GA::PerftTest
 *
 *	Move generation test that uses perft divide to make sure we're generating
 *	the right number of moves. These move counts are validated perft counts
 *	found on chessprogramming.org
 */
void GA::PerftTest(void)
{
	RunPerftTest(L"Initial", L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		(uint64_t[14]){ 1ULL, 20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL, 84998978956ULL, 2439530234167ULL,
			69352859712417ULL, 2097651003696806ULL, 62854969236701747ULL, 1981066775000396239ULL }, 6, true);

	RunPerftTest(L"Kiwipete", L"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 
		(uint64_t[7]){ 1ULL, 48ULL, 2039ULL, 97862LL, 4085603ULL, 193690690ULL, 8031647685ULL }, 5, true);

	RunPerftTest(L"Position 3", L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 
		(uint64_t[9]){ 1ULL, 14ULL, 191ULL, 2812ULL, 43238ULL, 674624ULL, 11030083ULL, 178633661ULL, 3009794393ULL }, 7, true);

	RunPerftTest(L"Position 4", L"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 
		(uint64_t[7]){ 1ULL, 6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL }, 6, true);

	RunPerftTest(L"Position 5", L"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 
		(uint64_t[6]){ 1ULL, 44ULL, 1486ULL, 62379ULL, 2103487ULL, 89941194ULL }, 5, true);

	RunPerftTest(L"Position 6", L"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 
		(uint64_t[10]){ 1ULL, 46ULL, 2079ULL, 89890ULL, 3894594ULL, 164075551ULL, 6923051137ULL, 287188994746ULL, 11923589843526ULL, 490154852788714ULL }, 5, true);
}


/*	GA::RunPerftTest
 *
 *	Runs one particular perft test starting with the original board position (szFEN). 
 *	Cycles through each depth from 1 to depthLast, verifying move counts in mpdepthcmv. 
 *	The tag is just used for debug output.
 */
void GA::RunPerftTest(const wchar_t tag[], const wchar_t szFEN[], uint64_t mpdepthcmv[], int depthLast, bool fDivide)
{
	LogOpen(tag, L"");
	/* TODO: share code with NewGame? */
	bdg.InitFEN(szFEN);
	InitClocks();
	uibd.InitGame();
	uiml.InitGame();
	StartGame();
	uibd.Redraw();

	bool fPassed = false;
	int depthFirst = fDivide ? 2 : 1;
	for (int depth = depthFirst; depth <= depthLast; depth++) {
		LogOpen(L"Depth", to_wstring(depth));
		uint64_t cmvExp = mpdepthcmv[depth];
		LogData(L"Expected: " + to_wstring(cmvExp));
		time_point tpStart = high_resolution_clock::now();
		uint64_t cmvAct;
		if (fDivide)
			cmvAct = CmvPerftDivide(depth);
		else
			cmvAct = CmvPerft(depth);
		time_point tpEnd = chrono::high_resolution_clock::now();
		LogData(L"Actual: " + to_wstring(cmvAct));
		duration dtp = tpEnd - tpStart;
		microseconds us = duration_cast<microseconds>(dtp);
		float sp = 1000.0f * (float)cmvAct / (float)us.count();
		LogData(L"Speed: " + to_wstring((int)round(sp)) + L" moves/ms");
		if (cmvExp != cmvAct) {
			LogClose(L"Depth", L"Failed", LGF::Normal);
			goto Done;
		}
		LogClose(L"Depth", L"Passed", LGF::Normal);
	}
	fPassed = true;
Done:
	LogClose(tag, fPassed ? L"Passed" : L"Failed", LGF::Normal);
}
