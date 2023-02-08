/*
 *
 *	uiml.cpp
 *
 *	The move list panel on the screen
 *
 */

#include "uiga.h"
#include "ga.h"
#include "resources/Resource.h"



AINFOPL ainfopl;


/*
 *
 *	SPINLVL
 * 
 *	The level spin control used to pick how smart the AI is in a computer player.
 * 
 */


SPINLVL::SPINLVL(UIPL* puiParent, int cmdUp, int cmdDown) : 
		SPIN(puiParent, cmdUp, cmdDown), uipl(*puiParent)
{
}

wstring SPINLVL::SzValue(void) const
{
	return to_wstring(uipl.uiga.ga.PplFromCpc(uipl.cpc)->Level());
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
UIPL::UIPL(UI& uiParent, UIGA& uiga, CPC cpc) : UI(&uiParent), uiga(uiga), cpc(cpc),
		spinlvl(this, cmdPlayerLvlUp+cpc, cmdPlayerLvlDown+cpc), 
		fChooser(false), iinfoplHit(-1), dyLine(8.0f)
{
}


void UIPL::CreateRsrc(void)
{
	UI::CreateRsrc();
	dyLine = SizFromSz(L"Ag", ptxText).height + 2 * 6.0f;
}


void UIPL::DiscardRsrc(void)
{
	UI::DiscardRsrc();
}


void UIPL::Layout(void)
{
	spinlvl.Show(!fChooser && uiga.ga.PplFromCpc(cpc)->FHasLevel());
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
		siz.height *= ainfopl.vinfopl.size();
	return siz;
}


/*	UIPL::Draw
 *
 *	Draws the player UI element, which is just a circle to indicate the
 *	color the player is playing, and their name.
 */
void UIPL::Draw(const RC& rcUpdate)
{
	if (fChooser)
		DrawChooser(rcUpdate);
	else {

		/* draw the circle indicating which side */

		RC rc = RcInterior();
		rc.left += 12.0f;
		SIZ siz = SizFromSz(L"Ag", ptxText);
		float dxyRadius = siz.height / 2.0f;
		ELL ell(PT(rc.left + dxyRadius, rc.top + 6.0f + dxyRadius), dxyRadius);

		AADC aadc(App().pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		FillEll(ell, cpc == cpcBlack ? pbrBlack: pbrWhite);
		DrawEll(ell, pbrText);

		/* and the player name */

		PL* ppl = uiga.ga.PplFromCpc(cpc);
		if (ppl) {
			wstring szName = ppl->SzName();
			rc.left += 3 * dxyRadius;
			siz = SizFromSz(ppl->SzName(), ptxText);
			rc.top += 6.0f;
			if (spinlvl.FVisible())
				rc.right = spinlvl.RcBounds().left - 1.0f;
			DrawSzFit(szName, ptxText, rc);
		}
	}
}


void UIPL::DrawChooser(const RC& rcUpdate)
{
	RC rc = RcInterior();
	for (INFOPL& infopl : ainfopl.vinfopl)
		DrawChooserItem(infopl, rc);
}


void UIPL::DrawChooserItem(const INFOPL& infopl, RC& rc)
{
	rc.bottom = rc.top + dyLine;

	BMP* pbmpLogo = PbmpFromPngRes(ainfopl.IdbFromInfopl(infopl));
	wstring sz = infopl.szName;
	if (infopl.tpl == tplAI)
		sz += L" (level " + to_wstring(infopl.level) + L")";
	SIZ siz = pbmpLogo->GetSize();
	RC rcTo = rc;
	rcTo.Inflate(0, -4.0f).Offset(12.0f, 0);
	rcTo.right = rcTo.left + rcTo.DyHeight() / siz.height * siz.width;
	DrawBmp(rcTo, pbmpLogo, RC(PT(0, 0), siz));
	SafeRelease(&pbmpLogo);
	DrawSz(sz, ptxText, RC(rcTo.right+13.0f, rcTo.top+2.0f, rc.right, rcTo.bottom-2.0f));

	rc.top = rc.bottom;
}


void UIPL::MouseHover(const PT& pt, MHT mht)
{
	if (fChooser)
		::SetCursor(App().hcurArrow);
	else
		::SetCursor(App().hcurUp);
}


void UIPL::StartLeftDrag(const PT& pt)
{
	SetFocus(this);
	if (fChooser)
		iinfoplHit = (int)((pt.y - RcInterior().top) / dyLine);
}


void UIPL::EndLeftDrag(const PT& pt, bool fClick)
{
	SetFocus(nullptr);
	if (fChooser) {
		int iinfopl = (int)((pt.y - RcInterior().top) / dyLine);
		if (iinfopl == iinfoplHit)
			Ga().SetPl(cpc, ainfopl.PplFactory(Ga(), iinfopl));
	}
	fChooser = !fChooser;
	puiParent->Layout();
	puiParent->Redraw();
}


void UIPL::LeftDrag(const PT& pt)
{
}


/*
 *
 *	UIGC implementation
 * 
 *	Game control button bar
 * 
 */


UIGC::UIGC(UIML& uiml) : UI(&uiml), uiga(uiml.uiga),
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
	if (uiga.ga.bdg.FGsPlaying()) {
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
	btnResign.Show(uiga.ga.bdg.FGsPlaying());
	btnOfferDraw.Show(uiga.ga.bdg.FGsPlaying());
}


SIZ UIGC::SizLayoutPreferred(void)
{
	SIZ siz = SizFromSz(L"0", ptxText);
	siz.width = -1.0f;
	if (uiga.ga.bdg.FGsPlaying())
		siz.height *= 1.5;
	else if (uiga.ga.bdg.FGsGameOver())
		siz.height *= 5.0f;
	else
		siz.height *= 1.5;
	return siz;
}

ColorF UIGC::CoBack(void) const
{
	return coButtonBarBack;
}


void UIGC::Draw(const RC& rcUpdate)
{
	if (!uiga.ga.bdg.FGsGameOver())
		return;

	float dyLine = SizFromSz(L"A", ptxText).height + 3.0f;

	RC rc = RcInterior();

	RC rcEndType = rc;
	rcEndType.top = rc.YCenter() - 3.0f*dyLine/2.0f;
	rcEndType.bottom = rcEndType.top + dyLine;
	RC rcResult = rcEndType;
	rcResult.Offset(0, dyLine);
	RC rcScore = rcResult;
	rcScore.Offset(0, dyLine);

	CPC cpcWin = cpcNil;

	switch (uiga.ga.bdg.gs) {
	case gsWhiteCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcEndType);
		cpcWin = cpcBlack;
		break;
	case gsBlackCheckMated:
		DrawSzCenter(L"Checkmate", ptxText, rcEndType);
		cpcWin = cpcWhite;
		break;
	case gsWhiteResigned:
		DrawSzCenter(L"White Resigned", ptxText, rcEndType);
		cpcWin = cpcBlack;
		break;
	case gsBlackResigned:
		DrawSzCenter(L"Black Resigned", ptxText, rcEndType);
		cpcWin = cpcWhite;
		break;
	case gsWhiteTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcEndType);
		cpcWin = cpcBlack;
		break;
	case gsBlackTimedOut:
		DrawSzCenter(L"Time Expired", ptxText, rcEndType);
		cpcWin = cpcWhite;
		break;
	case gsStaleMate:
		DrawSzCenter(L"Stalemate", ptxText, rcEndType);
		break;
	case gsDraw3Repeat:
		DrawSzCenter(L"3-Fold Repitition", ptxText, rcEndType);
		break;
	case gsDraw50Move:
		DrawSzCenter(L"50-Move", ptxText, rcEndType);
		break;
	case gsDrawAgree:
		DrawSzCenter(L"Draw Agreed", ptxText, rcEndType);
		break;
	case gsDrawDead:
		DrawSzCenter(L"Insufficient Material", ptxText, rcEndType);
		break;
	default:
		assert(false);
		break;
	}

	const wchar_t* szResult = L"Draw";
	const wchar_t* szScore = L"\x00bd-\x00bd";	/* 1/2-1/2 */
	if (cpcWin == cpcWhite) {
		szResult = L"White Wins";
		szScore = L"1-0";
	}
	else if (cpcWin == cpcBlack) {
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


TX* UICLOCK::ptxClock;
TX* UICLOCK::ptxClockNote;
TX* UICLOCK::ptxClockNoteBold;


void UICLOCK::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxClock)
		return;
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
							   40.0f, L"",
							   &ptxClock);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
							   12.0f, L"",
							   &ptxClockNote);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   12.0f, L"",
							   &ptxClockNoteBold);
}


