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
	ISTKPGN istkpgn(is);
	NewGame(new RULE(0, 0, 0));
	if (!ReadPGNHeaders(istkpgn))
		return;
	ReadPGNMoveList(istkpgn);
	this->spmv = SPMV::Animate;
	Redraw();
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
