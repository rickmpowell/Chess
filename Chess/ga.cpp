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


ID2D1SolidColorBrush* SPA::pbrTextSel;
IDWriteTextFormat* SPA::ptfTextSm;


/*	SPA::CreateRsrc
 *
 *	Static routine for creating the drawing objects necessary to draw the various
 *	screen panels.
 */
void SPA::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (pbrTextSel)
		return;
	prt->CreateSolidColorBrush(ColorF(0.8f, 0.0, 0.0), &pbrTextSel);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptfTextSm);
}


void SPA::DiscardRsrc(void)
{
	SafeRelease(&pbrTextSel); 
	SafeRelease(&pbrGridLine);
	SafeRelease(&ptfTextSm);
}


void SPA::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	SIZF sizf = SIZF(DxWidth(), DyHeight());
	PTF ptfCur;
	switch (ll) {
	case LL::Absolute:
		SetBounds(RCF(ptf, sizf));
		break;
	case LL::Right:
		ptfCur.x = pspa->rcfBounds.right + ptf.x;
		ptfCur.y = pspa->rcfBounds.top;
		SetBounds(RCF(ptfCur, sizf));
		break;
	case LL::None:
		break;
	default:
		assert(false);
		break;
	}
	SetShadow();
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


/*	SPA::DxWidth
 *
 *	Virtual function for returning the panel width, which is used for our simple
 *	panel layout system. May be ignored for certain layout options.
 */
float SPA::DxWidth(void) const
{
	return 250.0f;
}


float SPA::DyHeight(void) const
{
	return 250.0f;
}


HT* SPA::PhtHitTest(PTF ptf)
{
	RCF rcfInterior = RcfGlobalFromLocal(RcfInterior());
	if (!rcfInterior.FContainsPtf(ptf))
		return NULL;
	return new HT(ptf, HTT::Static, this);
}


void SPA::StartLeftDrag(HT* pht)
{
}


void SPA::EndLeftDrag(HT* pht)
{
}


void SPA::LeftDrag(HT* pht)
{
}

void SPA::MouseHover(HT* pht)
{
}





/*
 *
 *	SPATI class implementation
 * 
 *	The title screen panel
 * 
 */


IDWriteTextFormat* SPATI::ptfPlayers;
ID2D1Bitmap* SPATI::pbmpLogo;


 /*	SPATI::CreateRsrc
  *
  *	Creates the drawing resources for displaying the title screen
  *	panel. Note that this is a static routine working on global static
  *	resources that are shared by all instances of this class.
  */
void SPATI::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (ptfPlayers)
		return;

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptfPlayers);

	pbmpLogo = PbmpFromPngRes(idbLogo, prt, pfactwic);
}


/*	SPATI::DiscardRsrc
 *
 *	Deletes all resources associated with this screen panel. This is a
 *	static routine, and works on static class globals.
 */
void SPATI::DiscardRsrc(void)
{
	SafeRelease(&ptfPlayers);
	SafeRelease(&pbmpLogo);
}


SPATI::SPATI(GA* pga) : SPA(pga), szText(L"")
{
}


