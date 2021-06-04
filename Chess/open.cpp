/*
 *
 *	open.cpp
 * 
 *	Open files, parsing PGN files, some semi-general stream tokenization
 * 
 */

#include "ga.h"
#include <streambuf>


void GA::OpenPGNFile(const WCHAR szFile[])
{
	this->spmv = SPMV::Hidden;
	ifstream is(szFile, ifstream::in);
	Deserialize(is);
	this->spmv = SPMV::Animate;
	Redraw();
}


void GA::Deserialize(istream& is)
{
	ISTKPGN istkpgn(is);
	NewGame(new RULE(0, 0, 0));
	if (!DeserializeHeaders(istkpgn))
		return;
	DeserializeMoveList(istkpgn);
}


int GA::DeserializeGame(ISTKPGN& istkpgn)
{
	NewGame(new RULE(0, 0, 0));
	if (!DeserializeHeaders(istkpgn))
		return 0;
	DeserializeMoveList(istkpgn);
	return 1;
}


int GA::DeserializeHeaders(ISTKPGN& istkpgn)
{
	if (!DeserializeTag(istkpgn))
		return 0;
	while (DeserializeTag(istkpgn))
		;
	return 1;
}


int GA::DeserializeTag(ISTKPGN& istkpgn)
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


/*	GA::DeserializeMoveList
 *
 *	Reads the move list part of a single game from a PGN file. Yields control to
 *	other apps and lets the UI run. Returns 0 if the user pressed Esc to abort
 *	the PGN processing. Returns 1 if the move list was successfully processed.
 *	Can throw an exception on syntax errors in the PGN file
 */
int GA::DeserializeMoveList(ISTKPGN& istkpgn)
{
	while (DeserializeMove(istkpgn))
		PumpMsg();
	return 1;
}


int GA::DeserializeMove(ISTKPGN& istkpgn)
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
			ProcessTag(mpsztkpgn[isz].tkpgn, szVal);
			break;
		}
	}
}


void GA::ProcessTag(int tkpgn, const string& szVal)
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


/*
 *
 *	BDG move parsing
 *
 */


/*	BDG::ParseMv
 *
 *	Parses an algebraic move for the current board state. Returns 1 for
 *	a successful move, 0 for end game, and -1 for error.
 */
int BDG::ParseMv(const char*& pch, MV& mv) const
{
	vector<MV> rgmv;
	GenRgmv(rgmv, RMCHK::Remove);

	int rank, file;
	SQ sq1;
	TKMV tkmv = TkmvScan(pch, sq1);

	switch (tkmv) {
	case TKMV::King:
	case TKMV::Queen:
	case TKMV::Rook:
	case TKMV::Bishop:
	case TKMV::Knight:
	case TKMV::Pawn:
		return ParsePieceMv(rgmv, tkmv, pch, mv);
	case TKMV::Square:
		return ParseSquareMv(rgmv, sq1, pch, mv);
	case TKMV::File:
		return ParseFileMv(rgmv, sq1, pch, mv);
	case TKMV::Rank:
		return ParseRankMv(rgmv, sq1, pch, mv);
	case TKMV::CastleKing:
		file = fileKingKnight;
		goto BuildCastle;
	case TKMV::CastleQueen:
		file = fileQueenBishop;
	BuildCastle:
		rank = RankBackFromCpc(cpcToMove);
		mv = MV(SQ(rank, fileKing), SQ(rank, file));
		return 1;
	case TKMV::WhiteWins:
	case TKMV::BlackWins:
	case TKMV::Draw:
	case TKMV::InProgress:
		return 0;
	case TKMV::End:
		return 0;	// the move symbol is blank - can this happen?
	default:
		/* all other tokens are illegal */
		return -1;
	}
}


