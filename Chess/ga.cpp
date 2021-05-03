/*
 *
 *	ga.cpp
 * 
 *	Game implementation. A game is the whole big collection of
 *	everything that goes into playing the game, including the
 *	board, players, and display.
 * 
 */

#include "Chess.h"
#include "ga.h"



/*
 * 
 *	SPA class implementation
 *
 *	Screen panel implementation. Base class for the pieces of stuff
 *	that gets displayed on the screen
 *
 */


BRS* SPA::pbrTextSel;
TX* SPA::ptxTextSm;


/*	SPA::CreateRsrcClass
 *
 *	Static routine for creating the drawing objects necessary to draw the various
 *	screen panels.
 */
void SPA::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrTextSel)
		return;
	pdc->CreateSolidColorBrush(ColorF(0.8f, 0.0, 0.0), &pbrTextSel);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxTextSm);
}


void SPA::DiscardRsrcClass(void)
{
	SafeRelease(&pbrTextSel); 
	SafeRelease(&pbrGridLine);
	SafeRelease(&ptxTextSm);
}


void SPA::SetShadow(void)
{
#ifdef LATER
	ID2D1DeviceContext* pdc;
	ID2D1Effect* peffShadow;
	pdc->CreateEffect(CLSID_D2D1Shadow, &peffShadow);
	peffShadow->SetInput(0, bitmap);

	// Shadow is composited on top of a white surface to show opacity.
	ID2D1Effect* peffFlood;
	pdc->CreateEffect(CLSID_D2D1Flood, &peffFlood);
	peffFlood->SetValue(D2D1_FLOOD_PROP_COLOR, Vector4F(1.0f, 1.0f, 1.0f, 1.0f));
	ID2D1Effect* peffAffineTrans;
	pdc->CreateEffect(CLSID_D2D12DAffineTransform, &peffAffineTrans);
	peffAffineTrans->SetInputEffect(0, peffShadow);
	D2D1_MATRIX_3X2_F mx = Matrix3x2F::Translation(20, 20));
	peffAffineTrans->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, mx);
	ID2D1Effect* peffComposite;
	pdc->CreateEffect(CLSID_D2D1Composite, &peffComposite);

	peffComposite->SetInputEffect(0, peffFlood);
	peffComposite->SetInputEffect(1, peffAffineTrans);
	peffComposite->SetInput(2, bitmap);

	pdc->DrawImage(peffComposite);
#endif
}


/*	SPA::SPA
 *
 *	The screen panel constructor. 
 */
SPA::SPA(GA* pga) : UI(pga), ga(*pga)
{
}


/*	SPA::~SPA
 *
 *	The screen panel destructor
 */
SPA::~SPA(void)
{
}


/*	SPA::Draw
 *
 *	Base class for drawing a screen panel. The default implementation
 *	just fills the panel with the background brush.
 */
void SPA::Draw(const RCF* prcfUpdate)
{
	FillRcf(RcfInterior(), pbrBack);
}





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
	SPA::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UITI::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UIBD::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
	UIML::CreateRsrcClass(pdc, pfactdwr, pfactwic);
}


void GA::DiscardRsrcClass(void)
{
	UI::DiscardRsrcClass();
	SPA::DiscardRsrcClass();
	UITI::DiscardRsrcClass();
	UIBD::DiscardRsrcClass();
	UIML::DiscardRsrcClass();
	SafeRelease(&pbrDesktop);
}


GA::GA(APP& app) : UI(NULL), app(app), 
	uiti(this), uibd(this), uiml(this),
	puiCapt(NULL), puiHover(NULL)
{
	mpcpcppl[cpcWhite] = mpcpcppl[cpcBlack] = NULL;
	tmLast = 0L;
	mpcpctmClock[cpcWhite] = mpcpctmClock[cpcBlack] = 0;
}


