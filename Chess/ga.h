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
	bool fInPlay;

public:
	BDG bdg;	// board
	PL* mpcpcppl[CPC::ColorMax];	// players
	RULE* prule;
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
	 *	Modal game loop
	 */
	
	void Timer(UINT tid, DWORD tm);
	void PumpMsg(void);

	/*
	 *	Game control
	 */

	int Play(void);
	void NewGame(RULE* prule);
	void InitClocks(void);
	void StartGame(void);
	void EndGame(void);
	void MakeMv(MV mv, SPMV spmv);
	void SwitchClock(DWORD tmCur);
	void StartClock(CPC cpc, DWORD tmCur);
	void PauseClock(CPC cpc, DWORD tmCur);
	void UndoMv(void);
	void RedoMv(void);
	void MoveToImv(int imv);
	void GenRgmv(GMV& gmv);

	/*	
	 *	Logging
	 */

	virtual bool FDepthLog(LGT lgt, int& depth);
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
	virtual void ClearLog(void);
	virtual void SetDepthLog(int depth);
	virtual void InitLog(int depth);

	/*
	 *	Deserializing 
	 */

	void OpenPGNFile(const wchar_t szFile[]);
	void PlayPGNFiles(const wchar_t szPath[]);
	int PlayPGNFile(const wchar_t szFile[]);
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

	void Test(SPMV spmv);
	void ValidateFEN(const wchar_t* szFEN) const;
	void ValidatePieces(const wchar_t*& sz) const;
	void ValidateMoveColor(const wchar_t*& sz) const;
	void ValidateCastle(const wchar_t*& sz) const;
	void ValidateEnPassant(const wchar_t*& sz) const;
	void SkipWhiteSpace(const wchar_t*& sz) const;
	void SkipToWhiteSpace(const wchar_t*& sz) const;

	void UndoTest(void);
	int PlayUndoPGNFile(const wchar_t* szFile);
	void UndoFullGame(void);
};