/*
 *
 *	save.cpp
 * 
 *	Saving various formats of games/board positions
 * 
 */

#include "ga.h"



/*	GA::SavePGNFile
 *
 *	Saves the game state as a PGN file
 */
void GA::SavePGNFile(const wstring& szFile)
{
	ofstream os(szFile, ios_base::out);
	if (os.fail())
		return;

	Serialize(os);

	os.close();
}


/*	GA::Serialize
 *
 *	Serializes the current game state to a stream, which can be
 *	a file, or a memory buffer (which is used for clipboard)
 */
void GA::Serialize(ostream& os)
{
	SerializeHeaders(os);
	os << endl;
	SerializeMoveList(os);
}


void GA::SerializeHeaders(ostream& os)
{
	SerializeHeader(os, "Event", szEvent);
	SerializeHeader(os, "Site", szSite);
	SerializeHeader(os, "Round", szRound);

	SerializeHeader(os, "Date", SzPgnDate(tpStart));
	SerializeHeader(os, "White", mpcpcppl[cpcWhite]->SzName());
	SerializeHeader(os, "Black", mpcpcppl[cpcBlack]->SzName());

	/* FEN initial state */
	/* TODO only save this if it's not the default starting position */
	SerializeHeader(os, "FEN", (wstring)bdgInit);

	wstring szResult;
	if (FResultSz(bdg.gs, szResult))
		SerializeHeader(os, "Result", szResult);
}


bool GA::FResultSz(GS gs, wstring& sz) const
{
	switch (gs) {
	case gsBlackCheckMated:
	case gsBlackResigned:
	case gsBlackTimedOut:
		sz = L"1-0";
		break;
	case gsWhiteCheckMated:
	case gsWhiteResigned:
	case gsWhiteTimedOut:
		sz = L"0-1";
		break;
	case gsPlaying:
	case gsCanceled:
	case gsNotStarted:
	case gsPaused:
		return false;
	case gsStaleMate:
	case gsDrawDead:
	case gsDrawAgree:
	case gsDraw3Repeat:
	case gsDraw50Move:
		sz = L"1/2-1/2";
		break;
	default:
		assert(false);
		break;
	}
	return true;
}


wstring GA::SzPgnDate(time_point<system_clock> tp) const
{
	time_t tt = system_clock::to_time_t(tp);
	struct tm tm;
	localtime_s(&tm, &tt);
	wchar_t sz[80], *pch;
	pch = PchDecodeInt(tm.tm_year + 1900, sz);
	*pch++ = '.';
	pch = PchDecodeInt(tm.tm_mon + 1, pch);
	*pch++ = '.';
	pch = PchDecodeInt(tm.tm_mday, pch);
	*pch = 0;
	return sz;
}


void GA::SerializeHeader(ostream& os, const string& szTag, const wstring& szVal)
{
	/* TODO: escape quote marks? */
	os << string("[") + szTag + " \"" + SzFlattenWsz(szVal) + "\"]";
	os << endl;
}


void GA::SerializeMoveList(ostream& os)
{
	string szLine;

	BDG bdgSav = bdgInit;

	if (bdg.vmveGame.size() > 0) {
		/* first move needs some special handling to put leading "..." on move lists
		   when black was the first to move */
		WriteSzLine80(os, szLine, bdg.vmveGame.FImvFirstIsBlack()  ? "1... " : "1. ");
		SerializeMove(os, szLine, bdgSav, bdg.vmveGame[0]);
		for (int imve = 1; imve < bdg.vmveGame.size(); imve++) {
			MVE mve = bdg.vmveGame[imve];
			if (mve.fIsNil())
				continue;
			if (bdg.vmveGame.FImvIsWhite(imve))
				WriteSzLine80(os, szLine, to_string(bdg.vmveGame.NmvFromImv(imve)) + ". ");
			SerializeMove(os, szLine, bdgSav, mve);
		}
	}

	/* at the end of the move list, write result */

	wstring sz;
	if (FResultSz(bdgSav.gs, sz))
		WriteSzLine80(os, szLine, SzFlattenWsz(sz));

	assert(szLine.size() > 0);
	os << szLine;
	os << endl;
}

void GA::SerializeMove(ostream& os, string& szLine, BDG& bdg, MVE mve)
{
	wstring wsz = bdg.SzDecodeMv(mve, false);
	WriteSzLine80(os, szLine, bdg.SzFlattenMvSz(wsz) + " ");
	bdg.MakeMv(mve);
}

/*	GA::WriteSzLine80
 *
 *	Writes szAdd to the output stream os, using szLine as a line buffer
 *	which gets flushed at 80 character column 
 */
void GA::WriteSzLine80(ostream& os, string& szLine, const string& szAdd)
{
	if (szLine.size() + szAdd.size() > 80) {
		os << szLine;
		os << endl;
		szLine.clear();
	}
	szLine += szAdd;
}


/*
 *
 *	Move decoding
 * 
 */


/*	BDG::SzDecodeMv
 *
 *	Decodes a move into algebraic notation. Algebraic concise notation requires board
 *	context to do correctly, so the move must be a legal move on the board. Does not
 *	include postfix marks for check or checkmate (+ or #).
 */

