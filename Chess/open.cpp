/*
 *
 *	open.cpp
 * 
 *	Open files, parsing PGN files, some semi-general stream tokenization
 * 
 */

#include "ga.h"



EXPARSE::EXPARSE(const string& szMsg) : EX(szMsg)
{
}

EXPARSE::EXPARSE(const string& szMsg, ISTKPGN& istkpgn) : EX()
{
	string sz = format("{0}({1}) : error : {2}", SzFlattenWsz(istkpgn.SzStream()), istkpgn.line(), szMsg);
	set_what(sz);
}

EXPARSE::EXPARSE(const string& szMsg, const wchar_t* szFile, int line) : EX()
{
	char sz[1024];
	snprintf(sz, CArray(sz), szMsg.c_str(), SzFlattenWsz(szFile).c_str(), line);
	set_what(sz);
}


void GA::OpenPGNFile(const wchar_t szFile[])
{
	ifstream is(szFile, ifstream::in);
	ISTKPGN istkpgn(is);
	istkpgn.SetSzStream(szFile);
	PROCPGNGA procpgngaSav(*this, new PROCPGNOPEN(*this));
	Deserialize(istkpgn);
	puiga->uiml.UpdateContSize();
	puiga->uiml.SetSel(bdg.vmveGame.size() - 1, spmvHidden);
	puiga->uiml.FMakeVis(bdg.imveCurLast);
	puiga->Redraw();
}


/*	GA::FIsPgnData
 *
 *	Returns true if the beginning looks like a PGN file. As opposed to 
 *	a FEN string
 */
bool GA::FIsPgnData(const char* pch) const
{
	while (*pch == ' ' || *pch == '\n' || *pch == '\r')
		pch++;
	return *pch == '[';
}


/*	GA::Deserialize
 *
 *	Deserializes the stream as a PGN file. Throws an exception on parse errors
 *	or file errors. Returns 0 on success, 1 if we reached EOF.
 */
ERR GA::Deserialize(ISTKPGN& istkpgn)
{
	InitGame(nullptr, nullptr);
	try {
		ERR err = DeserializeHeaders(istkpgn);
		if (err != errNone)
			return err;
		DeserializeMoveList(istkpgn);
	}
	catch (...) {
		InitGame(nullptr, nullptr);
		throw;
	}
	return errNone;
}


/*	GA::DeserializeGame
 *
 *	Deserializes a single game from a PGN token stream. Return 0 on success,
 *	an error at EOF, or throws an exception on parse and file errors. 
 * 
 *	On errorsr, we attempt to position the stream pointer after the end of the
 *	game move list, so that additional games can still be deserialized.
 */
ERR GA::DeserializeGame(ISTKPGN& istkpgn)
{
	InitGame(nullptr, nullptr);
	istkpgn.SetImvCur(0);
	ERR err;
	try {
		err = DeserializeHeaders(istkpgn);
		if (err != errNone)
			return err;
	}
	catch (EXPARSE& ex) {
		(void)ex;
		istkpgn.ScanToBlankLine();
		istkpgn.ScanToBlankLine();
		throw;
	}
	try {
		err = DeserializeMoveList(istkpgn);
	}
	catch (EXPARSE& ex) {
		(void)ex;
		istkpgn.ScanToBlankLine();
		throw;
	}
	return err;
}


ERR GA::DeserializeHeaders(ISTKPGN& istkpgn)
{
	ERR err;
	if ((err = DeserializeTag(istkpgn)) != errNone)
		return err;
	pprocpgn->ProcessTag(tkpgnTagsStart, "");
	while (DeserializeTag(istkpgn) != errEndOfFile)
		;
	assert(pprocpgn);
	pprocpgn->ProcessTag(tkpgnTagsEnd, "");
	return errNone;
}


/*	GA::DeserializeTag
 *
 *	Deserializes a single tag from the PGN file header section. Returns 0 on success,
 *	returns errEndOfFile if end of tag section.
 */