void UICLOCK::DiscardRsrcClass(void)
{
	SafeRelease(&ptxClock);
	SafeRelease(&ptxClockNote);
	SafeRelease(&ptxClockNoteBold);
}


UICLOCK::UICLOCK(UIML& uiml, CPC cpc) : UI(&uiml), uiga(uiml.uiga), cpc(cpc), fFlag(false)
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
	SIZ siz = SizFromSz(L"0", ptxClock);
	siz.width = -1;
	siz.height *= 4.0f / 3.0f;
	if (uiga.ga.prule->CtmiTotal() >= 2)
		siz.height += SizFromSz(L"0", ptxList).height + 4.0f;
	return siz;
}


void UICLOCK::Flag(bool fFlagNew)
{
	fFlag = fFlagNew;
}


ColorF UICLOCK::CoBack(void) const
{
	DWORD dmsec = uiga.mpcpcdmsecClock[cpc];
	return FTimeOutWarning(dmsec) ? coClockWarningBack : coClockBack;
}

ColorF UICLOCK::CoText(void) const
{
	DWORD dmsec = uiga.mpcpcdmsecClock[cpc];
	return FTimeOutWarning(dmsec) ? coClockWarningText : coClockText;
}


void UICLOCK::Draw(const RC& rcUpdate)
{
	RC rcTime = DrawTimeControls(Ga().NmvNextFromCpc(cpc));

	/* break down time into parts */

	DWORD dmsec = uiga.mpcpcdmsecClock[cpc];
	unsigned hr = dmsec / (1000 * 60 * 60);
	dmsec = dmsec % (1000 * 60 * 60);
	unsigned min = dmsec / (1000 * 60);
	dmsec = dmsec % (1000 * 60);
	unsigned sec = dmsec / 1000;
	dmsec = dmsec % 1000;
	unsigned frac = dmsec;

	/* convert into text */

	wchar_t sz[20], * pch;
	pch = sz;
	pch = PchDecodeInt(hr, pch);
	*pch++ = chColon;
	*pch++ = L'0' + min / 10;
	*pch++ = L'0' + min % 10;
	*pch++ = chColon;
	*pch++ = L'0' + sec / 10;
	*pch++ = L'0' + sec % 10;
	*pch++ = chPeriod;
	*pch++ = L'0' + frac / 100;
	*pch = 0;

	/* print out the text piece at a time */

	TATX tatx(ptxClock, DWRITE_TEXT_ALIGNMENT_LEADING);
	SIZ sizDigit = SizFromSz(L"0", ptxClock);
	SIZ sizPunc = SizFromSz(L":", ptxClock);
	RC rc = rcTime;
	rc.bottom = rc.top + sizDigit.height;
	rc.Offset(0, rcTime.YCenter() - rc.YCenter());
	if (hr > 0) {
		float dxClock = sizDigit.width + sizPunc.width + 2*sizDigit.width + sizPunc.width + 2*sizDigit.width;
		rc.left = rc.XCenter() - dxClock/2;
		DrawAch(sz, 1, ptxClock, rc);	// hours
		rc.left += sizDigit.width;
		DrawColon(rc, frac);
		DrawAch(sz + 2, 2, ptxClock, rc); // minutes
		rc.left += 2*sizDigit.width;
		DrawColon(rc, frac);
		DrawAch(sz + 5, 2, ptxClock, rc); // seconds
	}
	else if (min > 0) {
		float dxClock = 2*sizDigit.width + sizPunc.width + 2*sizDigit.width;
		rc.left = (rc.left + rc.right - dxClock) / 2;
		DrawAch(sz + 2, 2, ptxClock, rc);	// minutes
		rc.left += 2*sizDigit.width;
		DrawColon(rc, frac);
		DrawAch(sz + 5, 2, ptxClock, rc);	// seconds 
	}
	else {
		float dxClock = sizDigit.width + sizPunc.width + 2*sizDigit.width + sizPunc.width + sizDigit.width;
		rc.left = rc.XCenter() - dxClock / 2;;
		DrawAch(sz + 3, 1, ptxClock, rc);	// minutes (=0)
		rc.left += sizDigit.width;
		DrawColon(rc, frac);
		DrawAch(sz + 5, 4, ptxClock, rc);	// seconds and tenths
	}


	if (fFlag)
		DrawFlag();
}


