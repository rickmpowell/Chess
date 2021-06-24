/*
 *
 *	panel.cpp
 * 
 *	Screen panels, or top level UI elements
 * 
 */

#include "panel.h"
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
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
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
UIP::UIP(GA* pga) : UI(pga), ga(*pga)
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
void UIP::Draw(RC rcUpdate)
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
void UIP::AdjustUIRcBounds(UI* pui, RC& rc, bool fTop)
{
	if (pui == NULL || !pui->FVisible())
		return;
	SIZ siz = pui->SizLayoutPreferred();
	assert(siz.height > 0.0f);
	if (fTop) {
		rc.top = rc.bottom;
		rc.bottom = rc.top + siz.height;
	}
	else {
		rc.bottom = rc.top;
		rc.top = rc.bottom - siz.height;
	}
	pui->SetBounds(rc);
	if (fTop)
		rc.bottom++;
	else
		rc.top--;
}


/*	UIP::FDepthLog
 *
 *	Logging helper, which adjusts the depth of our tree-structured log and returns true 
 *	if the caller should continue to actually log the data. This is used as an optimization
 *	so that we don't bother to actually construct the logging data if we're not going
 *	to actually save the data in the log (the user has control over logging depth).
 */
bool UIP::FDepthLog(LGT lgt, int& depth)
{
	return ga.uidb.FDepthLog(lgt, depth);
}


/*	UIP::AddLog
 *
 *	The point where we actually log the data to the log. We have a tree-structured log,
 *	with open/close tags for creating depth. Our structure is equivalent to XML, with
 *	attributes on the open tag. 
 */
void UIP::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	ga.uidb.AddLog(lgt, lgf, depth, tag, szData);
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
 */


UIPS::UIPS(GA* pga) : UIP(pga), rcView(0, 0, 0, 0), rcCont(0, 0, 0, 0)
{
}


void UIPS::SetView(const RC& rcView)
{
	this->rcView = rcView;
	this->rcView.right -= dxyScrollBarWidth;
}


void UIPS::SetContent(const RC& rcCont)
{
	this->rcCont = rcCont;
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


/*	UpdateContSize
 *
 *	Updates the size of the content area. Call this when you add content to
 *	the content area.
 */
void UIPS::UpdateContSize(const SIZ& siz)
{
	rcCont.bottom = rcCont.top + siz.height;
	rcCont.right = rcCont.left + siz.width;
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
void UIPS::AdjustRcView(RC rcViewNew)
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


/*	UIPS::ScrollTo
 *
 *	Scrolls the content rectangle by the given number of units.
 */
void UIPS::ScrollTo(float dy)
{
	float yTopNew = rcCont.top + dy;
	if (yTopNew > rcView.top)
		yTopNew = rcView.top;
	float dyCont = rcCont.DyHeight();
	float dyLine = DyLine();
	if (yTopNew + dyCont < rcView.top + dyLine)
		yTopNew = rcView.top + dyLine - dyCont;
	dy = yTopNew - rcCont.top;
	rcCont.Offset(0, dy);
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
	Redraw();
	return true;
}


/*	UIPS::Draw
 *
 *	A scrolling panel draw, which draws the content area and scrollbar
 */
void UIPS::Draw(RC rcUpdate)
{
	/* just redraw the entire content area clipped to the view */
	APP& app = App();
	app.pdc->PushAxisAlignedClip(RcGlobalFromLocal(rcView), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	FillRc(rcView, pbrBack);
	DrawContent(rcCont);
	app.pdc->PopAxisAlignedClip();
	DrawScrollBar();
}


void UIPS::DrawContent(RC rc)
{
}


void UIPS::DrawScrollBar(void)
{
	RC rcScroll = rcView;
	rcScroll.left = rcView.right;
	rcScroll.right = rcScroll.left + dxyScrollBarWidth;
	FillRc(rcScroll, pbrAltBack);
	RC rcThumb = rcScroll;
	if (rcCont.DyHeight() > 0) {
		rcThumb.top = rcScroll.top + rcScroll.DyHeight() * (rcView.top - rcCont.top) / rcCont.DyHeight();
		rcThumb.bottom = rcScroll.top + rcScroll.DyHeight() * (rcView.bottom - rcCont.top) / rcCont.DyHeight();
	}
	rcThumb.top = max(rcThumb.top, rcScroll.top);
	rcThumb.bottom = min(rcThumb.bottom, rcScroll.bottom);
	FillRc(rcThumb, pbrBack);
	FillRc(RC(rcThumb.left, rcThumb.top - 1, rcThumb.right, rcThumb.top), pbrGridLine);
	FillRc(RC(rcThumb.left, rcThumb.bottom, rcThumb.right, rcThumb.bottom + 1), pbrGridLine);
	FillRc(RC(rcThumb.left, rcThumb.top, rcThumb.left + 1, rcThumb.bottom), pbrGridLine);

}


void UIPS::MouseHover(PT pt, MHT mht)
{
	if (RcView().FContainsPt(pt))
		::SetCursor(App().hcurUpDown);
	else
		::SetCursor(App().hcurArrow);
}


void UIPS::ScrollWheel(PT pt, int dwheel)
{
	float dlifScroll = (float)dwheel / (float)WHEEL_DELTA;
	ScrollTo(DyLine() * dlifScroll);
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

void UIBB::Draw(RC rcUpdate)
{
	FillRc(rcUpdate, pbrBack);
}

void UIBB::AdjustBtnRcBounds(UI* pui, RC& rc, float dxWidth)
{
	if (pui == NULL || !pui->FVisible())
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


UITIP::UITIP(UI* puiParent) : UI(puiParent, false), puiOwner(NULL)
{
}


void UITIP::Draw(RC rcUpdate)
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