ERR GA::DeserializeTag(ISTKPGN& istkpgn)
{
	class TKS {
	public:
		TK* ptkStart, * ptkEnd;
		TK* ptkSym, * ptkVal;
		TKS(void) : ptkStart(nullptr), ptkEnd(nullptr), ptkSym(nullptr), ptkVal(nullptr) { }
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
			return errEndOfFile;	// blank line signifies end of tags
		case tkpgnLBracket:
			goto GotTag;
		default:
			throw EXPARSE("Expecting open bracket in tag list", istkpgn);
		}
	}
GotTag:
	/* read symbol, value, and end tag */
	tks.ptkSym = istkpgn.PtkNext();
	if ((int)*tks.ptkSym != tkpgnSymbol)
		throw EXPARSE("Expecting symbol", istkpgn);
	tks.ptkVal = istkpgn.PtkNext();
	if ((int)*tks.ptkVal != tkpgnString)
		throw EXPARSE("Expecting string value", istkpgn);
	tks.ptkEnd = istkpgn.PtkNext();
	if ((int)*tks.ptkEnd != tkpgnRBracket)
		throw EXPARSE("Expecting close bracket", istkpgn);

	/* we have a well-formed tag */
	ProcessTag(tks.ptkSym->sz(), tks.ptkVal->sz());

	return errNone;
}


/*	GA::DeserializeMoveList
 *
 *	Reads the move list part of a single game from a PGN file. Yields control to
 *	other apps and lets the UI run. Returns 0 if the user pressed Esc to abort
 *	the PGN processing. Returns 1 if the move list was successfully processed.
 *	Can throw an exception on syntax errors in the PGN file
 */
ERR GA::DeserializeMoveList(ISTKPGN& istkpgn)
{
	while (DeserializeMove(istkpgn) == errNone) {
		puiga->PumpMsg();
	}
	return errNone;
}


ERR GA::DeserializeMove(ISTKPGN& istkpgn)
{
	TK* ptk;
	for (ptk = istkpgn.PtkNext(); ; ) {
		switch ((int)*ptk) {
		case tkpgnSymbol:
			int imv;
			if (!FIsMoveNumber(ptk, imv)) {
				try {
					ParseAndProcessMove(ptk->sz());
				} 
				catch (EXPARSE& ex) {
					delete ptk;
					throw EXPARSE(ex.what(), istkpgn);
				}
				delete ptk;
				return errNone;
			}
			else {
				/* eat all the consecutive trailing periods after a move number */
				/* TODO: should communicate the move number back to the caller,
				 * and three dots signifies black is next to move, although I 
				 * think the triple-dot thing should only be legal as the first 
				 * move in the move list.
				 */
				int cchPeriod;
				for (cchPeriod = 0; ; cchPeriod++) {
					delete ptk;
					ptk = istkpgn.PtkNext();
					if ((int)*ptk != tkpgnPeriod)
						break;
				}
				istkpgn.SetImvCur(2 * (imv - 1) + (cchPeriod >= 3 ? 1 : 0));
			}
			break;
		case tkpgnStar:
		case tkpgnBlankLine:
		case tkpgnEnd:
			delete ptk;
			return errEndOfFile;
		default:
			istkpgn.UngetTk(ptk);
			return errEndOfFile;
		}
	}
	return errNone;
}


/*	GA::ProcessTag
 *
 *	Processes the tag/value pair in the PGN file stream. Performs the actual
 *	side effects of the tag.
 */
ERR GA::ProcessTag(const string& szTag, const string& szVal)
{
	assert(pprocpgn);

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
			pprocpgn->ProcessTag(mpsztkpgn[isz].tkpgn, szVal);
			break;
		}
	}
	return errNone;
}


/*	GA::ParseAndProcessMove
 *
 *	Parses and performs the move that has been extracted from the PGN stream.
 * 
 *	Returns an ERR for non-fatal errors (like eof), and throws exceptions on
 *	hard errors.
 */
ERR GA::ParseAndProcessMove(const string& szMove)
{
	assert(pprocpgn);

	MVE mve;
	const char* pch = szMove.c_str();
	if (bdg.ParseMve(pch, mve) == errEndOfFile)
		return errEndOfFile;
	pprocpgn->ProcessMv(mve);
	return errNone;
}


/*	GA:::FIsMoveNumber
 *
 *	Returns true if the token from the PGN token stream is a move number, 
 *	which is returned in w.
 */
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