RC UICLOCK::DrawTimeControls(int nmvSel) const
{
	RC rcInt = RcInterior();

	RULE* prule = uiga.ga.prule;
	if (prule->CtmiTotal() < 2) {
		DWORD dmsec = prule->TmiFromItmi(0).dmsecMove;
		if (dmsec > 0) {
			wstring sz = L"+" + to_wstring(dmsec / 1000) + L" sec";
			SIZ siz = SizFromSz(sz, ptxList);
			RC rc(rcInt.right - siz.width - 8, rcInt.bottom - siz.height - 1, rcInt.right, rcInt.bottom);
			DrawSz(sz, ptxList, rc);
		}
		return rcInt;
	}

	RC rcControls = rcInt;
	rcControls.top = rcControls.bottom - 16.0f;

	float dxTmi = rcInt.DxWidth() / prule->CtmiTotal();
	RC rcTmi = RC(rcControls.left, rcControls.top+1.0f, rcControls.left + dxTmi, rcControls.bottom);
	DrawTmi(prule->TmiFromItmi(0), rcTmi, nmvSel);
	for (int itmi = 1; itmi < prule->CtmiTotal(); itmi++) {
		rcTmi.Offset(dxTmi, 0);
		DrawTmi(prule->TmiFromItmi(itmi), rcTmi, nmvSel);
	}

	rcInt.bottom = rcControls.top;
	return rcInt;
}


