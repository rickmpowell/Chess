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


class GA;
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
	GA* pga;
	bool fInherited;
	HANDLE hfileStdin, hfileStdout;
	map<string, CMDU*> mpszpcmdu;
public:
	UCI(GA* pga);
	~UCI(void);

	void ConsolePump(void);
	bool FParseAndDispatch(string sz);
	void WriteSz(const string& sz);
	string SzReadLine(void);
};


class CMDU
{
protected:
	UCI& uci;
public:
	CMDU(UCI& uci) : uci(uci) { }
	virtual int Execute(string szArg) { return 1; }
};

