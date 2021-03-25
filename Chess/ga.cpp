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


ID2D1SolidColorBrush* SPA::pbrBack;
ID2D1SolidColorBrush* SPA::pbrText;
ID2D1SolidColorBrush* SPA::pbrTextSel;
ID2D1SolidColorBrush* SPA::pbrGridLine;
ID2D1SolidColorBrush* SPA::pbrAltBack;
IDWriteTextFormat* SPA::ptfText;
IDWriteTextFormat* SPA::ptfTextSm;


/*	SPA::CreateRsrc
 *
 *	Static routine for creating the drawing objects necessary to draw the various
 *	screen panels.
 */
void SPA::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr)
{
	if (!pbrBack) {
		prt->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
		prt->CreateSolidColorBrush(ColorF(0.4f, 0.4f, 0.4f), &pbrText);
		prt->CreateSolidColorBrush(ColorF(0.8f, 0.0, 0.0), &pbrTextSel);
		prt->CreateSolidColorBrush(ColorF(.8f, .8f, .8f), &pbrGridLine);
		prt->CreateSolidColorBrush(ColorF(.95f, .95f, .95f), &pbrAltBack);
		pfactdwr->CreateTextFormat(L"Arial", NULL,
			DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			16.0f, L"",
			&ptfText);
		pfactdwr->CreateTextFormat(L"Arial", NULL,
			DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			12.0f, L"",
			&ptfTextSm);


	}
}


void SPA::DiscardRsrc(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrText);
	SafeRelease(&pbrTextSel); 
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrAltBack);
	SafeRelease(&ptfText);
	SafeRelease(&ptfTextSm);
}


void SPA::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	switch (ll) {
	case LL::Absolute:
		rcfBounds.left = ptf.x;
		rcfBounds.top = ptf.y;
		rcfBounds.right = rcfBounds.left + DxWidth();
		rcfBounds.bottom = rcfBounds.top + DyHeight();
		break;
	case LL::Right:
		rcfBounds.left = pspa->rcfBounds.right + ptf.x;
		rcfBounds.top = pspa->rcfBounds.top;
		rcfBounds.right = rcfBounds.left + DxWidth();
		rcfBounds.bottom = rcfBounds.top + DyHeight();
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
SPA::SPA(GA& ga) : ga(ga)
{
}


/*	SPA::~SPA
 *
 *	The screen panel destructor
 */
SPA::~SPA(void)
{
}


/*	SPA::RcfBounds
 *
 *	Returns the bounds rectangle in screen panel local coordinates. This
 *	means upper left corner is always (0, 0)
 */
RCF SPA::RcfBounds(void) const
{
	RCF rcf = rcfBounds;
	rcf.Offset(-rcf.left, -rcf.top);
	return rcf;
}


/*	SPA::Draw
 *
 *	Base class for drawing a screen panel. The default implementation
 *	just fills the panel with the background brush.
 */
void SPA::Draw(void)
{
	FillRcf(RcfBounds(), pbrBack);
}


/*	SPA::Redraw
 *
 *	Simple helper to redraw panel.
 */
void SPA::Redraw(void)
{
	ID2D1RenderTarget* prt = PrtGet();
	prt->BeginDraw();
	Draw();
	prt->EndDraw();
}


/*	SPA::FillRcf
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local SPA coordinates
 */
void SPA::FillRcf(RCF rcf, ID2D1Brush* pbr) const
{
	rcf.Offset(rcfBounds.left, rcfBounds.top);
	PrtGet()->FillRectangle(&rcf, pbr);
}


/*	SPA::FillEllf
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local SPA coordinates
 */
void SPA::FillEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(rcfBounds.left, rcfBounds.top);
	PrtGet()->FillEllipse(&ellf, pbr);
}


/*	SPA::PrtGet
 *
 *	Returns the Direct2D render target that will be used to draw on the 
 *	screen panel.
 */
ID2D1RenderTarget* SPA::PrtGet(void) const
{
	return ga.app.prth;
}


/*	SPA::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local SPA coordinates.
 */
void SPA::DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	rcf.Offset(rcfBounds.left, rcfBounds.top);
	PrtGet()->DrawText(sz.c_str(), (UINT32)sz.size(), ptf, &rcf, pbr==NULL ? pbrText : pbr);
}


