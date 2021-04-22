/*
 *
 *	ml.cpp
 *
 *	The move list panel on the screen
 *
 */

#include "ml.h"
#include "ga.h"


/*
 *
 *	UIPL
 * 
 *	Player name UI element in the move  list. This is just a little static
 *	item.
 * 
 */


UIPL::UIPL(SPARGMV* pspargmv, PL* ppl, CPC cpc) : UI(pspargmv), ppl(ppl), cpc(cpc)
{
}


void UIPL::Draw(const RCF* prcfUpdate)
{
	ptfText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	wstring szColor = cpc == cpcWhite ? L"\x26aa  " : L"\x26ab  ";
	if (ppl)
		szColor += ppl->SzName();
	RCF rcf = RcfInterior();
	rcf.left += 12.0f;
	rcf.top += 4.0f;
	DrawSz(szColor, ptfText, rcf);
}


void UIPL::SetPl(PL* pplNew)
{
	ppl = pplNew;
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
	FillRcf(RCF(rcf.left, rcf.top, rcf.right, rcf.top + 1), pbrGridLine);
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
		rcf.left += 2 * dxfDigit;
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
	mpcpcpuipl[cpcWhite] = new UIPL(this, NULL, cpcWhite);
	mpcpcpuipl[cpcBlack] = new UIPL(this, NULL, cpcBlack);
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
	assert(col >= 0 && col < CArray(mpcoldxf) + 1);
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

	/*	position the clocks and player names */

	rcf = RcfInterior();
	rcf.bottom = rcf.top + 1.5f * dyfList;
	mpcpcpuipl[ga.spabd.cpcPointOfView ^ 1]->SetBounds(rcf);
	rcf.top = rcf.bottom;
	rcf.bottom = rcf.top + 4.0f * dyfList;
	mpcpcpuiclock[ga.spabd.cpcPointOfView ^ 1]->SetBounds(rcf);

	rcf = RcfInterior();
	rcf.top = rcf.bottom - 1.5f * dyfList;
	mpcpcpuipl[ga.spabd.cpcPointOfView]->SetBounds(rcf);
	rcf.bottom = rcf.top;
	rcf.top = rcf.bottom - 4.0f * dyfList;
	mpcpcpuiclock[ga.spabd.cpcPointOfView]->SetBounds(rcf);
}


/*	SPARGMV::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void SPARGMV::Draw(const RCF* prcfUpdate)
{
	SPAS::Draw(prcfUpdate); // draws content area of the scrollable area
}


void SPARGMV::SetPl(CPC cpc, PL* ppl)
{
	mpcpcpuipl[cpc]->SetPl(ppl);
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
			DrawMoveNumber(RcfFromCol(yfCont + (imv / 2) * dyfList, 0), imv / 2 + 1);
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
	Layout(PTF(0, 0), NULL, LL::None);
	Redraw();
}


/*	SPARGMV::UpdateContSize
 *
 *	Keeps the content rectangle of the move list content in sync with the data
 *	in the move list
 */
void SPARGMV::UpdateContSize(void)
{
	SPAS::UpdateContSize(PTF(RcfContent().DxfWidth(), ga.bdg.rgmvGame.size() / 2 * dyfList + dyfList));
}


bool SPARGMV::FMakeVis(int imv)
{
	return SPAS::FMakeVis(RcfContent().top + (imv / 2) * dyfList, dyfList);
}