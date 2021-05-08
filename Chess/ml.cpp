/*
 *
 *	ml.cpp
 *
 *	The move list panel on the screen
 *
 */

#include "ml.h"
#include "ga.h"
#include "Resource.h"


/*
 *
 *	UIPL
 * 
 *	Player name UI element in the move  list. This is just a little static
 *	item.
 * 
 */


UIPL::UIPL(UI* puiParent, CPC cpc) : UI(puiParent), cpc(cpc), ppl(NULL)
{
}


void UIPL::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);
	ptxText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	wstring szColor = cpc == cpcWhite ? L"\x26aa  " : L"\x26ab  ";
	if (ppl)
		szColor += ppl->SzName();
	RCF rcf = RcfInterior();
	rcf.left += 12.0f;
	rcf.top += 6.0f;
	DrawSz(szColor, ptxText, rcf);
}


void UIPL::SetPl(PL* pplNew)
{
	ppl = pplNew;
}


/*
 *
 *	UIGC implementation
 * 
 *	Game control button bar
 * 
 */


void UIGC::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
}


void UIGC::DiscardRsrcClass(void)
{
}


void UIGC::Layout(void)
{
	RCF rcf = RcfInterior();
	rcf.top += 4.0f;
	rcf.bottom -= 4.0f;
	rcf.left += 48.0f;
	SIZF sizf = pbtnResign->SizfImg();
	rcf.right = rcf.left + rcf.DyfHeight() * sizf.width / sizf.height;
	pbtnResign->SetBounds(rcf);

	rcf = RcfInterior();
	rcf.top += 4.0f;
	rcf.bottom -= 4.0f;
	rcf.right -= 48.0f;
	sizf = pbtnOfferDraw->SizfImg();
	rcf.left= rcf.right - (rcf.DyfHeight() * sizf.width / sizf.height);
	pbtnOfferDraw->SetBounds(rcf);
}


UIGC::UIGC(UIML* puiml) : UI(puiml)
{
	pbtnResign = new BTNIMG(this, cmdResign, RCF(0,0,0,0), idbWhiteFlag);
	pbtnOfferDraw = new BTNIMG(this, cmdOfferDraw, RCF(0,0,0,0), idbHandShake);
}


void UIGC::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);
}


/*
 *
 *	UICLOCK implementation
 *
 *	The chess clock implementation
 *
 */


IDWriteTextFormat* UICLOCK::ptxClock;

void UICLOCK::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxClock)
		return;
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 50.0f, L"",
		&ptxClock);
}


void UICLOCK::DiscardRsrcClass(void)
{
	SafeRelease(&ptxClock);
}


UICLOCK::UICLOCK(UIML* puiml, CPC cpc) : UI(puiml), ga(puiml->ga), cpc(cpc)
{
}


void UICLOCK::Draw(const RCF* prcfUpdate)
{
	DWORD tm = ga.mpcpctmClock[cpc];

	/* fill background */

	RCF rcf = RcfInterior();
	D2D1_COLOR_F coSav = pbrAltBack->GetColor();
	if (FTimeOutWarning(tm))
		pbrAltBack->SetColor(ColorF(1.0f, 0.9f, 0.9f));
	FillRcf(rcf, pbrAltBack);
	pbrAltBack->SetColor(coSav);
	rcf.top += 12.0f;

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

	ptxClock->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	const float dxfDigit = 30.0f;
	const float dxfPunc = 18.0f;
	if (hr > 0) {
		float dxfClock = dxfDigit + dxfPunc + 2.0f * dxfDigit + dxfPunc + 2.0f * dxfDigit;
		rcf.left = (rcf.left + rcf.right - dxfClock) / 2;
		DrawRgch(sz, 1, ptxClock, rcf);	// hours
		rcf.left += dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 2, 2, ptxClock, rcf); // minutes
		rcf.left += 2 * dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptxClock, rcf); // seconds
	}
	else if (min > 0) {
		float dxfClock = 2.0f * dxfDigit + dxfPunc + 2.0f * dxfDigit;
		rcf.left = (rcf.left + rcf.right - dxfClock) / 2;
		DrawRgch(sz + 2, 2, ptxClock, rcf);	// minutes
		rcf.left += 2 * dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptxClock, rcf);	// seconds 
	}
	else {
		float dxfClock = dxfDigit + dxfPunc + 2.0f * dxfDigit + dxfPunc + dxfDigit;
		rcf.left = (rcf.left + rcf.right - dxfClock) / 2;
		DrawRgch(sz + 3, 1, ptxClock, rcf);	// minutes (=0)
		rcf.left += dxfDigit;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 4, ptxClock, rcf);	// seconds and tenths
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
	DrawSz(L":", ptxClock, rcf, pbrText);
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

IDWriteTextFormat* UIGO::ptxScore;

void UIGO::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxScore)
		return;
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"",
		&ptxScore);
}


void UIGO::DiscardRsrcClass(void)
{
	SafeRelease(&ptxScore);
}


UIGO::UIGO(UIML* puiml, bool fVisible) : UI(puiml, fVisible), ga(puiml->ga)
{
}