APC ApcFromTkmv(TKMV tkmv)
{
	switch (tkmv) {
	case TKMV::Pawn: return APC::Pawn;
	case TKMV::Knight: return APC::Knight;
	case TKMV::Bishop: return APC::Bishop;
	case TKMV::Rook: return APC::Rook;
	case TKMV::Queen: return APC::Queen;
	case TKMV::King: return APC::King;
	default:
		break;
	}
	assert(false);
	return APC::Error;
}


int BDG::ParsePieceMv(const vector<MV>& rgmv, TKMV tkmv, const char*& pch, MV& mv) const
{
	SQ sq;

	APC apc = ApcFromTkmv(tkmv);
	tkmv = TkmvScan(pch, sq);

	switch (tkmv) {
	case TKMV::To:	/* B-c5 or Bxc5 */
	case TKMV::Take:
		if (TkmvScan(pch, sq) != TKMV::Square)
			return -1;
		if (!FMvMatchPieceTo(rgmv, apc, -1, -1, sq, mv))
			return -1;
		break;

	case TKMV::Square:	/* Bc5, Bd3c5, Bd3-c5, Bd3xc5 */
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == TKMV::To || tkmv == TKMV::Take) { /* eat the to/take */
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != TKMV::Square) { /* Bc5 */
			/* look up piece that can move to this square */
			if (!FMvMatchPieceTo(rgmv, apc, -1, -1, sq, mv))
				return -1;
			break;
		}
		/* Bd5c4 */
		pch = pchNext;
		if (!FMvMatchFromTo(rgmv, sq, sqTo, mv))
			return -1;
		break;
	}

	case TKMV::File: /* Bdc5 or Bd-c5 */
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == TKMV::To || tkmv == TKMV::Take) {
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != TKMV::Square)
			return -1;
		pch = pchNext;
		if (!FMvMatchPieceTo(rgmv, apc, -1, sq.file(), sqTo, mv))
			return -1;
		break;
	}

	case TKMV::Rank:
	{
		const char* pchNext = pch;
		SQ sqTo;
		tkmv = TkmvScan(pchNext, sqTo);
		if (tkmv == TKMV::To || tkmv == TKMV::Take) {
			pch = pchNext;
			tkmv = TkmvScan(pchNext, sqTo);
		}
		if (tkmv != TKMV::Square)
			return -1;
		pch = pchNext;
		if (!FMvMatchPieceTo(rgmv, apc, sq.rank(), -1, sqTo, mv))
			return -1;
		break;
	}

	default:
		return -1;
	}
	return ParseMvSuffixes(mv, pch);
}


int BDG::ParseSquareMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const
{
	/* e4, e4e5, e4-e5, e4xe5 */
	const char* pchNext = pch;
	SQ sqTo;

	TKMV tkmv = TkmvScan(pchNext, sq);
	if (tkmv == TKMV::To || tkmv == TKMV::Take) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sq);
	}
	if (TkmvScan(pchNext, sq) == TKMV::Square) {
		pch = pchNext;
		if (!FMvMatchFromTo(rgmv, sq, sqTo, mv))
			return -1;
	}
	else { /* e4 pawn move */
		if (!FMvMatchPieceTo(rgmv, APC::Pawn, -1, -1, sq, mv))
			return -1;
	}
	return ParseMvSuffixes(mv, pch);
}


int BDG::ParseMvSuffixes(MV& mv, const char*& pch) const
{
	const char* pchNext = pch;
	for (;;) {
		SQ sqT;
		switch (TkmvScan(pchNext, sqT)) {
		case TKMV::Check:
		case TKMV::Mate:
		case TKMV::EnPassant:
			pch = pchNext;
			break;
		case TKMV::Promote:
		{
			pch = pchNext;
			TKMV tkmv = TkmvScan(pchNext, sqT);
			APC apc = ApcFromTkmv(tkmv);
			if (apc == APC::Null)
				return -1;
			pch = pchNext;
			mv.SetApcPromote(apc);
			break;
		}
		case TKMV::End:
			return 1;
		case TKMV::WhiteWins:
		case TKMV::BlackWins:
		case TKMV::Draw:
		case TKMV::InProgress:
			return 0;
		default:
			return -1;
		}
	}
}