/*	SPA::DrawBmp
 *
 *	Helper function for drawing part of a bitmap on the screen panel. Destination
 *	coordinates are in local SPA coordinates.
 */
void SPA::DrawBmp(RCF rcfTo, ID2D1Bitmap* pbmp, RCF rcfFrom, float opacity) const
{
	rcfTo.Offset(rcfBounds.left, rcfBounds.top);
	PrtGet()->DrawBitmap(pbmp, rcfTo, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rcfFrom);
}


float SPA::DxWidth(void) const
{
	return 150.0f;
}


float SPA::DyHeight(void) const
{
	return 250.0f;
}


HT* SPA::PhtHitTest(PTF ptf)
{
	if (!rcfBounds.FContainsPtf(ptf))
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
 *	SPARGMV class implementation
 * 
 *	Move list implementation
 * 
 */


SPARGMV::SPARGMV(GA& ga) : SPAS(ga), imvSel(0)
{
}


SPARGMV::~SPARGMV(void)
{
}


IDWriteTextFormat* SPARGMV::ptfList;
IDWriteTextFormat* SPARGMV::ptfClock;


/*	SPARGMV::CreateRsrc
 *
 *	Creates the drawing resources for displaying the move list screen
 *	panel. Note that this is a static routine working on global static
 *	resources that are shared by all instances of this class.
 */
void SPARGMV::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr)
{
	if (ptfList)
		return;

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		14.0f, L"",
		&ptfList);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		50.0f, L"",
		&ptfClock);
}


/*	SPARGMV::DiscardRsrc
 *
 *	Deletes all resources associated with this screen panel. This is a 
 *	static routine, and works on static class globals.
 */
void SPARGMV::DiscardRsrc(void)
{
	SafeRelease(&ptfList);
	SafeRelease(&ptfClock);
}

float SPARGMV::mpcoldxf[] = { 40.0f, 70.0f, 70.0f, 20.0f };
float SPARGMV::dyfList = 20.0f;

float SPARGMV::XfFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldxf)+1);
	float xf = 0;
	for (int colT = 0; colT < col; colT++)
		xf += mpcoldxf[colT];
	return xf;
}


/*	SPARGMV::DxfFromCol
 *
 *	Returns the width of the col column in the move list
 */
float SPARGMV::DxfFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldxf));
	return mpcoldxf[col];
}


/*	SPARGMV::RcfFromCol
 *
 *	Returns the rectangle a for a specific column and row in the move
 *	list. yf is the top of the rectangle and is typically computed relative
 *	to the content rectangle, which is in panel coordinates.
 */
RCF SPARGMV::RcfFromCol(float yf, int col) const
{
	float xf = XfFromCol(col);
	return RCF(xf, yf, xf + DxfFromCol(col), yf + dyfList);
}


/*	SPARGMV::Layout
 *
 *	Layout helper for placing the move list panel on the game screen.
 *	We basically position it to the right of the board panel.
 */
void SPARGMV::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	SPAS::Layout(ptf, pspa, ll);
	RCF rcf = rcfBounds;
	rcf.Offset(-rcf.left, -rcf.top);
	rcf.top += 5.0f * dyfList;
	rcf.bottom -= 5.0f * dyfList;
	SetView(rcf);
	SetContent(rcf);
}


/*	SPARGMV::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void SPARGMV::Draw(void)
{
	SPAS::Draw(); // draws content area of the scrollable area

	/* draw fixed part of the panel */

	/* top player */

	RCF rcf = RcfBounds();
	rcf.bottom = RcfView().top - 1;
	DrawPl(ga.spabd.cpcPointOfView, rcf, true);
	rcf.top = rcf.bottom;
	rcf.bottom += 1;
	FillRcf(rcf, pbrGridLine);

	/* bottom player */

	rcf = RcfBounds();
	rcf.top = RcfView().bottom + 1;
	DrawPl(ga.spabd.cpcPointOfView, rcf, false);
	rcf.bottom = rcf.top;
	rcf.top -= 1;
	FillRcf(rcf, pbrGridLine);
}