void UIGO::Draw(const RCF* prcfUpdate)
{
	RCF rcfEndType = RcfInterior();
	FillRcf(rcfEndType, pbrBack);
	rcfEndType.bottom = rcfEndType.top + 36.0f;
	rcfEndType.Offset(0, 24.0f);
	RCF rcfResult = rcfEndType;
	rcfResult.Offset(0, 24.0f);
	RCF rcfScore = rcfResult;
	rcfScore.Offset(0, 24.0f);
	CPC cpcWin = cpcNil;

	switch (ga.bdg.gs) {
	case GS::WhiteCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::WhiteResigned:
		DrawSzCenter(L"White Resigned", ptxText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackResigned:
		DrawSzCenter(L"Black Resigned", ptxText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::WhiteTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcfEndType);
		cpcWin = cpcBlack;
		break;
	case GS::BlackTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcfEndType);
		cpcWin = cpcWhite;
		break;
	case GS::StaleMate:
		DrawSzCenter(L"Stalemate", ptxText, rcfEndType);
		break;
	case GS::Draw3Repeat:
		DrawSzCenter(L"3-Fold Repitition", ptxText, rcfEndType);
		break;
	case GS::Draw50Move:
		DrawSzCenter(L"50-Move", ptxText, rcfEndType);
		break;
	case GS::DrawAgree:
		DrawSzCenter(L"Draw Agreed", ptxText, rcfEndType);
		break;
	case GS::DrawDead:
		DrawSzCenter(L"Dead Position", ptxText, rcfEndType);
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
	DrawSzCenter(szResult, ptxText, rcfResult);
	DrawSzCenter(szScore, ptxScore, rcfScore);
}


/*
 *
 *	UIML class implementation
 *
 *	Move list implementation
 *
 */


UIML::UIML(GA* pga) : SPAS(pga), imvSel(0)
{
	mpcpcpuiclock[cpcWhite] = new UICLOCK(this, cpcWhite);
	mpcpcpuiclock[cpcBlack] = new UICLOCK(this, cpcBlack);
	mpcpcpuipl[cpcWhite] = new UIPL(this, cpcWhite);
	mpcpcpuipl[cpcBlack] = new UIPL(this, cpcBlack);
	puigo = new UIGO(this, false);
	puigc = new UIGC(this);
}


UIML::~UIML(void)
{
}


IDWriteTextFormat* UIML::ptxList;


/*	UIML::CreateRsrcClass
 *
 *	Creates the drawing resources for displaying the move list screen
 *	panel. Note that this is a static routine working on global static
 *	resources that are shared by all instances of this class.
 */
void UIML::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxList)
		return;

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"",
		&ptxList);

	UIGC::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UICLOCK::CreateRsrcClass(pdc, pfactdwr, pfactwic);
	UIGO::CreateRsrcClass(pdc, pfactdwr, pfactwic);
}


/*	UIML::DiscardRsrcClass
 *
 *	Deletes all resources associated with this screen panel. This is a
 *	static routine, and works on static class globals.
 */
void UIML::DiscardRsrcClass(void)
{
	SafeRelease(&ptxList);
	UIGC::DiscardRsrcClass();
	UICLOCK::DiscardRsrcClass();
	UIGO::DiscardRsrcClass();
}


float UIML::mpcoldxf[] = { 40.0f, 80.0f, 80.0f, 20.0f };
float UIML::dyfList = 20.0f;


float UIML::XfFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldxf) + 1);
	float xf = 0;
	for (int colT = 0; colT < col; colT++)
		xf += mpcoldxf[colT];
	return xf;
}


/*	UIML::DxfFromCol
 *
 *	Returns the width of the col column in the move list
 */
float UIML::DxfFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldxf));
	return mpcoldxf[col];
}


/*	UIML::RcfFromCol
 *
 *	Returns the rectangle a for a specific column and row in the move
 *	list. yf is the top of the rectangle and is typically computed relative
 *	to the content rectangle, which is in panel coordinates.
 */
RCF UIML::RcfFromCol(float yf, int col) const
{
	float xf = XfFromCol(col);
	return RCF(xf, yf, xf + DxfFromCol(col), yf + dyfList);
}


/*	UIML::Layout
 *
 *	Layout helper for placing the move list panel on the game screen.
 *	We basically position it to the right of the board panel.
 */
void UIML::Layout(void)
{
	/*	position the top clocks and player names */

	RCF rcf = RcfInterior();
	RCF rcfCont = rcf;
	rcf.bottom = rcf.top;
	AdjustUIRcfBounds(mpcpcpuipl[ga.uibd.cpcPointOfView ^ 1], rcf, true, 1.75f * dyfList);
	AdjustUIRcfBounds(mpcpcpuiclock[ga.uibd.cpcPointOfView ^ 1], rcf, true, 4.0f * dyfList);
	rcfCont.top = rcf.bottom;

	/* position the bottom clocks, player names, and game controls */

	rcf = RcfInterior();
	rcf.top = rcf.bottom;
	AdjustUIRcfBounds(mpcpcpuipl[ga.uibd.cpcPointOfView], rcf, false, 1.75f * dyfList);
	AdjustUIRcfBounds(mpcpcpuiclock[ga.uibd.cpcPointOfView], rcf, false, 4.0f * dyfList);
	AdjustUIRcfBounds(puigc, rcf, false, 1.5f * dyfList); 
	AdjustUIRcfBounds(puigo, rcf, false, 6.0f * dyfList);
	rcfCont.bottom = rcf.top;

	/* move list content is whatever is left */

	SetView(rcfCont);
	SetContent(rcfCont);
}


