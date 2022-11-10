/*
 *
 *	uci.cpp
 * 
 *	Universal Chess Interface. A stdin/stdout communication protocol so we can
 *	play other engines and join tournaments.
 * 
 */

#include "uci.h"



/*
 *
 *	UCI class
 * 
 *	The main Universal Chess Interface class. Creates a console and opens up the
 *	standard in/out handles so we can talk to a UCI GUI app.
 * 
 */


UCI::UCI(GA* pga) : pga(pga), hfileStdin(NULL), hfileStdout(NULL)
{
	fInherited = !::AllocConsole();
	hfileStdin = ::GetStdHandle(STD_INPUT_HANDLE);
	hfileStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
}


UCI::~UCI(void)
{
	if (!fInherited)
		::FreeConsole();
}


void UCI::WriteSz(const wstring& wsz)
{
	DWORD cb;
	string sz = SzFlattenWsz(wsz) + "\n";
	::WriteFile(hfileStdout, sz.c_str(), (DWORD)sz.length(), &cb, NULL);
}