wchar_t* UICLOCK::PchDecodeDmsec(DWORD dmsec, wchar_t* pch) const
{
	DWORD sec = dmsec / 1000;
	if (sec < 60) {
		pch = PchDecodeInt(sec, pch);
		*pch++ = 's';
		return pch;
	}
	DWORD min = sec / 60;
	sec %= 60;
	pch = PchDecodeInt(min, pch);
	if (sec == 0) {
		*pch++ = 'm';
		return pch;
	}

	*pch++ = ':';
	pch = PchDecodeInt(sec, pch);
	return pch;
}


void UICLOCK::DrawTmi(const TMI& tmi, RC rc, int nmvSel) const
{
	wchar_t sz[32], * pch = sz;

	pch = PchDecodeDmsec(tmi.dmsec, pch);
	if (tmi.dmsecMove) {
		*pch++ = L'+';
		pch = PchDecodeDmsec(tmi.dmsecMove, pch);
	}
	if (tmi.nmvLast != -1) {
		*pch++ = L'/';
		pch = PchDecodeInt(tmi.nmvLast - tmi.nmvFirst + 1, pch);
	}
	*pch++ = 0;
	bool fCur = tmi.FContainsNmv(nmvSel);
	COLORBRS colorbrsBack(pbrText, fCur ? coClockTCCurText : coClockTCText);
	COLORBRS colorbrsText(pbrBack, fCur ? coClockTCCurBack : coClockTCBack);
	FillRcBack(rc);
	DrawSzCenter(sz, fCur ? ptxClockNoteBold : ptxClockNote, rc, pbrText);
}


