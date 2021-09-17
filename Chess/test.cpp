/*
 *
 *	test.cpp
 * 
 *	A test suite to make sure we've got a good working game and
 *	move generation
 * 
 */

#include "ga.h"


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
	APP& app;
	GA& ga;
	TEST* ptestParent;
	vector<TEST*> rgptest;

public:
	TEST(GA& ga, TEST* ptestParent);
	virtual ~TEST(void);
	virtual wstring SzName(void) const;
	virtual wstring SzSubName(void) const;
	void Add(TEST* ptest);
	void Clear(void);
	bool FDepthLog(LGT lgt, int& depth);
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
	inline void LogTemp(const wstring& szData)
	{
		int depthLog;
		if (FDepthLog(LGT::Temp, depthLog))
			AddLog(LGT::Temp, LGF::Normal, depthLog, L"", szData);
	}
	void RunAll(void);
	virtual void Run(void);
	int Error(const string& szMsg);

	/* board validation */

	void ValidateFEN(const char* szFEN) const;
	void ValidatePieces(const char*& sz) const;
	void ValidateMoveColor(const char*& sz) const;
	void ValidateCastle(const char*& sz) const;
	void ValidateEnPassant(const char*& sz) const;
	void SkipWhiteSpace(const char*& sz) const;
	void SkipToWhiteSpace(const char*& sz) const;
};


TEST::TEST(GA& ga, TEST* ptestParent) : app(ga.App()), ga(ga), ptestParent(ptestParent)
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
	rgptest.push_back(ptest);
}

void TEST::Clear(void)
{
	for (; rgptest.size() > 0; rgptest.pop_back())
		delete rgptest.back();
}

void TEST::RunAll(void)
{
	LogOpen(SzName(), SzSubName());
	Run();
	for (TEST* ptest : rgptest) {
		ptest->RunAll();
	}
	LogClose(SzName(), L"Passed", LGF::Normal);
}


/*	TEST::Run
 *
 *	The leaf node test run.
 */
void TEST::Run(void)
{
}

bool TEST::FDepthLog(LGT lgt, int& depth)
{
	return ga.FDepthLog(lgt, depth);
}

void TEST::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	ga.AddLog(lgt, lgf, depth, tag, szData);
}

int TEST::Error(const string& szMsg)
{
	return app.Error(szMsg, MB_OK);
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
	APC apc = APC::Null;
	CPC cpc = CPC::NoColor;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case '/':  
			if (--rank < 0)
				throw EXPARSE("too many FEN ranks");
			sq = SQ(rank, 0); 
			break;
		case 'r': 
		case 'R': apc = APC::Rook; goto CheckPiece; 
		case 'n': 
		case 'N': apc = APC::Knight; goto CheckPiece;
		case 'b': 
		case 'B': apc = APC::Bishop; goto CheckPiece;
		case 'q': 
		case 'Q': apc = APC::Queen; goto CheckPiece;
		case 'k': 
		case 'K': apc = APC::King; goto CheckPiece;
		case 'p': 
		case 'P':
			apc = APC::Pawn;
CheckPiece:
			cpc = islower(*sz) ? CPC::Black : CPC::White;
			if (ga.bdg.ApcFromSq(sq) != apc || ga.bdg.CpcFromSq(sq) != cpc)
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
				if (!ga.bdg.FIsEmpty(sq))
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
	CPC cpc = CPC::NoColor;
	for (; *sz && *sz != ' '; sz++) {
		switch (*sz) {
		case 'b': 
			cpc = CPC::Black;
			break;
		case 'w':
		case '-': 
			cpc = CPC::White;
			break;
		default: 
			throw EXPARSE("invalid move color");
		}
	}
	if (ga.bdg.cpcToMove != cpc)
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
			cs |= csKing << CPC::White; 
			break;
		case 'q': 
			cs |= csQueen << CPC::White; 
			break;
		case 'K': 
			cs |= csKing << CPC::Black; 
			break;
		case 'Q': 
			cs |= csQueen << CPC::Black;
			break;
		case '-': 
			break;
		default: 
			throw EXPARSE("invalid castle state");
		}
	}
	if (ga.bdg.csCur != cs)
		throw EXFAILTEST("mismatched castle state");
	SkipToWhiteSpace(sz);
}