/*	GA::SetProcpgn
 *
 *	Sets the PGN handler in the game.
 */
void GA::SetProcpgn(PROCPGN* pprocpgn)
{
	if (this->pprocpgn) {
		delete this->pprocpgn;
		this->pprocpgn = nullptr;
	}
	this->pprocpgn = pprocpgn;
}


/*
 *
 *	PROCPGN class
 * 
 *	Processes PGN files. This is the base processing class, which is used
 *	during file/open. 
 * 
 */


PROCPGN::PROCPGN(GA& ga) : ga(ga)
{
}


ERR PROCPGNOPEN::ProcessMv(MVE mve)
{
	ga.bdg.MakeMv(mve);
	return errNone;
}


ERR PROCPGNOPEN::ProcessTag(int tkpgn, const string& szValue)
{
	switch (tkpgn) {
	case tkpgnWhite:
	case tkpgnBlack:
	{
		wstring wszVal(szValue.begin(), szValue.end());
		ga.mpcpcppl[tkpgn == tkpgnBlack ? cpcBlack : cpcWhite]->SetName(wszVal);
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
	return errNone;
}


/*	to_string(TKMV)
 *
 *	Creates a string representation of a token type
 */
string to_string(TKMV tkmv)
{
	switch (tkmv) {
	case tkmvError: return "<Error>";
	case tkmvEnd: return "<End>";
	case tkmvKing: return "K";
	case tkmvQueen: return "Q";
	case tkmvRook: return "R";
	case tkmvBishop: return "B";
	case tkmvKnight: return "N";
	case tkmvPawn: return "P";
	case tkmvSquare: return "<Square>";
	case tkmvFile: return "<File>";
	case tkmvRank: return "<Rank>";
	case tkmvTake: return "x";
	case tkmvTo: return "-";
	case tkmvPromote: return "=";
	case tkmvCheck: return "+";
	case tkmvMate: return "#";
	case tkmvEnPassant: return "e.p.";
	case tkmvCastleKing: return "O-O";
	case tkmvCastleQueen: return "O-O-O";
	case tkmvWhiteWins: return "1-0";
	case tkmvBlackWins: return "0-1";
	case tkmvDraw: return "1/2-1/2";
	case tkmvInProgress: return "*";
	default:
		break;
	}
	return "<no token>";
}


/*
 *
 *	BDG move parsing
 *
 */


/*	BDG::ParseMvu
 *
 *	Parses an algebraic move for the current board state. Returns 0 for
 *	a successful move, 1 for end game, and throws an EXPARSE exception on
 *	a parse error. 
 *	
 *	Note that the EXPARSE exception does not a build a full file/line error 
 *	message, because this parse code can be used for non-file related parsing.
 *	File loading code needs to intercept this exception and add more information
 *	to make the error message complete.
 */
ERR BDG::ParseMve(const char*& pch, MVE& mve)
{
	VMVE vmve;
	GenMoves(vmve, ggLegal);

	const char* pchInit = pch;
	int rank, file;
	SQ sq1;
	TKMV tkmv = TkmvScan(pch, sq1);

	switch (tkmv) {
	case tkmvKing:
	case tkmvQueen:
	case tkmvRook:
	case tkmvBishop:
	case tkmvKnight:
	case tkmvPawn:
		return ParsePieceMve(vmve, tkmv, pchInit, pch, mve);
	case tkmvSquare:
		return ParseSquareMve(vmve, sq1, pchInit, pch, mve);
	case tkmvFile:
		return ParseFileMve(vmve, sq1, pchInit, pch, mve);
	case tkmvRank:
		return ParseRankMve(vmve, sq1, pchInit, pch, mve);
	case tkmvCastleKing:
		file = fileKingKnight;
		goto BuildCastle;
	case tkmvCastleQueen:
		file = fileQueenBishop;
BuildCastle:
		rank = RankBackFromCpc(cpcToMove);
		mve = MVE(SQ(rank, fileKing), SQ(rank, file), PC(cpcToMove, apcKing));
		return errNone;
	case tkmvWhiteWins:
	case tkmvBlackWins:
	case tkmvDraw:
	case tkmvInProgress:
		return errEndOfFile;
	case tkmvEnd:
		return errEndOfFile;	// the move symbol is blank - can this happen?
	default:
		/* all other tokens are illegal */
		throw EXPARSE(format("Illegal token {}", to_string(tkmv)));
	}
}


APC ApcFromTkmv(TKMV tkmv)
{
	switch (tkmv) {
	case tkmvPawn: return apcPawn;
	case tkmvKnight: return apcKnight;
	case tkmvBishop: return apcBishop;
	case tkmvRook: return apcRook;
	case tkmvQueen: return apcQueen;
	case tkmvKing: return apcKing;
	default:
		break;
	}
	assert(false);
	return apcError;
}


ERR BDG::ParsePieceMve(const VMVE& vmve, TKMV tkmv, const char* pchInit, const char*& pch, MVE& mve) const
{
	SQ sq;
	APC apc = ApcFromTkmv(tkmv);
	tkmv = TkmvScan(pch, sq);

	switch (tkmv) {
	case tkmvTo:	/* B-c5 or Bxc5 */
	case tkmvTake:
		if (TkmvScan(pch, sq) != tkmvSquare)
			throw EXPARSE(format("Missing destination square (got {})", to_string(tkmv)));
		mve = MveMatchPieceTo(vmve, apc, -1, -1, sq, pchInit, pch);
		break;

	case tkmvSquare:	/* Bc5, Bd3c5, Bd3-c5, Bd3xc5 */
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == tkmvTo || tkmv == tkmvTake) { /* eat the to/take */
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != tkmvSquare) { /* Bc5 */
			/* look up piece that can move to this square */
			mve = MveMatchPieceTo(vmve, apc, -1, -1, sq, pchInit, pch);
			break;
		}
		/* Bd5c4 */
		pch = pchNext;
		mve = MveMatchFromTo(vmve, sq, sqTo, pchInit, pch);
		break;
	}

	case tkmvFile: /* Bdc5 or Bd-c5 */
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == tkmvTo || tkmv == tkmvTake) {
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != tkmvSquare)
			throw EXPARSE(format("Expected destination square (got {})", to_string(tkmv)));
		pch = pchNext;
		mve = MveMatchPieceTo(vmve, apc, -1, sq.file(), sqTo, pchInit, pch);
		break;
	}

	case tkmvRank:
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == tkmvTo || tkmv == tkmvTake) {
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != tkmvSquare)
			throw EXPARSE(format("Expected a square (got {})", to_string(tkmv)));
		pch = pchNext;
		mve = MveMatchPieceTo(vmve, apc, sq.rank(), -1, sqTo, pchInit, pch);
		break;
	}

	default:
		throw EXPARSE(format("Invalid token {}", to_string(tkmv)));
	}
	return ParseMveSuffixes(mve, pch);
}