int BDG::ParseFileMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const
{
	/* de4, d-e4, dxe4 */
	const char* pchNext = pch;
	SQ sqTo;
	TKMV tkmv = TkmvScan(pchNext, sqTo);
	if (tkmv == TKMV::To || tkmv == TKMV::Take) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sqTo);
	}
	if (tkmv != TKMV::Square)
		return -1;
	pch = pchNext;
	if (!FMvMatchPieceTo(rgmv, APC::Pawn, -1, sq.file(), sqTo, mv))
		return -1;
	return ParseMvSuffixes(mv, pch);
}


int BDG::ParseRankMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const
{
	/* 7e4, 7-e4, 7xe4 */
	const char* pchNext = pch;
	SQ sqTo;
	TKMV tkmv = TkmvScan(pchNext, sqTo);
	if (tkmv == TKMV::To || tkmv == TKMV::Take) {
		pch = pchNext;
		tkmv = TkmvScan(pchNext, sqTo);
	}
	if (tkmv != TKMV::Square)
		return -1;
	pch = pchNext;
	if (!FMvMatchPieceTo(rgmv, APC::Pawn, sq.rank(), -1, sqTo, mv))
		return -1;
	return ParseMvSuffixes(mv, pch);
}


bool BDG::FMvMatchPieceTo(const vector<MV>& rgmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, MV& mv) const
{
	for (MV mvSearch : rgmv) {
		if (mvSearch.sqTo() == sqTo && ApcFromSq(mvSearch.sqFrom()) == apc) {
			if (fileFrom != -1 && fileFrom != mvSearch.sqFrom().file())
				continue;
			if (rankFrom != -1 && rankFrom != mvSearch.sqFrom().rank())
				continue;
			mv = mvSearch;
			return true;
		}
	}
	return false;
}

bool BDG::FMvMatchFromTo(const vector<MV>& rgmv, SQ sqFrom, SQ sqTo, MV& mv) const
{
	for (MV mvSearch : rgmv) {
		if (mvSearch.sqFrom() == sqFrom && mvSearch.sqTo() == sqTo) {
			mv = mvSearch;
			return true;
		}
	}
	return false;
}


TKMV BDG::TkmvScan(const char*& pch, SQ& sq) const
{
	char ch;
	int rank, file;

	if (!*pch)
		return TKMV::End;

	struct {
		const char* szKeyWord;
		TKMV tkmv;
	} mpsztkmv[] = {
		{"0-0-0", TKMV::CastleQueen},
		{"O-O-O", TKMV::CastleQueen},
		{"0-0", TKMV::CastleKing},
		{"O-O", TKMV::CastleKing},
		{"e.p.", TKMV::EnPassant},
		{"1-0", TKMV::WhiteWins},
		{"0-1", TKMV::BlackWins},
		{"1/2-1/2", TKMV::Draw},
	};
	for (int isz = 0; isz < CArray(mpsztkmv); isz++) {
		int ich;
		for (ich = 0; mpsztkmv[isz].szKeyWord[ich]; ich++) {
			if (pch[ich] != mpsztkmv[isz].szKeyWord[ich])
				goto NextKeyWord;
		}
		pch += ich;
		return mpsztkmv[isz].tkmv;
	NextKeyWord:;
	}

	switch (ch = *pch++) {
	case 'P': return TKMV::Pawn;
	case 'B': return TKMV::Bishop;
	case 'N': return TKMV::Knight;
	case 'R': return TKMV::Rook;
	case 'Q': return TKMV::Queen;
	case 'K': return TKMV::King;
	case 'x': return TKMV::Take;
	case '-': return TKMV::To;
	case '+': return TKMV::Check;
	case '#': return TKMV::Mate;
	case '*': return TKMV::InProgress;
	case '=': return TKMV::Promote;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
		file = ch - 'a';
		if (*pch >= '1' && *pch <= '8') {
			sq = SQ(*pch++ - '1', file);
			return TKMV::Square;
		}
		sq = SQ(0, file);
		return TKMV::File;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
		rank = ch - '1';
		sq = SQ(rank, 0);
		return TKMV::Rank;
		break;
	default:
		return TKMV::Error;
	}
}


