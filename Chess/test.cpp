/*
 *
 *	test.cpp
 * 
 *	A test suite to make sure we've got a good working game and
 *	move generation
 * 
 */

#include "ga.h"
#include "uiga.h"



/*
 *
 *	Test harness
 * 
 */


/*
 *
 *	TEST base class
 *
 *	This is the base class for all the individual tests. Tests are organized as a 
 *	tree. Non-leaf nodes enumerate through child sub-tests
 *
 */


class TEST
{
protected:
	UIGA& uiga;
	TEST* ptestParent;
	vector<TEST*> vptest;

public:
	TEST(UIGA& uiga, TEST* ptestParent);
	virtual ~TEST(void);
	virtual wstring SzName(void) const;
	virtual wstring SzSubName(void) const;
	void Add(TEST* ptest);
	void Clear(void);
	ERR RunAll(void);
	ERR ErrRun(void);
	virtual void Run(void);
	int Error(const string& szMsg);
	bool FContinueTest(ERR err) const;

	/* independent board validation */

	void ValidateFEN(const char* szFEN) const;
	void ValidatePieces(const char*& sz) const;
	void ValidateMoveColor(const char*& sz) const;
	void ValidateCastle(const char*& sz) const;
	void ValidateEnPassant(const char*& sz) const;
	void SkipWhiteSpace(const char*& sz) const;
	void SkipToWhiteSpace(const char*& sz) const;
};


TEST::TEST(UIGA& uiga, TEST* ptestParent) : uiga(uiga), ptestParent(ptestParent)
{
}

TEST::~TEST(void)
{
	Clear();
}

wstring TEST::SzName(void) const
{
	return L"Test";
}

wstring TEST::SzSubName(void) const
{
	return L"";
}

void TEST::Add(TEST* ptest)
{
	vptest.push_back(ptest);
}

void TEST::Clear(void)
{
	for (; !vptest.empty(); vptest.pop_back())
		delete vptest.back();
}

ERR TEST::RunAll(void)
{
	LogOpen(SzName(), SzSubName(), lgfNormal);

	ERR err = ErrRun();
	if (FContinueTest(err)) {
		for (TEST* ptest : vptest) {
			err = ptest->RunAll();
			if (!FContinueTest(err))
				break;
		}
	}

	switch (err) {
	case errNone:
		LogClose(SzName(), L"Passed", lgfBold);
		break;
	default:
	case errFailed:
		LogClose(SzName(), L"Failed", lgfBold);
		break;
	case errInterrupted:
		LogClose(SzName(), L"Interrupted", lgfItalic);
		break;
	}
	return err;
}

ERR TEST::ErrRun(void)
{
	try {
		Run();
	}
	catch (exception& ex) {
		LogData(WszWidenSz(ex.what()));
		return errFailed;
	}
	catch (...) {
		LogData(L"Unknown exception");
		return errFailed;
	}
	return errNone;

}

bool TEST::FContinueTest(ERR err) const
{
	if (err == errInterrupted)
		return false;
	return true;
}


/*	TEST::Run
 *
 *	The leaf node test run.
 */
void TEST::Run(void)
{
}

int TEST::Error(const string& szMsg)
{
	return papp->Error(szMsg, MB_OK);
}


/*	TEST::ValidateFEN
 *
 *	Verifies the board matches the board state in the FEN string. Note that we very
 *	specifically do our own FEN parsing here.
 */
void TEST::ValidateFEN(const char* szFEN) const
{
	const char* sz = szFEN;
	ValidatePieces(sz);
	ValidateMoveColor(sz);
	ValidateCastle(sz);
	ValidateEnPassant(sz);
}

void TEST::ValidatePieces(const char*& sz) const
{
	SkipWhiteSpace(sz);
	int rank = 7;
	SQ sq = SQ(rank, 0);
	APC apc = apcNull;
	CPC cpc = cpcNil;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case '/':  
			if (--rank < 0)
				throw EXPARSE("too many FEN ranks");
			sq = SQ(rank, 0); 
			break;
		case 'r': 
		case 'R': apc = apcRook; goto CheckPiece; 
		case 'n': 
		case 'N': apc = apcKnight; goto CheckPiece;
		case 'b': 
		case 'B': apc = apcBishop; goto CheckPiece;
		case 'q': 
		case 'Q': apc = apcQueen; goto CheckPiece;
		case 'k': 
		case 'K': apc = apcKing; goto CheckPiece;
		case 'p': 
		case 'P':
			apc = apcPawn;
CheckPiece:
			cpc = islower(*sz) ? cpcBlack : cpcWhite;
			if (uiga.ga.bdg.ApcFromSq(sq) != apc || uiga.ga.bdg.CpcFromSq(sq) != cpc)
				throw EXFAILTEST("FEN piece mismatch at " + (string)sq);
			sq++; 
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
			for (int dsq = *sz - L'0'; dsq > 0; dsq--, sq++) {
				if (!uiga.ga.bdg.FIsEmpty(sq))
					throw EXFAILTEST("FEN piece mismatch at " + (string)sq);
			}
			break;
		default: 
			throw EXPARSE(string("invalid FEN piece '") + *sz + '\''); 
		}
	}

	SkipToWhiteSpace(sz);
}


