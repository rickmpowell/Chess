/*
 *
 *	open.cpp
 * 
 *	Open files, parsing PGN files, some semi-general stream tokenization
 * 
 */

#include "ga.h"
#include <streambuf>


EXPARSE::EXPARSE(const string& szMsg) : EX(szMsg)
{
}

EXPARSE::EXPARSE(const string& szMsg, ISTK& istk) : EX()
{
	char sz[1024];
	snprintf(sz, CArray(sz), szMsg.c_str(), SzFlattenWsz(L"").c_str(), istk.line());
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
	SetProcpgn(new PROCPGNOPEN(*this));
	ifstream is(szFile, ifstream::in);
	try {
		Deserialize(is);
	}
	catch (EX& ex) {
	}
	app.pga->SetProcpgn(nullptr);
	uiml.UpdateContSize();
	uiml.SetSel((int)bdg.vmvGame.size() - 1, SPMV::Hidden);
	uiml.FMakeVis(bdg.imvCur);
	Redraw();
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
ERR GA::Deserialize(istream& is)
{
	ISTKPGN istkpgn(is);
	NewGame(new RULE(0, 0, 0), SPMV::Hidden);
	try {
		ERR err;
		if ((err = DeserializeHeaders(istkpgn)) != ERR::None)
			return err;
		DeserializeMoveList(istkpgn);
	}
	catch (EX& ex) {
		NewGame(new RULE(0, 0, 0), SPMV::Hidden);
		throw ex;
	}
	return ERR::None;
}


/*	GA::DeserializeGame
 *
 *	Deserializes a single game from a PGN token stream. Return 0 on success,
 *	1 if we're at the EOF, or throws an exception on parse and file errors.
 */
ERR GA::DeserializeGame(ISTKPGN& istkpgn)
{
	NewGame(new RULE(0, 0, 0), SPMV::Hidden);
	ERR err;
	if ((err = DeserializeHeaders(istkpgn)) != ERR::None)
		return err;
	return DeserializeMoveList(istkpgn);
}


ERR GA::DeserializeHeaders(ISTKPGN& istkpgn)
{
	ERR err;
	if ((err = DeserializeTag(istkpgn)) != ERR::None)
		return err;
	pprocpgn->ProcessTag(tkpgnTagsStart, "");
	while (DeserializeTag(istkpgn) != ERR::EndOfFile)
		;
	assert(pprocpgn);
	pprocpgn->ProcessTag(tkpgnTagsEnd, "");
	return ERR::None;
}


/*	GA::DeserializeTag
 *
 *	Deserializes a single tag from the PGN file header section. Returns 0 on success,
 *	returns ERR::EndOfFile if end of tag section.
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
			return ERR::EndOfFile;	// blank line signifies end of tags
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

	return ERR::None;
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
	while (DeserializeMove(istkpgn) == ERR::None) {
		PumpMsg();
	}
	return ERR::None;
}


ERR GA::DeserializeMove(ISTKPGN& istkpgn)
{
	TK* ptk;
	for (ptk = istkpgn.PtkNext(); ; ) {
		switch ((int)*ptk) {
		case tkpgnSymbol:
			int w;
			if (!FIsMoveNumber(ptk, w)) {
				ProcessMove(ptk->sz());
				delete ptk;
				return ERR::None;
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
			return ERR::EndOfFile;
		default:
			istkpgn.UngetTk(ptk);
			return ERR::EndOfFile;
		}
	}
	return ERR::None;
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
	return ERR::None;
}


/*	GA::ProcessMove
 *
 *	Parses and performs the move that has been extracted from the PGN stream.
 */
ERR GA::ProcessMove(const string& szMove)
{
	assert(pprocpgn);

	MV mv;
	const char* pch = szMove.c_str();
	if (bdg.ParseMv(pch, mv) == ERR::EndOfFile)
		return ERR::EndOfFile;
	pprocpgn->ProcessMv(mv);
	return ERR::None;
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


ERR PROCPGNOPEN::ProcessMv(MV mv)
{
	ga.MakeMv(mv, SPMV::Hidden);
	return ERR::None;
}


ERR PROCPGNOPEN::ProcessTag(int tkpgn, const string& szValue)
{
	switch (tkpgn) {
	case tkpgnWhite:
	case tkpgnBlack:
	{
		wstring wszVal(szValue.begin(), szValue.end());
		ga.mpcpcppl[tkpgn == tkpgnBlack ? CPC::Black : CPC::White]->SetName(wszVal);
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
	return ERR::None;
}


/*	to_string(TKMV)
 *
 *	Creates a string representation of a token type
 */
string to_string(TKMV tkmv)
{
	switch (tkmv) {
	case TKMV::Error: return "<Error>";
	case TKMV::End: return "<End>";
	case TKMV::King: return "K";
	case TKMV::Queen: return "Q";
	case TKMV::Rook: return "R";
	case TKMV::Bishop: return "B";
	case TKMV::Knight: return "N";
	case TKMV::Pawn: return "P";
	case TKMV::Square: return "<Square>";
	case TKMV::File: return "<File>";
	case TKMV::Rank: return "<Rank>";
	case TKMV::Take: return "x";
	case TKMV::To: return "-";
	case TKMV::Promote: return "=";
	case TKMV::Check: return "+";
	case TKMV::Mate: return "#";
	case TKMV::EnPassant: return "e.p.";
	case TKMV::CastleKing: return "O-O";
	case TKMV::CastleQueen: return "O-O-O";
	case TKMV::WhiteWins: return "1-0";
	case TKMV::BlackWins: return "0-1";
	case TKMV::Draw: return "1/2-1/2";
	case TKMV::InProgress: return "*";
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


/*	BDG::ParseMv
 *
 *	Parses an algebraic move for the current board state. Returns 0 for
 *	a successful move, 1 for end game, and throws an EXPARSE exception on
 *	a parse error.
 */
ERR BDG::ParseMv(const char*& pch, MV& mv) const
{
	GMV gmv;
	GenGmv(gmv, RMCHK::Remove);

	const char* pchInit = pch;
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
		return ParsePieceMv(gmv, tkmv, pchInit, pch, mv);
	case TKMV::Square:
		return ParseSquareMv(gmv, sq1, pchInit, pch, mv);
	case TKMV::File:
		return ParseFileMv(gmv, sq1, pchInit, pch, mv);
	case TKMV::Rank:
		return ParseRankMv(gmv, sq1, pchInit, pch, mv);
	case TKMV::CastleKing:
		file = fileKingKnight;
		goto BuildCastle;
	case TKMV::CastleQueen:
		file = fileQueenBishop;
BuildCastle:
		rank = RankBackFromCpc(cpcToMove);
		mv = MV(SQ(rank, fileKing), SQ(rank, file));
		return ERR::None;
	case TKMV::WhiteWins:
	case TKMV::BlackWins:
	case TKMV::Draw:
	case TKMV::InProgress:
		return ERR::EndOfFile;
	case TKMV::End:
		return ERR::EndOfFile;	// the move symbol is blank - can this happen?
	default:
		/* all other tokens are illegal */
		throw EXPARSE(format("Illegal token {}", to_string(tkmv)));
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


ERR BDG::ParsePieceMv(const GMV& gmv, TKMV tkmv, const char* pchInit, const char*& pch, MV& mv) const
{
	SQ sq;

	APC apc = ApcFromTkmv(tkmv);
	tkmv = TkmvScan(pch, sq);

	switch (tkmv) {
	case TKMV::To:	/* B-c5 or Bxc5 */
	case TKMV::Take:
		if (TkmvScan(pch, sq) != TKMV::Square)
			throw EXPARSE(format("Missing destination square (got {})", to_string(tkmv)));
		mv = MvMatchPieceTo(gmv, apc, -1, -1, sq, pchInit, pch);
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
			mv = MvMatchPieceTo(gmv, apc, -1, -1, sq, pchInit, pch);
			break;
		}
		/* Bd5c4 */
		pch = pchNext;
		mv = MvMatchFromTo(gmv, sq, sqTo, pchInit, pch);
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
			throw EXPARSE(format("Expected destination square (got {})", to_string(tkmv)));
		pch = pchNext;
		mv = MvMatchPieceTo(gmv, apc, -1, sq.file(), sqTo, pchInit, pch);
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
			throw EXPARSE(format("Expected a square (got {})", to_string(tkmv)));
		pch = pchNext;
		mv = MvMatchPieceTo(gmv, apc, sq.rank(), -1, sqTo, pchInit, pch);
		break;
	}

	default:
		throw EXPARSE(format("Invalid token {}", to_string(tkmv)));
	}
	return ParseMvSuffixes(mv, pch);
}


ERR BDG::ParseSquareMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const
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
		mv = MvMatchFromTo(gmv, sq, sqTo, pchInit, pch);
	}
	else { /* e4: plain square is a pawn move */
		mv = MvMatchPieceTo(gmv, APC::Pawn, -1, -1, sq, pchInit, pch);
	}
	return ParseMvSuffixes(mv, pch);
}


ERR BDG::ParseMvSuffixes(MV& mv, const char*& pch) const
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
			if (apc == APC::Null || apc == APC::Pawn || apc == APC::King)
				throw EXPARSE("Not a valid promotion");
			pch = pchNext;
			mv.SetApcPromote(apc);
			break;
		}
		case TKMV::End:
			return ERR::None;
		case TKMV::WhiteWins:
		case TKMV::BlackWins:
		case TKMV::Draw:
		case TKMV::InProgress:
			return ERR::None;
		default:
			throw EXPARSE("Not a valid move suffix");
		}
	}

	return ERR::None;
}


