/*
 *
 *	uipa.cpp
 * 
 *	Screen panels, or top level UI elements
 * 
 */

#include "uipa.h"
#include "ga.h"
#include "debug.h"


/*
 *
 *	UIP class implementation
 *
 *	Screen panel implementation. Base class for the pieces of stuff
 *	that gets displayed on the screen
 *
 */


BRS* UIP::pbrTextSel;
TX* UIP::ptxTextSm;


/*	UIP::CreateRsrcClass
 *
 *	Static routine for creating the drawing objects necessary to draw the various
 *	screen panels.
 */
void UIP::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrTextSel)
		return;
	pdc->CreateSolidColorBrush(ColorF(0.8f, 0.0, 0.0), &pbrTextSel);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxTextSm);
}


void UIP::DiscardRsrcClass(void)
{
	SafeRelease(&pbrTextSel);
	SafeRelease(&pbrGridLine);
	SafeRelease(&ptxTextSm);
}


void UIP::SetShadow(void)
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


/*	UIP::UIP
 *
 *	The screen panel constructor.
 */
UIP::UIP(UIGA& uiga) : UI(&uiga), uiga(uiga)
{
}


/*	UIP::~UIP
 *
 *	The screen panel destructor
 */
UIP::~UIP(void)
{
}


/*	UIP::Draw
 *
 *	Base class for drawing a screen panel. The default implementation
 *	just fills the panel with the background brush.
 */
void UIP::Draw(const RC& rcUpdate)
{
	FillRc(RcInterior(), pbrBack);
}


/*	UIP::AdjustUIRcBounds
 *
 *	Helper layout function for creating top/bottom strip UIs in screen panels. If the
 *	child UI in pui is visible, it moves it just below (or above) the rc rectangle
 *	supplied, depending on fTop. We ask the child for its preferred height by calling
 *	SizLayoutPreferred.
 */
void UIP::AdjustUIRcBounds(UI& ui, RC& rc, bool fTop)
{
	if (!ui.FVisible())
		return;
	SIZ siz = ui.SizLayoutPreferred();
	assert(siz.height > 0.0f);
	if (fTop) {
		rc.top = rc.bottom;
		rc.bottom = rc.top + siz.height;
	}
	else {
		rc.bottom = rc.top;
		rc.top = rc.bottom - siz.height;
	}
	ui.SetBounds(rc);
	if (fTop)
		rc.bottom++;
	else
		rc.top--;
}


/*
 *
 *	UIPS
 * 
 *	Scrolling screen panel. Our scrolling model is designed around view and content
 *	rectangles. The view rectangle is the area within the content rectangle that is
 *	visible. Scrolling works by offsetting the content rectangle.
 * 
 *	In general, Direct2D drawing is fast enough that we don't implement actual
 *	scrolling on screen, we just redraw the entire area and use Direct2D's frame
 *	switching to get a flicker-free update.
 *
 *	Implementations of UIPS should implement DrawContent instead of Draw.
 * 
 */


UIPS::UIPS(UIGA& uiga) : UIP(uiga), rcView(0, 0, 0, 0), rcCont(0, 0, 0, 0), sbarVert(this)
{
}


void UIPS::Layout(void)
{
	RC rc = rcView;
	rc.left = rc.right;
	rc.right = rc.left + dxyScrollBarWidth;
	sbarVert.SetBounds(rc);
}


/*	UIPS::SetView
 *
 *	Sets the view rectangle, in local (relative to the UI) coordinates.
 */
void UIPS::SetView(const RC& rcView)
{
	this->rcView = rcView;
	this->rcView.right -= dxyScrollBarWidth;
	sbarVert.SetRangeView(rcView.top, rcView.bottom);
	UIPS::Layout();
}


void UIPS::SetContent(const RC& rcCont)
{
	this->rcCont = rcCont;
	sbarVert.SetRange(rcCont.top, rcCont.bottom);
	UIPS::Layout();
}


