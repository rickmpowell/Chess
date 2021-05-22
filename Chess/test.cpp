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
	this->spmv = spmv;
	NewGame(new RULE(0, 0, 0));
	ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	UndoTest();
	PlayPGNFiles(L"..\\Chess\\Test");
	this->spmv = SPMV::Animate;
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


/*	GA::PlayPGNFile
 *
 *	Plays the games in the PGN file given by szFile
 */
int GA::PlayPGNFile(const WCHAR szFile[])
{
	ifstream is(szFile, ifstream::in);

	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			wstring szVal(wcsrchr(szFile, L'\\')+1);
			szVal += L"\nGame ";
			szVal += to_wstring(igame + 1);
			uiti.SetText(szVal);
			if (PlayPGNGame(istkpgn) != 1)
				break;
		}
	}
	catch (int err)
	{
		if (err == 1) {
			WCHAR sz[58];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(NULL, sz, L"PGN File Error", MB_OK);
		}
		return err;
	}
	return 0;
}


int GA::PlayPGNGame(ISTKPGN& istkpgn)
{
	NewGame(new RULE(0, 0, 0));
	if (!ReadPGNHeaders(istkpgn))
		return 0;
	ReadPGNMoveList(istkpgn);
	return 1;
}


int GA::ReadPGNHeaders(ISTKPGN& istkpgn)
{
	if (!ReadPGNTag(istkpgn))
		return 0;
	while (ReadPGNTag(istkpgn))
		;
	return 1;
}


int GA::ReadPGNTag(ISTKPGN& istkpgn)
{
	class TKS {
	public:
		TK* ptkStart, * ptkEnd;
		TK* ptkSym, * ptkVal;
		TKS(void) : ptkStart(NULL), ptkSym(NULL), ptkVal(NULL), ptkEnd(NULL) { }
		~TKS(void) {
			if (ptkStart) delete ptkStart;
			if (ptkSym) delete ptkSym;
			if (ptkVal) delete ptkVal;
			if (ptkEnd) delete ptkEnd;
		}
	} tks;

	istkpgn.WhiteSpace(false);
	for ( ; ; ) {
		tks.ptkStart = istkpgn.PtkNext();
		switch ((int)*tks.ptkStart) {
		case tkpgnBlankLine:
		case tkpgnEnd:
			return 0;	// blank line signifies end of tags
		case tkpgnLBracket:
			goto GotTag;
		default:
			throw istkpgn.line();
		}
	}
GotTag:
	/* read symbol, value, and end tag */
	tks.ptkSym = istkpgn.PtkNext();
	if ((int)*tks.ptkSym != tkpgnSymbol)
		throw istkpgn.line();
	tks.ptkVal = istkpgn.PtkNext();
	if ((int)*tks.ptkVal != tkpgnString)
		throw istkpgn.line();
	tks.ptkEnd = istkpgn.PtkNext();
	if ((int)*tks.ptkEnd != tkpgnRBracket)
		throw istkpgn.line();

	/* we have a well-formed tag */
	ProcessTag(tks.ptkSym->sz(), tks.ptkVal->sz());

	return 1;
}


/*	GA::ReadPGNMoveList
 *
 *	Reads the move list part of a single game from a PGN file. Yields control to
 *	other apps and lets the UI run. Returns 0 if the user pressed Esc to abort
 *	the PGN processing. Returns 1 if the move list was successfully processed.
 *	Can throw an exception on syntax errors in the PGN file
 */
int GA::ReadPGNMoveList(ISTKPGN& istkpgn)
{
	while (ReadPGNMove(istkpgn))
		PumpMsg();
	return 1;
}


bool GA::FIsMoveNumber(TK* ptk, int& w) const
{
	w = 0;
	const string& sz = ptk->sz();
	for (int ich = 0; sz[ich]; ich++) {
		if (!isdigit(sz[ich]))
			return false;
		w = w * 10 + sz[ich] - '0';
	}
	return true;
}


