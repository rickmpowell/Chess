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


ID2D1SolidColorBrush* UIGA::pbrDesktop;


void UIGA::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrDesktop)
		return;
	pdc->CreateSolidColorBrush(ColorF(0.5f, 0.5f, 0.5f), &pbrDesktop);

	UI::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
	UIP::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UITI::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UIBD::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
	UIML::CreateRsrcClass(pdc, pfactdwr, pfactwic);
}


void UIGA::DiscardRsrcClass(void)
{
	UI::DiscardRsrcClass();
	UIP::DiscardRsrcClass();
	UITI::DiscardRsrcClass();
	UIBD::DiscardRsrcClass();
	UIML::DiscardRsrcClass();
	SafeRelease(&pbrDesktop);
}


UIGA::UIGA(APP& app, GA& ga) : UI(nullptr), app(app), ga(ga), 
		uiti(*this), uibd(*this), uiml(*this), uipvt(*this, cpcWhite), uidb(*this), uitip(this),
	puiCapt(nullptr), puiFocus(nullptr), puiHover(nullptr),
	spmvShow(spmvAnimate), fInPlay(false), tmLast(0L), tidClock(0)
{
	mpcpctmClock[cpcWhite] = mpcpctmClock[cpcBlack] = 0;

}

UIGA::~UIGA()
{
}


/*	UIGA::Draw
 *
 *	Draws the full game on the screen. For now, we have plenty of speed
 *	to do full redraws, so there's no attempt to optimize this.
 */
void UIGA::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrDesktop);
}


void UIGA::InvalRc(const RC& rc, bool fErase) const
{
	RECT rect;
	rect.left = (int)(rc.left - rcBounds.left);
	rect.top = (int)(rc.top - rcBounds.top);
	rect.right = (int)(rc.right - rcBounds.left);
	rect.bottom = (int)(rc.bottom - rcBounds.top);
	::InvalidateRect(app.hwnd, &rect, fErase);
}


APP& UIGA::App(void) const
{
	return this->app;
}


void UIGA::BeginDraw(void)
{
	app.CreateRsrc();
	app.pdc->BeginDraw();
}


void UIGA::EndDraw(void)
{
	if (app.pdc->EndDraw() == D2DERR_RECREATE_TARGET)
		app.DiscardRsrc();
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

	rc.top = dxyMargin;
	rc.left = rc.right + dxyMargin;
	/* make board a multiple of 8 pixels wide, which makes squares an even number of pixels
	   in size, so we get consistent un-antialiased square borders */
	rc.bottom = rcBounds.bottom - 100.0f;
	rc.bottom = rc.top + max(176.0f, rc.DyHeight());
	if ((int)rc.DyHeight() & 7)
		rc.bottom = rc.top + ((int)rc.DyHeight() & ~7);
	rc.right = rc.left + rc.DyHeight();
	uibd.SetBounds(rc);

	/* move list panel */

	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uiml.SizLayoutPreferred().width;
	uiml.SetBounds(rc);

	/* debug panel */

	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uidb.SizLayoutPreferred().width;
	rc.right = max(rc.right, rcBounds.right - dxyMargin);
	uidb.SetBounds(rc);
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
	if (puiCapt)
		pui = puiCapt;
	else
		pui = PuiFromPt(pt);
	if (pui)
		pt = pui->PtLocalFromGlobal(pt);
	*ppt = pt;
	return pui;
}


void UIGA::SetCapt(UI* pui)
{
	if (pui) {
		::SetCapture(app.hwnd);
		puiCapt = pui;
	}
}


void UIGA::ReleaseCapt(void)
{
	if (puiCapt == nullptr)
		return;
	puiCapt = nullptr;
	::ReleaseCapture();
}


void UIGA::SetHover(UI* pui)
{
	if (pui == puiHover)
		return;
	puiHover = pui;
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
	return app.cmdlist.SzTip(cmd);
}


void UIGA::DispatchCmd(int cmd)
{
	app.cmdlist.Execute(cmd);
}


TID UIGA::StartTimer(UI* pui, UINT dtm)
{
	TID tid = App().StartTimer(dtm);
	mptidpui[tid] = pui;
	return tid;
}