void SPATI::Draw(const RCF* prcfUpdate)
{
	SPA::Draw(prcfUpdate);
	
	/* draw the logo */

	RCF rcf = RcfInterior();
	D2D1_SIZE_F ptf = pbmpLogo->GetSize();
	RCF rcfLogo(0, 0, ptf.width, ptf.height);
	rcf.top = 10.0f;
	rcf.left = 20.0f;
	rcf.bottom = 80.0f;
	rcf.right = rcf.left + ptf.width * 70.0f / ptf.height;
	DrawBmp(rcf, pbmpLogo, rcfLogo, 1.0f);
	
	/* draw the type of game we're playing */

	rcf = RcfInterior();
	rcf.left += 80.f;
	rcf.top += 25.0f;
	DrawSz(wstring(L"Rapid \x2022 10+0 \x2022 Casual \x2022 Local Computer"), ptfPlayers, rcf);

	/* draw the players */

	PL* pplWhite = ga.PplFromCpc(cpcWhite);
	PL* pplBlack = ga.PplFromCpc(cpcBlack);
	rcf.top = 90.0f;
	rcf.left = 12.0f;
	ptfText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	DrawSz(wstring(L"\x26aa   ")+pplWhite->SzName(), ptfPlayers, rcf);
	rcf.top += 25.0f;
	DrawSz(wstring(L"\x26ab   ")+pplBlack->SzName(), ptfPlayers, rcf);

	rcf.left = 0;
	rcf.top += 25.0f;
	rcf.bottom = rcf.top + 1.0f;
	FillRcf(rcf, pbrGridLine);

	if (szText.size() <= 0)
		return;

	rcf = RcfInterior();
	rcf.top = rcf.bottom - 36.0f;
	DrawSz(szText, ptfTextSm, rcf);
}


void SPATI::SetText(const wstring& sz)
{
	szText = sz;
	Redraw();
}


/*
 *
 *	GA class
 * 
 *	Chess game implementation
 * 
 */

ID2D1SolidColorBrush* GA::pbrDesktop;


void GA::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (pbrDesktop)
		return;
	prt->CreateSolidColorBrush(ColorF(0.5f, 0.5f, 0.5f), &pbrDesktop);

	UI::CreateRsrc(prt, pfactd2d, pfactdwr, pfactwic);
	SPA::CreateRsrc(prt, pfactdwr, pfactwic);
	SPATI::CreateRsrc(prt, pfactdwr, pfactwic);
	SPABD::CreateRsrc(prt, pfactd2d, pfactdwr, pfactwic);
	UICLOCK::CreateRsrc(prt, pfactdwr, pfactwic);
	UIGO::CreateRsrc(prt, pfactdwr, pfactwic);
	SPARGMV::CreateRsrc(prt, pfactdwr, pfactwic);
}


void GA::DiscardRsrc(void)
{
	SPA::DiscardRsrc();
	SPATI::DiscardRsrc();
	SPABD::DiscardRsrc();
	SPARGMV::DiscardRsrc();
	UICLOCK::DiscardRsrc();
	UIGO::DiscardRsrc();
	SafeRelease(&pbrDesktop);
}


GA::GA(APP& app) : UI(NULL), app(app), 
	spati(this), spabd(this), spargmv(this),
	phtCapt(NULL)
{
	mpcpcppl[cpcWhite] = mpcpcppl[cpcBlack] = NULL;
	Layout();
}


GA::~GA(void)
{
	for (CPC cpc = 0; cpc < cpcMax; cpc++)
		if (mpcpcppl[cpc])
			delete mpcpcppl[cpc];
}


void GA::Resize(int dx, int dy)
{
	SetBounds(RCF(0, 0, (float)dx, (float)dy));
	Layout();
}