int GA::ReadPGNMove(ISTKPGN& istkpgn)
{
	TK* ptk;
	for (ptk = istkpgn.PtkNext(); ; ) {
		switch ((int)*ptk) {
		case tkpgnSymbol:
			int w;
			if (!FIsMoveNumber(ptk, w)) {
				ProcessMove(ptk->sz());
				delete ptk;
				return 1;
			}
			/* eat trailing periods */
			/* TODO: should communicate the move number back to the caller,
			 * and three dots signifies black is next to move
			 */
			do {
				delete ptk;
				ptk = istkpgn.PtkNext();
			} while ((int)*ptk == tkpgnPeriod);
			break;
		case tkpgnStar:
		case tkpgnBlankLine:
		case tkpgnEnd:
			delete ptk;
			return 0;
		default:
			istkpgn.UngetTk(ptk);
			return 0;
		}
	}
	return 1;
}


void GA::ProcessTag(const string& szTag, const string& szVal)
{
	struct {
		const char* sz;
		int tkpgn;
	} mpsztkpgn[] = {
		{"Event", tkpgnEvent},
		{"Site", tkpgnSite},
		{"Date", tkpgnDate},
		{"Round", tkpgnRound},
		{"White", tkpgnWhite},
		{"Black", tkpgnBlack},
		{"Result", tkpgnResult}
	};

	for (int isz = 0; isz < CArray(mpsztkpgn); isz++) {
		if (szTag == mpsztkpgn[isz].sz) {
			HandleTag(mpsztkpgn[isz].tkpgn, szVal);
			break;
		}
	}
}

void GA::HandleTag(int tkpgn, const string& szVal)
{
	switch (tkpgn) {
	case tkpgnWhite:
	case tkpgnBlack:
	{
		wstring wszVal(szVal.begin(), szVal.end());
		mpcpcppl[tkpgn == tkpgnBlack ? CPC::Black : CPC::White]->SetName(wszVal);
		uiti.Redraw();
		break;
	}
	case tkpgnEvent:
	case tkpgnSite:
	case tkpgnDate:
	case tkpgnRound:
	case tkpgnResult:
	default:
		break;
	}
}


void GA::ProcessMove(const string& szMove)
{
	MV mv;
	const char* pch = szMove.c_str();
	if (bdg.ParseMv(pch, mv) != 1)
		return;
	MakeMv(mv, spmv);
}


void GA::UndoTest(void)
{
	WIN32_FIND_DATA ffd;
	wstring szSpec(L"..\\Chess\\Test\\*.pgn");
	HANDLE hfind = FindFirstFile(szSpec.c_str(), &ffd);
	if (hfind == INVALID_HANDLE_VALUE)
		throw 1;
	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		wstring szSpec = wstring(L"..\\Chess\\Test\\") + ffd.cFileName;
		if (PlayUndoPGNFile(szSpec.c_str()) < 0)
			break;
		break;
	} while (FindNextFile(hfind, &ffd) != 0);
	FindClose(hfind);
}


int GA::PlayUndoPGNFile(const WCHAR* szFile)
{
	ifstream is(szFile, ifstream::in);

	try {
		ISTKPGN istkpgn(is);
		for (int igame = 0; ; igame++) {
			wstring szVal(wcsrchr(szFile, L'\\') + 1);
			szVal += L"\nGame ";
			szVal += to_wstring(igame + 1);
			uiti.SetText(szVal);
			if (PlayPGNGame(istkpgn) != 1)
				break;
			UndoFullGame();
			ValidateFEN(L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
	}
	catch (int err) {
		if (err == 1) {
			WCHAR sz[58];
			::wsprintf(sz, L"Error Line %d", err);
			::MessageBox(NULL, sz, L"PGN File Error", MB_OK);
		}
		return err;
	}
	return 0;
}


void GA::UndoFullGame(void)
{
	while (bdg.imvCur >= 0)
		UndoMv();
}