/*	UIPS::RcView
 *
 *	Returns the view rectangle of the scrolling screen panel, in screen panel
 *	coordinates.
 */
RC UIPS::RcView(void) const
{
	return rcView;
}


/*	UIPS::RcContent
 *
 *	Returns the content area of the screen panel, in screel panel coordinates. This is
 *	the correct coordinate system to draw in, as long as we're clipped to the view
 *	rect.
 */
RC UIPS::RcContent(void) const
{
	return rcCont;
}


/*	UIPS::UpdateContSize
 *
 *	Updates the size of the content area. Call this when you add content to
 *	the content area.
 */
void UIPS::UpdateContSize(const SIZ& siz)
{
	rcCont.bottom = rcCont.top + siz.height;
	rcCont.right = rcCont.left + siz.width;
	sbarVert.SetRange(rcCont.top, rcCont.bottom);
	UIPS::Layout();
}


/*	UIPS::AdjustRcView
 *
 *	Adjust the view and content rectangle based on a new view rectangle. The
 *	content rectangle is adjusted so that the width matches the view width,
 *	the content height doesn't change, and the scroll position remains the 
 *	same.
 * 
 *	Note that this is not a general purpose adjuster. It only works because
 *	all of our scrollers are full-width vertical scrolling areas, and fonts
 *	currently aren't scaling. 
 */
void UIPS::AdjustRcView(const RC& rcViewNew)
{
	float dy = rcCont.top - rcView.top;
	RC rcContNew = rcViewNew;
	rcContNew.bottom = rcContNew.top + rcCont.DyHeight();
	rcContNew.Offset(0, dy);
	SetView(rcViewNew);
	SetContent(rcContNew);
}


float UIPS::DyLine(void) const
{
	return 5.0f;
}


/*	UIPS::ScrollBy
 *
 *	Scrolls the content rectangle by the given number of units.
 */
void UIPS::ScrollBy(float dy)
{
	float yTopNew = rcCont.top + dy;
	ScrollTo(yTopNew);
}


/*	UIPS::ScrollTo
 *
 *	Scrolls the content rectangle so that yTopNew is its new top coordinate.
 */
void UIPS::ScrollTo(float yTopNew)
{
	if (yTopNew > rcView.top)
		yTopNew = rcView.top;
	float dyCont = rcCont.DyHeight();
	float dyLine = DyLine();
	if (yTopNew + dyCont < rcView.top + dyLine)
		yTopNew = rcView.top + dyLine - dyCont;
	float dy = yTopNew - rcCont.top;
	rcCont.Offset(0, dy);
	sbarVert.SetRange(rcCont.top, rcCont.bottom);
	Layout();
	Redraw();
}


/*	UIPS::FMakeVis
 *
 *	Makes the position y of height dy in the content area visible within the view
 *	rectangle. Returns true if we had to scroll to make the iteme visible, false if
 *	the item was already visible
 */
bool UIPS::FMakeVis(float y, float dy)
{
	if (y < rcView.top)
		rcCont.Offset(0, rcView.top - y);
	else if (y + dy > rcView.bottom)
		rcCont.Offset(0, rcView.bottom - (y + dy));
	else
		return false;
	sbarVert.SetRange(rcCont.top, rcCont.bottom);
	Layout();
	Redraw();
	return true;
}


/*	UIPS::Draw
 *
 *	A scrolling panel draw, which draws the content area and scrollbar
 */
