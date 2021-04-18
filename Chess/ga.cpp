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

WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch)
{
	if (imv / 10 != 0)
		pch = PchDecodeInt(imv / 10, pch);
	*pch++ = imv % 10 + L'0';
	*pch = '\0';
	return pch;
}


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
 *	UICLOCK implementation
 * 
 *	The chess clock implementation
 * 
 */


IDWriteTextFormat* UICLOCK::ptfClock;

void UICLOCK::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (ptfClock)
		return;
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 50.0f, L"",
		&ptfClock);
}


void UICLOCK::DiscardRsrc(void)
{
	SafeRelease(&ptfClock);
}


UICLOCK::UICLOCK(SPARGMV* pspargmv, CPC cpc) : UI(pspargmv), ga(pspargmv->ga), cpc(cpc)
{
}


void UICLOCK::Draw(const RCF* prcfUpdate)
{
	DWORD tm = ga.mpcpctmClock[cpc];

	/* fill edges and background */

	RCF rcf = RcfInterior();
	D2D1_COLOR_F coSav = pbrAltBack->GetColor();
	if (FTimeOutWarning(tm))
		pbrAltBack->SetColor(ColorF(1.0f, 0.9f, 0.9f));
	FillRcf(rcf, pbrAltBack);
	pbrAltBack->SetColor(coSav);
	FillRcf(RCF(rcf.left, rcf.top, rcf.right, rcf.top+1), pbrGridLine);
	FillRcf(RCF(rcf.left, rcf.bottom - 1, rcf.right, rcf.bottom), pbrGridLine);
	rcf.top += 11.0f;
	
	/* break down time into parts */

	unsigned hr = tm / (1000 * 60 * 60);
	tm = tm % (1000 * 60 * 60);
	unsigned min = tm / (1000 * 60);
	tm = tm % (1000 * 60);
	unsigned sec = tm / 1000;
	tm = tm % 1000;
	unsigned frac = tm;

	/* convert into text */

	WCHAR sz[20], * pch;
	pch = sz;
	pch = PchDecodeInt(hr, pch);
	*pch++ = ':';
	*pch++ = L'0' + min / 10;
	*pch++ = L'0' + min % 10;
	*pch++ = L':';
	*pch++ = L'0' + sec / 10;
	*pch++ = L'0' + sec % 10;
	*pch++ = L'.';
	*pch++ = L'0' + frac / 100;
	*pch = 0;
	
	/* print out the text piece at a time */

	ptfClock->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	const float dxfDigit = 30.0f;
	const float dxfPunc = 18.0f;
	if (hr > 0) {
		DrawRgch(sz, 1, ptfClock, rcf);	// hours
		rcf.left += dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 2, 2, ptfClock, rcf); // minutes
		rcf.left += 2 * dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptfClock, rcf); // seconds
	}
	else if (min > 0) {
		rcf.left += 30.0f;
		DrawRgch(sz + 2, 2, ptfClock, rcf);	// minutes
		rcf.left += 2*dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptfClock, rcf);	// seconds 
	}
	else {
		rcf.left += 23.0f;
		DrawRgch(sz + 3, 1, ptfClock, rcf);	// minutes (=0)
		rcf.left += dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 4, ptfClock, rcf);	// seconds and tenths
	}
}


bool UICLOCK::FTimeOutWarning(DWORD tm) const
{
	return tm < 1000 * 20;
}

void UICLOCK::DrawColon(RCF& rcf, unsigned frac) const
{
	if (frac < 500 && cpc == ga.bdg.cpcToMove)
		pbrText->SetOpacity(0.33f);
	rcf.top -= 3.0f;
	DrawSz(L":", ptfClock, rcf, pbrText);
	pbrText->SetOpacity(1.0f);
	rcf.top += 3.0f;
	rcf.left += 18.0f;
}


/*
 *
 *	UIGO implementation
 * 
 *	Game over sub-panel
 * 
 */

IDWriteTextFormat* UIGO::ptfScore;

void UIGO::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (ptfScore)
		return;
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"",
		&ptfScore);
}


void UIGO::DiscardRsrc(void)
{
	SafeRelease(&ptfScore);
}


UIGO::UIGO(SPARGMV* pspargmv, bool fVisible) : UI(pspargmv, fVisible), ga(pspargmv->ga)
{
}


