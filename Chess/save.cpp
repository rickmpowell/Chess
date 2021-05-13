/*
 *
 *	save.cpp
 * 
 *	Saving various formats of games/board positions
 * 
 */
#include "ga.h"
#include <fileapi.h>



void GA::SavePGNFile(const WCHAR szFile[])
{
	HANDLE hf = CreateFileW(szFile, 
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return;

	SavePGNHeaders(hf);
	WriteSz(hf, "\n");
	SavePGNMoveList(hf);

	CloseHandle(hf);
}


void GA::SavePGNHeaders(HANDLE hf)
{
	SavePGNHeader(hf, "Event", "Casual Game");
	SavePGNHeader(hf, "Site", "Local Computer");
	SavePGNHeader(hf, "Round", "1");

	/* TODO: real date */
	SavePGNHeader(hf, "Date", "2021.??.??");

	/* TODO: real player names */
	SavePGNHeader(hf, "White", "White");
	SavePGNHeader(hf, "Black", "Black");

	switch (bdg.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		SavePGNHeader(hf, "Result", "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		SavePGNHeader(hf, "Result", "1-0");
		break;
	case GS::Playing:
		break;
	default:
		SavePGNHeader(hf, "Result", "1/2-1/2");
		break;
	}
}


void GA::SavePGNHeader(HANDLE hf, const string& szTag, const string& szVal)
{
	WriteSz(hf, "[");
	WriteSz(hf, szTag);
	WriteSz(hf, " ");
	/* TODO: escape quote marks? */
	WriteSz(hf, string("\"") + szVal + "\"");
	WriteSz(hf, "]\n");
}


void GA::SavePGNMoveList(HANDLE hf)
{
	BDG bdgSave;
	bdgSave.NewGame();
	for (unsigned imv = 0; imv < (unsigned)bdg.rgmvGame.size(); imv++) {
		MV mv = bdg.rgmvGame[imv];
		if (imv % 2 == 0) {
			string sz = to_string(imv / 2 + 1);
			sz += ". ";
			WriteSz(hf, sz);
		}
		wstring wsz = bdgSave.SzDecodeMv(mv, false);
		WriteSz(hf, bdgSave.SzFlattenMvSz(wsz) + " ");
		bdgSave.MakeMv(mv);
	}

	/* when we're done, write result */

	switch (bdgSave.gs) {
	case GS::BlackCheckMated:
	case GS::BlackResigned:
	case GS::BlackTimedOut:
		WriteSz(hf, "0-1");
		break;
	case GS::WhiteCheckMated:
	case GS::WhiteResigned:
	case GS::WhiteTimedOut:
		WriteSz(hf, "1-0");
		break;
	case GS::Playing:
		break;
	default:
		WriteSz(hf, "1/2-1/2");
		break;
	}
}


void GA::WriteSz(HANDLE hf, const string& sz)
{
	DWORD cb;
	::WriteFile(hf, sz.c_str(), (DWORD)sz.size(), &cb, NULL);
}