void UIPS::Draw(const RC& rcUpdate)
{
	/* just redraw the entire content area clipped to the view */
	APP& app = App();
	app.pdc->PushAxisAlignedClip(RcGlobalFromLocal(rcView), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	FillRc(rcView, pbrBack);
	DrawContent(rcCont);
	app.pdc->PopAxisAlignedClip();
}


/*	UIPS::DrawContent
 *
 *	Virutal function for drawing the actual content area. This should be overriden
 *	by all implementations to provide the actual drawing code.
 */
void UIPS::DrawContent(const RC& rc)
{
}


void UIPS::MouseHover(const PT& pt, MHT mht)
{
	if (RcView().FContainsPt(pt))
		::SetCursor(App().hcurUpDown);
	else
		::SetCursor(App().hcurArrow);
}


void UIPS::ScrollWheel(const PT& pt, int dwheel)
{
	float dlifScroll = (float)dwheel / (float)WHEEL_DELTA;
	ScrollBy(DyLine() * dlifScroll);
}


void UIPS::ScrollLine(int dline)
{
	ScrollBy(dline * DyLine());
}


void UIPS::ScrollPage(int dpage)
{
	ScrollBy((RcView().DyHeight() - DyLine()) * dpage);
}


/*
 *
 *	SBAR implementation
 * 
 *	Scrollbar control
 * 
 */


SBAR::SBAR(UIPS* puipsParent) : UI(puipsParent), puips(puipsParent),
		yTopCont(0), yBotCont(100.0f), yTopView(0), yBotView(0),
		tidScroll(0), fScrollDelay(false), htsbarTrack(htsbarNone), ptMouseInit(0, 0), ptThumbTopInit(0, 0)
{
}


/*	SBAR::RcThumb
 *
 *	Returns the bounding box of the thumb
 */
RC SBAR::RcThumb(void) const
{
	float dyThumb = DyThumb();
	RC rcRange = RcInterior().Inflate(0.0, -3.0f);

	/* range within the scrollbar the top of the thumb can be within */

	float yMinTopThumb = rcRange.top;
	float yMaxTopThumb = rcRange.bottom - dyThumb;

	/* thumb rectangle in inset inside the range */

	RC rcThumb = rcRange;
	rcThumb.left += 2.5f;
	rcThumb.right -= 2.0f;

	/* and figure out where the top of the thumb should go */

	float pctTop = (yTopView - yTopCont) / (yBotCont - yTopCont);
	rcThumb.top = max(yMinTopThumb + (yMaxTopThumb - yMinTopThumb) * pctTop, rcRange.top);
	rcThumb.bottom = min(rcThumb.top + dyThumb, rcRange.bottom);

	return rcThumb;
}


float SBAR::DyThumb(void) const
{
	RC rcRange = RcInterior().Inflate(0.0, -3.0f);
	float dyThumb = dyThumbMin;
	if (yBotCont - yTopCont > 0)
		dyThumb = rcRange.DyHeight() * (yBotView - yTopView) / (yBotCont - yTopCont);
	dyThumb = clamp(dyThumb, dyThumbMin, rcRange.DyHeight());
	return dyThumb;
}


/*	SBAR::Draw
 *
 *	Draws the scrollbar
 */
void SBAR::Draw(const RC& rcUpdate)
{
	RC rcInt = RcInterior();
	RC rcThumb = RcThumb();

	/* and draw the scrollbar */

	FillRc(rcInt, pbrAltBack);
	RR rrThumb(rcThumb);
	rrThumb.radiusX = rrThumb.radiusY = rcThumb.DxWidth() / 2.0f;
	COLORBRS colorbrs(pbrGridLine, ColorF(0.75f, 0.75f, 0.75f));
	FillRr(rrThumb, pbrGridLine);
}


/*	SBAR::SetRange
 *
 *	Sets the content area range of the scrollbar
 */
void SBAR::SetRange(float yTop, float yBot)
{
	yTopCont = yTop;
	yBotCont = yBot;
}


/*	SBAR::SetRangeView
 *
 *	Sets the view range of the scrollbar
 */
void SBAR::SetRangeView(float yTop, float yBot)
{
	yTopView = yTop;
	yBotView = yBot;
}


/*	SBAR::HitTest
 *
 *	Hit tests the mouse point
 */
HTSBAR SBAR::HitTest(const PT& pt)
{
	RC rcInt = RcInterior();
	if (!rcInt.FContainsPt(pt))
		return htsbarNone;
	RC rcThumb = RcThumb();
	if (pt.y < rcThumb.top)
		return htsbarPageUp;
	if (pt.y < rcThumb.bottom)
		return htsbarThumb;
	if (pt.y < rcInt.bottom)
		return htsbarPageDown;
	return htsbarNone;
}


/*	SBAR::StartLeftDrag
 *
 *	Starts left-mouse button tracking in the scrollbar. Our interface does
 *	this: on line/page areas, starts a timer and auto-scrolls for as long as
 *	we keep the button down in the line/page area. For thumbing, we track
 *	the mouse for the new thumb position.
 */
void SBAR::StartLeftDrag(const PT& pt)
{
	SetCapt(this);

	switch (htsbarTrack = HitTest(ptMouseInit = pt)) {

	case htsbarPageUp:
		StartScrollRepeat();
		puips->ScrollPage(1);
		break;
	case htsbarPageDown:
		StartScrollRepeat();
		puips->ScrollPage(-1);
		break;

	case htsbarLineUp:
		StartScrollRepeat();
		puips->ScrollLine(1);
		break;
	case htsbarLineDown:
		StartScrollRepeat();
		puips->ScrollLine(-1);
		break;

	case htsbarThumb:
		ptThumbTopInit = RcThumb().PtTopLeft();
		break;

	default:
		htsbarTrack = htsbarNone;
		ReleaseCapt();
		break;
	}
}


/*	SBAR::EndLeftDrag
 *
 *	Stop the end left-mouse button dragging in the scrollbar. For paging/line
 *	scrolling, this just stops the scroll timer. For thumbing, it finishes
 *	up the thumb scroll.
 */
void SBAR::EndLeftDrag(const PT& pt)
{
	EndScrollRepeat();
	if (htsbarTrack == htsbarThumb)	// one last thumb positioning when thumbing
		LeftDrag(pt);
	htsbarTrack = htsbarNone;
	ReleaseCapt();
}


/*	SBAR::LeftDrag
 *
 *	Mouse dragging in the scrollbar. The only dragging we do is moving the
 *	thumb around, so this basically handles the drag thumbing.
 */
void SBAR::LeftDrag(const PT& ptMouse)
{
	if (htsbarTrack != htsbarThumb)
		return;
	HTSBAR htsbar = HitTest(ptMouse);
	if (htsbar == htsbarNone)
		return;

	/* compute where the top of the thumb is in the content area */

	RC rcThumb = RcThumb();
	float yThumbTopMin = RcInterior().top + 3.0f;
	float yThumbTopMax = RcInterior().bottom - 3.0f - rcThumb.DyHeight();
	float yThumbTopNew = clamp(ptThumbTopInit.y - ptMouseInit.y + ptMouse.y, yThumbTopMin, yThumbTopMax);
	float pct = (yThumbTopNew - yThumbTopMin) / (yThumbTopMax - yThumbTopMin);
	float yTopContNew = yTopView - pct * (yBotCont - yTopCont);
	
	/* and tell parent to do the actual scrolling */

	puips->ScrollTo(yTopContNew);
}


/*	SBAR::TickTimer
 *
 *	The only scrollbar timer is used to auto-repeat line and page scrolling
 *	while mousing down in the scrollbar.
 */
void SBAR::TickTimer(TID tid, UINT dtm)
{
	if (tid != tidScroll)
		return;
	HTSBAR htsbar = HitTest(PtLocalFromGlobal(PT(App().PtMessage())));
	if (htsbar != htsbarTrack)
		return;
	
	ContinueScrollRepeat();
	switch (htsbar) {
	case htsbarLineUp:
		puips->ScrollLine(1);
		break;
	case htsbarLineDown:
		puips->ScrollLine(-1);
		break;
	case htsbarPageUp:
		puips->ScrollPage(1);
		break;
	case htsbarPageDown:
		puips->ScrollPage(-1);
		break;
	default:
		assert(false);
		break;
	}
}


/*	SBAR::StartScrollRepeat
 *
 *	Stars the scroll repeater timer
 */
void SBAR::StartScrollRepeat(void)
{
	EndScrollRepeat();
	tidScroll = StartTimer(300);
	fScrollDelay = true;
}


/*	SBAR::ContinueScrollRepeat
 *
 *	Continues the scroll repeater timer. This should be called on every timer
 *	in order to handle the repeater delay
 */
void SBAR::ContinueScrollRepeat(void)
{
	if (!fScrollDelay)
		return;
	
	EndScrollRepeat();
	tidScroll = StartTimer(100);
	fScrollDelay = false;
}


/*	SBAR::EndScrollRepeat
 *
 *	Stops the scroll repeater
 */
void SBAR::EndScrollRepeat(void)
{
	if (!tidScroll)
		return;

	StopTimer(tidScroll);
	tidScroll = 0;
	fScrollDelay = false;
}


/*
 *
 *	UIBB
 * 
 *	Button bar in a scrolling panel
 * 
 */


UIBB::UIBB(UIPS* puiParent) : UI(puiParent)
{
}

void UIBB::Layout(void)
{
	/* TODO: do a standard layout of some kind */
}

SIZ UIBB::SizLayoutPreferred(void)
{
	return SIZ(-1.0f, 32.0f);
}

void UIBB::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrBack);
}

