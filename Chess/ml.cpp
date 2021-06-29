/*
 *
 *	ml.cpp
 *
 *	The move list panel on the screen
 *
 */

#include "ga.h"
#include "resources/Resource.h"



RGINFOPL rginfopl;




SPINLVL::SPINLVL(UIPL* puiParent, int cmdUp, int cmdDown) : SPIN(puiParent, cmdUp, cmdDown), uipl(*puiParent)
{
}

wstring SPINLVL::SzValue(void) const
{
	return to_wstring(uipl.ppl->Level());
}


/*
 *
 *	UIPL
 * 
 *	Player name UI element in the move  list. This is just a little static
 *	item.
 * 
 */


/*	UIPL::UIPL
 *
 *	Player name UI element constructor
 */
UIPL::UIPL(UI* puiParent, CPC cpc) : UI(puiParent), spinlvl(this, cmdPlayerLvlUp+cpc, cmdPlayerLvlDown+cpc), 
		ppl(nullptr), cpc(cpc), fChooser(false), iinfoplHit(-1), dyLine(8.0f)
{
}


void UIPL::CreateRsrc(void)
{
	UI::CreateRsrc();
	dyLine = SizSz(L"Ag", ptxText).height + 2 * 6.0f;
}


void UIPL::DiscardRsrc(void)
{
	UI::DiscardRsrc();
}


void UIPL::Layout(void)
{
	spinlvl.Show(!fChooser && ppl->FHasLevel());
	RC rc = RcInterior();
	rc.right -= 4.0f;
	rc.left = rc.right - 45.0f;
	spinlvl.SetBounds(rc);
}


/*	UIPL::SizLayoutPreferred
 *
 *	Returns the preferred height of the player name UI element for fitting
 *	in a vertically oriented screen panel.
 */
SIZ UIPL::SizLayoutPreferred(void)
{
	SIZ siz(-1.0f, dyLine);
	if (fChooser)
		siz.height *= rginfopl.vinfopl.size();
	return siz;
}


/*	UIPL::Draw
 *
 *	Draws the player UI element, which is just a circle to indicate the
 *	color the player is playing, and their name.
 */
void UIPL::Draw(RC rcUpdate)
{
	FillRc(rcUpdate, pbrBack);

	if (fChooser)
		DrawChooser(rcUpdate);
	else {

		/* draw the circle indicating which side */

		RC rc = RcInterior();
		rc.left += 12.0f;
		SIZ siz = SizSz(L"Ag", ptxText);
		float dxyRadius = siz.height / 2.0f;
		ELL ell(PT(rc.left + dxyRadius, rc.top + 6.0f + dxyRadius), PT(dxyRadius, dxyRadius));

		AADC aadc(App().pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		DrawEll(ell, pbrText);
		if (cpc == CPC::Black)
			FillEll(ell, pbrText);

		/* and the player name */

		if (ppl) {
			wstring szName = ppl->SzName();
			rc.left += 3 * dxyRadius;
			siz = SizSz(ppl->SzName(), ptxText);
			rc.top += 6.0f;
			if (spinlvl.FVisible())
				rc.right = spinlvl.RcBounds().left - 1.0f;
			DrawSzFit(szName, ptxText, rc);
		}
	}
}


void UIPL::DrawChooser(RC rcUpdate)
{
	RC rc = RcInterior();
	for (INFOPL& infopl : rginfopl.vinfopl)
		DrawChooserItem(infopl, rc);
}


void UIPL::DrawChooserItem(const INFOPL& infopl, RC& rc)
{
	rc.bottom = rc.top + dyLine;

	BMP* pbmpLogo = PbmpFromPngRes(rginfopl.IdbFromInfopl(infopl));
	wstring sz = infopl.szName;
	if (infopl.tpl == TPL::AI)
		sz += L" (level " + to_wstring(infopl.level) + L")";
	SIZ siz = pbmpLogo->GetSize();
	RC rcTo = rc;
	rcTo.Inflate(0, -4.0f).Offset(12.0f, 0);
	rcTo.right = rcTo.left + rcTo.DyHeight() / siz.height * siz.width;
	DrawBmp(rcTo, pbmpLogo, RC(PT(0, 0), siz));
	SafeRelease(&pbmpLogo);
	DrawSz(sz, ptxText, RC(rcTo.right+13.0f, rcTo.top+2.0, rc.right, rcTo.bottom-2.0f));

	rc.top = rc.bottom;
}


/*	UIPL::SetPl
 *
 *	Sets the player name to be displayed in the UI element. Does not take
 *	ownership of the PL. Owner is responsible for clearing the PL if the
 *	player changes.
 */
void UIPL::SetPl(PL* pplNew)
{
	ppl = pplNew;
}


void UIPL::MouseHover(PT pt, MHT mht)
{
	if (fChooser)
		::SetCursor(App().hcurArrow);
	else
		::SetCursor(App().hcurUp);
}


void UIPL::StartLeftDrag(PT pt)
{
	SetFocus(this);
	if (fChooser)
		iinfoplHit = (pt.y - RcInterior().top) / dyLine;
}


void UIPL::EndLeftDrag(PT pt)
{
	SetFocus(nullptr);
	if (fChooser) {
		int iinfopl = (pt.y - RcInterior().top) / dyLine;
		if (iinfopl == iinfoplHit)
			Ga().SetPl(cpc, rginfopl.PplFactory(Ga(), iinfopl));
	}
	fChooser = !fChooser;
	puiParent->Layout();
	puiParent->Redraw();
}


void UIPL::LeftDrag(PT pt)
{
}


/*
 *
 *	UIGC implementation
 * 
 *	Game control button bar
 * 
 */


UIGC::UIGC(UIML* puiml) : UI(puiml), ga(puiml->ga),
		btnResign(this, cmdResign, idbWhiteFlag), btnOfferDraw(this, cmdOfferDraw, idbHandShake),
		ptxScore(nullptr)
{
}



void UIGC::CreateRsrc(void)
{
	UI::CreateRsrc();
	if (ptxScore)
		return;
	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"",
		&ptxScore);
}