void TEST::ValidateMoveColor(const char*& sz) const
{
	SkipWhiteSpace(sz);
	CPC cpc = cpcNil;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case 'b': 
			cpc = cpcBlack;
			break;
		case 'w':
		case '-': 
			cpc = cpcWhite;
			break;
		default: 
			throw EXPARSE("invalid move color");
		}
	}
	if (uiga.ga.bdg.cpcToMove != cpc)
		throw EXFAILTEST("move color mismatch");
	SkipToWhiteSpace(sz);
}


void TEST::ValidateCastle(const char*& sz) const
{
	SkipWhiteSpace(sz);
	int cs = 0;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case 'k': 
			cs |= csKing << cpcWhite; 
			break;
		case 'q': 
			cs |= csQueen << cpcWhite; 
			break;
		case 'K': 
			cs |= csKing << cpcBlack; 
			break;
		case 'Q': 
			cs |= csQueen << cpcBlack;
			break;
		case '-': 
			break;
		default: 
			throw EXPARSE("invalid castle state");
		}
	}
	if (uiga.ga.bdg.csCur != cs)
		throw EXFAILTEST("mismatched castle state");
	SkipToWhiteSpace(sz);
}


void TEST::ValidateEnPassant(const char*& sz) const
{
	SkipWhiteSpace(sz);
	if (*sz == '-') {
		if (!uiga.ga.bdg.sqEnPassant.fIsNil())
			throw EXFAILTEST("en passant square mismatch");
	}
	else if (*sz >= 'a' && *sz <= 'h') {
		int file = *sz - 'a';
		sz++;
		if (*sz >= '1' && *sz <= '8') {
			if (uiga.ga.bdg.sqEnPassant != SQ(*sz - '1', file))
				throw EXFAILTEST("en passant square mismatch");
			sz++;
			if (*sz && *sz != ' ')
				throw EXPARSE("invalid en passant square");
		}
	}
	else
		throw EXPARSE("invalid en passant square");

	SkipToWhiteSpace(sz);
}


void TEST::SkipWhiteSpace(const char*& sz) const
{
	for (; *sz && *sz == ' '; sz++)
		;
}


void TEST::SkipToWhiteSpace(const char*& sz) const
{
	for (; *sz && *sz != ' '; sz++)
		;
}


/*
 *
 *	TESTNEWGAME
 * 
 */


class TESTNEWGAME : public TEST
{
public:
	TESTNEWGAME(UIGA& uiga, TEST* ptestParent) : TEST(uiga, ptestParent) { }
	wstring SzName(void) const { return L"New Game"; }

	virtual void Run(void)
	{
		uiga.InitGame(nullptr, nullptr);
		ValidateFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}
};


/* 
 *
 *	TESTPERFT
 * 
 */


class TESTPERFT : public TEST
{
public:
	TESTPERFT(UIGA& uiga, TEST* ptestParent) : TEST(uiga, ptestParent) { }	
	virtual wstring SzName(void) const { return L"Perft"; }

	virtual void Run(void)
	{
		uiga.PerftTest();
	}
};


/*
 *
 *	TESTUNDO
 * 
 *	Undo test. Reads a PGN file and plays and undoes every game in the file.
 * 
 */


class TESTUNDO : public TEST
{
public:
	TESTUNDO(UIGA& uiga, TEST* ptestParent) : TEST(uiga, ptestParent) { }
	virtual wstring SzName(void) const { return L"Undo"; }

