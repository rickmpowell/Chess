#pragma once

/*
 *
 *	ga.h
 *
 *	Chess game
 *
 */

#include "framework.h"
#include "pl.h"
#include "ui.h"
#include "ml.h"
#include "uibd.h"
#include "ti.h"
#include "bd.h"
#include "debug.h"


/*
 *	Timer IDs. Must be unique per application.
 */

const UINT tidClock = 1;



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

	UI* puiCapt;
	UI* puiFocus;
	UI* puiHover;
	SPMV spmv;

public:
	BDG bdg;	// board
	PL* mpcpcppl[CPC::ColorMax];	// players
	RULE* prule;
	DWORD mpcpctmClock[CPC::ColorMax];	// player clocks
	DWORD tmLast;	// time of last move

public:
	GA(APP& app);
	~GA(void);

	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void PresentSwch(void) const;
	virtual APP& AppGet(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void InvalRcf(RCF rcf, bool fErase) const;
	virtual void Layout(void);

	virtual void DispatchCmd(int cmd);
	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTipFromCmd(int cmd) const;

	UI* PuiHitTest(PTF* pptf, int x, int y);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);
	virtual void SetHover(UI* pui);

	virtual void SetFocus(UI* pui);
	UI* PuiFocus(void) const;

	inline PL*& PlFromCpc(CPC cpc) { return mpcpcppl[cpc]; }
	inline PL* PplFromCpc(CPC cpc) { return PlFromCpc(cpc); }
	void SetPl(CPC cpc, PL* ppl);

	void Timer(UINT tid, DWORD tm);
	void PumpMsg(void);

	int Play(void);
	void NewGame(RULE* prule);
	void StartGame(void);
	void EndGame(void);
	void MakeMv(MV mv, SPMV spmv);
	void SwitchClock(DWORD tmCur);
	void StartClock(CPC cpc, DWORD tmCur);
	void PauseClock(CPC cpc, DWORD tmCur);
	void UndoMv(void);
	void RedoMv(void);
	void MoveToImv(int imv);
	void GenRgmv(vector<MV>& rgmv);

	virtual void Log(LGT lgt, const wstring& sz);

	void Test(SPMV spmv);
	void ValidateFEN(const WCHAR* szFEN) const;
	void ValidatePieces(const WCHAR*& sz) const;
	void ValidateMoveColor(const WCHAR*& sz) const;
	void ValidateCastle(const WCHAR*& sz) const;
	void ValidateEnPassant(const WCHAR*& sz) const;
	void SkipWhiteSpace(const WCHAR*& sz) const;
	void SkipToWhiteSpace(const WCHAR*& sz) const;

	void OpenPGNFile(const WCHAR szFile[]);
	void PlayPGNFiles(const WCHAR szPath[]);
	int PlayPGNFile(const WCHAR szFile[]);
	void Deserialize(istream& is);
	int DeserializeGame(ISTKPGN& istkpgn);
	int DeserializeHeaders(ISTKPGN& istkpgn);
	int DeserializeMoveList(ISTKPGN& istkpgn);
	int DeserializeTag(ISTKPGN& istkpgn);
	int DeserializeMove(ISTKPGN& istkpgn);
	bool FIsMoveNumber(TK* ptk, int& w) const;
	void ProcessTag(const string& szTag, const string& szVal);
	void ProcessTag(int tkpgn, const string& szVal);
	void ProcessMove(const string& szMove);

	void SavePGNFile(const WCHAR szFile[]);
	void Serialize(ostream& os);
	void SerializeHeaders(ostream& os);
	void SerializeHeader(ostream& os, const string& szTag, const string& szVal);
	void SerializeMoveList(ostream& os);

	void UndoTest(void);
	int PlayUndoPGNFile(const WCHAR* szFile);
	void UndoFullGame(void);
};