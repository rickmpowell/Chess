/*
 *
 *	uiga.h
 * 
 *	User interface/windowing for the chess game.
 * 
 */

#pragma once
#include "ui.h"
#include "app.h"
#include "uipa.h"
#include "uci.h"
#include "uibd.h"
#include "uiti.h"
#include "uiml.h"
#include "debug.h"
#include "ga.h"



/*
 *
 *	UIGA class
 * 
 *	The main top-level game class. This class, because it's the parent of
 *	everything else in the UI heirarchy, handles a lot of dispatching and
 *	stuff, because the UI interfaces propagate a lot of UI elements up the
 *	UI ownership chain.
 * 
 */


class UIGA : public UI
{
	friend class UITI;
	friend class UIBD;
	friend class UIML;
	friend class UIDB;
	friend class UIPVT;
	friend class UIPCP;

public:
	static TX* ptxDesktop;
	static bool FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcStatic(void);

	GA& ga;
	APP& app;
	UITI uiti;
	UIBD uibd;
	UIML uiml;
	UIDB uidb;
	UITIP uitip;
	UIPVT uipvt;
	UIPCP uipcp;
	UIDT uidt;

	UI* puiDrag;	/* mouse drag capture */
	UI* puiFocus;	/* keyboard focus */
	UI* puiHover;	/* current hover UI */
	map<TID, UI*> mptidpui;	/* windows timer id to UI mapping */

	SPMV spmvShow;	/* play speed */
	bool fInPlay;
	TID tidClock;
	DWORD mpcpcdmsecClock[cpcMax];	// player clocks
	DWORD msecLast;	// time of last move

	bool fInTest;
	bool fInterruptPumpMsg;

public:
	UIGA(APP& app, GA& ga);
	~UIGA();

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
	virtual ColorF CoBack(void) const { return coGameBack; }
	virtual ColorF CoText(void) const { return coGameText; }

	/*
	 *	Commands
	 */

	virtual void DispatchCmd(int cmd);
	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTipFromCmd(int cmd) const;
	virtual bool FEnabledCmd(int cmd) const;
	bool FInBoardSetup(void) const;
	bool FEnableCmds(void) const;

	/*
	 *	Mouse and keyboard interface
	 */

	UI* PuiHitTest(PT* ppt, int x, int y);
	virtual void SetDrag(UI* pui);
	virtual void ReleaseDrag(void);
	virtual void SetHover(UI* pui);
	virtual void SetFocus(UI* pui);
	UI* PuiFocus(void) const;
	virtual void RedrawCursor(const RC& rcUpdate);

	/*
	 *	Timers
	 */

	virtual TID StartTimer(UI* pui, DWORD dmsec);
	virtual void StopTimer(UI* pui, TID tid);
	virtual void TickTimer(TID tid, DWORD msec);
	void DispatchTimer(TID tid, DWORD msec);

	/*
	 *	Modal game loop
	 */

	void Play(MVE mve, SPMV spmv);
	void PumpMsg(void);

	void InitGame(const wchar_t* szFEN, RULE* prule);
	void InitGame(void);
	void StartGame(SPMV spmv);
	void EndGame(SPMV spmv);
	void MakeMv(MVE mve, SPMV spmvMove);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);
	void MoveToImv(int imv, SPMV spmv);

	void InitClocks(void);
	void StartClock(CPC cpc, DWORD msecCur);
	void PauseClock(CPC cpc, DWORD msecCur);

	/*
	 *	Tests and validation
	 */

	void Test(void);
	void PerftTest(void);
	void RunPerftTest(const wchar_t tag[], const wchar_t szFEN[], const uint64_t mpdepthcmv[], int depthMax, bool fDivide);
	uint64_t CmvPerftDivide(int depth);
};
