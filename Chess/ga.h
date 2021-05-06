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
 *	GTM
 * 
 *	Game timing options
 * 
 */
class GTM
{
	DWORD tmGame;
	DWORD dtmMove;
public:
	GTM(void) : tmGame(3 * 60 * 1000), dtmMove(5 * 1000) { }
	DWORD TmGame(void) const {
		return tmGame;
	}
	DWORD DtmMove(void) const {
		return dtmMove;
	}
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
	UI* puiCapt;
	UI* puiHover;

public:
	BDG bdg;	// board
	PL* mpcpcppl[2];	// players
	GTM gtm;
	DWORD mpcpctmClock[2];	// player clocks
	DWORD tmLast;	// time of last move

public:
	GA(APP& app);
	~GA(void);

	void Init(void);

	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void PresentSwch(void) const;
	virtual APP& AppGet(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void InvalRcf(RCF rcf, bool fErase) const;
	virtual void Layout(void);

	virtual void DispatchCmd(int cmd);

	UI* PuiHitTest(PTF* pptf, int x, int y);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);
	virtual void SetHover(UI* pui);
	inline PL*& PlFromCpc(CPC cpc) { return mpcpcppl[cpc]; }
	inline PL* PplFromCpc(CPC cpc) { return PlFromCpc(cpc); }
	void SetPl(CPC cpc, PL* ppl);

	void Timer(UINT tid, DWORD tm);

	void Play(void);
	void NewGame(void);
	void StartGame(void);
	void EndGame(void);
	void MakeMv(MV mv, SPMV spmv);
	void SwitchClock(DWORD tmCur);
	void StartClock(CPC cpc, DWORD tmCur);
	void PauseClock(CPC cpc, DWORD tmCur);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);
	void GenRgmv(vector<MV>& rgmv);

	void Test(void);
	void ValidateFEN(const WCHAR* szFEN) const;
	void ValidatePieces(const WCHAR*& sz) const;
	void ValidateMoveColor(const WCHAR*& sz) const;
	void ValidateCastle(const WCHAR*& sz) const;
	void ValidateEnPassant(const WCHAR*& sz) const;
	void SkipWhiteSpace(const WCHAR*& sz) const;
	void SkipToWhiteSpace(const WCHAR*& sz) const;

	void PlayPGNFiles(const WCHAR szPath[]);
	int PlayPGNFile(const WCHAR szFile[]);
	int PlayPGNGame(ISTKPGN& istkpgn);
	int ReadPGNHeaders(ISTKPGN& istkpgn);
	int ReadPGNMoveList(ISTKPGN& istkpgn);
	int ReadPGNTag(ISTKPGN& istkpgn);
	int ReadPGNMove(ISTKPGN& istkpgn);
	bool FIsMoveNumber(TK* ptk, int& w) const;
	void ProcessTag(const string& szTag, const string& szVal);
	void HandleTag(int tkpgn, const string& szVal);
	void ProcessMove(const string& szMove);

	void UndoTest(void);
	int PlayUndoPGNFile(const WCHAR* szFile);
	void UndoFullGame(void);
};