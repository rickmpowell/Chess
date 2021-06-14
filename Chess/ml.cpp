/*
 *
 *	ml.cpp
 *
 *	The move list panel on the screen
 *
 */

#include "ga.h"
#include "resources/Resource.h"


/*
 *
 *	UIPL
 * 
 *	Player name UI element in the move  list. This is just a little static
 *	item.
 * 
 */


UIPL::UIPL(UI* puiParent, CPC cpc) : UI(puiParent), ppl(nullptr), cpc(cpc)
{
}


SIZF UIPL::SizfLayoutPreferred(void)
{
	SIZF sizf = SizfSz(L"A", ptxText);
	return SIZF(-1.0f, sizf.height + 8);
}


void UIPL::Draw(RCF rcfUpdate)
{
	FillRcf(rcfUpdate, pbrBack);

	RCF rcf = RcfInterior();
	rcf.left += 12.0f;

	ptxText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	wstring szColor = cpc == CPC::White ? L"\x26aa" : L"\x26ab";
	SIZF sizf = SizfSz(szColor, ptxText, rcf.DxfWidth(), rcf.DyfHeight());
	rcf.bottom = rcf.top + sizf.height;
	rcf.Offset(PTF(0, RcfInterior().YCenter() - rcf.YCenter()));
	DrawSz(szColor, ptxText, rcf);
	if (ppl) {
		wstring szName = ppl->SzName();
		rcf.left += sizf.width + sizf.width;
		DrawSz(szName, ptxText, rcf);
	}
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
	rcf.top += 1.0f;
	rcf.bottom -= 1.0f;
	rcf.left += 48.0f;
	SIZF sizf = pbtnResign->SizfImg();
	rcf.right = rcf.left + rcf.DyfHeight() * sizf.width / sizf.height;
	pbtnResign->SetBounds(rcf);

	rcf = RcfInterior();
	rcf.top += 2.0f;
	rcf.bottom -= 2.0f;
	rcf.right -= 48.0f;
	sizf = pbtnOfferDraw->SizfImg();
	rcf.left= rcf.right - (rcf.DyfHeight() * sizf.width / sizf.height);
	pbtnOfferDraw->SetBounds(rcf);
}


SIZF UIGC::SizfLayoutPreferred(void)
{
	SIZF sizf = SizfSz(L"0", ptxText);
	return SIZF(-1.0f, sizf.height*1.5f);
}


UIGC::UIGC(UIML* puiml) : UI(puiml)
{
	pbtnResign = new BTNIMG(this, cmdResign, idbWhiteFlag);
	pbtnOfferDraw = new BTNIMG(this, cmdOfferDraw, idbHandShake);
}


void UIGC::Draw(RCF rcfUpdate)
{
	FillRcf(rcfUpdate, pbrBack);
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
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 40.0f, L"",
		&ptxClock);
}


void UICLOCK::DiscardRsrcClass(void)
{
	SafeRelease(&ptxClock);
}


UICLOCK::UICLOCK(UIML* puiml, CPC cpc) : UI(puiml), ga(puiml->ga), cpc(cpc)
{
}


SIZF UICLOCK::SizfLayoutPreferred(void)
{
	SIZF sizf = SizfSz(L"0", ptxClock);
	return SIZF(-1.0, sizf.height * 4.0f / 3.0f);
}


