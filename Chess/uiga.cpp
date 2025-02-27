/*
 *
 *	uiga.cpp
 * 
 *	The game user-interface. The "game" is the top-level ui element of the
 *	whole program. This is just the user-interface part of the game and doesn't
 *	include the abstract game information that can be driven externally by a
 *	non-graphical driver.
 * 
 */

#include "uiga.h"



UIGA::UIGA(APP& app, GA& ga) :	UI(nullptr),
								app(app), ga(ga),
								uiti(*this), uibd(*this), uiml(*this), uipvt(*this, cpcWhite), uidb(*this), uipcp(*this), uidt(*this),
								uitip(this),
								puiDrag(nullptr), puiFocus(nullptr), puiHover(nullptr),
								ptxDesktop(nullptr),
								spmvShow(spmvAnimate), fInPlay(false), msecLast(0L), tidClock(0), fInTest(false), fInterruptPumpMsg(false)
{
	mpcpcdmsecClock[cpcWhite] = mpcpcdmsecClock[cpcBlack] = 0;
	uitip.BringToTop();
}


UIGA::~UIGA()
{
	DiscardRsrc();
}


bool UIGA::FCreateRsrc(void)
{
	if (ptxDesktop)
		return false;
	ptxDesktop = PtxCreate(40.0f, true, false);
	return true;
}


void UIGA::DiscardRsrc(void)
{
	SafeRelease(&ptxDesktop);
}


/*	UIGA::Draw
 *
 *	Draws the full game on the screen. For now, we have plenty of speed
 *	to do full redraws, so there's no attempt to optimize this.
 */
void UIGA::Draw(const RC& rcUpdate)
{
#ifndef NDEBUG
	RC rc(0, -15.0f, 0, 0);
	SIZ siz = SizFromSz(L"DEBUG", ptxDesktop, rcBounds.DxWidth(), rcBounds.DyHeight());
	rc.right = rc.left + siz.width + 10.0f;
	rc.bottom = rc.top + siz.height - 2.0f;
	while (rc.top < rcBounds.bottom) {
		DrawSz(L"DEBUG", ptxDesktop, rc);
		rc.Offset(rc.DxWidth(), 0);
		if (rc.left > rcBounds.right)
			rc.Offset(-rc.left + 10.0f, rc.DyHeight());
	}
#endif
}


void UIGA::InvalRc(const RC& rc, bool fErase) const
{
	RECT rect = {
		(int)floor(rc.left - rcBounds.left),
		(int)floor(rc.top - rcBounds.top),
		(int)ceil(rc.right - rcBounds.left),
		(int)ceil(rc.bottom - rcBounds.top) };
	::InvalidateRect(app.hwnd, &rect, fErase);
}


APP& UIGA::App(void) const
{
	return this->app;
}


void UIGA::BeginDraw(void)
{
	app.FCreateRsrc();
	if (APP::FCreateRsrcStatic(app.pdc, app.pfactd2, app.pfactdwr, app.pfactwic))
		EnsureRsrc(true);
	app.pdc->BeginDraw();
}


void UIGA::EndDraw(void)
{
	if (app.pdc->EndDraw() == D2DERR_RECREATE_TARGET) {
		APP::DiscardRsrcStatic();
		DiscardAllRsrc();
	}
	PresentSwch();
}


void UIGA::PresentSwch(void) const
{
	DXGI_PRESENT_PARAMETERS pp = { 0 };
	app.pswch->Present1(1, 0, &pp);
}


/*	UIGA::Layout
 *
 *	Lays out the panels on the game board
 */
