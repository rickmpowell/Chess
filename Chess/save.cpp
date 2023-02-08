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
	SerializeHeader(os, "FEN", bdgInit.SzFEN());

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
		WriteSzLine80(os, szLine, FImvFirstIsBlack()  ? "1... " : "1. ");
		SerializeMove(os, szLine, bdgSav, bdg.vmveGame[0]);
		for (int imve = 1; imve < bdg.vmveGame.size(); imve++) {
			MVE mve = bdg.vmveGame[imve];
			if (mve.fIsNil())
				continue;
			if (FImvIsWhite(imve))
				WriteSzLine80(os, szLine, to_string(NmvFromImv(imve)) + ". ");
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

void GA::SerializeMove(ostream& os, string& szLine, BDG& bdg, MVU mvu)
{
	wstring wsz = bdg.SzDecodeMvu(mvu, false);
	WriteSzLine80(os, szLine, bdg.SzFlattenMvuSz(wsz) + " ");
	bdg.MakeMvu(mvu);
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

wstring BDG::SzDecodeMvu(MVU mvu, bool fPretty)
{
	VMVE vmve;
	GenVmve(vmve, ggPseudo);

	/* if destination square is unique, just include the destination square */
	SQ sqFrom = mvu.sqFrom();
	APC apc = mvu.apcMove();
	SQ sqTo = mvu.sqTo();
	SQ sqCapture = sqTo;

	wchar_t sz[16];
	wchar_t* pch = sz;

	switch (apc) {
	case apcPawn:
		if (sqTo == sqEnPassant)
			sqCapture = SQ(sqTo.rank() ^ 1, sqTo.file());
		break;

	case apcKing:
		if (sqFrom.file() == fileKing) {
			if (sqTo.file() == fileKingKnight)
				goto FinishCastle;
			if (sqTo.file() == fileQueenBishop) {
				*pch++ = L'O';
				*pch++ = chMinus;
FinishCastle:
				*pch++ = L'O';
				*pch++ = chMinus;
				*pch++ = L'O';
				goto FinishMove;
			}
		}
		break;
	case apcKnight:
	case apcBishop:
	case apcRook:
	case apcQueen:
		break;
	default:
		assert(false);
		break;
	}

	if (apc != apcPawn) {
		if (!fPretty)
			*pch++ = mpapcch[apc];
		else
			*pch++ = mpapcchFig[CpcFromSq(sqFrom)][apc];
	}

	/* for ambiguous moves, we need to add various from square qualifiers depending
	 * on where the ambiguity is */

	if (FMvuApcAmbiguous(vmve, mvu)) {
		if (!FMvuApcFileAmbiguous(vmve, mvu))
			*pch++ = L'a' + sqFrom.file();
		else {
			if (FMvuApcRankAmbiguous(vmve, mvu))
				*pch++ = L'a' + sqFrom.file();
			*pch++ = L'1' + sqFrom.rank();
		}
	}
	else if (apc == apcPawn && !FIsEmpty(sqCapture))
		*pch++ = L'a' + sqFrom.file();

	/* if we fall out, there is no ambiguity with the apc moving to the
	   destination square */
	if (!FIsEmpty(sqCapture))
		*pch++ = fPretty ? chMultiply : 'x';
	*pch++ = L'a' + sqTo.file();
	*pch++ = L'1' + sqTo.rank();

	if (apc == apcPawn && sqTo == sqEnPassant) {
		if (fPretty)
			*pch++ = chNonBreakingSpace;	
		*pch++ = L'e';
		*pch++ = L'.';
		*pch++ = L'p';
		*pch++ = L'.';
	}

	{
	APC apcPromote = mvu.apcPromote();
	if (apcPromote != apcNull) {
		*pch++ = chEqual;
		*pch++ = mpapcch[apcPromote];
	}
	}

FinishMove:
	*pch = 0;
	return wstring(sz);
}


bool BDG::FMvuApcAmbiguous(const VMVE& vmve, MVU mvu) const
{
	SQ sqFrom = mvu.sqFrom();
	SQ sqTo = mvu.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mvu.apcMove())
			return true;
	}
	return false;
}


bool BDG::FMvuApcRankAmbiguous(const VMVE& vmve, MVU mvu) const
{
	SQ sqFrom = mvu.sqFrom();
	SQ sqTo = mvu.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mvu.apcMove() && mveOther.sqFrom().rank() == sqFrom.rank())
			return true;
	}
	return false;
}


bool BDG::FMvuApcFileAmbiguous(const VMVE& vmve, MVU mvu) const
{
	SQ sqFrom = mvu.sqFrom();
	SQ sqTo = mvu.sqTo();
	for (MVE mveOther : vmve) {
		if (mveOther.sqTo() != sqTo || mveOther.sqFrom() == sqFrom)
			continue;
		if (mveOther.apcMove() == mvu.apcMove() && mveOther.sqFrom().file() == sqFrom.file())
			return true;
	}
	return false;
}


string BDG::SzFlattenMvuSz(const wstring& wsz) const
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
wstring BDG::SzDecodeMvuPost(MVU mvu) const
{
	SQ sqFrom = mvu.sqFrom();
	SQ sqTo = mvu.sqTo();
	wstring sz;
	sz += L'a' + sqFrom.file();
	sz += to_wstring(sqFrom.rank() + 1);
	if (mvu.fIsCapture())
		sz += L'x';
	sz += L'a' + sqTo.file();
	sz += to_wstring(sqTo.rank() + 1);
	if (mvu.apcPromote() != apcNull) {
		sz += chEqual;
		sz += mpapcch[mvu.apcPromote()];
	}
	return sz;
}