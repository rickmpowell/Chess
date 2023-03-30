/*
 *
 *	uci.h
 *
 *	Access to the Universal Chess Interface. This is a stdin/stdout interface
 *	and a polling loop, along with queues needed for application threads to 
 *	post commands to the UCI driving app.
 * 
 *	This runs on a Windows GUI application, which won't normally have a Win32
 *	console created on startup. So there is some complicated console junk 
 *	we have to deal with.
 * 
 */

#pragma once
#include "framework.h"
#include "uiga.h"


struct GO
{
	int cmveSearch;
	int dSearch;
	int mpcpcdtm[cpcMax];
	int mpcpcdtmInc[cpcMax];
	int dtmSearch;
	TTM ttm;
};


class CMDU;


/*
 *
 *	The UCI class. 
 *
 *	Opens the console and creates the stdin/stdout file handles and dispatches
 *	UCI commands to the appropriate place. If stdin/out is already provided by
 *	the parent process, it will use that console for in/out.
 * 
 */


class UCI
{
public:
	UIGA* puiga;
	bool fInherited, fStdinPipe;
	HANDLE hfileStdin, hfileStdout;
	map<string, CMDU*> mpszpcmdu;
public:
	UCI(UIGA* puiga);
	~UCI(void);

	int ConsolePump(void);
	bool FParseAndDispatch(const char* sz);
	void WriteSz(const string& sz);
	int FStdinAvailable(void);
	string SzReadLine(void);

	void SetPosition(const char*& sz);
	void SetPosition(void);
	void MakeMove(const string& sz);
	MV MvParse(const string& sz);
	SQ SqParse(const char*& sz);
	APC ApcParse(const char*& sz);
	string SzDecodeMv(MVE mve);
	string SzDecodeSq(SQ sq);

	void Go(const GO& go);
};


class CMDU
{
protected:
	UCI& uci;
public:
	CMDU(UCI& uci) : uci(uci) { }
	virtual int Execute(string szArg) { return 1; }
};