/*
 *
 *	ISTKPGN
 * 
 *	Input stream PGN file tokenizer
 * 
 */


ISTKPGN::ISTKPGN(istream& is) : ISTK(is), fWhiteSpace(false)
{
}


void ISTKPGN::WhiteSpace(bool fReturn)
{
	fWhiteSpace = fReturn;
}


bool ISTKPGN::FIsSymbol(char ch, bool fFirst) const
{
	if (ch >= 'a' && ch <= 'z')
		return true;
	if (ch >= 'A' && ch <= 'Z')
		return true;
	if (ch >= '0' && ch <= '9')
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
	while (FIsWhite(ch));
	return ch;
}


TK* ISTKPGN::PtkNext(void)
{
	if (ptkPrev) {
		TK* ptk = ptkPrev;
		ptkPrev = NULL;
		return ptk;
	}

Retry:
	char ch = ChNext();

	if (FIsWhite(ch)) {
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
		while (!FIsEnd(ch) && ch != '\n');
		if (!fWhiteSpace)
			goto Retry;
		return new TK(tkpgnLineComment);

	case '{':
		/* bracketed comments */
		do {
			if (FIsEnd(ch = ChNext()))
				throw line();
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
		if (!FIsDigit(ch))
			throw line();
		do {
			w = w * 10 + ch - '0';
			ch = ChNext();
		} while (!FIsDigit(ch));
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
			if (FIsEnd(ch))
				throw line();
			if (ch == '\\') {
				ch = ChNext();
				if (FIsEnd(ch))
					throw line();
			}
			*pch++ = ch;
		} while (ch != '"');
		*(pch - 1) = 0;
		return new TKSZ(tkpgnString, szString);
	}

	default:
#ifdef NOPE
		/* can you believe this? PGN files don't have integers as a token */
		if (FIsDigit(ch)) {
			int w = 0;
			do {
				w = w * 10 + ch - '0';
				ch = ChNext();
			} while (FIsDigit(ch));
			UngetCh(ch);
			return new TKW(tkpgnInteger, w);
		}
#endif

		/* symbols? ignore any characters that are illegal in this context */
		if (!FIsSymbol(ch, true))
			goto Retry;
		{
			char szSym[256];
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
	return NULL;
}


/*
 *
 *	ISTK class
 *
 *	A generic token input stream class. We'll build this up to be a
 *	general purpose scanner eventually.
 *
 */


ISTK::ISTK(istream& is) : is(is), li(1), ptkPrev(NULL)
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
		li++;
		return '\n';
	}
	else if (ch == '\n')
		li++;
	return ch;
}


void ISTK::UngetCh(char ch)
{
	assert(ch != '\0');
	if (ch == '\n')
		li--;
	is.unget();
}


bool ISTK::FIsWhite(char ch) const
{
	return ch == ' ' || ch == '\t';
}


bool ISTK::FIsEnd(char ch) const
{
	return ch == '\0';
}


bool ISTK::FIsDigit(char ch) const
{
	return ch >= '0' && ch <= '9';
}


bool ISTK::FIsSymbol(char ch, bool fFirst) const
{
	if (ch >= 'a' && ch <= 'z')
		return true;
	if (ch >= 'A' && ch <= 'Z')
		return true;
	if (ch == '_')
		return true;
	if (fFirst)
		return false;
	if (ch >= '0' && ch <= '9')
		return true;
	return false;
}


void ISTK::UngetTk(TK* ptk)
{
	ptkPrev = ptk;
}


int ISTK::line(void) const
{
	return li;
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


TK::~TK(void)
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


TKSZ::~TKSZ(void)
{
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