void SPARGMV::DrawPl(CPC cpcPointOfView, RCF rcfArea, bool fTop) const
{
	RCF rcfPl = rcfArea;
	CPC cpc = cpcPointOfView ^ (int)fTop;
	if (fTop)
		rcfPl.bottom = rcfPl.top + 1.5f * dyfList;
	else
		rcfPl.top = rcfPl.bottom - 1.5f * dyfList;

	wstring szName = ga.PplFromCpc(cpc)->SzName();
	ptfList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	wstring szColor = cpc == cpcWhite ? L"\x25ef" : L"\x2b24";
	rcfPl.left += 8.0f;
	rcfPl.top += 5.0f;
	DrawSz(szColor, ptfList, rcfPl);
	rcfPl.left += 20.0f; rcfPl.top += 3.0f;
	DrawSz(szName, ptfList, rcfPl);
	rcfPl.top -= 8.0f;

	RCF rcfClock = rcfArea;
	if (fTop)
		rcfClock.top = rcfPl.bottom;
	else
		rcfClock.bottom = rcfPl.top;
	DrawClock(cpc, rcfClock);
}


void SPARGMV::DrawClock(CPC cpc, RCF rcfArea) const
{
	FillRcf(rcfArea, pbrAltBack);
	ptfClock->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	rcfArea.top += 5.0f;
	DWORD tick = ga.mpcpctickClock[cpc];
	WCHAR sz[20], *pch;
	unsigned hr = tick / (100 * 60 * 60);
	tick = tick % (100 * 60 * 60);
	unsigned min = tick / (100 * 60);
	tick = tick % (100 * 60);
	unsigned sec = tick / 100;
	tick = tick % 100;
	unsigned frac = tick;
	pch = sz;
	if (hr > 0) {
		pch = PchDecodeInt(hr, pch);
		*pch++ = ':';
	}
	*pch++ = L'0' + min / 10;
	*pch++ = L'0' + min % 10;
	*pch++ = L':';
	*pch++ = L'0' + sec / 10;
	*pch++ = L'0' + sec % 10;
	if (hr == 0 && min == 0 && sec < 30) {
		*pch++ = L'.';
		*pch++ = L'0' + frac / 10;
	}
	*pch = 0;
	DrawSz(wstring(sz), ptfClock, rcfArea);
}


/*	SPSARGMV::RcfFromImv
 *
 *	Returns the rectangle of the imv move in the move list. Coordinates
 *	are relative to the content rectangle, which is in turn relative to
 *	the panel.
 */
RCF SPARGMV::RcfFromImv(int imv) const
{
	int rw = imv / 2;
	float yf = RcfContent().top + rw * dyfList;
	return RcfFromCol(yf, 1 + (imv % 2));
}


/*	SPARGMV::DrawContent
 *
 *	Draws the content of the scrollable part of the move list screen
 *	panel.
 */
void SPARGMV::DrawContent(const RCF& rcfCont)
{
	BDG bdgT(bdgInit);
	float yfCont = RcfContent().top;
	for (unsigned imv = 0; imv < ga.bdg.rgmvGame.size(); imv++) {
		MV mv = ga.bdg.rgmvGame[imv];
		if (imv % 2 == 0)
			DrawMoveNumber(RcfFromCol(yfCont + (imv/2)*dyfList, 0), imv/2 + 1);
		DrawMv(RcfFromImv(imv), bdgT, mv);
		bdgT.MakeMv(mv);
	}
	DrawSel(imvSel);
}


/*	SPARGMV::SetSel
 *
 *	Sets the selection
 */
void SPARGMV::SetSel(int imv)
{
	imvSel = imv;
	Redraw();
}


/*	SPARGMV::DrawSel
 *
 *	Draws the selection in the move list.
 */
void SPARGMV::DrawSel(int imv)
{
}


/*	SPARGMV::DrawMv
 *
 *	Draws the text of an individual move in the rectangle given, using the
 *	given board bdg and move mv. Note that the text of the decoded move is
 *	dependent on the board to take advantage of shorter text when there are
 *	no ambiguities. 
 */
void SPARGMV::DrawMv(RCF rcf, const BDG& bdg, MV mv)
{
	rcf.left += 4.0f;
	rcf.top += 4.0f;
	rcf.bottom -= 2.0f;
	wstring sz = bdg.SzDecodeMv(mv);
	ptfList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	DrawSz(sz, ptfList, rcf);
}


/*	SPARGMV::DrawMoveNumber
 *
 *	Draws the move number in the move list. Followed by a period. Rectangle
 *	to draw the text within is supplied by caller.
 */