void UICLOCK::Draw(RCF rcfUpdate)
{
	DWORD tm = ga.mpcpctmClock[cpc];

	/* fill background */

	RCF rcf = RcfInterior();
	D2D1_COLOR_F coSav = pbrAltBack->GetColor();
	if (FTimeOutWarning(tm))
		pbrAltBack->SetColor(ColorF(1.0f, 0.9f, 0.9f));
	FillRcf(rcf, pbrAltBack);
	pbrAltBack->SetColor(coSav);

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
	SIZF sizfDigit = SizfSz(L"0", ptxClock, 1000.0f, 1000.0f);
	SIZF sizfPunc = SizfSz(L":", ptxClock, 1000.0f, 1000.0f);
	rcf.bottom = rcf.top + sizfDigit.height;
	rcf.Offset(0, RcfInterior().YCenter() - rcf.YCenter());
	if (hr > 0) {
		float dxfClock = sizfDigit.width + sizfPunc.width + 2*sizfDigit.width + sizfPunc.width + 2*sizfDigit.width;
		rcf.left = rcf.XCenter() - dxfClock/2;
		DrawRgch(sz, 1, ptxClock, rcf);	// hours
		rcf.left += sizfDigit.width;
		DrawColon(rcf, frac);
		DrawRgch(sz + 2, 2, ptxClock, rcf); // minutes
		rcf.left += 2*sizfDigit.width;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptxClock, rcf); // seconds
	}
	else if (min > 0) {
		float dxfClock = 2*sizfDigit.width + sizfPunc.width + 2*sizfDigit.width;
		rcf.left = (rcf.left + rcf.right - dxfClock) / 2;
		DrawRgch(sz + 2, 2, ptxClock, rcf);	// minutes
		rcf.left += 2*sizfDigit.width;
		DrawColon(rcf, frac);
		DrawRgch(sz + 5, 2, ptxClock, rcf);	// seconds 
	}
	else {
		float dxfClock = sizfDigit.width + sizfPunc.width + 2*sizfDigit.width + sizfPunc.width + sizfDigit.width;
		rcf.left = rcf.XCenter() - dxfClock / 2;;
		DrawRgch(sz + 3, 1, ptxClock, rcf);	// minutes (=0)
		rcf.left += sizfDigit.width;
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
	SIZF sizfPunc = SizfSz(L":", ptxClock, rcf.DxfWidth(), rcf.DyfHeight());
	DrawSz(L":", ptxClock, rcf, pbrText);
	pbrText->SetOpacity(1.0f);
	rcf.left += sizfPunc.width;
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
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
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


/*	UIGO::SizfLayoutPreferred
 *
 *	Returns the preferred height of the UI element. Used by our parent
 *	to layout the UIML. 
 */
SIZF UIGO::SizfLayoutPreferred(void)
{
	SIZF sizf = SizfSz(L"A", ptxText);
	return SIZF(-1.0f, sizf.height * 5.0f);
}


void UIGO::Draw(RCF rcfUpdate)
{
	float dyfLine = SizfSz(L"A", ptxText).height + 3.0f;

	RCF rcf = RcfInterior();
	FillRcf(rcf, pbrBack);

	RCF rcfEndType = rcf;
	rcfEndType.top = rcf.YCenter() - (3.0f * dyfLine) / 2;
	rcfEndType.bottom = rcfEndType.top + dyfLine;
	RCF rcfResult = rcfEndType;
	rcfResult.Offset(0, dyfLine);
	RCF rcfScore = rcfResult;
	rcfScore.Offset(0, dyfLine);

	CPC cpcWin = CPC::NoColor;

	switch (ga.bdg.gs) {
	case GS::WhiteCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcfEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcfEndType);
		cpcWin = CPC::White;
		break;
	case GS::WhiteResigned:
		DrawSzCenter(L"White Resigned", ptxText, rcfEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackResigned:
		DrawSzCenter(L"Black Resigned", ptxText, rcfEndType);
		cpcWin = CPC::White;
		break;
	case GS::WhiteTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcfEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcfEndType);
		cpcWin = CPC::White;
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
		DrawSzCenter(L"Insufficient Material", ptxText, rcfEndType);
		break;
	default:
		assert(false);
		break;
	}

	const WCHAR* szResult = L"Draw";
	const WCHAR* szScore = L"\x00bd-\x00bd";	/* 1/2-1/2 */
	if (cpcWin == CPC::White) {
		szResult = L"White Wins";
		szScore = L"1-0";
	}
	else if (cpcWin == CPC::Black) {
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


UIML::UIML(GA* pga) : UIPS(pga),  
		dxfCellMarg(4.0f), dyfCellMarg(0.5f), dyfList(0), imvSel(0),
		uigo(this, false), uigc(this)
{
	for (int col = 0; col < CArray(mpcoldxf); col++)
		mpcoldxf[col] = 0.0f;
	mpcpcpuiclock[CPC::White] = new UICLOCK(this, CPC::White);
	mpcpcpuiclock[CPC::Black] = new UICLOCK(this, CPC::Black);
	mpcpcpuipl[CPC::White] = new UIPL(this, CPC::White);
	mpcpcpuipl[CPC::Black] = new UIPL(this, CPC::Black);
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

	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 17.0f, L"",
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
	RCF rcfView = rcf;
	rcf.bottom = rcf.top;
	AdjustUIRcfBounds(mpcpcpuipl[ga.uibd.cpcPointOfView ^ 1], rcf, true);
	AdjustUIRcfBounds(mpcpcpuiclock[ga.uibd.cpcPointOfView ^ 1], rcf, true);
	rcfView.top = rcf.bottom;

	/* position the bottom clocks, player names, and game controls */

	rcf = RcfInterior();
	rcf.top = rcf.bottom;
	AdjustUIRcfBounds(mpcpcpuipl[ga.uibd.cpcPointOfView], rcf, false);
	AdjustUIRcfBounds(mpcpcpuiclock[ga.uibd.cpcPointOfView], rcf, false);
	AdjustUIRcfBounds(&uigc, rcf, false); 
	AdjustUIRcfBounds(&uigo, rcf, false);
	rcfView.bottom = rcf.top;

	/* move list content is whatever is left */
	
	AdjustRcfView(rcfView);
}


SIZF UIML::SizfLayoutPreferred(void)
{
	/* I think this is the longest possible move text */
	SIZF sizf = SizfSz(L"\x2659" L"f" L"\x00d7" L"g6" L"\x202f" L"e.p.+", ptxList);
	dyfList = sizf.height + 2*dyfCellMarg;

	mpcoldxf[0] = dxfCellMarg+SizfSz(L"200.", ptxList).width;
	mpcoldxf[1] = mpcoldxf[2] = dxfCellMarg + sizf.width;
	mpcoldxf[3] = dxyfScrollBarWidth;

	return SIZF(XfFromCol(4), -1.0f);
}


float UIML::DyfLine(void) const
{
	return dyfList;
}



/*	UIML::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void UIML::Draw(RCF rcfUpdate)
{
	FillRcf(rcfUpdate, pbrGridLine);
	UIPS::Draw(rcfUpdate); // draws content area of the scrollable area
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
	float yf = RcfContent().top + 4.0f + rw * dyfList;
	return RcfFromCol(yf, 1 + (imv % 2));
}


/*	UIML::DrawContent
 *
 *	Draws the content of the scrollable part of the move list screen
 *	panel.
 */
void UIML::DrawContent(RCF rcfCont)
{
	BDG bdgT(bdgInit);
	float yfCont = RcfContent().top;
	for (unsigned imv = 0; imv < ga.bdg.vmvGame.size(); imv++) {
		MV mv = ga.bdg.vmvGame[imv];
		if (imv % 2 == 0) {
			RCF rcf = RcfFromCol(yfCont + 4.0f + (imv / 2) * dyfList, 0);
			DrawMoveNumber(rcf, imv / 2 + 1);
		}
		if (!mv.fIsNil()) {
			RCF rcf = RcfFromImv(imv);
			if (imv == imvSel)
				FillRcf(rcf, pbrHilite);
			DrawAndMakeMv(rcf, bdgT, mv);
		}
	}
}


/*	UIML::SetSel
 *
 *	Sets the selection
 */
void UIML::SetSel(int imv, SPMV spmv)
{
	imvSel = imv;
	if (spmv != SPMV::Hidden)
		if (!FMakeVis(imvSel))
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
	wstring sz = bdg.SzMoveAndDecode(mv);
	ptxList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	rcf.top += dyfCellMarg;
	rcf.bottom -= dyfCellMarg;
	rcf.left += dxfCellMarg;
	DrawSz(sz, ptxList, rcf);
}


/*	UIML::DrawMoveNumber
 *
 *	Draws the move number in the move list. Followed by a period. Rectangle
 *	to draw the text within is supplied by caller.
 */
void UIML::DrawMoveNumber(RCF rcf, int imv)
{
	rcf.top += dyfCellMarg;
	rcf.bottom -= dyfCellMarg;
	rcf.right -= dxfCellMarg;
	WCHAR sz[8];
	WCHAR* pch = PchDecodeInt(imv, sz);
	*pch++ = L'.';
	*pch = 0;
	ptxList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSz(wstring(sz), ptxList, rcf);
}


void UIML::NewGame(void)
{
	uigo.Show(false);
	ShowClocks(ga.prule->TmGame() != 0);
	bdgInit = ga.bdg;
	UpdateContSize();
}


void UIML::ShowClocks(bool fTimed)
{
	mpcpcpuiclock[CPC::White]->Show(fTimed);
	mpcpcpuiclock[CPC::Black]->Show(fTimed);
}


/*	UIML::EndGame
 *
 *	Notification that a game is over and we should display the end
 *	game result sub-panel
 */
void UIML::EndGame(void)
{
	uigo.Show(true);
}


/*	UIML::UpdateContSize
 *
 *	Keeps the content rectangle of the move list content in sync with the data
 *	in the move list
 */
void UIML::UpdateContSize(void)
{
	float dyf = (ga.bdg.vmvGame.size()+1) / 2 * dyfList;
	if (dyf == 0)
		dyf = dyfList;
	UIPS::UpdateContSize(SIZF(RcfContent().DxfWidth(), 4.0f + dyf));
}


bool UIML::FMakeVis(int imv)
{
	return UIPS::FMakeVis(RcfContent().top + 4.0f + (imv / 2) * dyfList, dyfList);
}

HTML UIML::HtmlHitTest(PTF ptf, int* pimv)
{
	if (ptf.x < 0 || ptf.x >= RcfContent().right)
		return HTML::Miss;
	if (ptf.x > RcfView().right) {
		/* TODO: move this into UIPS */
		return HTML::Thumb;
	}

	int li = (int)floor((ptf.y - RcfContent().top) / DyfLine());
	if (ptf.x < mpcoldxf[0])
		return HTML::MoveNumber;
	int imv = -1;
	if (ptf.x < mpcoldxf[0] + mpcoldxf[1])
		imv = li * 2;
	else if (ptf.x < mpcoldxf[0] + mpcoldxf[1] + mpcoldxf[2])
		imv = li * 2 + 1;
	if (imv < 0)
		return HTML::EmptyBefore;
	if (imv >= (int)ga.bdg.vmvGame.size())
		return HTML::EmptyAfter;
	*pimv = imv;
	return HTML::List;
}

void UIML::StartLeftDrag(PTF ptf)
{
	int imv;
	HTML html = HtmlHitTest(ptf, &imv);
	if (html != HTML::List)
		return;
	SetSel(imv, SPMV::Fast);
	ga.MoveToImv(imv);
}

void UIML::EndLeftDrag(PTF ptf)
{
}

void UIML::LeftDrag(PTF ptf)
{
}


void UIML::KeyDown(int vk)
{
	switch (vk) {
	case VK_UP:
	case VK_LEFT:
		ga.MoveToImv(ga.bdg.imvCur - 1);
		break;
	case VK_DOWN:
	case VK_RIGHT:
		ga.MoveToImv(ga.bdg.imvCur + 1);
		break;
	case VK_HOME:
		ga.MoveToImv(0);
		break;
	case VK_END:
		ga.MoveToImv((int)ga.bdg.vmvGame.size() - 1);
		break;
	case VK_PRIOR:
		ga.MoveToImv(ga.bdg.imvCur - 5*2);
		break;
	case VK_NEXT:
		ga.MoveToImv(ga.bdg.imvCur + 5*2);
		break;
	default:
		break;
	}
}