ERR BDG::ParseFileMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const
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
		throw EXPARSE(format("Expected a destination square (got {})", to_string(tkmv)));
	pch = pchNext;
	mv = MvMatchPieceTo(gmv, APC::Pawn, -1, sq.file(), sqTo, pchInit, pch);
	return ParseMvSuffixes(mv, pch);
}


ERR BDG::ParseRankMv(const GMV& gmv, SQ sq, const char* pchInit, const char*& pch, MV& mv) const
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
		throw EXPARSE(format("Expected a destination square (got {})", to_string(tkmv)));
	pch = pchNext;
	mv = MvMatchPieceTo(gmv, APC::Pawn, sq.rank(), -1, sqTo, pchInit, pch);
	return ParseMvSuffixes(mv, pch);
}


MV BDG::MvMatchPieceTo(const GMV& gmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const
{
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mvSearch = gmv[imv];
		if (mvSearch.sqTo() == sqTo && ApcFromSq(mvSearch.sqFrom()) == apc) {
			if (fileFrom != -1 && fileFrom != mvSearch.sqFrom().file())
				continue;
			if (rankFrom != -1 && rankFrom != mvSearch.sqFrom().rank())
				continue;
			return mvSearch;
		}
	}
	throw EXPARSE(format("{} is not a legal move", string(pchFirst, pchLim - pchFirst)));
}

MV BDG::MvMatchFromTo(const GMV& gmv, SQ sqFrom, SQ sqTo, const char* pchFirst, const char* pchLim) const
{
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mvSearch = gmv[imv];
		if (mvSearch.sqFrom() == sqFrom && mvSearch.sqTo() == sqTo)
			return mvSearch;
	}
	throw EXPARSE(format("{} is not a legal move", string(pchFirst, pchLim-pchFirst)));
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
		ptkPrev = nullptr;
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
	return nullptr;
}


/*
 *
 *	ISTK class
 *
 *	A generic token input stream class. We'll build this up to be a
 *	general purpose scanner eventually.
 *
 */


ISTK::ISTK(istream& is) : li(1), is(is), ptkPrev(nullptr)
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