void SPARGMV::DrawMoveNumber(RCF rcf, int imv)
{
	rcf.top += 4.0f;
	rcf.bottom -= 2.0f;
	rcf.right -= 4.0f;
	WCHAR sz[8];
	WCHAR* pch = PchDecodeInt(imv, sz);
	*pch++ = L'.';
	*pch = 0;
	ptfList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSz(wstring(sz), ptfList, rcf);
}


WCHAR* SPARGMV::PchDecodeInt(unsigned imv, WCHAR* pch) const
{
	if (imv / 10 != 0)
		pch = PchDecodeInt(imv / 10, pch);
	*pch++ = imv % 10 + L'0';
	*pch = '\0';
	return pch;
}


float SPARGMV::DxWidth(void) const
{
	float dxf = 0;
	for (int col = 0; col < CArray(mpcoldxf); col++)
		dxf += mpcoldxf[col];
	return dxf;
}


float SPARGMV::DyHeight(void) const
{
	return ga.spabd.DyHeight();
}


void SPARGMV::NewGame(void)
{
	bdgInit = ga.bdg;
	UpdateContSize();
}


void SPARGMV::UpdateContSize(void)
{
	SPAS::UpdateContSize(PTF(RcfContent().DxfWidth(), ga.bdg.rgmvGame.size()/2*dyfList + dyfList));
}


bool SPARGMV::FMakeVis(int imv)
{
 	return SPAS::FMakeVis(RcfContent().top+(imv/2)*dyfList, dyfList);
}


/*
 *
 *	SPATI class implementation
 * 
 *	The title screen panel
 * 
 */


SPATI::SPATI(GA& ga) : SPA(ga), szText(L"")
{
}

void SPATI::Draw(void)
{
	SPA::Draw();
	PL* pplWhite = ga.PplFromCpc(cpcWhite);
	PL* pplBlack = ga.PplFromCpc(cpcBlack);
	wstring sz = pplWhite->SzName() + L"\n v. \n" + pplBlack->SzName();
	size_t cch = sz.length();
	ptfText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	RCF rcf = RcfBounds();
	rcf.top += 40.0f;
	DrawSz(sz, ptfText, rcf);

	if (szText.size() <= 0)
		return;

	rcf = RcfBounds();
	rcf.top = rcfBounds.bottom - 36.0f;
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


void GA::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	SPA::CreateRsrc(prt, pfactdwr);
	SPABD::CreateRsrc(prt, pfactd2d, pfactdwr, pfactwic);
	SPARGMV::CreateRsrc(prt, pfactdwr);
}


void GA::DiscardRsrc(void)
{
	SPA::DiscardRsrc();
	SPABD::DiscardRsrc();
	SPARGMV::DiscardRsrc();
}


GA::GA(APP& app) : app(app), 
	spati(*this), spabd(*this), spargmv(*this),
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
	rcfBounds = RCF(0, 0, (float)dx, (float)dy);
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
void GA::Draw(void)
{
	spati.Draw();
	spabd.Draw();
	spargmv.Draw();
}


/*	GA::NewGame 
 *
 *	Starts a new game, initializing everything and redrawing the screen
 */
void GA::NewGame(void)
{
	mpcpctickClock[cpcWhite] = gtm.TickGame();
	mpcpctickClock[cpcBlack] = gtm.TickGame();
	bdg.NewGame();
	spabd.NewGame();
	spargmv.NewGame();
}


void GA::Redraw(bool fBackground)
{
	app.prth->BeginDraw();
	if (fBackground)
		app.prth->Clear(ColorF(0.5f, 0.5f, 0.5f));
	spati.Draw();
	spabd.Draw();
	spargmv.Draw();
	app.prth->EndDraw();
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


void GA::StartClock(CPC cpc, DWORD tickCur)
{
}


void GA::StopClock(CPC cpc, DWORD tickCur)
{
}


void GA::MakeMv(MV mv, bool fRedraw)
{
	StopClock(bdg.cpcToMove, 0);
	mpcpctickClock[bdg.cpcToMove] += gtm.DtickMove();
	spabd.MakeMv(mv, fRedraw);
	StartClock(bdg.cpcToMove, 0);
	if (fRedraw) {
		spargmv.UpdateContSize();
		if (!spargmv.FMakeVis((int)bdg.rgmvGame.size()-1))
			Redraw(false);
	}
}