ERR BDG::ParseSquareMve(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVE& mve) const
{
	/* e4, e4e5, e4-e5, e4xe5 */
	const char* pchNext = pch;
	SQ sqTo;

	TKMV tkmv = TkmvScan(pchNext, sq);
	if (tkmv == tkmvTo || tkmv == tkmvTake) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sq);
	}
	if (TkmvScan(pchNext, sq) == tkmvSquare) {
		pch = pchNext;
		mve = MveMatchFromTo(vmve, sq, sqTo, pchInit, pch);
	}
	else { /* e4: plain square is a pawn move */
		mve = MveMatchPieceTo(vmve, apcPawn, -1, -1, sq, pchInit, pch);
	}
	return ParseMveSuffixes(mve, pch);
}


ERR BDG::ParseMveSuffixes(MVE& mve, const char*& pch) const
{
	const char* pchNext = pch;
	for (;;) {
		SQ sqT;
		switch (TkmvScan(pchNext, sqT)) {
		case tkmvCheck:
		case tkmvMate:
		case tkmvEnPassant:
			pch = pchNext;
			break;
		case tkmvPromote:
		{
			pch = pchNext;
			TKMV tkmv = TkmvScan(pchNext, sqT);
			APC apc = ApcFromTkmv(tkmv);
			if (apc == apcNull || apc == apcPawn || apc == apcKing)
				throw EXPARSE("Not a valid promotion");
			pch = pchNext;
			mve.SetApcPromote(apc);
			break;
		}
		case tkmvEnd:
			return errNone;
		case tkmvWhiteWins:
		case tkmvBlackWins:
		case tkmvDraw:
		case tkmvInProgress:
			return errNone;
		default:
			throw EXPARSE("Not a valid move suffix");
		}
	}

	return errNone;
}