	virtual void Run(void)
	{
		WIN32_FIND_DATA ffd;
		wstring szSpec(L"..\\Chess\\Test\\Players\\*.pgn");
		HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
		if (hfind == INVALID_HANDLE_VALUE)
			throw EX("No PGN files found");
		while (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			if (FindNextFile(hfind, &ffd) == 0) {
				FindClose(hfind);
				return;
			}

		FindClose(hfind);
		szSpec = wstring(L"..\\Chess\\Test\\Players\\") + ffd.cFileName;
		LogOpen(ffd.cFileName, L"", lgfNormal);
		try {
			PlayUndoPGNFile(szSpec.c_str());
			LogClose(ffd.cFileName, L"Passed", lgfNormal);
		}
		catch (EX& ex) {
			(void)ex;
			LogClose(ffd.cFileName, L"Failed", lgfBold);
			throw;
		}
	}


	void PlayUndoPGNFile(const wchar_t* szFile)
	{
		ifstream is(szFile, ifstream::in);

		wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
		PROCPGNGA procpgngaSav(uiga.ga, new PROCPGNTESTUNDO(uiga.ga));
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			LogTemp(wjoin(L"Game ", igame + 1));
			if (uiga.ga.DeserializeGame(istkpgn) != errNone)
				break;
			UndoFullGame();
			ValidateFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
	}

	void UndoFullGame(void)
	{
		while (uiga.ga.bdg.imveCurLast >= 0) {
			BDG bdgInit = uiga.ga.bdg;
			uiga.UndoMv(spmvHidden);
			uiga.RedoMv(spmvHidden);
			assert(uiga.ga.bdg == bdgInit);
			if (uiga.ga.bdg != bdgInit)
				throw EXFAILTEST();
			uiga.UndoMv(spmvHidden);
		}
	}
};


ERR PROCPGNTESTUNDO::ProcessTag(int tkpgn, const string& szValue)
{
	return PROCPGNTEST::ProcessTag(tkpgn, szValue);
}


ERR PROCPGNTESTUNDO::ProcessMv(MVE mve)
{
	static BDG bdgInit = ga.bdg;
	ga.bdg.MakeMv(mve);
	static BDG bdgNew = ga.bdg;
	ga.bdg.UndoMv();
	if (bdgInit != ga.bdg)
		throw EXFAILTEST();
	ga.bdg.RedoMv();
	if (bdgNew != ga.bdg)
		throw EXFAILTEST();
	return errNone;
}


/*
 *
 *	TESTPGNS
 * 
 */


class TESTPGNS : public TEST
{
	wstring szSub;
public:
	TESTPGNS(UIGA& uiga, TEST* ptestParent, const wstring& szSub) : TEST(uiga, ptestParent), szSub(szSub) { }
	virtual wstring SzName(void) const { return L"Play"; }
	virtual wstring SzSubName(void) const { return szSub; }

	virtual void Run(void)
	{
		PlayPGNFiles(wstring(L"..\\Chess\\Test\\") + szSub);
	}

	void PlayPGNFiles(const wstring& szPath)
	{
		WIN32_FIND_DATA ffd;
		wstring szSpec = szPath + L"\\*.pgn";
		HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
		if (hfind == INVALID_HANDLE_VALUE)
			throw EX("No .pgn files found");
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			wstring szSpec = szPath + L"\\" + ffd.cFileName;
			try {
				LogOpen(ffd.cFileName, L"", lgfNormal);
				PlayPGNFile(szSpec.c_str());
				LogClose(ffd.cFileName, L"Passed", lgfNormal);
			}
			catch (exception& ex) {
				LogData(WszWidenSz(ex.what()));
				LogClose(ffd.cFileName, L"Failed", lgfBold);
				FindClose(hfind);
				throw;
			}
		} while (FindNextFile(hfind, &ffd) != 0);
		FindClose(hfind);
	}

	void PlayPGNFile(const wchar_t szFile[])
	{
		ifstream is(szFile, ifstream::in);

		wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
		PROCPGNGA procpgngaSav(uiga.ga, new PROCPGNTEST(uiga.ga));
		ERR err;
		ISTKPGN istkpgn(is);
		istkpgn.SetSzStream(szFileBase);
		int igame = 0;
		do {
			igame++;
			LogTemp(wjoin(L"Game ", igame));
			err = uiga.ga.DeserializeGame(istkpgn);
		} while (err == errNone);
	}
};


ERR PROCPGNTEST::ProcessTag(int tkpgn, const string& szValue)
{
	return PROCPGNOPEN::ProcessTag(tkpgn, szValue);
}


ERR PROCPGNTEST::ProcessMv(MVE mve)
{
	return PROCPGNOPEN::ProcessMv(mve);
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
	VMVE vmve;
	bdg.GenVmve(vmve, ggPseudo);
	uint64_t cmv = 0;
	for (MVE mve : vmve) {
		bdg.MakeMv(mve);
		if (!bdg.FInCheck(~bdg.cpcToMove))
			cmv += CmvPerft(depth - 1);
		bdg.UndoMv();
	}
	return cmv;
}

uint64_t GA::CmvPerftBulk(int depth)
{
	VMVE vmve;
	bdg.GenVmve(vmve, ggLegal);
	if (depth <= 1)
		return vmve.cmve();
	uint64_t cmv = 0;
	for (MVE mve : vmve) {
		bdg.MakeMv(mve);
		cmv += CmvPerftBulk(depth - 1);
		bdg.UndoMv();
	}
	return cmv;
}