bool UICLOCK::FTimeOutWarning(DWORD dmsec) const
{
	return dmsec < 1000 * 20;
}


void UICLOCK::DrawColon(RC& rc, unsigned frac) const
{
	OPACITYBR opacitybr(pbrText, (frac < 500 && cpc == uiga.ga.bdg.cpcToMove) ? 0.33f : 1.0f);
	SIZ sizPunc = SizFromSz(L":", ptxClock);
	DrawSz(L":", ptxClock, rc, pbrText);
	rc.left += sizPunc.width;
}


void UICLOCK::DrawFlag(void) const
{
	RC rc = RcInterior();
	PT apt[] = { {0, 0}, {12, 0}, {12, 30}, {0, 12}, {0, 0} };
	GEOM* pgeom = PgeomCreate(apt, CArray(apt));
	FillGeom(pgeom, PT(24.0f, 0), 1, coClockFlag);
	SafeRelease(&pgeom);
}


/*
 *
 *	UIML class implementation
 *
 *	Move list panel, which is player and clock bars along with the scrolling move list.
 *
 */


UIML::UIML(UIGA& uiga) : UIPS(uiga),  
		ptxList(nullptr), ptxPiece(nullptr), dxCellMarg(4.0f), dyCellMarg(0.5f), dyList(0), dyListBaseline(0), imvuSel(0),
		uiplWhite(*this, uiga, cpcWhite), uiplBlack(*this, uiga, cpcBlack), 
		uiclockWhite(*this, cpcWhite), uiclockBlack(*this, cpcBlack), uigc(*this)
{
	for (int col = 0; col < CArray(mpcoldx); col++)
		mpcoldx[col] = 0.0f;
	uiplWhite.SetCoBack(coButtonBarBack);
	uiplBlack.SetCoBack(coButtonBarBack);
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
	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
									 DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"",
									 &ptxPiece);
}


/*	UIML::DiscardRsrc
 *
 *	Deletes all resources associated with this screen panel. 
 */
void UIML::DiscardRsrc(void)
{
	UI::DiscardRsrc();
	SafeRelease(&ptxList);
	SafeRelease(&ptxPiece);
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


UIPL& UIML::UiplFromCpc(CPC cpc)
{
	return cpc == cpcWhite ? uiplWhite : uiplBlack;
}

UICLOCK& UIML::UiclockFromCpc(CPC cpc)
{
	return cpc == cpcWhite ? uiclockWhite : uiclockBlack;
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
	AdjustUIRcBounds(UiplFromCpc(~uiga.uibd.cpcPointOfView), rc, true);
	AdjustUIRcBounds(UiclockFromCpc(~uiga.uibd.cpcPointOfView), rc, true);
	rcView.top = rc.bottom;

	/* position the bottom clocks, player names, and game controls */

	rc = RcInterior();
	rc.top = rc.bottom;
	AdjustUIRcBounds(UiplFromCpc(uiga.uibd.cpcPointOfView), rc, false);
	AdjustUIRcBounds(UiclockFromCpc(uiga.uibd.cpcPointOfView), rc, false);
	AdjustUIRcBounds(uigc, rc, false); 
	rcView.bottom = rc.top;

	/* move list content is whatever is left */
	
	AdjustRcView(rcView);

	UIPS::Layout();
}


/*	UIML::SizLayoutPreferred
 *
 *	Returns the preferred size of the move list. We only care about the width.
 */
SIZ UIML::SizLayoutPreferred(void)
{
	/* I think this is the longest possible move text */
	SIZ sizText = SizFromSz(L"f" L"\x00d7" L"g6" L"\x202f" L"e.p.+", ptxList);
	SIZ sizPiece = SizFromSz(L"\x2654", ptxPiece);
	dyList = max(sizText.height, sizPiece.height) +2*dyCellMarg;
	dyListBaseline = DyBaselineSz(L"\x2654", ptxPiece);

	mpcoldx[0] = dxCellMarg+SizFromSz(L"100.", ptxList).width;
	mpcoldx[1] = mpcoldx[2] = dxCellMarg + sizText.width + sizPiece.width;
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
void UIML::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrGridLine);
	UIPS::Draw(rcUpdate); // draws content area of the scrollable area
}


