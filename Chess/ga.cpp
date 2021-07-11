/*
 *
 *	ga.cpp
 * 
 *	Game implementation. A game is the whole big collection of
 *	everything that goes into playing the game, including the
 *	board, players, and display.
 * 
 */

#include "ga.h"


/*
 *
 *	GA class
 * 
 *	Chess game implementation
 * 
 */


ID2D1SolidColorBrush* GA::pbrDesktop;


void GA::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
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


void GA::DiscardRsrcClass(void)
{
	UI::DiscardRsrcClass();
	UIP::DiscardRsrcClass();
	UITI::DiscardRsrcClass();
	UIBD::DiscardRsrcClass();
	UIML::DiscardRsrcClass();
	SafeRelease(&pbrDesktop);
}


GA::GA(APP& app) : UI(nullptr), app(app), 
	uiti(this), uibd(this), uiml(this), uidb(this), uitip(this), 
	puiCapt(nullptr), puiFocus(nullptr), puiHover(nullptr), spmv(SPMV::Animate), fInPlay(false), prule(nullptr)

{
	mpcpcppl[CPC::White] = mpcpcppl[CPC::Black] = nullptr;
	tmLast = 0L;
	mpcpctmClock[CPC::White] = mpcpctmClock[CPC::Black] = 0;
}


GA::~GA(void)
{
	for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc)
		if (mpcpcppl[cpc])
			delete mpcpcppl[cpc];
	if (prule)
		delete prule;
}


/*	GA::SetPl
 *
 *	Sets the player of the given color. Takes ownership of the PL, which
 *	will be freed when the game is done with it.
 */
void GA::SetPl(CPC cpc, PL* ppl)
{
	if (mpcpcppl[cpc])
		delete mpcpcppl[cpc];
	mpcpcppl[cpc] = ppl;
	uiml.SetPl(cpc, ppl);
	uiti.SetPl(cpc, ppl);
}


/*	GA::Draw
 *
 *	Draws the full game on the screen. For now, we have plenty of speed
 *	to do full redraws, so there's no attempt to optimize this.
 */
void GA::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrDesktop);
}


void GA::InvalRc(const RC& rc, bool fErase) const
{
	RECT rect;
	rect.left = (int)(rc.left - rcBounds.left);
	rect.top = (int)(rc.top - rcBounds.top);
	rect.right = (int)(rc.right - rcBounds.left);
	rect.bottom = (int)(rc.bottom - rcBounds.top);
	::InvalidateRect(app.hwnd, &rect, fErase);
}


APP& GA::App(void) const
{
	return this->app;
}


void GA::BeginDraw(void)
{
	app.CreateRsrc();
	app.pdc->BeginDraw();
}


void GA::EndDraw(void)
{
	if (app.pdc->EndDraw() == D2DERR_RECREATE_TARGET)
		app.DiscardRsrc();
	PresentSwch();
}


void GA::PresentSwch(void) const
{
	DXGI_PRESENT_PARAMETERS pp = { 0 };
	app.pswch->Present1(1, 0, &pp);
}


/*	GA::Layout
 *
 *	Lays out the panels on the game board
 */
void GA::Layout(void)
{
	float dxyMargin = 10.0f;

	RC rc(dxyMargin, dxyMargin, dxyMargin+210.0f, dxyMargin+240.0f);
	uiti.SetBounds(rc);

	rc.left = rc.right + dxyMargin;
	/* make board a multiple of 8 pixels wide, which makes squares an even number of pixels
	   in size, so we get consistent un-antialiased square borders */
	rc.bottom = rcBounds.bottom - 100.0f;
	rc.bottom = rc.top + max(176.0f, rc.DyHeight());
	if ((int)rc.DyHeight() & 7)
		rc.bottom = rc.top + ((int)rc.DyHeight() & ~7);
	rc.right = rc.left + rc.DyHeight();
	uibd.SetBounds(rc);

	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uiml.SizLayoutPreferred().width;
	uiml.SetBounds(rc);

	rc.left = rc.right + dxyMargin;
	rc.right = rc.left + uidb.SizLayoutPreferred().width;
	uidb.SetBounds(rc);
}


