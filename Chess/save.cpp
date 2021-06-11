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
	SerializeHeader(os, "Event", "Casual Game");
	SerializeHeader(os, "Site", "Local Computer");
	SerializeHeader(os, "Round", "1");

	/* TODO: real date */
	SerializeHeader(os, "Date", "2021.??.??");

	/* TODO: real player names */
	SerializeHeader(os, "White", "White");
	SerializeHeader(os, "Black", "Black");

	switch (bdg.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		SerializeHeader(os, "Result", "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		SerializeHeader(os, "Result", "1-0");
		break;
	case GS::Playing:
		break;
	default:
		SerializeHeader(os, "Result", "1/2-1/2");
		break;
	}
}


void GA::SerializeHeader(ostream& os, const string& szTag, const string& szVal)
{
	/* TODO: escape quote marks? */
	os << string("[") + szTag + " \"" + szVal + "\"]";
	os << endl;
}


void GA::SerializeMoveList(ostream& os)
{
	BDG bdgSave;
	bdgSave.NewGame();
	string szLine;
	for (unsigned imv = 0; imv < (unsigned)bdg.vmvGame.size(); imv++) {
		MV mv = bdg.vmvGame[imv];
		if (imv % 2 == 0)
			WriteSzLine80(os, szLine, to_string(imv/2 + 1) + ". ");
		wstring wsz = bdgSave.SzDecodeMv(mv, false);
		WriteSzLine80(os, szLine, bdgSave.SzFlattenMvSz(wsz) + " ");
		bdgSave.MakeMv(mv);
	}

	/* when we're done, write result */

	switch (bdgSave.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		WriteSzLine80(os, szLine, "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		WriteSzLine80(os, szLine, "1-0");
		break;
	case GS::Playing:
		break;
	default:
		WriteSzLine80(os, szLine, "1/2-1/2");
		break;
	}

	assert(szLine.size() > 0);
	os << szLine;
	os << endl;
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

const wchar_t mpapcch[] = { L' ', L'P', L'N', L'B', L'R', L'Q', L'K', L'X' };
const wchar_t mpapcchFig[CPC::ColorMax][APC::ActMax] =
{
	{ L' ', L'\x2659', L'\x2658', L'\x2657', L'\x2656', L'\x2655', L'\x2654' },
	{ L' ', L'\x265f', L'\x265e', L'\x265d', L'\x265c', L'\x265b', L'\x265a' }
};

wstring BDG::SzDecodeMv(MV mv, bool fPretty) const
{
	GMV gmv;
	GenRgmv(gmv, RMCHK::NoRemove);

	/* if destination square is unique, just include the destination square */
	SQ sqFrom = mv.sqFrom();
	APC apc = ApcFromSq(sqFrom);
	SQ sqTo = mv.sqTo();
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

	if (apc != APC::Pawn) {
		if (!fPretty)
			*pch++ = mpapcch[apc];
		else
			*pch++ = mpapcchFig[CpcFromSq(sqFrom)][apc];
	}

	/* for ambiguous moves, we need to add various from square qualifiers depending
	 * on where the ambiguity is */

	if (FMvApcAmbiguous(gmv, mv)) {
		if (!FMvApcFileAmbiguous(gmv, mv))
			*pch++ = L'a' + sqFrom.file();
		else {
			if (FMvApcRankAmbiguous(gmv, mv))
				*pch++ = L'a' + sqFrom.file();
			*pch++ = L'1' + sqFrom.rank();
		}
	}
	else if (apc == APC::Pawn && !FIsEmpty(sqCapture))
		*pch++ = L'a' + sqFrom.file();

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
		APC apcPromote = mv.apcPromote();
		if (apcPromote != APC::Null) {
			*pch++ = L'=';
			*pch++ = mpapcch[apcPromote];
		}
	}

FinishMove:
	*pch = 0;
	return wstring(sz);
}


bool BDG::FMvApcAmbiguous(const GMV& gmv, MV mv) const
{
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mvOther = gmv[imv];
		if (mvOther.sqTo() != sqTo || mvOther.sqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.sqFrom()) == ApcFromSq(sqFrom))
			return true;
	}
	return false;
}


bool BDG::FMvApcRankAmbiguous(const GMV& gmv, MV mv) const
{
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mvOther = gmv[imv];
		if (mvOther.sqTo() != sqTo || mvOther.sqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.sqFrom()) == ApcFromSq(sqFrom) && mvOther.sqFrom().rank() == sqFrom.rank())
			return true;
	}
	return false;
}


bool BDG::FMvApcFileAmbiguous(const GMV& gmv, MV mv) const
{
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MV mvOther = gmv[imv];
		if (mvOther.sqTo() != sqTo || mvOther.sqFrom() == sqFrom)
			continue;
		if (ApcFromSq(mvOther.sqFrom()) == ApcFromSq(sqFrom) && mvOther.sqFrom().file() == sqFrom.file())
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


/*	BDG::SzDecodeMvPost
 *
 *	Does a pseudo-decode of a move after it's been made on the board. Not to be used
 *	for official decoding, because ambiguities are not removed and stuff like promotion
 *	and en passant are not available, but useful for logging and debuggingn
 */
wstring BDG::SzDecodeMvPost(MV mv) const
{
	SQ sqFrom = mv.sqFrom();
	SQ sqTo = mv.sqTo();
	wstring sz;
	sz += L'a' + sqFrom.file();
	sz += to_wstring(sqFrom.rank() + 1);
	if (mv.apcCapture() != APC::Null)
		sz += L'x';
	sz += L'a' + sqTo.file();
	sz += to_wstring(sqTo.rank() + 1);
	if (mv.apcPromote() != APC::Null) {
		sz += L'=';
		sz += mpapcch[mv.apcPromote()];
	}
	return sz;
}