void UIGA::StopTimer(UI* pui, TID tid)
{
	App().StopTimer(tid);
	mptidpui.erase(mptidpui.find(tid));
}


void UIGA::DispatchTimer(TID tid, UINT tmCur)
{
	if (mptidpui.find(tid) == mptidpui.end())
		return;
	mptidpui[tid]->TickTimer(tid, tmCur);
}


void UIGA::TickTimer(TID tid, UINT tmCur)
{
	CPC cpcToMove = ga.bdg.cpcToMove;
	if (ga.prule->TmGame(cpcToMove) == 0)
		return;
	DWORD dtm = tmCur - tmLast;
	if (dtm > mpcpctmClock[cpcToMove]) {
		dtm = mpcpctmClock[cpcToMove];
		ga.bdg.SetGs(cpcToMove == cpcWhite ? gsWhiteTimedOut : gsBlackTimedOut);
		ga.EndGame(spmvAnimate);
	}
	mpcpctmClock[cpcToMove] -= dtm;
	tmLast = tmCur;
	uiml.UiclockFromCpc(cpcToMove).Redraw();
}

void UIGA::NewGame(RULE* prule, SPMV spmv)
{
	ga.SetRule(prule);
	InitGame(GA::szInitFEN, spmv);
}


void UIGA::InitGame(const wchar_t* szFEN, SPMV spmv)
{
	spmvShow = spmv;

	ga.InitGame(szFEN);
	InitClocks();
	uibd.InitGame();
	uiml.InitGame();
	SetFocus(&uiml);
}


void UIGA::InitClocks(void)
{
	tmLast = 0;
	mpcpctmClock[cpcWhite] = ga.prule->TmGame(cpcWhite);
	mpcpctmClock[cpcBlack] = ga.prule->TmGame(cpcBlack);
}


void UIGA::StartClocks(void)
{
	if (ga.prule->TmGame(cpcWhite)) {
		tmLast = app.TmMessage();
		tidClock = UI::StartTimer(10);
		StartClock(ga.bdg.cpcToMove, tmLast);
	}
}

void UIGA::StartClock(CPC cpc, DWORD tmCur)
{
	if (ga.prule->TmGame(cpc) == 0)
		return;
}


void UIGA::PauseClock(CPC cpc, DWORD tmCur)
{
	if (ga.prule->TmGame(cpc) == 0)
		return;
	mpcpctmClock[cpc] -= tmCur - tmLast;
	uiml.UiclockFromCpc(cpc).Redraw();
}


void UIGA::SwitchClock(DWORD tmCur)
{
	if (ga.prule->TmGame(cpcWhite) == 0)
		return;

	if (tmLast) {
		PauseClock(ga.bdg.cpcToMove, tmCur);
		mpcpctmClock[ga.bdg.cpcToMove] += ga.prule->DtmMove(cpcWhite);
	}
	else {
		tidClock = UI::StartTimer(10);
	}
	tmLast = tmCur;
	StartClock(~ga.bdg.cpcToMove, tmCur);
}


/*	UIGA::Play
 *
 *	Starts playing the game with the current game state.
 */
int UIGA::Play(void)
{
	struct INPLAY
	{
		UIGA& uiga;
		INPLAY(UIGA& uiga) : uiga(uiga) { uiga.fInPlay = true; }
		~INPLAY() { uiga.fInPlay = false; }
	} inplay(*this);

	InitLog(2);
	LogOpen(L"Game", L"");
	ga.bdg.SetGs(gsPlaying);
	StartClocks();

	try {
		do {
			SPMV spmv = spmvAnimate;
			MV mv = ga.PplToMove()->MvGetNext(spmv);
			if (mv.fIsNil())
				throw EXINT();
			ga.MakeMv(mv, spmv);
			ga.SavePGNFile(papp->SzAppDataPath() + L"\\current.pgn");
		} while (ga.bdg.gs == gsPlaying);
	}
	catch (...) {
		LogClose(L"Game", L"Game aborted", LGF::Bold);
		papp->Error(L"Game play has been aborted.", MB_OK);
		return 1;
	}
	LogClose(L"Game", L"", LGF::Normal);
	return 0;
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
	}
}