void UIGC::DiscardRsrc(void)
{
	SafeRelease(&ptxScore);
}


void UIGC::Layout(void)
{
	if (ga.bdg.gs == GS::Playing) {
		RC rc = RcInterior().Inflate(PT(0, -1.0f));
		rc.left += 48.0f;
		SIZ siz = btnResign.SizImg();
		rc.right = rc.left + rc.DyHeight() * siz.width / siz.height;
		btnResign.SetBounds(rc);

		rc = RcInterior().Inflate(PT(0, -1.0f));
		rc.right -= 48.0f;
		siz = btnOfferDraw.SizImg();
		rc.left = rc.right - (rc.DyHeight() * siz.width / siz.height);
		btnOfferDraw.SetBounds(rc);
	}
	btnResign.Show(ga.bdg.gs == GS::Playing);
	btnOfferDraw.Show(ga.bdg.gs == GS::Playing);
}


SIZ UIGC::SizLayoutPreferred(void)
{
	SIZ siz = SizSz(L"0", ptxText);
	siz.width = -1.0f;
	siz.height *= (ga.bdg.gs == GS::Playing) ? 1.5f : 5.0f;
	return siz;
}

void UIGC::Draw(RC rcUpdate)
{
	FillRc(rcUpdate, pbrBack);
	if (ga.bdg.gs == GS::Playing)
		return;

	float dyLine = SizSz(L"A", ptxText).height + 3.0f;

	RC rc = RcInterior();

	RC rcEndType = rc;
	rcEndType.top = rc.YCenter() - 3.0f*dyLine/2.0f;
	rcEndType.bottom = rcEndType.top + dyLine;
	RC rcResult = rcEndType;
	rcResult.Offset(0, dyLine);
	RC rcScore = rcResult;
	rcScore.Offset(0, dyLine);

	CPC cpcWin = CPC::NoColor;

	switch (ga.bdg.gs) {
	case GS::WhiteCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcEndType);
		cpcWin = CPC::White;
		break;
	case GS::WhiteResigned:
		DrawSzCenter(L"White Resigned", ptxText, rcEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackResigned:
		DrawSzCenter(L"Black Resigned", ptxText, rcEndType);
		cpcWin = CPC::White;
		break;
	case GS::WhiteTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcEndType);
		cpcWin = CPC::Black;
		break;
	case GS::BlackTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcEndType);
		cpcWin = CPC::White;
		break;
	case GS::StaleMate:
		DrawSzCenter(L"Stalemate", ptxText, rcEndType);
		break;
	case GS::Draw3Repeat:
		DrawSzCenter(L"3-Fold Repitition", ptxText, rcEndType);
		break;
	case GS::Draw50Move:
		DrawSzCenter(L"50-Move", ptxText, rcEndType);
		break;
	case GS::DrawAgree:
		DrawSzCenter(L"Draw Agreed", ptxText, rcEndType);
		break;
	case GS::DrawDead:
		DrawSzCenter(L"Insufficient Material", ptxText, rcEndType);
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
	DrawSzCenter(szResult, ptxText, rcResult);
	DrawSzCenter(szScore, ptxScore, rcScore);
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
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
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