/*	UIML::RcFromImv
 *
 *	Returns the rectangle of the imv move in the move list. Coordinates
 *	are relative to the content rectangle, which is in turn relative to
 *	the panel.
 */
RC UIML::RcFromImv(int imv) const
{
	int nmv = Ga().NmvFromImv(imv);
	float y = RcContent().top + 4.0f + (nmv-1)*dyList;
	return RcFromCol(y, 1 + !Ga().FImvIsWhite(imv));
}


/*	UIML::DrawContent
 *
 *	Draws the content of the scrollable part of the move list screen
 *	panel.
 */
void UIML::DrawContent(const RC& rcCont)
{
	BDG bdgT(bdgInit);
	float yCont = RcContent().top + 4.0f;
	if (Ga().FImvFirstIsBlack())
		DrawMoveNumber(RcFromCol(yCont, 0), 1);
	for (int imve = 0; imve < Ga().bdg.vmveGame.size(); imve++) {
		MVE mve = uiga.ga.bdg.vmveGame[imve];
		if (Ga().FImvIsWhite(imve)) {
			int nmv = Ga().NmvFromImv(imve);
			RC rc = RcFromCol(yCont + (nmv-1) * dyList, 0);
			DrawMoveNumber(rc, nmv);
		}
		if (mve.fIsNil())
			continue;
		RC rc = RcFromImv(imve);
		if (imve == imvuSel)
			FillRc(rc, pbrHilite);
		DrawAndMakeMvu(rc, bdgT, mve);
	}
}


/*	UIML::SetSel
 *
 *	Sets the selection
 */
void UIML::SetSel(int imvu, SPMV spmv)
{
	imvuSel = imvu;
	if (spmv != spmvHidden)
		if (!FMakeVis(imvuSel))
			Redraw();
}


/*	UIML::DrawSel
 *
 *	Draws the selection in the move list.
 */
void UIML::DrawSel(int imvu)
{
}


bool FChChessPiece(wchar_t ch)
{
	return in_range(ch, chWhiteKing, chBlackPawn);
}


/*	UIML::DrawAndMakeMvu
 *
 *	Draws the text of an individual move in the rectangle given, using the
 *	given board bdg and move mv, and makes the move on the board. Note that
 *	the text of the decoded move is dependent on the board to take advantage
 *	of shorter text when there are no ambiguities.
 */
void UIML::DrawAndMakeMvu(const RC& rc, BDG& bdg, MVU mvu)
{
	wstring sz = bdg.SzMoveAndDecode(mvu);
	TATX tatxSav(ptxList, DWRITE_TEXT_ALIGNMENT_LEADING);
	RC rcText = rc;
	rcText.top += dyCellMarg;
	rcText.bottom -= dyCellMarg;
	rcText.left += dxCellMarg;
	if (FChChessPiece(sz[0])) {
		wchar_t szPiece[2] = { sz[0], 0 };
		float dx = SizFromSz(szPiece, ptxPiece).width;
		DrawSzBaseline(szPiece, ptxPiece, rcText, dyListBaseline);
		rcText.left += dx;
		DrawSzBaseline(sz.c_str()+1, ptxList, rcText, dyListBaseline);
	}
	else {
		DrawSzBaseline(sz, ptxList, rcText, dyListBaseline);
	}
}