void UIBB::AdjustBtnRcBounds(UI* pui, RC& rc, float dxWidth)
{
	if (pui == nullptr || !pui->FVisible())
		return;
	rc.left = rc.right + 10.0f;
	rc.right = rc.left + dxWidth;
	pui->SetBounds(rc);
}


/*
 *
 *	UITIP class implementation
 *
 *	Tooltip user interface item
 *
 */


UITIP::UITIP(UI* puiParent) : UI(puiParent, false), puiOwner(nullptr)
{
}


void UITIP::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	FillRc(rc, pbrText);
	rc.Inflate(PT(-1.0, -1.0));
	FillRc(rc, pbrTip);
	if (puiOwner) {
		wstring sz = puiOwner->SzTip();
		if (!sz.empty())
			DrawSz(sz, ptxTip, rc.Inflate(PT(-5.0f, -3.0f)), pbrText);
	}
}


/*	UITIP::AttachOwner
 *
 *	Attach the tip UI elemennt to the specified UI element. Positions the tip UI near
 *	the owner.
 */
void UITIP::AttachOwner(UI* pui)
{
	puiOwner = pui;
	if (!puiOwner)
		return;
	RC rcOwner = puiOwner->RcInterior();
	rcOwner = puiOwner->RcGlobalFromLocal(rcOwner);
	wstring szTip = puiOwner->SzTip();
	SIZ sizTip = SizSz(szTip, ptxTip, 1000.0f, 1000.0f);
	sizTip.height += 8.0f;
	sizTip.width += 14.0f;
	RC rcTip = RC(PT(rcOwner.XCenter(), rcOwner.top - sizTip.height - 1.0f), sizTip);
	RC rcDesk = puiParent->RcInterior();
	if (rcTip.top < rcDesk.top)
		rcTip.Offset(PT(0.0f, sizTip.height + rcOwner.DyHeight() + 1.0f));
	if (rcTip.left < rcDesk.left)
		rcTip.Offset(PT(sizTip.width + rcOwner.DxWidth() + 1.0f, 0.0f));
	if (rcTip.bottom > rcDesk.bottom)
		rcTip.Offset(PT(0.0f, -(sizTip.height + rcOwner.DyHeight() + 1.0f)));
	if (rcTip.right > rcDesk.right)
		rcTip.Offset(PT(-(sizTip.width + rcOwner.DxWidth() + 1.0f), 0.0f));
	SetBounds(rcTip);
}