void UICLOCK::CreateRsrc(void)
{
	UI::CreateRsrc();
	APP& app = App();
	CreateRsrcClass(app.pdc, app.pfactdwr, app.pfactwic);
}

void UICLOCK::DiscardRsrc(void)
{
	UI::DiscardRsrc();
	DiscardRsrcClass();
}

SIZ UICLOCK::SizLayoutPreferred(void)
{
	SIZ siz = SizSz(L"0", ptxClock);
	return SIZ(-1.0, siz.height * 4.0f / 3.0f);
}


void UICLOCK::Draw(RC rcUpdate)
{
	DWORD tm = ga.mpcpctmClock[cpc];

	/* fill background */

	RC rc = RcInterior();
	COLORBRS colorbrs(pbrAltBack, FTimeOutWarning(tm) ? ColorF(1.0f, 0.9f, 0.9f) : pbrAltBack->GetColor());
	FillRc(rc, pbrAltBack);

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

	TATX tatx(ptxClock, DWRITE_TEXT_ALIGNMENT_LEADING);
	SIZ sizDigit = SizSz(L"0", ptxClock);
	SIZ sizPunc = SizSz(L":", ptxClock);
	rc.bottom = rc.top + sizDigit.height;
	rc.Offset(0, RcInterior().YCenter() - rc.YCenter());
	if (hr > 0) {
		float dxClock = sizDigit.width + sizPunc.width + 2*sizDigit.width + sizPunc.width + 2*sizDigit.width;
		rc.left = rc.XCenter() - dxClock/2;
		DrawRgch(sz, 1, ptxClock, rc);	// hours
		rc.left += sizDigit.width;
		DrawColon(rc, frac);
		DrawRgch(sz + 2, 2, ptxClock, rc); // minutes
		rc.left += 2*sizDigit.width;
		DrawColon(rc, frac);
		DrawRgch(sz + 5, 2, ptxClock, rc); // seconds
	}
	else if (min > 0) {
		float dxClock = 2*sizDigit.width + sizPunc.width + 2*sizDigit.width;
		rc.left = (rc.left + rc.right - dxClock) / 2;
		DrawRgch(sz + 2, 2, ptxClock, rc);	// minutes
		rc.left += 2*sizDigit.width;
		DrawColon(rc, frac);
		DrawRgch(sz + 5, 2, ptxClock, rc);	// seconds 
	}
	else {
		float dxClock = sizDigit.width + sizPunc.width + 2*sizDigit.width + sizPunc.width + sizDigit.width;
		rc.left = rc.XCenter() - dxClock / 2;;
		DrawRgch(sz + 3, 1, ptxClock, rc);	// minutes (=0)
		rc.left += sizDigit.width;
		DrawColon(rc, frac);
		DrawRgch(sz + 5, 4, ptxClock, rc);	// seconds and tenths
	}
}


bool UICLOCK::FTimeOutWarning(DWORD tm) const
{
	return tm < 1000 * 20;
}


void UICLOCK::DrawColon(RC& rc, unsigned frac) const
{
	OPACITYBR opacitybr(pbrText, (frac < 500 && cpc == ga.bdg.cpcToMove) ? 0.33f : 1.0f);
	SIZ sizPunc = SizSz(L":", ptxClock);
	DrawSz(L":", ptxClock, rc, pbrText);
	rc.left += sizPunc.width;
}


/*
 *
 *	UIML class implementation
 *
 *	Move list panel, which is player and clock bars along with the scrolling move list.
 *
 */