GA::~GA(void)
{
	for (CPC cpc = 0; cpc < cpcMax; cpc++)
		if (mpcpcppl[cpc])
			delete mpcpcppl[cpc];
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
void GA::Draw(const RCF* prcfUpdate)
{
	if (!prcfUpdate)
		prcfUpdate = &rcfBounds;
	FillRcf(*prcfUpdate, pbrDesktop);
}


void GA::InvalRcf(RCF rcf, bool fErase) const
{
	RECT rc;
	rc.left = (int)(rcf.left - rcfBounds.left);
	rc.top = (int)(rcf.top - rcfBounds.top);
	rc.right = (int)(rcf.right - rcfBounds.left);
	rc.bottom = (int)(rcf.bottom - rcfBounds.top);
	::InvalidateRect(app.hwnd, &rc, fErase);
}


APP& GA::AppGet(void) const
{
	return app;
}


void GA::BeginDraw(void)
{
	app.CreateRsrc();
	app.pdc->BeginDraw();
	app.pdc->SetTransform(Matrix3x2F::Identity());
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


/*	GA;:Layout
 *
 *	Lays out the panels on the game board
 */
void GA::Layout(void)
{
	RCF rcf(10.0f, 10.0f, 220.0f, 240.0f);
	uiti.SetBounds(rcf);

	rcf.left = rcf.right + 10.0f;
	rcf.bottom = rcfBounds.bottom - 40.0f;
	rcf.right = rcf.left + rcf.DyfHeight();
	uibd.SetBounds(rcf);

	rcf.left = rcf.right + 10.0f;
	rcf.right = rcf.left + 220.0f;
	uiml.SetBounds(rcf);
}


/*	GA::NewGame 
 *
 *	Starts a new game, initializing everything and redrawing the screen
 */
void GA::NewGame(void)
{
	bdg.NewGame();
	uibd.NewGame();
	uiml.NewGame();
	StartGame();
}


void GA::StartGame(void)
{
	tmLast = 0;
	mpcpctmClock[cpcWhite] = gtm.TmGame();
	mpcpctmClock[cpcBlack] = gtm.TmGame();
}


void GA::EndGame(void)
{
	app.DestroyTimer(tidClock);
	uiml.EndGame();
}


UI* GA::PuiHitTest(PTF* pptf, int x, int y)
{
	PTF ptf((float)x, (float)y);
	UI* pui;
	if (puiCapt)
		pui = puiCapt;
	else
		pui = PuiFromPtf(ptf);
	if (pui)
		ptf = pui->PtfLocalFromGlobal(ptf);
	*pptf = ptf;
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
	if (puiCapt == NULL)
		return;
	puiCapt = NULL;
	::ReleaseCapture();
}


void GA::SetHover(UI* pui)
{
	if (pui == puiHover)
		return;
	puiHover = pui;
}


void GA::DispatchCmd(int cmd)
{
	app.cmdlist.Execute(cmd);
}


void GA::Timer(UINT tid, DWORD tmCur)
{
	DWORD dtm = tmCur - tmLast;
	if (dtm > mpcpctmClock[bdg.cpcToMove]) {
		dtm = mpcpctmClock[bdg.cpcToMove];
		bdg.SetGs(bdg.cpcToMove == cpcWhite ? GS::WhiteTimedOut : GS::BlackTimedOut);
		EndGame();
	}
	mpcpctmClock[bdg.cpcToMove] -= dtm;
	tmLast = tmCur;
	uiml.mpcpcpuiclock[bdg.cpcToMove]->Redraw();
}


void GA::StartClock(CPC cpc, DWORD tmCur)
{
}


void GA::PauseClock(CPC cpc, DWORD tmCur)
{
	mpcpctmClock[cpc] -= tmCur - tmLast;
	uiml.mpcpcpuiclock[cpc]->Redraw();
}


void GA::SwitchClock(DWORD tmCur)
{
	if (tmLast) {
		PauseClock(bdg.cpcToMove, tmCur);
		mpcpctmClock[bdg.cpcToMove] += gtm.DtmMove();
	}
	else {
		app.CreateTimer(tidClock, 10);
	}
	tmLast = tmCur;
	StartClock(bdg.cpcToMove^1, tmCur);
}


void GA::MakeMv(MV mv, SPMV spmv)
{
	DWORD tm = app.TmMessage();
	SwitchClock(tm == 0 ? 1 : tm);
	uibd.MakeMv(mv, spmv);
	if (bdg.gs != GS::Playing)
		EndGame();
	if (spmv != SPMV::Hidden) {
		uiml.UpdateContSize();
		if (!uiml.FMakeVis((int)bdg.rgmvGame.size()-1))
			Redraw();
	}
}


/*	GA::UndoMv
 *
 *	Moves the current move pointer back one through the move list and undoes
 *	the last move on the game board.
 */
void GA::UndoMv(SPMV spmv)
{
	uibd.UndoMv(spmv);
	if (spmv != SPMV::Hidden)
		uiml.Redraw();
}


/*	GA::RedoMv
 *
 *	Moves the current move pointer forward through the move list and remakes
 *	the next move on the game board.
 */
void GA::RedoMv(SPMV spmv)
{
	uibd.RedoMv(spmv);
	if (spmv != SPMV::Hidden)
		uiml.Redraw();
}


void GA::GenRgmv(vector<MV>& rgmv)
{
	bdg.GenRgmv(rgmv, RMCHK::Remove);
}


void GA::Play(void)
{
	NewGame();
	do {
		PL* ppl = mpcpcppl[bdg.cpcToMove];
		MV mv = ppl->MvGetNext(this->bdg);
		MakeMv(mv, SPMV::Animate);
	} while (bdg.gs == GS::Playing);
}