void TEST::ValidateEnPassant(const char*& sz) const
{
	SkipWhiteSpace(sz);
	if (*sz == '-') {
		if (!ga.bdg.sqEnPassant.fIsNil())
			throw EXFAILTEST("en passant square mismatch");
	}
	else if (*sz >= 'a' && *sz <= 'h') {
		int file = *sz - 'a';
		sz++;
		if (*sz >= '1' && *sz <= '8') {
			if (ga.bdg.sqEnPassant != SQ(*sz - '1', file))
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
	TESTNEWGAME(GA& ga, TEST* ptestParent) : TEST(ga, ptestParent) { }
	wstring SzName(void) const { return L"New Game"; }

	virtual void Run(void)
	{
		ga.NewGame(new RULE(0, 0, 0), SPMV::Hidden);
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
	TESTPERFT(GA& ga, TEST* ptestParent) : TEST(ga, ptestParent) { }	
	virtual wstring SzName(void) const { return L"Perft"; }

	virtual void Run(void)
	{
		ga.PerftTest();
	}
};


/*
 *
 *	TESTUNDO
 * 
 */


class TESTUNDO : public TEST
{
public:
	TESTUNDO(GA& ga, TEST* ptestParent) : TEST(ga, ptestParent) { }
	virtual wstring SzName(void) const { return L"Undo"; }

	virtual void Run(void)
	{
		WIN32_FIND_DATA ffd;
		wstring szSpec(L"..\\Chess\\Test\\Players\\*.pgn");
		HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
		if (hfind == INVALID_HANDLE_VALUE)
			throw EX("No PGN files found");
		while (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			if (FindNextFile(hfind, &ffd) == 0)
				goto Done;
		{
		wstring szSpec = wstring(L"..\\Chess\\Test\\Players\\") + ffd.cFileName;
		PlayUndoPGNFile(szSpec.c_str());
		}
Done:
		FindClose(hfind);
	}


	ERR PlayUndoPGNFile(const wchar_t* szFile)
	{
		ifstream is(szFile, ifstream::in);

		wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
		LogOpen(szFileBase, L"");
		PROCPGNGA procpgngaSav(ga, new PROCPGNTESTUNDO(ga));
		ERR err;
		try {
			ISTKPGN istkpgn(is);
			for (int igame = 0; ; igame++) {
				LogTemp(wstring(L"Game ") + to_wstring(igame + 1));
				if ((err = ga.DeserializeGame(istkpgn)) != ERR::None)
					break;
				UndoFullGame();
				ValidateFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
			}
			LogClose(szFileBase, L"Passed", LGF::Normal);
		}
		catch (exception& ex) {
			Error(ex.what());
			LogClose(szFileBase, L"Failed", LGF::Normal);
		}
		return err;
	}

	void UndoFullGame(void)
	{
		while (ga.bdg.imvCur >= 0) {
			BDG bdgInit = ga.bdg;
			ga.UndoMv(SPMV::Hidden);
			ga.RedoMv(SPMV::Hidden);
			assert(ga.bdg == bdgInit);
			if (ga.bdg != bdgInit)
				throw EXFAILTEST();
			ga.UndoMv(SPMV::Hidden);
		}
	}
};


ERR PROCPGNTESTUNDO::ProcessTag(int tkpgn, const string& szValue)
{
	return PROCPGNTEST::ProcessTag(tkpgn, szValue);
}


ERR PROCPGNTESTUNDO::ProcessMv(MV mv)
{
	BDG bdgInit = ga.bdg;
	ga.MakeMv(mv, SPMV::Hidden);
	BDG bdgNew = ga.bdg;
	ga.UndoMv(SPMV::Hidden);
	if (bdgInit != ga.bdg)
		throw EXFAILTEST();
	ga.RedoMv(SPMV::Hidden);
	if (bdgNew != ga.bdg)
		throw EXFAILTEST();
	return ERR::None;
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
	TESTPGNS(GA& ga, TEST* ptestParent, const wstring& szSub) : TEST(ga, ptestParent), szSub(szSub) { }
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
				PlayPGNFile(szSpec.c_str());
			}
			catch (exception& ex) {
				break;
			}
		} while (FindNextFile(hfind, &ffd) != 0);
		FindClose(hfind);
	}

	ERR PlayPGNFile(const wchar_t szFile[])
	{
		ifstream is(szFile, ifstream::in);

		wstring szFileBase(wcsrchr(szFile, L'\\') + 1);
		LogOpen(szFileBase, L"");
		PROCPGNGA procpgngaSav(ga, new PROCPGNTEST(ga));
		ERR err;
		try {
			ISTKPGN istkpgn(is);
			istkpgn.SetSzStream(szFileBase);
			int igame = 0;
			do {
				LogTemp(wstring(L"Game ") + to_wstring(++igame));
				err = ga.DeserializeGame(istkpgn);
			} while (err == ERR::None);
			LogClose(szFileBase, L"Passed", LGF::Normal);
		}
		catch (EXPARSE& ex) {
			Error(ex.what());
			LogClose(szFileBase, L"Failed", LGF::Bold);
			throw ex;
		}
		catch (exception& ex) {
			Error(ex.what());
			LogClose(szFileBase, L"Failed", LGF::Bold);
			throw ex;
		}
		return err;
	}
};


ERR PROCPGNTEST::ProcessTag(int tkpgn, const string& szValue)
{
	return PROCPGNOPEN::ProcessTag(tkpgn, szValue);
}


ERR PROCPGNTEST::ProcessMv(MV mv)
{
	return PROCPGNOPEN::ProcessMv(mv);
}


/*	GA::Test
 *
 *	This is the top-level test script.
 */
void GA::Test(void)
{
	TEST testRoot(*this, nullptr);
	testRoot.Add(new TESTNEWGAME(*this, &testRoot));
	//testRoot.Add(new TESTPERFT(*this, &testRoot));
	//testRoot.Add(new TESTUNDO(*this, &testRoot));
	testRoot.Add(new TESTPGNS(*this, &testRoot, L"Players"));

	InitLog(3);
	testRoot.RunAll();
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
	bdg.GenGmv(gmv, RMCHK::Remove);
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
	bdg.GenGmv(gmv, RMCHK::Remove);
	if (depthPerft == 1)
		return gmv.cmv();
	uint64_t cmv = 0;
	//BDG bdgInit = bdg;
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

struct {
	const wchar_t* szTitle;
	const wchar_t* szFEN;
	unsigned long long rgull[20];
	int cull;
} rgperfttest[] = {
	/*
	 *	perft test suite from algerbrex
	 * 
	 *	https://github.com/algerbrex/blunder/blob/main/tests/perftsuite.txt
	 */
	{L"Mini White King-Side Castle", L"4k3/8/8/8/8/8/8/4K2R w K - 0 1", {1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL}, 6},
	{L"Mini White Queen-Side Castle", L"4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", {1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL}, 6},
	{L"Mini Black King-Side Castle", L"4k2r/8/8/8/8/8/8/4K3 w k - 0 1", {1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL}, 6},
	{L"Mini Black Queen-Side Castle", L"r3k3/8/8/8/8/8/8/4K3 w q - 0 1", {1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL}, 6},
	{L"Perftsuite 7", L"4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", {1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL}, 6},
	{L"Perftsuite 8", L"r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", {1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL}, 6},
	{L"Perftsuite 9", L"8/8/8/8/8/8/6k1/4K2R w K - 0 1", {1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL}, 6},
	{L"Perftsuite 10", L"8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", {1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL}, 6},
	{L"Perftsuite 11", L"4k2r/6K1/8/8/8/8/8/8 w k - 0 1", {1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL}, 6},
	{L"Perftsuite 12", L"r3k3/1K6/8/8/8/8/8/8 w q - 0 1", {1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724}, 6 },
	{L"Perftsuite 13", L"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL}, 6 },
	{L"Perftsuite 14", L"r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL}, 6 },
	{L"Perftsuite 15", L"r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL}, 6 },
	{L"Perftsuite 16", L"r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL}, 6 },
	{L"Perftsuite 17", L"1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL}, 6 },
	{L"Perftsuite 18", L"2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL}, 6 },
	{L"Perftsuite 19", L"r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL}, 6 },
	{L"Perftsuite 20", L"4k3/8/8/8/8/8/8/4K2R b K - 0 1", { 1ULL, 5ULL, 75ULL, 459ULL, 8290ULL, 47635ULL, 899442ULL}, 6 }, 
	{L"Perftsuite 21", L"4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", { 1ULL, 5ULL, 80ULL, 493ULL, 8897ULL, 52710ULL, 1001523ULL}, 6},
	{L"Perftsuite 22", L"4k2r/8/8/8/8/8/8/4K3 b k - 0 1", { 1ULL, 15ULL, 66ULL, 1197ULL, 7059ULL, 133987ULL, 764643ULL}, 6},
	{L"Perftsuite 23", L"r3k3/8/8/8/8/8/8/4K3 b q - 0 1", { 1ULL, 16ULL, 71ULL, 1287ULL, 7626ULL, 145232ULL, 846648ULL}, 6},
	{L"Perftsuite 24", L"4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", { 1ULL, 5ULL, 130ULL, 782ULL, 22180ULL, 118882ULL, 3517770ULL}, 6},
	{L"Perftsuite 25", L"r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", { 1ULL, 26ULL, 112ULL, 3189ULL, 17945ULL, 532933ULL, 2788982ULL}, 6},
	{L"Perftsuite 26", L"8/8/8/8/8/8/6k1/4K2R b K - 0 1", { 1ULL, 3ULL, 32ULL, 134ULL, 2073ULL, 10485ULL, 179869ULL}, 6},
	{L"Perftsuite 27", L"8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", { 1ULL, 4ULL, 49ULL, 243ULL, 3991ULL, 20780ULL, 367724ULL}, 6},
	{L"Perftsuite 28", L"4k2r/6K1/8/8/8/8/8/8 b k - 0 1", { 1ULL, 12ULL, 38ULL, 564ULL, 2219ULL, 37735ULL, 185867ULL}, 6},
	{L"Perftsuite 29", L"r3k3/1K6/8/8/8/8/8/8 b q - 0 1", { 1ULL, 15ULL, 65ULL, 1018ULL, 4573ULL, 80619ULL, 413018ULL}, 6},
	{L"Perftsuite 30", L"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", { 1ULL, 26ULL, 568ULL, 13744ULL, 314346ULL, 7594526ULL, 179862938ULL}, 6},
	{L"Perftsuite 31", L"r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", { 1ULL, 26ULL, 583ULL, 14252ULL, 334705ULL, 8198901ULL, 198328929ULL}, 6},
	{L"Perftsuite 32", L"r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", { 1ULL, 25ULL, 560ULL, 13592ULL, 317324ULL, 7710115ULL, 185959088ULL}, 6},
	{L"Perftsuite 33", L"r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", { 1ULL, 25ULL, 560ULL, 13607ULL, 320792ULL, 7848606ULL, 190755813ULL}, 6},
	{L"Perftsuite 34", L"1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", { 1ULL, 25ULL, 567ULL, 14095ULL, 328965ULL, 8153719ULL, 195629489ULL}, 6},
	{L"Perftsuite 35", L"2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", { 1ULL, 25ULL, 548ULL, 13502ULL, 312835ULL, 7736373ULL, 184411439ULL}, 6},
	{L"Perftsuite 36", L"r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", { 1ULL, 25ULL, 547ULL, 13579ULL, 316214ULL, 7878456ULL, 189224276ULL}, 6},
	{L"Perftsuite 37", L"8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", { 1ULL, 14ULL, 195ULL, 2760ULL, 38675ULL, 570726ULL, 8107539ULL}, 6},
	{L"Perftsuite 38", L"8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", { 1ULL, 11ULL, 156ULL, 1636ULL, 20534ULL, 223507ULL, 2594412ULL}, 6},
	{L"Perftsuite 39", L"8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", { 1ULL, 19ULL, 289ULL, 4442ULL, 73584ULL, 1198299ULL, 19870403ULL}, 6},
	{L"Perftsuite 40", L"K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL}, 6},
	{L"Perftsuite 41", L"k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL}, 6},
	{L"Perftsuite 42", L"8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", { 1ULL, 15ULL, 193ULL, 2816ULL, 40039ULL, 582642ULL, 8503277ULL}, 6},
	{L"Perftsuite 43", L"8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", { 1ULL, 16ULL, 180ULL, 2290ULL, 24640ULL, 288141ULL, 3147566ULL}, 6},
	{L"Perftsuite 44", L"8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", { 1ULL, 4ULL, 68ULL, 1118ULL, 16199ULL, 281190ULL, 4405103ULL}, 6},
	{L"Perftsuite 45", L"K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", { 1ULL, 17ULL, 54ULL, 835ULL, 5910ULL, 92250ULL, 688780ULL}, 6},
	{L"Perftsuite 46", L"k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", { 1ULL, 3ULL, 51ULL, 345ULL, 5301ULL, 38348ULL, 588695ULL}, 6},
	{L"Perftsuite 47", L"B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", { 1ULL, 17ULL, 278ULL, 4607ULL, 76778ULL, 1320507ULL, 22823890ULL}, 6},
	{L"Perftsuite 48", L"8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", { 1ULL, 21ULL, 316ULL, 5744ULL, 93338ULL, 1713368ULL, 28861171ULL}, 6},
	{L"Perftsuite 49", L"k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL}, 6},
	{L"Perftsuite 50", L"K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL}, 6},
	{L"Perftsuite 51", L"B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", { 1ULL, 6ULL, 106ULL, 1829ULL, 31151ULL, 530585ULL, 9250746ULL}, 6},
	{L"Perftsuite 52", L"8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", { 1ULL, 17ULL, 309ULL, 5133ULL, 93603ULL, 1591064ULL, 29027891ULL}, 6},
	{L"Perftsuite 53", L"k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", { 1ULL, 7ULL, 143ULL, 1416ULL, 31787ULL, 310862ULL, 7382896ULL}, 6},
	{L"Perftsuite 54", L"K7/b7/1b6/1b6/8/8/8/k6B b - -0 1", { 1ULL, 21ULL, 144ULL, 3242ULL, 32955ULL, 787524ULL, 7881673ULL}, 6},
	{L"Perftsuite 55", L"7k/RR6/8/8/8/8/rr6/7K w - - 0 1", { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL}, 6},
	{L"Perftsuite 56", L"R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", { 1ULL, 36ULL, 1027ULL, 29215ULL, 771461ULL, 20506480ULL, 525169084ULL}, 6},
	{L"Perftsuite 57", L"7k/RR6/8/8/8/8/rr6/7K b - - 0 1", { 1ULL, 19ULL, 275ULL, 5300ULL, 104342ULL, 2161211ULL, 44956585ULL}, 6},
	{L"Perftsuite 58", L"R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", { 1ULL, 36ULL, 1027ULL, 29227ULL, 771368ULL, 20521342ULL, 524966748ULL}, 6},
	{L"Perftsuite 59", L"6kq/8/8/8/8/8/8/7K w - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 60", L"6KQ/8/8/8/8/8/8/7k b - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 61", L"K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL}, 6},
	{L"Perftsuite 62", L"6qk/8/8/8/8/8/8/7K b - - 0 1", { 1ULL, 22ULL, 43ULL, 1015ULL, 4167ULL, 105749ULL, 419369ULL}, 6},
	{L"Perftsuite 63", L"6KQ/8/8/8/8/8/8/7k b - - 0 1", { 1ULL, 2ULL, 36ULL, 143ULL, 3637ULL, 14893ULL, 391507ULL}, 6},
	{L"Perftsuite 64", L"K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", { 1ULL, 6ULL, 35ULL, 495ULL, 8349ULL, 166741ULL, 3370175ULL}, 6},
	{L"Perftsuite 65", L"8/8/8/8/8/K7/P7/k7 w - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 66", L"8/8/8/8/8/7K/7P/7k w - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 67", L"K7/p7/k7/8/8/8/8/8 w - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 68", L"7K/7p/7k/8/8/8/8/8 w - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 69", L"8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", { 1ULL, 7ULL, 35ULL, 210ULL, 1091ULL, 7028ULL, 34834ULL}, 6},
	{L"Perftsuite 70", L"8/8/8/8/8/K7/P7/k7 b - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 71", L"8/8/8/8/8/7K/7P/7k b - - 0 1", { 1ULL, 1ULL, 3ULL, 12ULL, 80ULL, 342ULL, 2343ULL}, 6},
	{L"Perftsuite 72", L"K7/p7/k7/8/8/8/8/8 b - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 73", L"7K/7p/7k/8/8/8/8/8 b - - 0 1", { 1ULL, 3ULL, 7ULL, 43ULL, 199ULL, 1347ULL, 6249ULL}, 6},
	{L"Perftsuite 74", L"8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", { 1ULL, 5ULL, 35ULL, 182ULL, 1091ULL, 5408ULL, 34822ULL}, 6},
	{L"Perftsuite 75", L"8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL}, 6},
	{L"Perftsuite 76", L"4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", { 1ULL, 2ULL, 8ULL, 44ULL, 282ULL, 1814ULL, 11848ULL}, 6},
	{L"Perftsuite 77", L"8/8/7k/7p/7P/7K/8/8 w - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 78", L"8/8/k7/p7/P7/K7/8/8 w - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 79", L"8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL}, 6},
	{L"Perftsuite 80", L"8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL}, 6},
	{L"Perftsuite 81", L"8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL}, 6},
	{L"Perftsuite 82", L"k7/8/3p4/8/3P4/8/8/7K w - - 0 1", { 1ULL, 4ULL, 15ULL, 90ULL, 534ULL, 3450ULL, 20960ULL}, 6},
	{L"Perftsuite 83", L"8/8/7k/7p/7P/7K/8/8 b - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 84", L"8/8/k7/p7/P7/K7/8/8 b - - 0 1", { 1ULL, 3ULL, 9ULL, 57ULL, 360ULL, 1969ULL, 10724ULL}, 6},
	{L"Perftsuite 85", L"8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", { 1ULL, 5ULL, 25ULL, 180ULL, 1294ULL, 8296ULL, 53138ULL}, 6},
	{L"Perftsuite 86", L"8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", { 1ULL, 8ULL, 61ULL, 411ULL, 3213ULL, 21637ULL, 158065ULL}, 6},
	{L"Perftsuite 87", L"8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", { 1ULL, 8ULL, 61ULL, 483ULL, 3213ULL, 23599ULL, 157093ULL}, 6},
	{L"Perftsuite 88", L"k7/8/3p4/8/3P4/8/8/7K b - - 0 1", { 1ULL, 4ULL, 15ULL, 89ULL, 537ULL, 3309ULL, 21104ULL}, 6},
	{L"Perftsuite 89", L"7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", { 1ULL, 4ULL, 19ULL, 117ULL, 720ULL, 4661ULL, 32191ULL}, 6},
	{L"Perftsuite 90", L"7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", { 1ULL, 5ULL, 19ULL, 116ULL, 716ULL, 4786ULL, 30980ULL}, 6},
	{L"Perftsuite 91", L"k7/8/8/7p/6P1/8/8/K7 w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 92", L"k7/8/7p/8/8/6P1/8/K7 w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 93", L"k7/8/8/6p1/7P/8/8/K7 w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 94", L"k7/8/6p1/8/8/7P/8/K7 w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 95", L"k7/8/8/3p4/4p3/8/8/7K w - - 0 1", { 1ULL, 3ULL, 15ULL, 84ULL, 573ULL, 3013ULL, 22886ULL}, 6},
	{L"Perftsuite 96", L"k7/8/3p4/8/8/4P3/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL}, 6},
	{L"Perftsuite 97", L"7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 19ULL, 117ULL, 720ULL, 5014ULL, 32167ULL}, 6},
	{L"Perftsuite 98", L"7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", { 1ULL, 4ULL, 19ULL, 117ULL, 712ULL, 4658ULL, 30749ULL}, 6},
	{L"Perftsuite 99", L"k7/8/8/7p/6P1/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 100", L"k7/8/7p/8/8/6P1/8/K7 b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 101", L"k7/8/8/6p1/7P/8/8/K7 b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6},
	{L"Perftsuite 102", L"k7/8/6p1/8/8/7P/8/K7 b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6},
	{L"Perftsuite 103", L"k7/8/8/3p4/4p3/8/8/7K b - - 0 1", { 1ULL, 5ULL, 15ULL, 102ULL, 569ULL, 4337ULL, 22579ULL}, 6 },
	{L"Perftsuite 104", L"k7/8/3p4/8/8/4P3/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4271ULL, 28662ULL}, 6 },
	{L"Perftsuite 105", L"7k/8/8/p7/1P6/8/8/7K w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 106", L"7k/8/p7/8/8/1P6/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 107", L"7k/8/8/1p6/P7/8/8/7K w - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 108", L"7k/8/1p6/8/8/P7/8/7K w - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 109", L"k7/7p/8/8/8/8/6P1/K7 w - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 110", L"k7/6p1/8/8/8/8/7P/K7 w - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 111", L"3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL}, 6 },
	{L"Perftsuite 112", L"7k/8/8/p7/1P6/8/8/7K b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 113", L"7k/8/p7/8/8/1P6/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 114", L"7k/8/8/1p6/P7/8/8/7K b - - 0 1", { 1ULL, 5ULL, 22ULL, 139ULL, 877ULL, 6112ULL, 41874ULL}, 6 },
	{L"Perftsuite 115", L"7k/8/1p6/8/8/P7/8/7K b - - 0 1", { 1ULL, 4ULL, 16ULL, 101ULL, 637ULL, 4354ULL, 29679ULL}, 6 },
	{L"Perftsuite 116", L"k7/7p/8/8/8/8/6P1/K7 b - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 117", L"k7/6p1/8/8/8/8/7P/K7 b - - 0 1", { 1ULL, 5ULL, 25ULL, 161ULL, 1035ULL, 7574ULL, 55338ULL}, 6 },
	{L"Perftsuite 118", L"3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", { 1ULL, 7ULL, 49ULL, 378ULL, 2902ULL, 24122ULL, 199002ULL}, 6 },
	{L"Perftsuite 119", L"8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL}, 6 },
	{L"Perftsuite 120", L"n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL}, 6 },
	{L"Perftsuite 121", L"8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL}, 6 },
	{L"Perftsuite 122", L"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL}, 6 },
	{L"Perftsuite 123", L"8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", { 1ULL, 11ULL, 97ULL, 887ULL, 8048ULL, 90606ULL, 1030499ULL}, 6 },
	{L"Perftsuite 124", L"n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", { 1ULL, 24ULL, 421ULL, 7421ULL, 124608ULL, 2193768ULL, 37665329ULL}, 6 },
	{L"Perftsuite 125", L"8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", { 1ULL, 18ULL, 270ULL, 4699ULL, 79355ULL, 1533145ULL, 28859283ULL}, 6 },
	{L"Perftsuite 126", L"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", { 1ULL, 24ULL, 496ULL, 9483ULL, 182838ULL, 3605103ULL, 71179139ULL}, 6 },

	{ L"Perftsuite 127", L"8/8/1k6/8/2pP4/8/5BK1/8 b - d3 0 1", {1ULL, 0, 0, 0, 0, 0, 824064ULL}, 6 },
	{ L"Perftsuite 128", L"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", {1ULL, 0, 0, 0, 0, 0, 1440467ULL}, 6 },
	{ L"Perftsuite 129", L"8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1", {1ULL, 0, 0, 0, 0, 0, 1440467ULL}, 6 },
	{L"Perftsuite 130", L"5k2/8/8/8/8/8/8/4K2R w K - 0 1", {1ULL, 0, 0, 0, 0, 0, 661072ULL}, 6},
	{L"Perftsuite 131", L"4k2r/8/8/8/8/8/8/5K2 b k - 0 1", {1ULL, 0, 0, 0, 0, 0, 661072ULL}, 6},
	{L"Perftsuite 132", L"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", {1ULL, 0, 0, 0, 0, 0, 803711ULL}, 6},
	{L"Perftsuite 133", L"r3k3/8/8/8/8/8/8/3K4 b q - 0 1", {1ULL, 0, 0, 0, 0, 0, 803711ULL}, 6},
	{L"Perftsuite 134", L"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 1274206ULL}, 4},
	{L"Perftsuite 135", L"r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1", {1ULL, 0, 0, 0, 1274206ULL}, 4},
	{L"Perftsuite 136", L"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", {1ULL, 0, 0, 0, 1720476ULL}, 4},
	{L"Perftsuite 137", L"r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 1720476ULL}, 4},
	{L"Perftsuite 138", L"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 3821001ULL}, 6},
	{L"Perftsuite 139", L"3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 3821001ULL}, 6},
	{L"Perftsuite 140", L"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", {1ULL, 0, 0, 0, 0, 1004658ULL}, 5},
	{L"Perftsuite 141", L"5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 1004658ULL}, 5},
	{L"Perftsuite 142", L"4k3/1P6/8/8/8/8/K7/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 217342ULL}, 6},
	{L"Perftsuite 143", L"8/k7/8/8/8/8/1p6/4K3 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 217342ULL}, 6},
	{L"Perftsuite 144", L"8/P1k5/K7/8/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 92683ULL}, 6},
	{L"Perftsuite 145", L"8/8/8/8/8/k7/p1K5/8 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 92683ULL}, 6},
	{L"Perftsuite 146", L"K1k5/8/P7/8/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 2217ULL}, 6},
	{L"Perftsuite 147", L"8/8/8/8/8/p7/8/k1K5 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 2217ULL}, 6},
	{L"Perftsuite 148", L"8/k1P5/8/1K6/8/8/8/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 567584ULL}, 7},
	{L"Perftsuite 149", L"8/8/8/8/1k6/8/K1p5/8 b - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 567584ULL}, 7},
	{L"Perftsuite 150", L"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", {1ULL, 0, 0, 0, 23527ULL}, 4},
	{L"Perftsuite 151", L"8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 1", {1ULL, 0, 0, 0, 23527ULL}, 4 },
	{L"Perftsuite 152", L"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", {1ULL, 0, 0, 0, 0, 193690690ULL}, 5 },
	{L"Perftsuite 153", L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 11030083ULL}, 6 },
	{L"Perftsuite 154", L"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1ULL, 0, 0, 0, 0, 15833292ULL}, 5 },
	{L"Perftsuite 155", L"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 1", {1ULL, 0, 0, 53392ULL}, 3 },
	{L"Perftsuite 156", L"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 1", {1ULL, 0, 0, 0, 0, 164075551ULL}, 5 },
	{L"Perftsuite 157", L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1ULL, 0, 0, 0, 0, 0, 0, 178633661ULL}, 7 },
	{L"Perftsuite 158", L"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1ULL, 0, 0, 0, 0, 0, 706045033ULL}, 6 },
	{L"Perftsuite 159", L"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1ULL, 0, 0, 0, 0, 89941194ULL}, 5 },
	{L"Perftsuite 160", L"1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", {1ULL, 0, 0, 0, 0, 1063513ULL}, 5 },
	{L"Perftsuite 161", L"3k4/3p4/8/K1P4r/8/8/8/8 b - -0 1", {1ULL, 0, 0, 0, 0, 0, 1134888ULL}, 6 },
	{L"Perftsuite 162", L"8/8/4k3/8/2p5/8/B2P2K1/8 w - -0 1", {1ULL, 0, 0, 0, 0, 0, 1015133ULL}, 6 },

	/* perft tests from chessprogramming.org */

	{L"Initial", L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 
			{1ULL, 20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL, 84998978956ULL, 
		     2439530234167ULL, 69352859712417ULL, 2097651003696806ULL, 62854969236701747ULL, 1981066775000396239ULL}, 6},
	{ L"Kiwipete", L"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", { 1ULL, 48ULL, 2039ULL, 97862LL, 4085603ULL, 193690690ULL, 8031647685ULL }, 5 },
	{ L"Position 3", L"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", { 1ULL, 14ULL, 191ULL, 2812ULL, 43238ULL, 674624ULL, 11030083ULL, 178633661ULL, 3009794393ULL }, 7 },
	{ L"Position 4", L"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", { 1ULL, 6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL}, 6 },
	{ L"Position 5", L"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1ULL, 44ULL, 1486ULL, 62379ULL, 2103487ULL, 89941194ULL}, 5 },
	{ L"Position 6", L"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
			{ 1ULL, 46ULL, 2079ULL, 89890ULL, 3894594ULL, 164075551ULL, 6923051137ULL, 287188994746ULL, 11923589843526ULL, 
		      490154852788714ULL }, 5 }
};

void GA::PerftTest(void)
{
	for (int iperfttest = 0; iperfttest < CArray(rgperfttest); iperfttest++)
		RunPerftTest(rgperfttest[iperfttest].szTitle,
			rgperfttest[iperfttest].szFEN,
			rgperfttest[iperfttest].rgull,
			rgperfttest[iperfttest].cull, true);
}


/*	GA::RunPerftTest
 *
 *	Runs one particular perft test starting with the original board position (szFEN). 
 *	Cycles through each depth from 1 to depthLast, verifying move counts in mpdepthcmv. 
 *	The tag is just used for debug output.
 */
void GA::RunPerftTest(const wchar_t tag[], const wchar_t szFEN[], const uint64_t mpdepthcmv[], int depthLast, bool fDivide)
{
	LogOpen(tag, L"");
	InitGame(szFEN, SPMV::Hidden);
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
		LogOpen(L"Depth", to_wstring(depth));
		LogData(L"Expected: " + to_wstring(cmvExp));
		
		/* time the perft */
		
		time_point<high_resolution_clock> tpStart = high_resolution_clock::now();
		uint64_t cmvAct = fDivide ? CmvPerftDivide(depth) : CmvPerft(depth);
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
			LogClose(L"Depth", L"Failed", LGF::Normal);
			goto Done;
		}
		LogClose(L"Depth", L"Passed", LGF::Normal);

	}
	fPassed = true;

Done:
	LogClose(tag, fPassed ? L"Passed" : L"Failed", LGF::Normal);
}
