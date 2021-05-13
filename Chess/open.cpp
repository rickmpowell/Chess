/*
 *
 *	open.cpp
 * 
 *	Open files
 * 
 */

#include "ga.h"
#include <streambuf>


void GA::OpenPGNFile(const WCHAR szFile[])
{
	ifstream is(szFile, ifstream::in);
	ISTKPGN istkpgn(is);
	NewGame(new RULE(0, 0, 0));
	if (!ReadPGNHeaders(istkpgn))
		return;
	ReadPGNMoveList(istkpgn);
}