/*	GA::NewGame 
 *
 *	Starts a new game with the given rule set, initializing everything and redrawing 
 *	the screen
 */
void GA::NewGame(RULE* prule)
{
	if (this->prule)
		delete this->prule;
	this->prule = prule;
	
	InitClocks();
	bdg.NewGame();
	uibd.NewGame();
	uiml.NewGame();
	StartGame();
}


void GA::StartGame(void)
{
	SetFocus(&uiml);
}


void GA::InitClocks(void)
{
	tmLast = 0;
	mpcpctmClock[CPC::White] = prule->TmGame();
	mpcpctmClock[CPC::Black] = prule->TmGame();
}


void GA::EndGame(void)
{
	if (prule->TmGame())
		app.DestroyTimer(tidClock);
	if (spmv != SPMV::Hidden)
		uiml.EndGame();	
}


void GA::SetFocus(UI* pui)
{
	puiFocus = pui;
}


UI* GA::PuiFocus(void) const
{
	return puiFocus;
}


UI* GA::PuiHitTest(PT* ppt, int x, int y)
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


void GA::SetCapt(UI* pui)
{
	if (pui) {
		::SetCapture(app.hwnd);
		puiCapt = pui;
	}
}


void GA::ReleaseCapt(void)
{
	if (puiCapt == nullptr)
		return;
	puiCapt = nullptr;
	::ReleaseCapture();
}


void GA::SetHover(UI* pui)
{
	if (pui == puiHover)
		return;
	puiHover = pui;
}


void GA::ShowTip(UI* puiAttach, bool fShow)
{
	uitip.AttachOwner(puiAttach);
	uitip.Show(fShow);
	if (fShow)
		uitip.Redraw();
	else
		Redraw();
}


wstring GA::SzTipFromCmd(int cmd) const
{
	return app.cmdlist.SzTip(cmd);
}


void GA::DispatchCmd(int cmd)
{
	app.cmdlist.Execute(cmd);
}


void GA::Timer(UINT tid, DWORD tmCur)
{
	if (prule->TmGame() == 0)
		return;
	DWORD dtm = tmCur - tmLast;
	if (dtm > mpcpctmClock[bdg.cpcToMove]) {
		dtm = mpcpctmClock[bdg.cpcToMove];
		bdg.SetGs(bdg.cpcToMove == CPC::White ? GS::WhiteTimedOut : GS::BlackTimedOut);
		EndGame();
	}
	mpcpctmClock[bdg.cpcToMove] -= dtm;
	tmLast = tmCur;
	uiml.mpcpcpuiclock[bdg.cpcToMove]->Redraw();
}


void GA::StartClock(CPC cpc, DWORD tmCur)
{
	if (prule->TmGame() == 0)
		return;
}


void GA::PauseClock(CPC cpc, DWORD tmCur)
{
	if (prule->TmGame() == 0)
		return;
	mpcpctmClock[cpc] -= tmCur - tmLast;
	uiml.mpcpcpuiclock[cpc]->Redraw();
}


void GA::SwitchClock(DWORD tmCur)
{
	if (prule->TmGame() == 0)
		return;

	if (tmLast) {
		PauseClock(bdg.cpcToMove, tmCur);
		mpcpctmClock[bdg.cpcToMove] += prule->DtmMove();
	}
	else {
		app.CreateTimer(tidClock, 10);
	}
	tmLast = tmCur;
	StartClock(~bdg.cpcToMove, tmCur);
}


void GA::MakeMv(MV mv, SPMV spmvMove)
{
	DWORD tm = app.TmMessage();
	SwitchClock(tm == 0 ? 1 : tm);
	uibd.MakeMv(mv, spmvMove);
	if (bdg.gs != GS::Playing)
		EndGame();
	if (spmvMove != SPMV::Hidden) {
		uiml.UpdateContSize();
		uiml.SetSel(bdg.imvCur, spmvMove);
	}
}