void UIGA::Layout(void)
{
	float dxyMargin = 10.0f;

	/* title box panel */

	RC rc(dxyMargin, dxyMargin, dxyMargin + 210.0f, dxyMargin + 240.0f);
	uiti.SetBounds(rc);

	/* piece value table panel */

	rc.top = rc.bottom + dxyMargin;
	rc.bottom = rcBounds.bottom - dxyMargin;
	uipvt.SetBounds(rc);

	/* board panel */

	RC rcBoard;
	rcBoard.top = dxyMargin;
	rcBoard.left = rc.right + dxyMargin;
	/* make board a multiple of 8 pixels wide, which makes squares an even number of pixels
	   in size, so we get consistent un-antialiased square borders */
	rcBoard.bottom = rcBounds.bottom - 180.0f;
	rcBoard.bottom = rcBoard.top + max(176.0f, rcBoard.DyHeight());
	if ((int)rcBoard.DyHeight() & 7)
		rcBoard.bottom = rcBoard.top + ((int)rcBoard.DyHeight() & ~7);
	rcBoard.right = rcBoard.left + rcBoard.DyHeight();
	uibd.SetBounds(rcBoard);

	/* board edit piece placement panel */

	rc = rcBoard;
	rc.top = rc.bottom + dxyMargin;
	rc.bottom = rcBounds.bottom - dxyMargin;
	rc.right = rc.left + 1200.0f;
	uipcp.SetBounds(rc);

	/* move list panel */

	rc = rcBoard;
	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uiml.SizLayoutPreferred().width;
	uiml.SetBounds(rc);

	/* debug panel */

	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uidb.SizLayoutPreferred().width;
	rc.right = max(rc.right, rcBounds.right - dxyMargin);
	uidb.SetBounds(rc);

	/* draw test window just goes on top */

	rc = RcInterior();
	rc.Inflate(-220, -80);
	uidt.SetBounds(rc);

}


void UIGA::SetFocus(UI* pui)
{
	puiFocus = pui;
}


UI* UIGA::PuiFocus(void) const
{
	return puiFocus;
}


UI* UIGA::PuiHitTest(PT* ppt, int x, int y)
{
	PT pt((float)x, (float)y);
	UI* pui;
	if (puiDrag)
		pui = puiDrag;
	else
		pui = PuiFromPt(pt);
	if (pui)
		pt = pui->PtLocalFromGlobal(pt);
	*ppt = pt;
	return pui;
}


void UIGA::SetDrag(UI* pui)
{
	if (pui) {
		::SetCapture(app.hwnd);
		puiDrag = pui;
	}
}


void UIGA::ReleaseDrag(void)
{
	if (puiDrag == nullptr)
		return;
	puiDrag = nullptr;
	::ReleaseCapture();
}


void UIGA::SetHover(UI* pui)
{
	if (pui == puiHover)
		return;
	puiHover = pui;
}


void UIGA::RedrawCursor(const RC& rcUpdate)
{
	if (!puiDrag)
		return;
	puiDrag->DrawCursor(this, rcUpdate);
}


void UIGA::SetDefCursor(void)
{
	App().SetCursor(App().hcurArrow);
}


void UIGA::ShowTip(UI* puiAttach, bool fShow)
{
	uitip.AttachOwner(puiAttach);
	uitip.Show(fShow);
	if (fShow)
		uitip.Redraw();
	else
		Redraw();
}


wstring UIGA::SzTipFromCmd(int cmd) const
{
	return app.vcmd.SzTip(cmd);
}


void UIGA::DispatchCmd(int cmd)
{
	app.vcmd.Execute(cmd);
}


bool UIGA::FEnabledCmd(int cmd) const
{
	return app.vcmd.FEnabled(cmd);
}


/*	UIGA::FEnableCmds
 *
 *	There are states where we need to bulk disable a lot of the UI. This function is that
 *	bulk state. Any commands that need to work outside this state need to override
 *	CMD::FEnabled, and any commands that override CMD::FEnabled need to take this state
 *	into consideration.
 */
bool UIGA::FEnableCmds(void) const
{
	return !fInTest;
}


/*	UIGA::FInBoardSetup
 *
 *	Returns true if we're in board setup mode. Game play is blocked while we're in this
 *	mode.
 */
bool UIGA::FInBoardSetup(void) const
{
	return uipcp.FVisible();
}


TID UIGA::StartTimer(UI* pui, DWORD dmsec)
{
	TID tid = App().StartTimer(dmsec);
	mptidpui[tid] = pui;
	return tid;
}


void UIGA::StopTimer(UI* pui, TID tid)
{
	App().StopTimer(tid);
	mptidpui.erase(mptidpui.find(tid));
}


void UIGA::DispatchTimer(TID tid, DWORD msecCur)
{
	if (mptidpui.find(tid) == mptidpui.end())
		return;
	mptidpui[tid]->TickTimer(tid, msecCur);
}