void GA::Layout(void)
{
	PTF ptfMargin = PTF(10.0f, 10.0f);
	spati.Layout(ptfMargin, NULL, LL::Absolute);
	spabd.Layout(ptfMargin, &spati, LL::Right);
	spargmv.Layout(ptfMargin, &spabd, LL::Right);
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


ID2D1RenderTarget* GA::PrtGet(void) const
{
	return app.prth;
}


void GA::BeginDraw(void)
{
	app.CreateRsrc();
	ID2D1RenderTarget* prt = PrtGet();
	prt->BeginDraw();
	prt->SetTransform(Matrix3x2F::Identity());
}


void GA::EndDraw(void)
{
	if (PrtGet()->EndDraw() == D2DERR_RECREATE_TARGET)
		app.DiscardRsrc();
}


/*	GA::NewGame 
 *
 *	Starts a new game, initializing everything and redrawing the screen
 */
void GA::NewGame(void)
{
	bdg.NewGame();
	spabd.NewGame();
	spargmv.NewGame();
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
	spargmv.EndGame();
}


/*	GA::PhtHitTest
 *
 *	Figures out where the point passed is hitting on the screen. If
 *	there is capture, it forces a hit on the captured panel. Returns
 *	an allocated HT on success, NULL if we hit nothing.
 */
HT* GA::PhtHitTest(PTF ptf)
{
	if (phtCapt) 
		return phtCapt->pspa->PhtHitTest(ptf);

	HT* pht;
	if ((pht = spati.PhtHitTest(ptf)) != NULL)
		return pht;
	if ((pht = spabd.PhtHitTest(ptf)) != NULL)
		return pht;
	if ((pht = spargmv.PhtHitTest(ptf)) != NULL)
		return pht;
	return NULL;
}


/*	GA::MouseMove
 *
 *	Handles mouse move messages over the game. If capture has been taken
 *	by a screen panel, we assume a button is being held down and dragging
 *	is going on. Otherwise we're just hovering.
 */
void GA::MouseMove(HT* pht)
{
	if (phtCapt)
		phtCapt->pspa->LeftDrag(pht);
	else if (pht == NULL)
		::SetCursor(app.hcurArrow);
	else {
		switch (pht->htt) {
		case HTT::Static:
			::SetCursor(app.hcurArrow);
			break;
		case HTT::MoveablePc:
			::SetCursor(app.hcurMove);
			break;
		case HTT::UnmoveablePc:
			::SetCursor(app.hcurNo);
			break;
		case HTT::EmptyPc:
		case HTT::OpponentPc:
			::SetCursor(app.hcurNo);
			break;
		default:
			::SetCursor(app.hcurArrow);
			break;
		}
		pht->pspa->MouseHover(pht);
	}
}


void GA::SetCapt(HT* pht)
{
	ReleaseCapt();
	if (pht) {
		::SetCapture(app.hwnd);
		phtCapt = pht->PhtClone();
	}
}


void GA::ReleaseCapt(void)
{
	if (phtCapt == NULL)
		return;
	delete phtCapt;
	phtCapt = NULL;
	::ReleaseCapture();
}


void GA::LeftDown(HT* pht)
{
	SetCapt(pht);
	if (pht)
		pht->pspa->StartLeftDrag(pht);
}


void GA::LeftUp(HT* pht)
{
	if (phtCapt) {
		phtCapt->pspa->EndLeftDrag(pht);
		ReleaseCapt();
	}
	else if (pht)
		pht->pspa->EndLeftDrag(pht);
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
	spargmv.mpcpcpuiclock[bdg.cpcToMove]->Redraw();
}


void GA::StartClock(CPC cpc, DWORD tmCur)
{
}


void GA::PauseClock(CPC cpc, DWORD tmCur)
{
	mpcpctmClock[cpc] -= tmCur - tmLast;
	spargmv.mpcpcpuiclock[cpc]->Redraw();
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


void GA::MakeMv(MV mv, bool fRedraw)
{
	DWORD tm = app.TmMessage();
	SwitchClock(tm == 0 ? 1 : tm);
	spabd.MakeMv(mv, fRedraw);
	if (bdg.gs != GS::Playing)
		EndGame();
	if (fRedraw) {
		spargmv.UpdateContSize();
		if (!spargmv.FMakeVis((int)bdg.rgmvGame.size()-1))
			Redraw();
	}
}


/*	GA::UndoMv
 *
 *	Moves the current move pointer back one through the move list and undoes
 *	the last move on the game board.
 */
void GA::UndoMv(bool fRedraw)
{
	spabd.UndoMv(fRedraw);
	if (fRedraw)
		spargmv.Redraw();
}


/*	GA::RedoMv
 *
 *	Moves the current move pointer forward through the move list and remakes
 *	the next move on the game board.
 */
void GA::RedoMv(bool fRedraw)
{
	spabd.RedoMv(fRedraw);
	if (fRedraw)
		spargmv.Redraw();
}