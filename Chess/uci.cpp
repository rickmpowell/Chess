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


/*	UCI::SetPosition
 *	
 *	Sets the board to the starting position
 */
void UCI::SetPosition(void)
{
	puiga->InitGame();
}


/*	UCI::SetPosition
 *
 *	Sets the board to the given FEN starting position
 */
void UCI::SetPosition(const char*& sz)
{
	puiga->InitGameFen(sz, nullptr);
}


void UCI::MakeMove(const string& szMove)
{
	MV mv = MvParse(szMove);
	puiga->MakeMv(puiga->ga.bdg.MveFromMv(mv), spmvHidden);
}


MV UCI::MvParse(const string& szMove)
{
	const char* sz = szMove.c_str();
	if (sz == "0000")
		return mvNil;
	SQ sqFrom = SqParse(sz);
	SQ sqTo = SqParse(sz);
	APC apcPromote = ApcParse(sz);
	MV mv(sqFrom, sqTo);
	mv.SetApcPromote(apcPromote);
	return mv;
}

SQ UCI::SqParse(const char*& sz)
{
	while (isspace(*sz))
		sz++;
	if (!in_range(*sz, 'a', 'h'))
		return sqNil;
	int file = *sz++ - 'a';
	if (!in_range(*sz, '1', '8'))
		return sqNil;
	int rank = *sz++ - '1';
	return SQ(rank, file);
}


APC UCI::ApcParse(const char*& sz)
{
	APC apc = apcNull;
	switch (*sz++) {
	case 'n': apc = apcKnight; break;
	case 'b': apc = apcBishop; break;
	case 'r': apc = apcRook; break;
	case 'q': apc = apcQueen; break;
	default: sz--; break;
	}
	return apc;
}


void UCI::Go(const GO& go)
{
	SPMV spmv;
	MVE mve = puiga->ga.PplToMove()->MveGetNext(spmv);
	WriteSz(string("bestmove ") + SzDecodeMv(mve));
}


string UCI::SzDecodeMv(MVE mve)
{
	if (mve.fIsNil())
		return "0000";
	string sz = SzDecodeSq(mve.sqFrom()) + SzDecodeSq(mve.sqTo());
	if (mve.apcPromote() != apcNull)
		sz += " pnbrqk "[mve.apcPromote()];
	return sz;
}

string UCI::SzDecodeSq(SQ sq)
{
	char sz[3] = { 0 };
	sz[0] = (char)('a' + sq.file());
	sz[1] = (char)('1' + sq.rank());
	return sz;
}


/*	UCI::ConsolePump
 *
 *	The console in/out pump that runs the UCI/WinBoard interface, which is simply 
 *	a stdin/stdout console.
 */
int UCI::ConsolePump(void)
{
	while (true) {
		if (FStdinAvailable()) {
			string szLine = SzReadLine();
			if (FParseAndDispatch(szLine.c_str()))
				break;
		}
		puiga->PumpMsg();
	}
	return 0;
}


string SzNextWord(const char*& sz)
{
	string szWord;
	while (isspace(*sz))
		sz++;
	if (!*sz)
		return "";
	while (*sz && !isspace(*sz))
		szWord += *sz++;
	return szWord;
}


int WNextInt(const char*& sz)
{
	return atoi(SzNextWord(sz).c_str());
}

bool UCI::FParseAndDispatch(const char* sz)
{
	/* find keyword */

	string szKey;
	do {
		szKey = SzNextWord(sz);
		if (szKey.empty())
			return false;	// unknown command, just ignore
	} while (mpszpcmdu.find(szKey) == mpszpcmdu.end());
	CMDU* pcmdu = mpszpcmdu[szKey];
	return pcmdu->Execute(sz) == 0;
}