UIML::UIML(GA* pga) : UIPS(pga),  
		ptxList(nullptr), dxCellMarg(4.0f), dyCellMarg(0.5f), dyList(0), imvSel(0),
		uigc(this)
{
	for (int col = 0; col < CArray(mpcoldx); col++)
		mpcoldx[col] = 0.0f;
	mpcpcpuiclock[CPC::White] = new UICLOCK(this, CPC::White);
	mpcpcpuiclock[CPC::Black] = new UICLOCK(this, CPC::Black);
	mpcpcpuipl[CPC::White] = new UIPL(this, CPC::White);
	mpcpcpuipl[CPC::Black] = new UIPL(this, CPC::Black);
}


UIML::~UIML(void)
{
}


/*	UIML::CreateRsrc
 *
 *	Creates the drawing resources for displaying the move list screen
 *	panel. 
 */
void UIML::CreateRsrc(void)
{
	UI::CreateRsrc();
	if (ptxList)
		return;

	/* fonts */

	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 17.0f, L"",
		&ptxList);
}


/*	UIML::DiscardRsrc
 *
 *	Deletes all resources associated with this screen panel. 
 */
void UIML::DiscardRsrc(void)
{
	UI::DiscardRsrc();
	SafeRelease(&ptxList);
}


float UIML::XFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldx) + 1);
	float x = 0;
	for (int colT = 0; colT < col; colT++)
		x += mpcoldx[colT];
	return x;
}


/*	UIML::DxFromCol
 *
 *	Returns the width of the col column in the move list
 */
float UIML::DxFromCol(int col) const
{
	assert(col >= 0 && col < CArray(mpcoldx));
	return mpcoldx[col];
}


/*	UIML::RcFromCol
 *
 *	Returns the rectangle a for a specific column and row in the move
 *	list. y is the top of the rectangle and is typically computed relative
 *	to the content rectangle, which is in panel coordinates.
 */
RC UIML::RcFromCol(float y, int col) const
{
	float x = XFromCol(col);
	return RC(x, y, x + DxFromCol(col), y + dyList);
}


/*	UIML::Layout
 *
 *	Layout helper for placing the move list panel on the game screen.
 *	We basically position it to the right of the board panel.
 */
void UIML::Layout(void)
{
	/*	position the top clocks and player names */

	RC rc = RcInterior();
	RC rcView = rc;
	rc.bottom = rc.top;
	AdjustUIRcBounds(mpcpcpuipl[~ga.uibd.cpcPointOfView], rc, true);
	AdjustUIRcBounds(mpcpcpuiclock[~ga.uibd.cpcPointOfView], rc, true);
	rcView.top = rc.bottom;

	/* position the bottom clocks, player names, and game controls */

	rc = RcInterior();
	rc.top = rc.bottom;
	AdjustUIRcBounds(mpcpcpuipl[ga.uibd.cpcPointOfView], rc, false);
	AdjustUIRcBounds(mpcpcpuiclock[ga.uibd.cpcPointOfView], rc, false);
	AdjustUIRcBounds(&uigc, rc, false); 
	rcView.bottom = rc.top;

	/* move list content is whatever is left */
	
	AdjustRcView(rcView);
}


SIZ UIML::SizLayoutPreferred(void)
{
	/* I think this is the longest possible move text */
	SIZ siz = SizSz(L"\x2659" L"f" L"\x00d7" L"g6" L"\x202f" L"e.p.+", ptxList);
	dyList = siz.height + 2*dyCellMarg;

	mpcoldx[0] = dxCellMarg+SizSz(L"100.", ptxList).width;
	mpcoldx[1] = mpcoldx[2] = dxCellMarg + siz.width;
	mpcoldx[3] = dxyScrollBarWidth;

	return SIZ(XFromCol(4), -1.0f);
}


float UIML::DyLine(void) const
{
	return dyList;
}



/*	UIML::Draw
 *
 *	Draws the move list screen panel, which includes a top header box and
 *	a scrollable move list
 */
void UIML::Draw(RC rcUpdate)
{
	FillRc(rcUpdate, pbrGridLine);
	UIPS::Draw(rcUpdate); // draws content area of the scrollable area
}


void UIML::SetPl(CPC cpc, PL* ppl)
{
	mpcpcpuipl[cpc]->SetPl(ppl);
}


