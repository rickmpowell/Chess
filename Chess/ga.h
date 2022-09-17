/*
 *
 *	ga.h
 *
 *	Chess game
 *
 */

#pragma once

#include "ti.h"
#include "uibd.h"
#include "ml.h"
#include "pl.h"
#include "debug.h"


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


class APP;

class GA : public UI
{
	friend class SPA;
	friend class UITI;
	friend class UIBD;
	friend class UIML;
	friend class UIDB;

protected:
	static BRS* pbrDesktop;

public:
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

	APP& app;
	UITI uiti;
	UIBD uibd;
	UIML uiml;
	UIDB uidb;
	UITIP uitip;

	UI* puiCapt;	/* mouse capture */
	UI* puiFocus;	/* keyboard focus */
	UI* puiHover;	/* current hover UI */
	map<TID, UI*> mptidpui;	/* windows timer id to UI mapping */

	bool fInPlay;

public:
	BDG bdg;	// board
	BDG bdgInit;	// initial board used to start the game (used on FEN load)
	PL* mpcpcppl[CPC::ColorMax];	// players
	RULE* prule;
	PROCPGN* pprocpgn;	/* process pgn file handler */
	TID tidClock;
	DWORD mpcpctmClock[CPC::ColorMax];	// player clocks
	DWORD tmLast;	// time of last move

public:
	GA(APP& app);
	~GA(void);

	/*
	 *	Players
	 */

	inline PL*& PlFromCpc(CPC cpc) 
	{
		return mpcpcppl[cpc]; 
	}
	
	inline PL* PplFromCpc(CPC cpc) 
	{
		return PlFromCpc(cpc); 
	}

	void SetPl(CPC cpc, PL* ppl);

	/*
	 *	Drawing
	 */

	virtual void Draw(const RC& rcUpdate);
	virtual void PresentSwch(void) const;
	virtual APP& App(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void InvalRc(const RC& rc, bool fErase) const;
	virtual void Layout(void);

	/*
	 *	Commands
	 */

	virtual void DispatchCmd(int cmd);
	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTipFromCmd(int cmd) const;

	/*
	 *	Mouse and keyboard interface
	 */

	UI* PuiHitTest(PT* ppt, int x, int y);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);
	virtual void SetHover(UI* pui);
	virtual void SetFocus(UI* pui);
	UI* PuiFocus(void) const;

	/*
	 *	Timers
	 */

	virtual TID StartTimer(UI* pui, UINT dtm);
	virtual void StopTimer(UI* pui, TID tid);
	virtual void TickTimer(TID tid, UINT dtm);
	void DispatchTimer(TID tid, UINT tm);

	/*
	 *	Modal game loop
	 */
	
	void PumpMsg(void);

	/*
	 *	Game control
	 */

	static const wchar_t szInitFEN[];
	int Play(void);
	void NewGame(RULE* prule, SPMV spmv);
	void InitGame(const wchar_t* szFEN, SPMV spmv);
	void InitClocks(void);
	void EndGame(SPMV spmv);
	void MakeMv(MV mv, SPMV spmv);
	void SwitchClock(DWORD tmCur);
	void StartClock(CPC cpc, DWORD tmCur);
	void PauseClock(CPC cpc, DWORD tmCur);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);
	void MoveToImv(int64_t imv, SPMV spmv);
	void GenGmv(GMV& gmv);

	/*	
	 *	Logging
	 */

	virtual bool FDepthLog(LGT lgt, int& depth);
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
	virtual void ClearLog(void);
	virtual void SetDepthLog(int depth);
	virtual void InitLog(int depth);
	virtual int DepthLog(void) const;

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

	/*
	 *	Tests and validation
	 */

	void Test(void);


	void PerftTest(void);
	void RunPerftTest(const wchar_t tag[], const wchar_t szFEN[], const uint64_t mpdepthcmv[], int depthMax, bool fDivide);
	uint64_t CmvPerft(int depth);
	uint64_t CmvPerftBulk(int depth);
	uint64_t CmvPerftDivide(int depth);
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