void UIGA::TickTimer(TID tid, DWORD msecCur)
{
	CPC cpcToMove = ga.bdg.cpcToMove;
	if (ga.prule->FUntimed())
		return;
	DWORD dmsec = msecCur - msecLast;
	if (dmsec > mpcpcdmsecClock[cpcToMove]) {
		dmsec = mpcpcdmsecClock[cpcToMove];
		ga.bdg.SetGs(cpcToMove == cpcWhite ? gsWhiteTimedOut : gsBlackTimedOut);
		uiml.UiclockFromCpc(cpcToMove).Flag(true);
		EndGame(spmvAnimate);
	}
	mpcpcdmsecClock[cpcToMove] -= dmsec;
	msecLast = msecCur;
	uiml.UiclockFromCpc(cpcToMove).Redraw();
}


/*	UIGA::InitGameEpd
 *
 *	Initializes a game with the given EPD starting positoin and rules. szEpd 
 *	and prule may be null for default values.
 *
 *	Game is not started. Call StartGame to get the game moving.
 */
void UIGA::InitGameEpd(const char* sz, RULE* prule)
{
	ga.InitGameFen(sz, prule);
	InitClocks();
	uibd.InitGame();
	uibd.SetEpdProperties(sz);
	uiml.InitGame();
	SetFocus(&uiml);
}


void UIGA::InitGameFen(const char*& sz, RULE* prule)
{
	ga.InitGameFen(sz, prule);
	InitClocks();
	uibd.InitGame();
	uiml.InitGame();
	SetFocus(&uiml);
}


void UIGA::InitGameStart(RULE* prule)
{
	ga.InitGameStart(prule);
	InitClocks();
	uibd.InitGame();
	uiml.InitGame();
	SetFocus(&uiml);
}

void UIGA::InitGame(void)
{
	ga.InitGame();
	InitClocks();
	uibd.InitGame();
	uiml.InitGame();
	SetFocus(&uiml);
}


/*	UIGA::StartGame
 *
 *	Starts the game going, which includes turning on the clocks
 */
void UIGA::StartGame(SPMV spmv)
{
	spmvShow = spmv;
	ga.StartGame();
	uibd.StartGame();	
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; cpc++)
		ga.mpcpcppl[cpc]->StartGame();

	tidClock = StartTimer(this, 10);
	msecLast = app.MsecMessage();
	StartClock(ga.bdg.cpcToMove, app.MsecMessage());
	uiml.Relayout();
	uiml.Redraw();
}


/*	UIGA::EndGame
 *
 *	Ends the game, after a draw or someone wins.
 */
void UIGA::EndGame(SPMV spmv)
{
	if (tidClock) {
		app.StopTimer(tidClock);
		tidClock = 0;
	}
	if (spmv != spmvHidden)
		uiml.EndGame();
}


/*	UIGA::InitClocks
 *
 *	Sets up the clocks in preparation of the game starting
 */
void UIGA::InitClocks(void)
{
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc) {
		mpcpcdmsecClock[cpc] = ga.prule->DmsecAddBlock(cpc, 1);
		ga.SetTimeRemaining(cpc, mpcpcdmsecClock[cpc]);
		uiml.UiclockFromCpc(cpc).Flag(false);
	}
}


/*	UIGA::StartClock
 *
 *	Starts the given player's clock.
 */
void UIGA::StartClock(CPC cpc, DWORD msecCur)
{
	if (ga.prule->FUntimed())
		return;
	msecLast = msecCur;
	ga.SetTimeRemaining(cpc, mpcpcdmsecClock[cpc]);
}


/*	UIGA::PauseClock
 *
 *	Pauses the given player's clock at the end of his turn. msecCur is
 *	used to adjust time remaining until we flag. Adjusts the clocks
 *	in preparation for the move about to be made.
 * 
 *	The move has not yet happened when this is called. 
 */
void UIGA::PauseClock(CPC cpc, DWORD msecCur)
{
	if (ga.prule->FUntimed())
		return;
	mpcpcdmsecClock[cpc] -= msecCur - msecLast;
	int nmvThis = ga.bdg.vmveGame.NmvFromImv(ga.bdg.imveCurLast+1);
	mpcpcdmsecClock[cpc] +=
		ga.prule->DmsecAddMove(cpc, nmvThis) + ga.prule->DmsecAddBlock(cpc, nmvThis+1);
	ga.SetTimeRemaining(cpc, mpcpcdmsecClock[cpc]);
	uiml.UiclockFromCpc(cpc).Redraw();
}