/*	SPSARGMV::RcFromImv
 *
 *	Returns the rectangle of the imv move in the move list. Coordinates
 *	are relative to the content rectangle, which is in turn relative to
 *	the panel.
 */
RC UIML::RcFromImv(int imv) const
{
	int rw = imv / 2;
	float y = RcContent().top + 4.0f + rw * dyList;
	return RcFromCol(y, 1 + (imv % 2));
}


/*	UIML::DrawContent
 *
 *	Draws the content of the scrollable part of the move list screen
 *	panel.
 */
void UIML::DrawContent(RC rcCont)
{
	BDG bdgT(bdgInit);
	float yCont = RcContent().top;
	for (unsigned imv = 0; imv < ga.bdg.vmvGame.size(); imv++) {
		MV mv = ga.bdg.vmvGame[imv];
		if (imv % 2 == 0) {
			RC rc = RcFromCol(yCont + 4.0f + (imv / 2) * dyList, 0);
			DrawMoveNumber(rc, imv / 2 + 1);
		}
		if (!mv.fIsNil()) {
			RC rc = RcFromImv(imv);
			if (imv == imvSel)
				FillRc(rc, pbrHilite);
			DrawAndMakeMv(rc, bdgT, mv);
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
void UIML::DrawAndMakeMv(RC rc, BDG& bdg, MV mv)
{
	wstring sz = bdg.SzMoveAndDecode(mv);
	TATX tatxSav(ptxList, DWRITE_TEXT_ALIGNMENT_LEADING);
	rc.top += dyCellMarg;
	rc.bottom -= dyCellMarg;
	rc.left += dxCellMarg;
	DrawSz(sz, ptxList, rc);
}


/*	UIML::DrawMoveNumber
 *
 *	Draws the move number in the move list. Followed by a period. Rectangle
 *	to draw the text within is supplied by caller.
 */
void UIML::DrawMoveNumber(RC rc, int imv)
{
	rc.top += dyCellMarg;
	rc.bottom -= dyCellMarg;
	rc.right -= dxCellMarg;
	WCHAR sz[8];
	WCHAR* pch = PchDecodeInt(imv, sz);
	*pch++ = L'.';
	*pch = 0;
	TATX tatxSav(ptxList, DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSz(wstring(sz), ptxList, rc);
}


void UIML::NewGame(void)
{
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
	Layout();
	Redraw();
}


/*	UIML::UpdateContSize
 *
 *	Keeps the content rectangle of the move list content in sync with the data
 *	in the move list
 */
void UIML::UpdateContSize(void)
{
	float dy = (ga.bdg.vmvGame.size()+1) / 2 * dyList;
	if (dy == 0)
		dy = dyList;
	UIPS::UpdateContSize(SIZ(RcContent().DxWidth(), 4.0f + dy));
}


bool UIML::FMakeVis(int imv)
{
	return UIPS::FMakeVis(RcContent().top + 4.0f + (imv / 2) * dyList, dyList);
}

HTML UIML::HtmlHitTest(PT pt, int* pimv)
{
	if (pt.x < 0 || pt.x >= RcContent().right)
		return HTML::Miss;
	if (pt.x > RcView().right) {
		/* TODO: move this into UIPS */
		return HTML::Thumb;
	}

	int li = (int)floor((pt.y - RcContent().top) / DyLine());
	if (pt.x < mpcoldx[0])
		return HTML::MoveNumber;
	int imv = -1;
	if (pt.x < mpcoldx[0] + mpcoldx[1])
		imv = li * 2;
	else if (pt.x < mpcoldx[0] + mpcoldx[1] + mpcoldx[2])
		imv = li * 2 + 1;
	if (imv < 0)
		return HTML::EmptyBefore;
	if (imv >= (int)ga.bdg.vmvGame.size())
		return HTML::EmptyAfter;
	*pimv = imv;
	return HTML::List;
}

void UIML::StartLeftDrag(PT pt)
{
	int imv;
	HTML html = HtmlHitTest(pt, &imv);
	if (html != HTML::List)
		return;
	SetSel(imv, SPMV::Fast);
	ga.MoveToImv(imv);
}

void UIML::EndLeftDrag(PT pt)
{
}

void UIML::LeftDrag(PT pt)
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