ERR BDG::ParseFileMve(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVE& mve) const
{
	/* de4, d-e4, dxe4 */
	const char* pchNext = pch;
	SQ sqTo;
	TKMV tkmv = TkmvScan(pchNext, sqTo);
	if (tkmv == tkmvTo || tkmv == tkmvTake) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sqTo);
	}
	if (tkmv != tkmvSquare)
		throw EXPARSE(format("Expected a destination square (got {})", to_string(tkmv)));
	pch = pchNext;
	mve = MveMatchPieceTo(vmve, apcPawn, -1, sq.file(), sqTo, pchInit, pch);
	return ParseMveSuffixes(mve, pch);
}


ERR BDG::ParseRankMve(const VMVE& vmve, SQ sq, const char* pchInit, const char*& pch, MVE& mve) const
{
	/* 7e4, 7-e4, 7xe4 */
	const char* pchNext = pch;
	SQ sqTo;
	TKMV tkmv = TkmvScan(pchNext, sqTo);
	if (tkmv == tkmvTo || tkmv == tkmvTake) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sqTo);
	}
	if (tkmv != tkmvSquare)
		throw EXPARSE(format("Expected a destination square (got {})", to_string(tkmv)));
	pch = pchNext;
	mve = MveMatchPieceTo(vmve, apcPawn, sq.rank(), -1, sqTo, pchInit, pch);
	return ParseMveSuffixes(mve, pch);
}


