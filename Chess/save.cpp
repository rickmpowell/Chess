/*
 *
 *	save.cpp
 * 
 *	Saving various formats of games/board positions
 * 
 */
#include "ga.h"



void GA::SavePGNFile(const WCHAR szFile[])
{
	ofstream os(szFile, ios_base::out);
	if (os.fail())
		return;

	SavePGNHeaders(os);
	os << endl;
	SavePGNMoveList(os);

	os.close();
}


void GA::SavePGNHeaders(ofstream& os)
{
	SavePGNHeader(os, "Event", "Casual Game");
	SavePGNHeader(os, "Site", "Local Computer");
	SavePGNHeader(os, "Round", "1");

	/* TODO: real date */
	SavePGNHeader(os, "Date", "2021.??.??");

	/* TODO: real player names */
	SavePGNHeader(os, "White", "White");
	SavePGNHeader(os, "Black", "Black");

	switch (bdg.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		SavePGNHeader(os, "Result", "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		SavePGNHeader(os, "Result", "1-0");
		break;
	case GS::Playing:
		break;
	default:
		SavePGNHeader(os, "Result", "1/2-1/2");
		break;
	}
}


void GA::SavePGNHeader(ofstream& os, const string& szTag, const string& szVal)
{
	WriteSz(os, "[");
	WriteSz(os, szTag);
	WriteSz(os, " ");
	/* TODO: escape quote marks? */
	WriteSz(os, string("\"") + szVal + "\"");
	WriteSz(os, "]\n");
}


void GA::SavePGNMoveList(ofstream& os)
{
	BDG bdgSave;
	bdgSave.NewGame();
	for (unsigned imv = 0; imv < (unsigned)bdg.rgmvGame.size(); imv++) {
		MV mv = bdg.rgmvGame[imv];
		if (imv % 2 == 0) {
			string sz = to_string(imv / 2 + 1);
			sz += ". ";
			WriteSz(os, sz);
		}
		wstring wsz = bdgSave.SzDecodeMv(mv, false);
		WriteSz(os, bdgSave.SzFlattenMvSz(wsz) + " ");
		bdgSave.MakeMv(mv);
	}

	/* when we're done, write result */

	switch (bdgSave.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		WriteSz(os, "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		WriteSz(os, "1-0");
		break;
	case GS::Playing:
		break;
	default:
		WriteSz(os, "1/2-1/2");
		break;
	}
}


void GA::WriteSz(ofstream& os, const string& sz)
{
	os << sz;
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

	if (apc != APC::Pawn) {
		if (!fPretty)
			*pch++ = mpapcch[apc];
		else
			*pch++ = mpapcchFig[CpcFromSq(sqFrom)][apc];
	}

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


/*	BDG::SzDecodeMvPost
 *
 *	Does a pseudo-decode of a move after it's been made on the board. Not to be used
 *	for official decoding, because ambiguities are not removed and stuff like promotion
 *	and en passant are not available, but useful for logging and debuggingn
 */
wstring BDG::SzDecodeMvPost(MV mv) const
{
	SQ sqFrom = mv.SqFrom();
	SQ sqTo = mv.SqTo();
	wstring sz;
	sz += L'a' + sqFrom.file();
	sz += to_wstring(sqFrom.rank() + 1);
	if (mv.ApcCapture() != APC::Null)
		sz += L'x';
	sz += L'a' + sqTo.file();
	sz += to_wstring(sqTo.rank() + 1);
	if (mv.ApcPromote() != APC::Null) {
		sz += L'=';
		sz += mpapcch[mv.ApcPromote()];
	}
	return sz;
}