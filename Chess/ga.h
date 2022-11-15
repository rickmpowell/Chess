/*
 *
 *	ga.h
 *
 *	Chess game
 *
 */

#pragma once

#include "ti.h"
#include "uiga.h"
#include "uibd.h"
#include "ml.h"
#include "pl.h"
#include "debug.h"
#include "uci.h"


/*
 *
 *	IGA class
 *
 *	This is the interface that defines communication from/to the GA game
 *	to various outside objects, which can include things like a graphical
 *	UI, or a UCI server.
 *
 *	It should be multiple-inherited into some other object.
 *
 */


class IGA
{
	GA& ga;
public:
	IGA(GA& ga) : ga(ga) { }
	~IGA() { }
};

class UIGA;



/*
 *
 *	PROCPGN class
 * 
 *	A little communication class to handle final processing of PGN files. A
 *	virtual class that is used to catch the tags and move list as they are
 *	read.
 * 
 */


class PROCPGN
{
protected:
	GA& ga;
public:
	PROCPGN(GA& ga);
	virtual ~PROCPGN(void) { }
	virtual ERR ProcessMv(MV mv) = 0;
	virtual ERR ProcessTag(int tkpgn, const string& szVal) = 0;
};


class PROCPGNOPEN : public PROCPGN
{
public:
	PROCPGNOPEN(GA& ga) : PROCPGN(ga) { }
	virtual ERR ProcessMv(MV mv);
	virtual ERR ProcessTag(int tkpgn, const string& szVal);
};


class PROCPGNPASTE : public PROCPGNOPEN
{
public:
	PROCPGNPASTE(GA& ga) : PROCPGNOPEN(ga) { }
	virtual ERR ProcessMv(MV mv);
	virtual ERR ProcessTag(int tkpgn, const string& szVal);
};


class PROCPGNTEST : public PROCPGNOPEN
{
public:
	PROCPGNTEST(GA& ga) : PROCPGNOPEN(ga) { }
	virtual ERR ProcessMv(MV mv);
	virtual ERR ProcessTag(int tkpgn, const string& szVal);
};


class PROCPGNTESTUNDO : public PROCPGNTEST
{
public:
	PROCPGNTESTUNDO(GA& ga) : PROCPGNTEST(ga) { }
	virtual ERR ProcessMv(MV mv);
	virtual ERR ProcessTag(int tkpgn, const string& szVal);
};


/*
 *
 *	GA class
 * 
 *	The chess game. 
 * 
 */


class GA
{
	friend class SPA;
	friend class UIGA;

public:
	BDG bdg;	// board
	BDG bdgInit;	// initial board used to start the game (used on FEN load)
	PL* mpcpcppl[cpcMax];	// players
	RULE* prule;
	PROCPGN* pprocpgn;	/* process pgn file handler */

	UIGA* puiga;	/* TODO: turn this into an interface */

public:
	GA(void);
	~GA(void);
	void SetUiga(UIGA* puiga) { this->puiga = puiga; }

	/*
	 *	Players
	 */

	inline PL*& PlFromCpc(CPC cpc) { return mpcpcppl[cpc]; }
	inline PL* PplFromCpc(CPC cpc) { return PlFromCpc(cpc); }
	inline PL* PplToMove(void) { return PplFromCpc(bdg.cpcToMove); }
	void SetPl(CPC cpc, PL* ppl);

	inline CPC CpcFromPpl(PL* ppl) const
	{
		for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
			if (mpcpcppl[cpc] == ppl)
				return cpc;
		return cpcNil;
	}

	/*
	 *	Game control
	 */

	static const wchar_t szInitFEN[];

	void NewGame(RULE* prule);
	void InitGame(const wchar_t* szFEN);
	void SetRule(RULE* prule);
	void EndGame(void);

	void MakeMv(MV mv);
	void UndoMv(void);
	void RedoMv(void);
	void MoveToImv(int64_t imv);
	void GenVemv(VEMV& vemv);

	/*
	 *	Deserializing 
	 */

	void SetProcpgn(PROCPGN* pprocpgn);
	void OpenPGNFile(const wchar_t szFile[]);
	ERR Deserialize(ISTKPGN& istkpgn);
	ERR DeserializeGame(ISTKPGN& istkpgn);
	ERR DeserializeHeaders(ISTKPGN& istkpgn);
	ERR DeserializeMoveList(ISTKPGN& istkpgn);
	ERR DeserializeTag(ISTKPGN& istkpgn);
	ERR DeserializeMove(ISTKPGN& istkpgn);
	bool FIsMoveNumber(TK* ptk, int& w) const;
	ERR ProcessTag(const string& szTag, const string& szVal);
	ERR ParseAndProcessMove(const string& szMove);
	bool FIsPgnData(const char* pch) const;

	/*
	 *	Serialization
	 */

	void SavePGNFile(const wstring& szFile);
	void Serialize(ostream& os);
	void SerializeHeaders(ostream& os);
	void SerializeHeader(ostream& os, const string& szTag, const string& szVal);
	void SerializeMoveList(ostream& os);
	void WriteSzLine80(ostream& os, string& szLine, const string& szAdd);

	uint64_t CmvPerft(int depth);
	uint64_t CmvPerftBulk(int depth);
};


struct PROCPGNGA
{
	GA& ga;

	PROCPGNGA(GA& ga, PROCPGN* pprocpgn) : ga(ga)
	{
		Set(pprocpgn);
	}

	void Set(PROCPGN* pprocpgn)
	{
		ga.SetProcpgn(pprocpgn);
	}

	~PROCPGNGA(void)
	{
		Set(nullptr);
	}
};