void UIGA::MakeMv(MVE mve, SPMV spmvMove)
{
	DWORD msec = app.MsecMessage();
	PauseClock(ga.bdg.cpcToMove, msec);
	StartClock(~ga.bdg.cpcToMove, msec);
	uibd.MakeMv(mve, spmvMove);
	if (!ga.bdg.FGsPlaying())
		EndGame(spmvMove);
	if (spmvMove != spmvHidden) {
		uiml.UpdateContSize();
		uiml.SetSel(ga.bdg.imveCurLast, spmvMove);
	}
}


/*	UIGA::UndoMv
 *
 *	Moves the current move pointer back one through the move list and undoes
 *	the last move on the game board.
 */
void UIGA::UndoMv(SPMV spmv)
{
	MoveToImv(ga.bdg.imveCurLast - 1, spmv);
}


/*	UIGA::RedoMv
 *
 *	Moves the current move pointer forward through the move list and remakes
 *	the next move on the game board.
 */
void UIGA::RedoMv(SPMV spmv)
{
	MoveToImv(ga.bdg.imveCurLast + 1, spmv);
}


/*	GA::MoveToImv
 *
 *	Move to the given move number in the move list, animating the board as in the
 *	current game speed mode. imv can be -1 to go to the start of the game, up to
 *	the last move the game.
 */
void UIGA::MoveToImv(int imv, SPMV spmv)
{
	imv = clamp(imv, -1, ga.bdg.vmveGame.size() - 1);
	if (FSpmvAnimate(spmv) && abs(ga.bdg.imveCurLast - imv) > 1) {
		spmv = spmvAnimateFast;
		if (abs(ga.bdg.imveCurLast - imv) > 5)
			spmv = spmvAnimateVeryFast;
	}
	while (ga.bdg.imveCurLast > imv)
		uibd.UndoMv(spmv);
	while (ga.bdg.imveCurLast < imv)
		uibd.RedoMv(spmv);
	if (spmv != spmvHidden)
		uiml.Relayout();	// in case game over state changed
	uiml.SetSel(ga.bdg.imveCurLast, spmv);
}


/*	UIGA::Play
 *
 *	Starts playing the game from the current game state. First move to
 *	play is sent in mv, which may be nil. spmv is the speed of the 
 *	animation to use on the board.
 */
void UIGA::Play(MVE mve, SPMV spmv)
{
	fInPlay = true;
	InitLog();
	LogOpen(L"Game", L"", lgfBold);
	StartGame(spmvAnimate);

	if (!mve.fIsNil())
		MakeMv(mve, spmv);

	do {
		SPMV spmv = spmvAnimate;
		/* TEMPORARY - until we're threaded, we need to restore the board if user accidentally clicked 
		   undo or clicked on the move list while the AI is thinking; the AI generates a move for the
		   board that we were in at the start */
		int imveSav = ga.bdg.imveCurLast;
		mve = ga.PplToMove()->MveGetNext(spmv);
		MoveToImv(imveSav, spmvFast);
		if (mve.fIsNil())
			break;
		MakeMv(mve, spmv);
		ga.SavePGNFile(papp->SzAppDataPath() + L"\\current.pgn");
	} while (ga.bdg.FGsPlaying());

	if (ga.bdg.FGsCanceled()) {
		papp->Error(L"Game has been canceled.", MB_OK);
		LogClose(L"Game", L"canceled", lgfItalic);
	}
	else {
		LogClose(L"Game", L"over", lgfNormal);
	}

	fInPlay = false;
}


/*	UIGA::PumpMsg
 *
 *	Pumps and dispatches a system message. Throws an exception if the user
 *	hit escape.
 */
void UIGA::PumpMsg(void)
{
	MSG msg;
	while (::PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE | PM_NOYIELD)) {
		switch (msg.message) {
		case WM_TIMER:
			::PeekMessageW(&msg, msg.hwnd, msg.message, msg.message, PM_REMOVE);
			DispatchTimer(msg.wParam, msg.time);
			continue;
		case WM_QUIT:
			throw EXINT();
		default:
			::PeekMessageW(&msg, msg.hwnd, msg.message, msg.message, PM_REMOVE);
			if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
				throw EXINT();
			break;
		}
		if (::TranslateAccelerator(msg.hwnd, app.haccel, &msg))
			continue;
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
		if (fInTest && fInterruptPumpMsg) {
			fInterruptPumpMsg = false;
			throw EXINT();
		}
	}
}