const wchar_t mpapcch[] = { chSpace, L'P', L'N', L'B', L'R', L'Q', L'K', L'X' };
const wchar_t mpapcchFig[cpcMax][apcMax] =
{
	{ chSpace, chWhitePawn, chWhiteKnight, chWhiteBishop, chWhiteRook, chWhiteQueen, chWhiteKing },
	{ chSpace, chBlackPawn, chBlackKnight, chBlackBishop, chBlackRook, chBlackQueen, chBlackKing }
};

wstring BDG::SzDecodeMv(MVE mve, bool fPretty)
{
	VMVE vmve;
	GenMoves(vmve, ggPseudo);

	SQ sqCapture = mve.sqTo();

	wchar_t sz[16] = { 0 };
	wchar_t* pch = sz;

	/* get ready to handle en passant and castles, and write out the piece
	   character for non-pawn moves */

	switch (mve.apcMove()) {
	case apcPawn:
		if (mve.sqTo() == sqEnPassant)
			sqCapture = SQ(mve.sqTo().rank() ^ 1, mve.sqTo().file());
		break;

	case apcKing:
		if (mve.sqFrom().file() == fileKing) {
			if (mve.sqTo().file() == fileKingKnight)
				return wstring(L"O-O");
			if (mve.sqTo().file() == fileQueenBishop)
				return wstring(L"O-O-O");
		}
		[[fallthrough]];
	default:
		if (!fPretty)
			*pch++ = mpapcch[mve.apcMove()];
		else
			*pch++ = mpapcchFig[CpcFromSq(mve.sqFrom())][mve.apcMove()];
		break;
	}

	/* for ambiguous moves, we need to add various from-square qualifiers depending
	   on where the ambiguity is; for pawn captures, we just show the file of the
	   pawn that is doing the capturing, since that uniequely specifies the pawn */

	if (FMvApcAmbiguous(vmve, mve)) {
		if (!FMvApcFileAmbiguous(vmve, mve))
			*pch++ = L'a' + mve.sqFrom().file();
		else {
			if (FMvApcRankAmbiguous(vmve, mve))
				*pch++ = L'a' + mve.sqFrom().file();
			*pch++ = L'1' + mve.sqFrom().rank();
		}
	}
	else if (mve.apcMove() == apcPawn && !FIsEmpty(sqCapture))
		*pch++ = L'a' + mve.sqFrom().file();

	/* and put the destination square */

	if (!FIsEmpty(sqCapture))
		*pch++ = fPretty ? chMultiply : L'x';
	*pch++ = L'a' + mve.sqTo().file();
	*pch++ = L'1' + mve.sqTo().rank();

	/* en passant */

	if (mve.apcMove() == apcPawn && mve.sqTo() == sqEnPassant) {
		if (fPretty)
			*pch++ = chNonBreakingSpace;	
		*pch++ = L'e';
		*pch++ = L'.';
		*pch++ = L'p';
		*pch++ = L'.';
	}

	/* promotions */

	if (mve.apcPromote() != apcNull) {
		*pch++ = chEqual;
		*pch++ = mpapcch[mve.apcPromote()];
	}

	/* and wrap up the string in a wstring and return it */

	*pch = 0;
	return wstring(sz);
}


bool BDG::FMvApcAmbiguous(const VMVE& vmve, MVE mve) const
{
	SQ sqFrom = mve.sqFrom();
	SQ sqTo = mve.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mve.apcMove())
			return true;
	}
	return false;
}


bool BDG::FMvApcRankAmbiguous(const VMVE& vmve, MVE mve) const
{
	SQ sqFrom = mve.sqFrom();
	SQ sqTo = mve.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mve.apcMove() && mveOther.sqFrom().rank() == sqFrom.rank())
			return true;
	}
	return false;
}


bool BDG::FMvApcFileAmbiguous(const VMVE& vmve, MVE mve) const
{
	SQ sqFrom = mve.sqFrom();
	SQ sqTo = mve.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mve.apcMove() && mveOther.sqFrom().file() == sqFrom.file())
			return true;
	}
	return false;
}


string BDG::SzFlattenMvSz(const wstring& wsz) const
{
	string sz;
	for (int ich = 0; wsz[ich]; ich++) {
		switch (wsz[ich]) {
		case chNonBreakingSpace:
			sz += ' ';
			break;
		case chMultiply:
			sz += 'x';
			break;
		case chEnDash:
			sz += '-';
			break;
		default:
			sz += (char)wsz[ich];
			break;
		}
	}
	return sz;
}


/*	BDG::SzDecodeMvPost
 *
 *	Does a pseudo-decode of a move after it's been made on the board. Not to be used
 *	for official decoding, because ambiguities are not removed and stuff like promotion
 *	and en passant are not available, but useful for logging and debuggingn
 */
wstring BDG::SzDecodeMvPost(MVE mve) const
{
	if (mve.fIsNil())
		return L"--";
	SQ sqFrom = mve.sqFrom();
	SQ sqTo = mve.sqTo();
	wstring sz;
	sz += L'a' + sqFrom.file();
	sz += to_wstring(sqFrom.rank() + 1);
	if (mve.fIsCapture())
		sz += L'x';
	sz += L'a' + sqTo.file();
	sz += to_wstring(sqTo.rank() + 1);
	if (mve.apcPromote() != apcNull) {
		sz += chEqual;
		sz += mpapcch[mve.apcPromote()];
	}
	return sz;
}