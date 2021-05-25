/*
 *
 *	parsedecode.cpp
 * 
 *	Parsing and decoding moves, board, games, and various standard file formats.
 *	Most of this is part of the BDG class.
 *
 */

#include "Chess.h"




/*	BDG::SzDecodeMv
 *
 *	Decodes a move into algebraic notation. Algebraic concise notation requires board 
 *	context to do correctly, so the move must be a legal move on the board. Does not 
 *	include postfix marks for check or checkmate (+ or #). 
 */

const wchar_t mpapcch[] = { L' ', L'P', L'N', L'B', L'R', L'Q', L'K', L'X' };
const wchar_t mpapcchFig[CPC::ColorMax][APC::ActMax] = 
{
	{ L' ', L'\x2659', L'\x2658', L'\x2657', L'\x2656', L'\x2655', L'\x2654' },
	{ L' ', L'\x265f', L'\x265e', L'\x265d', L'\x265c', L'\x265b', L'\x265a' }
};

wstring BDG::SzDecodeMv(MV mv, bool fPretty) const
{
	vector<MV> rgmv;
	GenRgmv(rgmv, RMCHK::NoRemove);

	/* if destination square is unique, just include the destination square */
	SQ sqFrom = mv.SqFrom();
	APC apc = ApcFromSq(sqFrom);
	SQ sqTo = mv.SqTo();
	SQ sqCapture = sqTo;

	WCHAR sz[16];
	WCHAR* pch = sz;

	switch (apc) {
	case APC::Pawn:
		if (sqTo == sqEnPassant)
			sqCapture = SQ(sqTo.rank() ^ 1, sqTo.file());
		break;

	case APC::King:
		if (sqFrom.file() == fileKing) {
			if (sqTo.file() == fileKingKnight)
				goto FinishCastle;
			if (sqTo.file() == fileQueenBishop) {
				*pch++ = L'O';
				*pch++ = L'-';
FinishCastle:
				*pch++ = L'O';
				*pch++ = L'-';
				*pch++ = L'O';
				goto FinishMove;
			}
		}
		break;
	case APC::Knight:
	case APC::Bishop:
	case APC::Rook:
	case APC::Queen:
		break;
	}

	if (!fPretty)
		*pch++ = mpapcch[apc];
	else 
		*pch++ = mpapcchFig[CpcFromSq(sqFrom)][apc];

	/* for ambiguous moves, we need to add various from square qualifiers depending
	 * on where the ambiguity is */

	if (FMvApcAmbiguous(rgmv, mv)) {
		if (!FMvApcFileAmbiguous(rgmv, mv))
			*pch++ = L'a' + sqFrom.file();
		else {
			if (FMvApcRankAmbiguous(rgmv, mv))
				*pch++ = L'a' + sqFrom.file();
			*pch++ = L'1' + sqFrom.rank();
		}
	}
 
	/* if we fall out, there is no ambiguity with the apc moving to the
	   destination square */
	if (!FIsEmpty(sqCapture))
		*pch++ = fPretty ? L'\x00d7' : 'x';
	*pch++ = L'a' + sqTo.file();
	*pch++ = L'1' + sqTo.rank();

	if (apc == APC::Pawn && sqTo == sqEnPassant) {
		if (fPretty)
			*pch++ = L'\x202f';
		*pch++ = L'e';
		*pch++ = L'.';
		*pch++ = L'p';
		*pch++ = L'.';
	}

	{ 
	APC apcPromote = mv.ApcPromote();
	if (apcPromote != APC::Null) {
		*pch++ = L'=';
		*pch++ = mpapcch[apcPromote];
	} 
	}

FinishMove:
	*pch = 0;
	return wstring(sz);
}


bool BDG::FMvApcAmbiguous(const vector<MV>& rgmv, MV mv) const
{
	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	for (MV mvOther : rgmv) {
		if (mvOther.SqTo() != sqTo || mvOther.SqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.SqFrom()) == ApcFromSq(sqFrom))
			return true;
	}
	return false;
}


bool BDG::FMvApcRankAmbiguous(const vector<MV>& rgmv, MV mv) const
{
	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	for (MV mvOther : rgmv) {
		if (mvOther.SqTo() != sqTo || mvOther.SqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.SqFrom()) == ApcFromSq(sqFrom) && mvOther.SqFrom().rank() == sqFrom.rank())
			return true;
	}
	return false;
}


bool BDG::FMvApcFileAmbiguous(const vector<MV>& rgmv, MV mv) const
{
	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	for (MV mvOther : rgmv) {
		if (mvOther.SqTo() != sqTo || mvOther.SqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.SqFrom()) == ApcFromSq(sqFrom) && mvOther.SqFrom().file() == sqFrom.file())
			return true;
	}
	return false;
}


string BDG::SzFlattenMvSz(const wstring& wsz) const
{
	string sz;
	for (int ich = 0; wsz[ich]; ich++) {
		switch (wsz[ich]) {
		case L'\x00a0':
			sz += ' ';
			break;
		case L'\x00d7':
			sz += 'x';
			break;
		case L'\x2013':
			sz += '-';
			break;
		default:
			sz += (char)wsz[ich];
			break;
		}
	}
	return sz;
}


/*	BDG::SzMoveAndDecode
 *
 *	Makes a move and returns the decoded text of the move. This is the only
 *	way to get postfix marks on the move text, like '+' for check, or '#' for
 *	checkmate.
 */
wstring BDG::SzMoveAndDecode(MV mv)
{
	wstring sz = SzDecodeMv(mv, true);
	CPC cpc = CpcFromSq(mv.SqFrom());
	MakeMv(mv);
	if (FSqAttacked(cpc, mptpcsq[~cpc][tpcKing]))
		sz += L'+';
	return sz;
}


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
		if (mvSearch.SqTo() == sqTo && ApcFromSq(mvSearch.SqFrom()) == apc) {
			if (fileFrom != -1 && fileFrom != mvSearch.SqFrom().file())
				continue;
			if (rankFrom != -1 && rankFrom != mvSearch.SqFrom().rank())
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
		if (mvSearch.SqFrom() == sqFrom && mvSearch.SqTo() == sqTo) {
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
 *	Input stream tokenizer for PGN file format.
 *
 */


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
		*(pch-1) = 0;
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