MVE BDG::MveMatchPieceTo(const VMVE& vmve, APC apc, int rankFrom, int fileFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const
{
	for (const MVE& mveSearch : vmve) {
		if (mveSearch.sqTo() == sqTo && mveSearch.apcMove() == apc) {
			if (fileFrom != -1 && fileFrom != mveSearch.sqFrom().file())
				continue;
			if (rankFrom != -1 && rankFrom != mveSearch.sqFrom().rank())
				continue;
			return mveSearch;
		}
	}
	throw EXPARSE(format("{} is not a legal move", string(pchFirst, pchLim - pchFirst)));
}

MVE BDG::MveMatchFromTo(const VMVE& vmve, SQ sqFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const
{
	for (const MVE& mveSearch : vmve) {
		if (mveSearch.sqFrom() == sqFrom && mveSearch.sqTo() == sqTo)
			return mveSearch;
	}
	throw EXPARSE(format("{} is not a legal move", string(pchFirst, pchLim-pchFirst)));
}


TKMV BDG::TkmvScan(const char*& pch, SQ& sq) const
{
	int file;

	if (!*pch)
		return tkmvEnd;

	struct {
		const char* szKeyWord;
		TKMV tkmv;
	} mpsztkmv[] = {
		{"0-0-0", tkmvCastleQueen},
		{"O-O-O", tkmvCastleQueen},
		{"0-0", tkmvCastleKing},
		{"O-O", tkmvCastleKing},
		{"e.p.", tkmvEnPassant},
		{"1-0", tkmvWhiteWins},
		{"0-1", tkmvBlackWins},
		{"1/2-1/2", tkmvDraw},
	};
	for (int isz = 0; isz < CArray(mpsztkmv); isz++) {
		int ich;
		for (ich = 0; mpsztkmv[isz].szKeyWord[ich]; ich++) {
			if (pch[ich] != mpsztkmv[isz].szKeyWord[ich])
				goto NextKeyWord;
		}
		pch += ich;
		return mpsztkmv[isz].tkmv;
NextKeyWord: ;
	}

	char ch = chNull;
	switch (ch = *pch++) {
	case 'P': return tkmvPawn;
	case 'B': return tkmvBishop;
	case 'N': return tkmvKnight;
	case 'R': return tkmvRook;
	case 'Q': return tkmvQueen;
	case 'K': return tkmvKing;
	case 'x': return tkmvTake;
	case '-': return tkmvTo;
	case '+': return tkmvCheck;
	case '#': return tkmvMate;
	case '*': return tkmvInProgress;
	case '=': return tkmvPromote;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
		file = ch - 'a';
		if (in_range(*pch, '1', '8')) {
			sq = SQ(*pch++ - '1', file);
			return tkmvSquare;
		}
		sq = SQ(0, file);
		return tkmvFile;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
		sq = SQ(ch - '1', 0);
		return tkmvRank;
	default:
		return tkmvError;
	}
}


/*
 *
 *	ISTKPGN
 * 
 *	Input stream PGN file tokenizer
 * 
 */


ISTKPGN::ISTKPGN(istream& is) : ISTK(is), fWhiteSpace(false), imvCur(0)
{
}


void ISTKPGN::WhiteSpace(bool fReturn)
{
	fWhiteSpace = fReturn;
}


bool ISTKPGN::FIsSymbol(char ch, bool fFirst) const
{
	if (in_range(ch, 'a', 'z') || in_range(ch, 'A', 'Z') || in_range(ch, '0', '9'))
		return true;
	if (fFirst)
		return false;
	return ch == '_' || ch == '+' || ch == '#' || ch == '=' || ch == ':' || ch == '-' || ch == '/';
}


char ISTKPGN::ChSkipWhite(void)
{
	char ch;
	do
		ch = ChNext();
	while (isblank(ch));
	return ch;
}


TK* ISTKPGN::PtkNext(void)
{
	if (ptkPrev) {
		TK* ptk = ptkPrev;
		ptkPrev = nullptr;
		return ptk;
	}

Retry:
	char ch = ChNext();

	if (isblank(ch)) {
		ch = ChSkipWhite();
		if (fWhiteSpace) {
			UngetCh(ch);
			return new TK(tkpgnWhiteSpace);
		}
	}
	if (ch == '\n') {
		ch = ChSkipWhite();
		if (ch == '\n') {
			do ch = ChSkipWhite(); while (ch == '\n');
			if (ch != '\0')
				UngetCh(ch);
			return new TK(tkpgnBlankLine);
		}
		if (fWhiteSpace) {
			UngetCh(ch);
			return new TK(tkpgnWhiteSpace);
		}
	}

	switch (ch) {

	case '\0':
		return new TK(tkpgnEnd);
	case '.':
		return new TK(tkpgnPeriod);
	case '*':
		return new TK(tkpgnStar);
	case '#':
		return new TK(tkpgnPound);
	case '(':
		return new TK(tkpgnLParen);
	case ')':
		return new TK(tkpgnRParen);
	case '[':
		return new TK(tkpgnLBracket);
	case ']':
		return new TK(tkpgnRBracket);
	case '<':
		return new TK(tkpgnLAngleBracket);
	case '>':
		return new TK(tkpgnRAngleBracket);

	case ';':
		/* single line comments */
		do
			ch = ChNext();
		while (ch && ch != '\n');
		if (!fWhiteSpace)
			goto Retry;
		return new TK(tkpgnLineComment);

	case '{':
		/* bracketed comments */
		do {
			if (!(ch = ChNext()))
				throw EXPARSE("missing end of comment");
		} while (ch != '}');
		if (!fWhiteSpace)
			goto Retry;
		return new TK(tkpgnComment);

	case '$':
	{
		/* numeric annotations */
		/* TODO: what should we actually do with these things? */
		int w = 0;
		ch = ChNext();
		if (!in_range(ch, '0', '9'))
			throw EXPARSE("invalid numeric annotation integer");
		do {
			w = w * 10 + ch - '0';
			ch = ChNext();
		} while (!in_range(ch, '0', '9'));
		UngetCh(ch);
		return new TKW(tkpgnNumAnno, w);
	}

	case '"':
		/* strings */
	{
		char szString[1024];
		char* pch = szString;
		do {
			ch = ChNext();
			if (!ch)
				throw EXPARSE("missing closing quote on string");
			if (ch == '\\') {
				ch = ChNext();
				if (!ch)
					throw EXPARSE("improper character escape in string");
			}
			*pch++ = ch;
		} while (ch != '"');
		*(pch - 1) = 0;
		return new TKSZ(tkpgnString, szString);
	}

	default:
		/* symbols? ignore any characters that are illegal in this context */
		if (!FIsSymbol(ch, true))
			goto Retry;
		{
			/* BUG: check for overflow */
			char szSym[256] = { 0 };
			char* pchSym = szSym;
			do {
				*pchSym++ = ch;
				ch = ChNext();
			} while (FIsSymbol(ch, false));
			if (ch)
				UngetCh(ch);
			*pchSym = 0;
			return new TKSZ(tkpgnSymbol, szSym);
		}
	}

	assert(false);
	return nullptr;
}


/*	ISTKPGN:::ScanToBlankLine
 *
 *	Scans through the stream until it encounters a blank line or the end of stream.
 */
void ISTKPGN::ScanToBlankLine(void)
{
	for (TK* ptk = PtkNext(); (int)*ptk != tkpgnEnd && (int)*ptk != tkpgnBlankLine; ptk = PtkNext())
		;
}


/*	ISTKPGN::SetImvCur
 *
 *	We keep track of the current move number in the move, which is helpful
 *	for error reporting
 */
void ISTKPGN::SetImvCur(int imv)
{
	imvCur = imv;
}


/*
 *
 *	ISTK class
 *
 *	A generic token input stream class. We'll build this up to be a
 *	general purpose scanner eventually.
 *
 */


ISTK::ISTK(istream& is) : liCur(1), is(is), ptkPrev(nullptr), szStream(L"<stream>")
{
}


ISTK::~ISTK(void)
{
}


ISTK::operator bool()
{
	return (bool)is;
}


/*	ISTK::ChNext
 *
 *	Reads the next character from the input stream. Returns null character at
 *	EOF, and coallesces end of line characters into \n for all platforms.
 */
char ISTK::ChNext(void)
{
	if (is.eof())
		return '\0';
	char ch;
	if (!is.get(ch))
		return '\0';
	if (ch == '\r') {
		if (is.peek() == '\n')
			is.get(ch);
		liCur++;
		return '\n';
	}
	else if (ch == '\n')
		liCur++;
	return ch;
}


void ISTK::UngetCh(char ch)
{
	assert(ch != '\0');
	if (ch == '\n')
		liCur--;
	is.unget();
}


bool ISTK::FIsSymbol(char ch, bool fFirst) const
{
	if (in_range(ch, 'a', 'z') || in_range(ch, 'A', 'Z') || ch == '_')
		return true;
	if (fFirst)
		return false;
	if (in_range(ch, '0', '9'))
		return true;
	return false;
}


void ISTK::UngetTk(TK* ptk)
{
	ptkPrev = ptk;
}


int ISTK::line(void) const
{
	return liCur;
}


wstring ISTK::SzStream(void) const
{
	return szStream;
}


void ISTK::SetSzStream(const wstring& sz)
{
	szStream = sz;
}

/*
 *
 *	TK class
 *
 *	A simple file scanner token class. These are virtual classes, so
 *	they need to be allocated.
 *
 */


static const string szNull("");


TK::TK(int tk) : tk(tk)
{
}


TK::operator int() const
{
	return tk;
}


bool TK::FIsString(void) const
{
	return false;
}


bool TK::FIsInteger(void) const
{
	return false;
}


const string& TK::sz(void) const
{
	return szNull;
}


/*
 *
 *	TKSZ
 * 
 *	String token.
 * 
 */


TKSZ::TKSZ(int tk, const string& sz) : TK(tk), szToken(sz)
{
}


TKSZ::TKSZ(int tk, const char* sz) : TK(tk)
{
	szToken = string(sz);
}


bool TKSZ::FIsString(void) const
{
	return true;
}


const string& TKSZ::sz(void) const
{
	return szToken;
}


/*
 *
 *	TKW
 * 
 */


TKW::TKW(int tk, int w) : TK(tk), wToken(w)
{
}


bool TKW::FIsInteger(void) const
{
	return true;
}


int TKW::w(void) const
{
	return wToken;
}