/*	GA::UndoMv
 *
 *	Moves the current move pointer back one through the move list and undoes
 *	the last move on the game board.
 */
void GA::UndoMv(void)
{
	MoveToImv(bdg.imvCur - 1);
}


/*	GA::RedoMv
 *
 *	Moves the current move pointer forward through the move list and remakes
 *	the next move on the game board.
 */
void GA::RedoMv(void)
{
	MoveToImv(bdg.imvCur + 1);
}


/*	GA::MoveToImv
 *
 *	Move to the given move number in the move list, animating the board as in the
 *	current game speed mode. imv can be -1 to go to the start of the game, up to
 *	the last move the game.
 */
void GA::MoveToImv(int imv)
{
	imv = peg(imv, -1, (int)bdg.vmvGame.size() - 1);
	SPMV spmvSav = spmv;
	if (FSpmvAnimate(spmv) && abs(bdg.imvCur - imv) > 1) {
		spmv = SPMV::AnimateFast;
		if (abs(bdg.imvCur - imv) > 5)
			spmv = SPMV::AnimateVeryFast;
	}
	while (bdg.imvCur > imv)
		uibd.UndoMv(spmv);
	while (bdg.imvCur < imv)
		uibd.RedoMv(spmv);
	spmv = spmvSav;
	uiml.Layout();	// in case game over state changed
	uiml.SetSel(bdg.imvCur, spmv);
}


void GA::GenRgmv(GMV& gmv)
{
	bdg.GenRgmv(gmv, RMCHK::Remove);
}


/*	GA::Play
 *
 *	Starts playing the game with the current game state.
 */
int GA::Play(void)
{
	struct INPLAY
	{
		GA& ga;
		INPLAY(GA& ga) : ga(ga) { ga.fInPlay = true; }
		~INPLAY() { ga.fInPlay = false; }
	} inplay(*this);

	InitLog(2);
	LogOpen(L"Game", L"");
	try {
		do {
			PL* ppl = mpcpcppl[bdg.cpcToMove];
			SPMV spmvT = spmv;
			MV mv = ppl->MvGetNext(spmvT);
			assert(!mv.fIsNil());
			MakeMv(mv, spmvT);
			SavePGNFile(app.SzAppDataPath() + L"\\current.pgn");
		} while (bdg.gs == GS::Playing);
	}
	catch (int err) {
		::MessageBoxW(app.hwnd, L"Game play has been aborted.", nullptr, MB_OK);
		return err;
	}
	LogClose(L"Game", L"", LGF::Normal);
	return 0;
}


/*	GA::PumpMsg
 *
 *	Pumps and dispatches a system message. Throws an exception if the user 
 *	hit escape.
 */
void GA::PumpMsg(void)
{
	MSG msg;
	while (::PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE|PM_NOYIELD)) {
		switch (msg.message) {
		case WM_KEYDOWN:
			::PeekMessageW(&msg, msg.hwnd, msg.message, msg.message, PM_REMOVE);
			if (msg.wParam == VK_ESCAPE)
				throw -1;
			break;
		case WM_QUIT:
			throw -1;
		default:
			::PeekMessageW(&msg, msg.hwnd, msg.message, msg.message, PM_REMOVE);
			break;
		}
	if (::TranslateAccelerator(msg.hwnd, app.haccel, &msg))
		continue;
	::TranslateMessage(&msg);
	::DispatchMessageW(&msg);
	}
}


bool GA::FDepthLog(LGT lgt, int& depth)
{
	return uidb.FDepthLog(lgt, depth);
}


void GA::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	uidb.AddLog(lgt, lgf, depth, tag, szData);
}


void GA::ClearLog(void)
{
	uidb.ClearLog();
}


void GA::SetDepthLog(int depth)
{
	uidb.SetDepthLog(depth);
}


void GA::InitLog(int depth)
{
	uidb.InitLog(depth);
}