void UIGO::Draw(const RCF* prcfUpdate)
{
	RCF rcfEndType = RcfInterior();
	rcfEndType.bottom = rcfEndType.top + 20.0f;
	rcfEndType.Offset(0, 20.0f);
	RCF rcfResult = rcfEndType;
	rcfResult.Offset(0, 20.0f);
	RCF rcfScore = rcfResult;
	rcfScore.Offset(0, 20.0f);
	CPC cpcWin = cpcNil;

	switch (ga.bdg.gs) {
	case GS::WhiteCheckMated:
		DrawSzCenter(L"Checkmate", ptfText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackCheckMated:
		DrawSzCenter(L"Checkmate", ptfText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::WhiteResigned:
		DrawSzCenter(L"White Resigned", ptfText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackResigned:
		DrawSzCenter(L"Black Resigned", ptfText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::WhiteTimedOut:
		DrawSzCenter(L"Time Expired", ptfText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackTimedOut:
		DrawSzCenter(L"Time Expired", ptfText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::StaleMate:
		DrawSzCenter(L"Stalemate", ptfText, rcfEndType);
		break;
	case GS::Draw3Repeat:
		DrawSzCenter(L"3-Fold Repitition", ptfText, rcfEndType);
		break;
	case GS::Draw50Move:
		DrawSzCenter(L"50-Move", ptfText, rcfEndType);
		break;
	case GS::DrawAgree:
		DrawSzCenter(L"Draw Agreed", ptfText, rcfEndType);
		break;
	case GS::DrawDead:
		DrawSzCenter(L"Dead Position", ptfText, rcfEndType);
		break;
	default:
		assert(false);
		break;
	}

	const WCHAR* szResult = L"Draw";
	const WCHAR* szScore = L"\x00bd-\x00bd";
	if (cpcWin == cpcWhite) {
		szResult = L"White Wins";
		szScore = L"1-0";
	}
	else if (cpcWin == cpcBlack) {
		szResult = L"Black Wins";
		szScore = L"0-1";
	}
	DrawSzCenter(szResult, ptfText, rcfResult);
	DrawSzCenter(szScore, ptfScore, rcfScore);
}


/*
 *
 *	SPARGMV class implementation
 * 
 *	Move list implementation
 * 
 */


SPARGMV::SPARGMV(GA* pga) : SPAS(pga), imvSel(0)
{
	mpcpcpuiclock[cpcWhite] = new UICLOCK(this, cpcWhite);
	mpcpcpuiclock[cpcBlack] = new UICLOCK(this, cpcBlack);
	puigo = new UIGO(this, false);
}


SPARGMV::~SPARGMV(void)
{
}


IDWriteTextFormat* SPARGMV::ptfList;


/*	SPARGMV::CreateRsrc
 *
 *	Creates the drawing resources for displaying the move list screen
 *	panel. Note that this is a static routine working on global static
 *	resources that are shared by all instances of this class.
 */
void SPARGMV::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (ptfList)
		return;

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"",
		&ptfList);
}


/*	SPARGMV::DiscardRsrc
 *
 *	Deletes all resources associated with this screen panel. This is a 
 *	static routine, and works on static class globals.
 */
void SPARGMV::DiscardRsrc(void)
{
	SafeRelease(&ptfList);
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
	
	RCF rcf = RcfInterior();
	rcf.top += 5.5f * dyfList;
	rcf.bottom -= 5.5f * dyfList;
	if (puigo->FVisible()) {
		float dyf = 6.0f * dyfList;
		rcf.bottom -= dyf;
		puigo->SetBounds(RCF(0, rcf.bottom, rcf.DxfWidth(), rcf.bottom + dyf));
	}
	SetView(rcf);
	SetContent(rcf);
	rcf = RcfInterior();
	rcf.top += 1.5f * dyfList;
	rcf.bottom = rcf.top + 4.0f * dyfList;
	mpcpcpuiclock[ga.bdg.cpcToMove^1]->SetBounds(rcf);
	rcf = RcfInterior();
	rcf.bottom -= 1.5f * dyfList;
	rcf.top = rcf.bottom - 4.0f * dyfList;
	mpcpcpuiclock[ga.bdg.cpcToMove]->SetBounds(rcf);
}


/*	SPARGMV::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void SPARGMV::Draw(const RCF* prcfUpdate)
{
	SPAS::Draw(prcfUpdate); // draws content area of the scrollable area

	/* draw fixed part of the panel */

	/* top player */

	RCF rcf = RcfInterior();
	rcf.bottom = RcfView().top - 1;
	DrawPl(ga.spabd.cpcPointOfView, rcf, true);
	rcf.top = rcf.bottom;
	rcf.bottom += 1;
	FillRcf(rcf, pbrGridLine);

	/* bottom player */

	rcf = RcfInterior();
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
	wstring szColor = cpc == cpcWhite ? L"\x26aa  " : L"\x26ab  ";
	szColor += szName;
	rcfPl.left += 12.0f;
	rcfPl.top += 5.0f;
	DrawSz(szColor, ptfList, rcfPl);
	rcfPl.top -= 5.0f;
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
		if (!mv.FIsNil())
			DrawAndMakeMv(RcfFromImv(imv), bdgT, mv);
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


/*	SPARGMV::DrawAndMakeMv
 *
 *	Draws the text of an individual move in the rectangle given, using the
 *	given board bdg and move mv, and makes the move on the board. Note that 
 *	the text of the decoded move is dependent on the board to take advantage 
 *	of shorter text when there are no ambiguities. 
 */
void SPARGMV::DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv)
{
	rcf.left += 4.0f;
	rcf.top += 4.0f;
	rcf.bottom -= 2.0f;
	wstring sz = bdg.SzMoveAndDecode(mv);
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
	puigo->Show(false);
	Layout(PTF(0, 0), NULL, LL::None);
	bdgInit = ga.bdg;
	UpdateContSize();
}


/*	SPARGMV::EndGame
 *
 *	Notification that a game is over and we should display the end
 *	game result sub-panel
 */
void SPARGMV::EndGame(void)
{
	puigo->Show(true);
	Layout(PTF(0,0), NULL, LL::None);
	Redraw();
}


/*	SPARGMV::UpdateContSize
 *
 *	Keeps the content rectangle of the move list content in sync with the data
 *	in the move list
 */
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