uint64_t UIGA::CmvPerftDivide(int depthPerft)
{
	if (depthPerft == 0)
		return 1;
	assert(depthPerft >= 1);
	VMVE vmve;
	ga.bdg.GenVmve(vmve, ggLegal);
	if (depthPerft == 1)
		return vmve.cmve();
	uint64_t cmv = 0;
#ifndef NDEBUG
	BDG bdgInit = ga.bdg;
#endif
	for (MVE mve : vmve) {
		ga.bdg.MakeMv(mve);
		LogOpen(TAG(ga.bdg.SzDecodeMvPost(mve), ATTR(L"FEN", (wstring)ga.bdg)), L"", lgfNormal);
		uint64_t cmvMove = ga.CmvPerft(depthPerft - 1);
		cmv += cmvMove;
		ga.bdg.UndoMv();
		LogClose(ga.bdg.SzDecodeMvPost(mve), to_wstring(cmvMove), lgfNormal);
		assert(ga.bdg == bdgInit);
	}
	return cmv;
}


/*	GA::PerftTest
 *
 *	Move generation test that uses perft divide to make sure we're generating
 *	the right number of moves. These move counts are validated perft counts
 *	found on chessprogramming.org
 */

struct {
	const wchar_t* szTitle;
	const char* szFEN;
	unsigned long long aull[20];
	int cull;
} aperfttest[] = {
	/*
	 * perft tests from chessprogramming.org 
	 */
	{L"Initial", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
			{1ULL, 20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL, 84998978956ULL,
			 2439530234167ULL, 69352859712417ULL, 2097651003696806ULL, 62854969236701747ULL, 1981066775000396239ULL}, 6},
	{ L"Kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", { 1ULL, 48ULL, 2039ULL, 97862LL, 4085603ULL, 193690690ULL, 8031647685ULL }, 5 },
	{ L"Position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", { 1ULL, 14ULL, 191ULL, 2812ULL, 43238ULL, 674624ULL, 11030083ULL, 178633661ULL, 3009794393ULL }, 7 },
	{ L"Position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", { 1ULL, 6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL}, 6 },
	{ L"Position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1ULL, 44ULL, 1486ULL, 62379ULL, 2103487ULL, 89941194ULL}, 5 },
	{ L"Position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
			{ 1ULL, 46ULL, 2079ULL, 89890ULL, 3894594ULL, 164075551ULL, 6923051137ULL, 287188994746ULL, 11923589843526ULL,
			  490154852788714ULL }, 5 },
	/*
	 *	perft test suite from algerbrex
	 *	https://github.com/algerbrex/blunder/blob/main/testdata/perftsuite.epd
	 */
	{L"Perftsuite 3", "4k3/8/8/8/8/8/8/4K2R w K - 0 1", {1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL}, 6},
	{L"Perftsuite 4", "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", {1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL}, 6},
	{L"Perftsuite 5", "4k2r/8/8/8/8/8/8/4K3 w k - 0 1", {1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL}, 6},
	{L"Perftsuite 6", "r3k3/8/8/8/8/8/8/4K3 w q - 0 1", {1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL}, 6},
	{L"Perftsuite 7", "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", {1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL}, 6},
	{L"Perftsuite 8", "r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", {1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL}, 6},
	{L"Perftsuite 9", "8/8/8/8/8/8/6k1/4K2R w K - 0 1", {1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL}, 6},
	{L"Perftsuite 10", "8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", {1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL}, 6},
	{L"Perftsuite 11", "4k2r/6K1/8/8/8/8/8/8 w k - 0 1", {1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL}, 6},
	{L"Perftsuite 12", "r3k3/1K6/8/8/8/8/8/8 w q - 0 1", {1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724}, 6 },
	{L"Perftsuite 13", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL}, 6 },
	{L"Perftsuite 14", "r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL}, 6 },
	{L"Perftsuite 15", "r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL}, 6 },
	{L"Perftsuite 16", "r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL}, 6 },
	{L"Perftsuite 17", "1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL}, 6 },
	{L"Perftsuite 18", "2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL}, 6 },
	{L"Perftsuite 19", "r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL}, 6 },
	{L"Perftsuite 20", "4k3/8/8/8/8/8/8/4K2R b K - 0 1", { 1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL}, 6 }, 
	{L"Perftsuite 21", "4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", { 1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL}, 6},
	{L"Perftsuite 22", "4k2r/8/8/8/8/8/8/4K3 b k - 0 1", { 1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL}, 6},
	{L"Perftsuite 23", "r3k3/8/8/8/8/8/8/4K3 b q - 0 1", { 1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL}, 6},
	{L"Perftsuite 24", "4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", { 1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL}, 6},
	{L"Perftsuite 25", "r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", { 1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL}, 6},
	{L"Perftsuite 26", "8/8/8/8/8/8/6k1/4K2R b K - 0 1", { 1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL}, 6},
	{L"Perftsuite 27", "8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", { 1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724ULL}, 6},
	{L"Perftsuite 28", "4k2r/6K1/8/8/8/8/8/8 b k - 0 1", { 1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL}, 6},
	{L"Perftsuite 29", "r3k3/1K6/8/8/8/8/8/8 b q - 0 1", { 1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL}, 6},
	{L"Perftsuite 30", "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL}, 6},
	{L"Perftsuite 31", "r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL}, 6},
	{L"Perftsuite 32", "r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL}, 6},
	{L"Perftsuite 33", "r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL}, 6},
	{L"Perftsuite 34", "1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL}, 6},
	{L"Perftsuite 35", "2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL}, 6},
	{L"Perftsuite 36", "r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL}, 6},
	{L"Perftsuite 37", "8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", { 1ULL, 14ULL, 195ULL, 2760ULL, 38675ULL, 570726ULL, 8107539ULL}, 6},
	{L"Perftsuite 38", "8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", { 1ULL, 11ULL, 156ULL, 1636ULL, 20534ULL, 223507ULL, 2594412ULL}, 6},
	{L"Perftsuite 39", "8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", { 1ULL, 19ULL, 289ULL, 4442ULL, 73584ULL, 1198299ULL, 19870403ULL}, 6},
	{L"Perftsuite 40", "K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL}, 6},
	{L"Perftsuite 41", "k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL}, 6},
	{L"Perftsuite 42", "8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", { 1ULL, 15ULL, 193ULL, 2816ULL, 40039ULL, 582642ULL, 8503277ULL}, 6},
	{L"Perftsuite 43", "8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", { 1ULL, 16ULL, 180ULL, 2290ULL, 24640ULL, 288141ULL, 3147566ULL}, 6},
	{L"Perftsuite 44", "8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", { 1ULL, 4ULL, 68ULL, 1118ULL, 16199ULL, 281190ULL, 4405103ULL}, 6},
	{L"Perftsuite 45", "K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL}, 6},
	{L"Perftsuite 46", "k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL}, 6},
	{L"Perftsuite 47", "B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", { 1ULL, 17ULL, 278ULL, 4607ULL, 76778ULL, 1320507ULL, 22823890ULL}, 6},
	{L"Perftsuite 48", "8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", { 1ULL, 21ULL, 316ULL, 5744ULL, 93338ULL, 1713368ULL, 28861171ULL}, 6},
	{L"Perftsuite 49", "k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL}, 6},
	{L"Perftsuite 50", "K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL}, 6},
	{L"Perftsuite 51", "B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", { 1ULL, 6ULL, 106ULL, 1829ULL, 31151ULL, 530585ULL, 9250746ULL}, 6},
	{L"Perftsuite 52", "8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", { 1ULL, 17ULL, 309ULL, 5133ULL, 93603ULL, 1591064ULL, 29027891ULL}, 6},
	{L"Perftsuite 53", "k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL}, 6},
	{L"Perftsuite 54", "K7/b7/1b6/1b6/8/8/8/k6B b - -0 1", { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL}, 6},
	{L"Perftsuite 55", "7k/RR6/8/8/8/8/rr6/7K w - - 0 1", { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL}, 6},
	{L"Perftsuite 56", "R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", { 1ULL, 36ULL, 1027ULL, 29215ULL, 771461ULL, 20506480ULL, 525169084ULL}, 6},
	{L"Perftsuite 57", "7k/RR6/8/8/8/8/rr6/7K b - - 0 1", { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL}, 6},
	{L"Perftsuite 58", "R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", { 1ULL, 36ULL, 1027ULL, 29227ULL, 771368ULL, 20521342ULL, 524966748ULL}, 6},
	{L"Perftsuite 59", "6kq/8/8/8/8/8/8/7K w - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 60", "6KQ/8/8/8/8/8/8/7k b - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 61", "K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL}, 6},
	{L"Perftsuite 62", "6qk/8/8/8/8/8/8/7K b - - 0 1", { 1ULL, 22ULL, 43ULL, 1015ULL, 4167ULL, 105749ULL, 419369ULL}, 6},
	{L"Perftsuite 63", "6KQ/8/8/8/8/8/8/7k b - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 64", "K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL}, 6},
	{L"Perftsuite 65", "8/8/8/8/8/K7/P7/k7 w - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 66", "8/8/8/8/8/7K/7P/7k w - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 67", "K7/p7/k7/8/8/8/8/8 w - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 68", "7K/7p/7k/8/8/8/8/8 w - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 69", "8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", { 1ULL, 7ULL, 35ULL, 210ULL, 1091ULL, 7028ULL, 34834ULL}, 6},
	{L"Perftsuite 70", "8/8/8/8/8/K7/P7/k7 b - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 71", "8/8/8/8/8/7K/7P/7k b - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 72", "K7/p7/k7/8/8/8/8/8 b - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 73", "7K/7p/7k/8/8/8/8/8 b - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 74", "8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", { 1ULL, 5ULL, 35ULL, 182ULL, 1091ULL, 5408ULL, 34822ULL}, 6},
	{L"Perftsuite 75", "8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL}, 6},
	{L"Perftsuite 76", "4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL}, 6},
	{L"Perftsuite 77", "8/8/7k/7p/7P/7K/8/8 w - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 78", "8/8/k7/p7/P7/K7/8/8 w - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 79", "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL}, 6},
	{L"Perftsuite 80", "8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL}, 6},
	{L"Perftsuite 81", "8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL}, 6},
	{L"Perftsuite 82", "k7/8/3p4/8/3P4/8/8/7K w - - 0 1", { 1ULL, 4ULL, 15ULL, 90ULL, 534ULL, 3450ULL, 20960ULL}, 6},
	{L"Perftsuite 83", "8/8/7k/7p/7P/7K/8/8 b - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 84", "8/8/k7/p7/P7/K7/8/8 b - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 85", "8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL}, 6},
	{L"Perftsuite 86", "8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL}, 6},
	{L"Perftsuite 87", "8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL}, 6},
	{L"Perftsuite 88", "k7/8/3p4/8/3P4/8/8/7K b - - 0 1", { 1ULL, 4ULL, 15ULL, 89ULL, 537ULL, 3309ULL, 21104ULL}, 6},
	{L"Perftsuite 89", "7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", { 1ULL, 4ULL, 19ULL, 117ULL, 720ULL, 4661ULL, 32191ULL}, 6},
	{L"Perftsuite 90", "7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", { 1ULL, 5ULL, 19ULL, 116ULL, 716ULL, 4786ULL, 30980ULL}, 6},
	{L"Perftsuite 91", "k7/8/8/7p/6P1/8/8/K7 w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 92", "k7/8/7p/8/8/6P1/8/K7 w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 93", "k7/8/8/6p1/7P/8/8/K7 w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 94", "k7/8/6p1/8/8/7P/8/K7 w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 95", "k7/8/8/3p4/4p3/8/8/7K w - - 0 1", { 1ULL, 3ULL, 15ULL, 84ULL, 573ULL, 3013ULL, 22886ULL}, 6},
	{L"Perftsuite 96", "k7/8/3p4/8/8/4P3/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL}, 6},
	{L"Perftsuite 97", "7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 19ULL, 117ULL, 720ULL, 5014ULL, 32167ULL}, 6},
	{L"Perftsuite 98", "7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", { 1ULL, 4ULL, 19ULL, 117ULL, 712ULL, 4658ULL, 30749ULL}, 6},
	{L"Perftsuite 99", "k7/8/8/7p/6P1/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 100", "k7/8/7p/8/8/6P1/8/K7 b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 101", "k7/8/8/6p1/7P/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 102", "k7/8/6p1/8/8/7P/8/K7 b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 103", "k7/8/8/3p4/4p3/8/8/7K b - - 0 1", { 1ULL, 5ULL, 15ULL, 102ULL, 569ULL, 4337ULL, 22579ULL}, 6 },
	{L"Perftsuite 104", "k7/8/3p4/8/8/4P3/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL}, 6 },
	{L"Perftsuite 105", "7k/8/8/p7/1P6/8/8/7K w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 106", "7k/8/p7/8/8/1P6/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 107", "7k/8/8/1p6/P7/8/8/7K w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 108", "7k/8/1p6/8/8/P7/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 109", "k7/7p/8/8/8/8/6P1/K7 w - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 110", "k7/6p1/8/8/8/8/7P/K7 w - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 111", "3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL}, 6 },
	{L"Perftsuite 112", "7k/8/8/p7/1P6/8/8/7K b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 113", "7k/8/p7/8/8/1P6/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 114", "7k/8/8/1p6/P7/8/8/7K b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 115", "7k/8/1p6/8/8/P7/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 116", "k7/7p/8/8/8/8/6P1/K7 b - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 117", "k7/6p1/8/8/8/8/7P/K7 b - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 118", "3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL}, 6 },
	{L"Perftsuite 119", "8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL}, 6 },
	{L"Perftsuite 120", "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL}, 6 },
	{L"Perftsuite 121", "8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL}, 6 },
	{L"Perftsuite 122", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL}, 6 },
	{L"Perftsuite 123", "8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL}, 6 },
	{L"Perftsuite 124", "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL}, 6 },
	{L"Perftsuite 125", "8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL}, 6 },
	{L"Perftsuite 126", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL}, 6 },

	{ L"Perftsuite 127", "8/8/1k6/8/2pP4/8/5BK1/8 b - d3 0 1", {1ULL, 0, 0, 0, 0, 0, 824064ULL}, 6 },
	{ L"Perftsuite 128", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", {1ULL, 0, 0, 0, 0, 0, 1440467ULL}, 6 },
	{ L"Perftsuite 129", "8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1", {1ULL, 0, 0, 0, 0, 0, 1440467ULL}, 6 },
	{L"Perftsuite 130", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", {1ULL, 0, 0, 0, 0, 0, 661072ULL}, 6},
	{L"Perftsuite 131", "4k2r/8/8/8/8/8/8/5K2 b k - 0 1", {1ULL, 0, 0, 0, 0, 0, 661072ULL}, 6},
	{L"Perftsuite 132", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", {1ULL, 0, 0, 0, 0, 0, 803711ULL}, 6},
	{L"Perftsuite 133", "r3k3/8/8/8/8/8/8/3K4 b q - 0 1", {1ULL, 0, 0, 0, 0, 0, 803711ULL}, 6},
	{L"Perftsuite 134", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 1274206ULL}, 4},
	{L"Perftsuite 135", "r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1", {1ULL, 0, 0, 0, 1274206ULL}, 4},
	{L"Perftsuite 136", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", {1ULL, 0, 0, 0, 1720476ULL}, 4},
	{L"Perftsuite 137", "r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 1720476ULL}, 4},
	{L"Perftsuite 138", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 3821001ULL}, 6},
	{L"Perftsuite 139", "3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 3821001ULL}, 6},
	{L"Perftsuite 140", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", {1ULL, 0, 0, 0, 0, 1004658ULL}, 5},
	{L"Perftsuite 141", "5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 1004658ULL}, 5},
	{L"Perftsuite 142", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 217342ULL}, 6},
	{L"Perftsuite 143", "8/k7/8/8/8/8/1p6/4K3 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 217342ULL}, 6},
	{L"Perftsuite 144", "8/P1k5/K7/8/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 92683ULL}, 6},
	{L"Perftsuite 145", "8/8/8/8/8/k7/p1K5/8 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 92683ULL}, 6},
	{L"Perftsuite 146", "K1k5/8/P7/8/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 2217ULL}, 6},
	{L"Perftsuite 147", "8/8/8/8/8/p7/8/k1K5 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 2217ULL}, 6},
	{L"Perftsuite 148", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 567584ULL}, 7},
	{L"Perftsuite 149", "8/8/8/8/1k6/8/K1p5/8 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 567584ULL}, 7},
	{L"Perftsuite 150", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", {1ULL, 0, 0, 0, 23527ULL}, 4},
	{L"Perftsuite 151", "8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 1", {1ULL, 0, 0, 0, 23527ULL}, 4 },
	{L"Perftsuite 152", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 0, 193690690ULL}, 5 },
	{L"Perftsuite 153", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 11030083ULL}, 6 },
	{L"Perftsuite 154", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1ULL, 0, 0, 0, 0, 15833292ULL}, 5 },
	{L"Perftsuite 155", "rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 1", {1ULL, 0, 0, 53392ULL}, 3 },
	{L"Perftsuite 156", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 1", {1ULL, 0, 0, 0, 0, 164075551ULL}, 5 },
	{L"Perftsuite 157", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 178633661ULL}, 7 },
	{L"Perftsuite 158", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1ULL, 0, 0, 0, 0, 0, 706045033ULL}, 6 },
	{L"Perftsuite 159", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1ULL, 0, 0, 0, 0, 89941194ULL}, 5 },
	{L"Perftsuite 160", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", {1ULL, 0, 0, 0, 0, 1063513ULL}, 5 },
	{L"Perftsuite 161", "3k4/3p4/8/K1P4r/8/8/8/8 b - -0 1", {1ULL, 0, 0, 0, 0, 0, 1134888ULL}, 6 },
	{L"Perftsuite 162", "8/8/4k3/8/2p5/8/B2P2K1/8 w - -0 1", {1ULL, 0, 0, 0, 0, 0, 1015133ULL}, 6 }
};

void UIGA::PerftTest(void)
{
	for (int iperfttest = 0; iperfttest < CArray(aperfttest); iperfttest++)
		RunPerftTest(aperfttest[iperfttest].szTitle,
			aperfttest[iperfttest].szFEN,
			aperfttest[iperfttest].aull,
			aperfttest[iperfttest].cull, true);
}


/*	GA::RunPerftTest
 *
 *	Runs one particular perft test starting with the original board position (szFEN). 
 *	Cycles through each depth from 1 to depthLast, verifying move counts in mpdepthcmv. 
 *	The tag is just used for debug output.
 */
void UIGA::RunPerftTest(const wchar_t tag[], const char szFEN[], const uint64_t mpdepthcmv[], int depthLast, bool fDivide)
{
	LogOpen(tag, L"", lgfBold);
	InitGame(szFEN, nullptr);
	uibd.Redraw();

	bool fPassed = false;
	int depthFirst = fDivide ? 2 : 1;

	for (int depth = depthFirst; depth <= depthLast; depth++) {

		uint64_t cmvExp = mpdepthcmv[depth];
		if (cmvExp == 0)
			continue;
#ifndef NDEBUG
		/* in debug mode, these tests take too damn long */
		if (cmvExp > 4000000ULL)
			break;
#endif

		/* what we expect to happen */

		PumpMsg();
		LogOpen(L"Depth", to_wstring(depth), lgfNormal);
		LogData(L"Expected: " + to_wstring(cmvExp));

		/* time the perft */

		time_point<high_resolution_clock> tpStart = high_resolution_clock::now();
		uint64_t cmvAct = fDivide ? CmvPerftDivide(depth) : ga.CmvPerft(depth);
		time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();

		/* display the results */

		LogData(L"Actual: " + to_wstring(cmvAct));
		duration dtp = tpEnd - tpStart;
		microseconds us = duration_cast<microseconds>(dtp);
		float sp = 1000.0f * (float)cmvAct / (float)us.count();
		LogData(L"Speed: " + to_wstring((int)round(sp)) + L" moves/ms");
		PumpMsg();

		/* and handle success/failure */

		if (cmvExp != cmvAct) {
			LogClose(L"Depth", L"Failed", lgfBold);
			goto Done;
		}
		LogClose(L"Depth", L"Passed", lgfNormal);
	}
	fPassed = true;

Done:
	if (fPassed) {
		LogClose(tag, L"Passed", lgfNormal);
	}
	else {
		LogClose(tag, L"Failed", lgfBold);
	}
}


/*	UIGA::Test
 *
 *	This is the top-level test script.
 */
void UIGA::Test(void)
{
	if (fInTest) {
		fInterruptPumpMsg = true;
		return;
	}
	TEST testRoot(*this, nullptr);
	testRoot.Add(new TESTNEWGAME(*this, &testRoot));
	testRoot.Add(new TESTPERFT(*this, &testRoot));
	testRoot.Add(new TESTUNDO(*this, &testRoot));
	testRoot.Add(new TESTPGNS(*this, &testRoot, L"Players"));

	fInTest = true;
	InitLog();
	testRoot.RunAll();
	fInTest = false;
}


/*
 * 
 *	UIDT screen panel
 * 
 *	Drawing test panel, for testing rendering primitives in the UI object. This is just
 *	a test canvas to experiment with various drawing primitives and see exactly what
 *	they do. Any code left here is left over from some previous test. Feel free to 
 *	remove, replace, or rewrite.
 * 
 */


UIDT::UIDT(UIGA& uiga) : UIP(uiga), ptxTest(nullptr), ptxTest2(nullptr)
{
	SetCoBack(coGridLine);
}


UIDT::~UIDT(void)
{
	DiscardRsrc();
}


bool UIDT::FCreateRsrc(void)
{
	if (ptxTest)
		return false;

	ptxTest = PtxCreate(30.0f, false, false);
	ptxTest2 = PtxCreate(20.0f, false, false);
	
	return true;
}

void UIDT::ReleaseRsrc(void)
{
	SafeRelease(&ptxTest);
	SafeRelease(&ptxTest2);
}


void UIDT::AdvanceDrawSz(const wstring& sz, TX* ptx, RC& rc)
{
	float dx = SizFromSz(sz, ptx).width;
	DrawSz(sz, ptx, rc);
	rc.left += dx;
}

void UIDT::AdvanceDrawSzFit(const wstring& sz, TX* ptx, RC& rc)
{
	float dx = SizFromSz(sz, ptx).width;
	DrawSzFit(sz, ptx, rc);
	rc.left += dx;
}

void UIDT::AdvanceDrawSzBaseline(const wstring& sz, TX* ptx, RC& rc, float dyBaseline)
{
	float dx = SizFromSz(sz, ptx).width;
	DrawSzBaseline(sz, ptx, rc, dyBaseline);
	rc.left += dx;
}


void UIDT::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	rc.Inflate(-20, -20);

	FillRc(rc, coBlack);
	rc.Inflate(-1, -1);
	FillRc(rc, coWhite);
	rc.Inflate(-1, -1);

	AdvanceDrawSz(L"BOO!", ptxTest, rc);
	AdvanceDrawSz(L"ace", ptxTest, rc);
	AdvanceDrawSz(L"gawk", ptxTest, rc);
	AdvanceDrawSz(L"gpq", ptxTest, rc);
	AdvanceDrawSz(L"gp0q", ptxTest, rc);
	AdvanceDrawSz(L"0123456789", ptxTest, rc);

	wchar_t sz[20] = { '1', '.', ' ', chBlackRook, 'g', '-', 'g', '3', ' ', ' ', chWhiteBishop, '-', 'b', '5', 0 };
	rc.top += SizFromSz(L"0", ptxTest).height;
	rc.bottom = rc.top + SizFromSz(sz, ptxTest2).height;
	rc.left = 20;
	float dyBaseline = DyBaselineSz(sz, ptxTest2);
	AdvanceDrawSzBaseline(L"1. ", ptxTest2, rc, dyBaseline);
	AdvanceDrawSzBaseline(sz, ptxTest2, rc, dyBaseline);

}