void UIML::AdjustUIRcfBounds(UI* pui, RCF& rcf, bool fTop, float dyfHeight)
{
	if (pui == NULL || !pui->FVisible())
		return;
	if (fTop) {
		rcf.top = rcf.bottom;
		rcf.bottom = rcf.top + dyfHeight;
	}
	else {
		rcf.bottom = rcf.top;
		rcf.top = rcf.bottom - dyfHeight;
	}
	pui->SetBounds(rcf);
	if (fTop)
		rcf.bottom++;
	else
		rcf.top--;
}


/*	UIML::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void UIML::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrGridLine);
	SPAS::Draw(prcfUpdate); // draws content area of the scrollable area
}


void UIML::SetPl(CPC cpc, PL* ppl)
{
	mpcpcpuipl[cpc]->SetPl(ppl);
}


/*	SPSARGMV::RcfFromImv
 *
 *	Returns the rectangle of the imv move in the move list. Coordinates
 *	are relative to the content rectangle, which is in turn relative to
 *	the panel.
 */
RCF UIML::RcfFromImv(int imv) const
{
	int rw = imv / 2;
	float yf = RcfContent().top + rw * dyfList;
	return RcfFromCol(yf, 1 + (imv % 2));
}


/*	UIML::DrawContent
 *
 *	Draws the content of the scrollable part of the move list screen
 *	panel.
 */
void UIML::DrawContent(const RCF& rcfCont)
{
	BDG bdgT(bdgInit);
	float yfCont = RcfContent().top;
	for (unsigned imv = 0; imv < ga.bdg.rgmvGame.size(); imv++) {
		MV mv = ga.bdg.rgmvGame[imv];
		if (imv % 2 == 0)
			DrawMoveNumber(RcfFromCol(yfCont + (imv / 2) * dyfList, 0), imv / 2 + 1);
		if (!mv.FIsNil())
			DrawAndMakeMv(RcfFromImv(imv), bdgT, mv);
	}
	DrawSel(imvSel);
}


/*	UIML::SetSel
 *
 *	Sets the selection
 */
void UIML::SetSel(int imv)
{
	imvSel = imv;
	Redraw();
}


/*	UIML::DrawSel
 *
 *	Draws the selection in the move list.
 */
void UIML::DrawSel(int imv)
{
}


/*	UIML::DrawAndMakeMv
 *
 *	Draws the text of an individual move in the rectangle given, using the
 *	given board bdg and move mv, and makes the move on the board. Note that
 *	the text of the decoded move is dependent on the board to take advantage
 *	of shorter text when there are no ambiguities.
 */
void UIML::DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv)
{
	rcf.left += 4.0f;
	rcf.top += 4.0f;
	rcf.bottom -= 2.0f;
	wstring sz = bdg.SzMoveAndDecode(mv);
	ptxList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	DrawSz(sz, ptxList, rcf);
}


/*	UIML::DrawMoveNumber
 *
 *	Draws the move number in the move list. Followed by a period. Rectangle
 *	to draw the text within is supplied by caller.
 */
void UIML::DrawMoveNumber(RCF rcf, int imv)
{
	rcf.top += 4.0f;
	rcf.bottom -= 2.0f;
	rcf.right -= 4.0f;
	WCHAR sz[8];
	WCHAR* pch = PchDecodeInt(imv, sz);
	*pch++ = L'.';
	*pch = 0;
	ptxList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSz(wstring(sz), ptxList, rcf);
}


void UIML::NewGame(void)
{
	puigo->Show(false);
	bool fTimed = ga.prule->TmGame() != 0;
	mpcpcpuiclock[cpcWhite]->Show(fTimed);
	mpcpcpuiclock[cpcBlack]->Show(fTimed);

	bdgInit = ga.bdg;
	UpdateContSize();
}


/*	UIML::EndGame
 *
 *	Notification that a game is over and we should display the end
 *	game result sub-panel
 */
void UIML::EndGame(void)
{
	puigo->Show(true);
}


/*	UIML::UpdateContSize
 *
 *	Keeps the content rectangle of the move list content in sync with the data
 *	in the move list
 */
void UIML::UpdateContSize(void)
{
	SPAS::UpdateContSize(PTF(RcfContent().DxfWidth(), ga.bdg.rgmvGame.size() / 2 * dyfList + dyfList));
}


bool UIML::FMakeVis(int imv)
{
	return SPAS::FMakeVis(RcfContent().top + (imv / 2) * dyfList, dyfList);
}