/*	UIML::DrawMoveNumber
 *
 *	Draws the move number in the move list. Followed by a period. Rectangle
 *	to draw the text within is supplied by caller.
 */
void UIML::DrawMoveNumber(const RC& rc, int imv)
{
	RC rcText = rc;
	rcText.top += dyCellMarg;
	rcText.bottom -= dyCellMarg;
	rcText.right -= dxCellMarg;
	wchar_t sz[8];
	wchar_t* pch = PchDecodeInt(imv, sz);
	*pch++ = chPeriod;
	*pch = 0;
	TATX tatxSav(ptxList, DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSzBaseline(wstring(sz), ptxList, rcText, dyListBaseline);
}


void UIML::InitGame(void)
{
	ShowClocks(!uiga.ga.prule->FUntimed());
	bdgInit = uiga.ga.bdg;
	UpdateContSize();
}


void UIML::ShowClocks(bool fTimed)
{
	uiclockWhite.Show(fTimed);
	uiclockBlack.Show(fTimed);
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
	float dy = (uiga.ga.bdg.vmveGame.size()+1) / 2 * dyList;
	if (dy == 0)
		dy = dyList;
	UIPS::UpdateContSize(SIZ(RcContent().DxWidth(), 4.0f + dy));
}


bool UIML::FMakeVis(int imv)
{
	return UIPS::FMakeVis(RcContent().top + 4.0f + (imv / 2) * dyList, dyList);
}

HTML UIML::HtmlHitTest(const PT& pt, int* pimv)
{
	if (pt.x < 0 || pt.x >= RcContent().right)
		return htmlMiss;
	if (pt.x > RcView().right) {
		/* TODO: move this into UIPS */
		return htmlThumb;
	}

	int li = (int)floor((pt.y - RcContent().top) / DyLine());
	if (pt.x < mpcoldx[0])
		return htmlMoveNumber;
	int imv = -1;
	if (pt.x < mpcoldx[0] + mpcoldx[1])
		imv = li * 2;
	else if (pt.x < mpcoldx[0] + mpcoldx[1] + mpcoldx[2])
		imv = li * 2 + 1;
	if (Ga().FImvFirstIsBlack())
		imv--;
	if (imv < 0)
		return htmlEmptyBefore;
	if (imv >= uiga.ga.bdg.vmveGame.size())
		return htmlEmptyAfter;
	*pimv = imv;
	return htmlList;
}

void UIML::StartLeftDrag(const PT& pt)
{
	int imv;
	HTML html = HtmlHitTest(pt, &imv);
	if (html != htmlList)
		return;
	SetSel(imv, spmvFast);
	uiga.MoveToImv(imv, spmvAnimate);
}

void UIML::EndLeftDrag(const PT& pt, bool fClick)
{
}

void UIML::LeftDrag(const PT& pt)
{
}


void UIML::KeyDown(int vk)
{
	switch (vk) {
	case VK_UP:
	case VK_LEFT:
		uiga.MoveToImv(uiga.ga.bdg.imveCurLast - 1, spmvAnimate);
		break;
	case VK_DOWN:
	case VK_RIGHT:
		uiga.MoveToImv(uiga.ga.bdg.imveCurLast + 1, spmvAnimate);
		break;
	case VK_HOME:
		uiga.MoveToImv(0, spmvAnimate);
		break;
	case VK_END:
		uiga.MoveToImv(uiga.ga.bdg.vmveGame.size() - 1, spmvAnimate);
		break;
	case VK_PRIOR:
		uiga.MoveToImv(uiga.ga.bdg.imveCurLast - 5*2, spmvAnimate);
		break;
	case VK_NEXT:
		uiga.MoveToImv(uiga.ga.bdg.imveCurLast + 5*2, spmvAnimate);
		break;
	default:
		break;
	}
}