void UCI::WriteSz(const string& sz)
{
	DWORD cb;
	string nsz = sz + "\n";
	::WriteFile(hfileStdout, nsz.c_str(), (DWORD)nsz.length(), &cb, NULL);
	Log(lgtData, lgfNormal, 0, TAG(WszWidenSz(sz)), L"");
}


int UCI::FStdinAvailable(void)
{
	DWORD cb;
	if (fStdinPipe) {
		if (!::PeekNamedPipe(hfileStdin, NULL, 0, NULL, &cb, NULL))
			cb = 1;
	}
	else {
		::GetNumberOfConsoleInputEvents(hfileStdin, &cb);
		if (cb <= 1)
			cb = 0;
	}
	return cb;
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

	Log(lgtData, lgfBold, 0, TAG(WszWidenSz(sz)), L"");
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
		const char* sz = szArg.c_str();
		szArg = SzNextWord(sz);
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
		uci.puiga->InitGameStart(nullptr);
		uci.puiga->StartGame(spmvAnimate);
		return 1;
	}
};

class CMDUPOSITION : public CMDU
{
public:
	CMDUPOSITION(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		const char* sz = szArg.c_str();
		string szCmd = SzNextWord(sz);
		if (szCmd == "startpos")
			uci.SetPosition();
		else if (szCmd == "fen")
			uci.SetPosition(sz);
		if (SzNextWord(sz) == "moves") {
			for (string szMove = SzNextWord(sz); !szMove.empty(); szMove = SzNextWord(sz))
				uci.MakeMove(szMove);
		}
		uci.puiga->Redraw();
		return 1;
	}
};

class CMDUGO : public CMDU
{
public:
	CMDUGO(UCI& uci) : CMDU(uci) { }

	virtual int Execute(string szArg)
	{
		GO go;
		go.ttm = ttmSmart;
		go.mpcpcdtm[cpcWhite] = 5 * 60 * 1000;
		go.mpcpcdtm[cpcBlack] = 5 * 60 * 1000;
		go.mpcpcdtmInc[cpcWhite] = 1000;
		go.mpcpcdtmInc[cpcBlack] = 1000;

		const char* sz = szArg.c_str();
		for (string szCmd = SzNextWord(sz); !szCmd.empty(); szCmd = SzNextWord(sz)) {
			if (szCmd == "searchmoves") {
			}
			else if (szCmd == "ponder") {
			}
			else if (szCmd == "wtime") {
				go.ttm = ttmSmart;
				go.mpcpcdtm[cpcWhite] = WNextInt(sz);
			}
			else if (szCmd == "btime") {
				go.ttm = ttmSmart;
				go.mpcpcdtm[cpcBlack] = WNextInt(sz);
			}
			else if (szCmd == "winc") {
				go.mpcpcdtmInc[cpcWhite] = WNextInt(sz);
			}
			else if (szCmd == "binc") {
				go.mpcpcdtmInc[cpcBlack] = WNextInt(sz);
			}
			else if (szCmd == "movestogo") {
			}
			else if (szCmd == "depth") {
				go.ttm = ttmConstDepth;
				go.dSearch = WNextInt(sz);
			}
			else if (szCmd == "nodes") {
				go.ttm = ttmConstNodes;
				go.cmveSearch = WNextInt(sz);
			}
			else if (szCmd == "mate") {
			}
			else if (szCmd == "movetime") {
				go.ttm = ttmTimePerMove;
				go.dtmSearch = WNextInt(sz);
			}
			else if (szCmd == "infinite") {
				go.ttm = ttmInfinite;
			}
		}

		uci.Go(go);
		
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
	/* set up stdin/out */

	fInherited = !::AllocConsole();
	hfileStdin = ::GetStdHandle(STD_INPUT_HANDLE);
	hfileStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD cb;
	fStdinPipe = !::GetConsoleMode(hfileStdin, &cb);
	if (!fStdinPipe) {
		::SetConsoleMode(hfileStdin, cb & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
		::FlushConsoleInputBuffer(hfileStdin);
	}

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


