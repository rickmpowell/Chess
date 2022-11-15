/*
 *
 *	uci.cpp
 * 
 *	Universal Chess Interface. A stdin/stdout communication protocol so we can
 *	play other engines and join tournaments.
 * 
 */

#include "uci.h"
#include "ga.h"



/*
 *
 *	UCI class
 * 
 *	The main Universal Chess Interface class. Creates a console and opens up the
 *	standard in/out handles so we can talk to a UCI GUI app.
 * 
 */


/*	UCI::ConsolePump
 */
void UCI::ConsolePump(void)
{
	while (true) {
		string szLine = SzReadLine();
		if (FParseAndDispatch(szLine))
			break;
		puiga->PumpMsg();
	}
}

string SzNextWord(const string& sz, int& ichFirst)
{
	int ichLast;
	string szWord;
	for (; ; ichFirst++) {
		if (sz[ichFirst] == 0)
			return "";
		if (!isspace(sz[ichFirst]))
			break;
	}

	for (ichLast = ichFirst; sz[ichLast + 1] && !isspace(sz[ichLast + 1]); ichLast++)
		;
	szWord = sz.substr(ichFirst, ichLast - ichFirst + 1);
	ichFirst = ichLast + 1;
	return szWord;
}


bool UCI::FParseAndDispatch(string sz)
{
	/* find keyword */

	int ich = 0;
	string szKey;
	do {
		szKey = SzNextWord(sz, ich);
		if (szKey.empty())
			return false;	// unknown command, just ignore
	} while (mpszpcmdu.find(szKey) == mpszpcmdu.end());
	CMDU* pcmdu = mpszpcmdu[szKey];
	return pcmdu->Execute(sz.substr(ich)) == 0;
}


void UCI::WriteSz(const string& sz)
{
	DWORD cb;
	string nsz = sz + "\n";
	::WriteFile(hfileStdout, nsz.c_str(), (DWORD)nsz.length(), &cb, NULL);
	Log(LGT::Data, LGF::Normal, 0, TAG(WszWidenSz(sz)), L"");
}


string UCI::SzReadLine(void)
{
	DWORD cch;
	char ch;

	/* skip leading whitespace, including end of lines */

	do {
		if (!::ReadFile(hfileStdin, &ch, 1, &cch, NULL))
			throw 1;
	} while (isspace(ch));

	/* put the rest of the line in a string */

	string sz;
	sz.push_back(ch);
	while (1) {
		if (!::ReadFile(hfileStdin, &ch, 1, &cch, NULL))
			throw 1;
		if (ch == '\r' || ch == '\n')
			break;
		sz.push_back(ch);
	}

	Log(LGT::Data, LGF::Bold, 0, TAG(WszWidenSz(sz)), L"");
	return sz;
}

/*	uci command */

class CMDUUCI : public CMDU
{
public:
	CMDUUCI(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		uci.WriteSz("id name SQ Chess 0.1.20221110");
		uci.WriteSz("id author Rick Powell");
		uci.WriteSz("uciok");
		return 1;
	}
};

class CMDUDEBUG : public CMDU
{
public:
	CMDUDEBUG(UCI& uci) : CMDU(uci) { }

	int Execute(string szArg)
	{
		int ich = 0;
		szArg = SzNextWord(szArg, ich);
		if (szArg == "on") {
			;
		}
		else if (szArg == "off") {
			;
		}
		return 1;
	}
};

class CMDUISREADY : public CMDU
{
public:
	CMDUISREADY(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		uci.WriteSz("readyok");
		return 1;
	}
};

class CMDUSETOPTION : public CMDU
{
public:
	CMDUSETOPTION(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 1;
	}
};

class CMDUREGISTER : public CMDU
{
public:
	CMDUREGISTER(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 1;
	}
};

class CMDUUCINEWGAME : public CMDU
{
public:
	CMDUUCINEWGAME(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		uci.puiga->NewGame(new RULE, spmvAnimateFast);
		return 1;
	}
};

class CMDUPOSITION : public CMDU
{
public:
	CMDUPOSITION(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		int ich = 0;
		string szCmd = SzNextWord(szArg, ich);

		if (szCmd == "startpos") {
		}
		else if (szCmd == "fen") {
		}
		return 1;
	}
};

class CMDUGO : public CMDU
{
public:
	CMDUGO(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 1;
	}
};

class CMDUSTOP : public CMDU
{
public:
	CMDUSTOP(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 1;
	}
};

class CMDUPONDERHIT : public CMDU
{
public:
	CMDUPONDERHIT(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 1;
	}
};

class CMDUQUIT : public CMDU
{
public:
	CMDUQUIT(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		return 0;
	}
};


UCI::UCI(UIGA* puiga) : puiga(puiga), hfileStdin(NULL), hfileStdout(NULL)
{
	fInherited = !::AllocConsole();
	hfileStdin = ::GetStdHandle(STD_INPUT_HANDLE);
	hfileStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);

	/* uci command dispatch */

	mpszpcmdu["uci"] = new CMDUUCI(*this);
	mpszpcmdu["debug"] = new CMDUDEBUG(*this);
	mpszpcmdu["isready"] = new CMDUISREADY(*this);
	mpszpcmdu["setoption"] = new CMDUSETOPTION(*this);
	mpszpcmdu["register"] = new CMDUREGISTER(*this);
	mpszpcmdu["ucinewgame"] = new CMDUUCINEWGAME(*this);
	mpszpcmdu["position"] = new CMDUPOSITION(*this);
	mpszpcmdu["go"] = new CMDUGO(*this);
	mpszpcmdu["stop"] = new CMDUSTOP(*this);
	mpszpcmdu["ponderhit"] = new CMDUPONDERHIT(*this);
	mpszpcmdu["quit"] = new CMDUQUIT(*this);
}


UCI::~UCI(void)
{
	if (!fInherited)
		::FreeConsole();
}


