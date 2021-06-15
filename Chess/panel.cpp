/*
 *
 *	panel.cpp
 * 
 *	Screen panels, or top level UI elements
 * 
 */

#include "panel.h"
#include "ga.h"


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
void UIP::Draw(RCF rcfUpdate)
{
	FillRcf(RcfInterior(), pbrBack);
}


/*	UIP::AdjustUIRcfBounds
 *
 *	Helper layout function for creating top/bottom strip UIs in screen panels. If the
 *	child UI in pui is visible, it moves it just below (or above) the rcf rectangle
 *	supplied, depending on fTop. We ask the child for its preferred height by calling
 *	SizfLayoutPreferred.
 */
void UIP::AdjustUIRcfBounds(UI* pui, RCF& rcf, bool fTop)
{
	if (pui == NULL || !pui->FVisible())
		return;
	SIZF sizf = pui->SizfLayoutPreferred();
	assert(sizf.height > 0.0f);
	if (fTop) {
		rcf.top = rcf.bottom;
		rcf.bottom = rcf.top + sizf.height;
	}
	else {
		rcf.bottom = rcf.top;
		rcf.top = rcf.bottom - sizf.height;
	}
	pui->SetBounds(rcf);
	if (fTop)
		rcf.bottom++;
	else
		rcf.top--;
}


/*
 *
 *	UIPS
 * 
 *	Scrolling screen panel
 * 
 */


UIPS::UIPS(GA* pga) : UIP(pga), rcfView(0, 0, 0, 0), rcfCont(0, 0, 0, 0)
{
}


void UIPS::SetView(const RCF& rcfView)
{
	this->rcfView = rcfView;
	this->rcfView.right -= dxyfScrollBarWidth;
}


void UIPS::SetContent(const RCF& rcfCont)
{
	this->rcfCont = rcfCont;
}


RCF UIPS::RcfView(void) const
{
	return rcfView;
}


RCF UIPS::RcfContent(void) const
{
	RCF rcf = rcfCont;
	return rcf;
}


/*	UpdateContSize
 *
 *	Updates the size of the content area. Call this when you add content to
 *	the content area.
 */
void UIPS::UpdateContSize(const SIZF& sizf)
{
	rcfCont.bottom = rcfCont.top + sizf.height;
	rcfCont.right = rcfCont.left + sizf.width;
}


/*	UIPS::AdjustRcfView
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
void UIPS::AdjustRcfView(RCF rcfViewNew)
{
	float dyf = rcfCont.top - rcfView.top;
	RCF rcfContNew = rcfViewNew;
	rcfContNew.bottom = rcfContNew.top + rcfCont.DyfHeight();
	rcfContNew.Offset(0, dyf);
	SetView(rcfViewNew);
	SetContent(rcfContNew);
}


float UIPS::DyfLine(void) const
{
	return 5.0f;
}



/*	UIPS::ScrollTo
 *
 *	Scrolls the content rectangle by the given number of units.
 */
void UIPS::ScrollTo(float dyf)
{
	float yfTopNew = rcfCont.top + dyf;
	if (yfTopNew > rcfView.top)
		yfTopNew = rcfView.top;
	float dyfCont = rcfCont.DyfHeight();
	float dyfLine = DyfLine();
	if (yfTopNew + dyfCont < rcfView.top + dyfLine)
		yfTopNew = rcfView.top + dyfLine - dyfCont;
	dyf = yfTopNew - rcfCont.top;
	rcfCont.Offset(0, dyf);
	Redraw();
}


bool UIPS::FMakeVis(float yf, float dyf)
{
	if (yf < rcfView.top)
		rcfCont.Offset(0, rcfView.top - yf);
	else if (yf + dyf > rcfView.bottom)
		rcfCont.Offset(0, rcfView.bottom - (yf + dyf));
	else
		return false;
	Redraw();
	return true;
}


void UIPS::Draw(RCF rcfUpdate)
{
	/* just redraw the entire content area clipped to the view */
	APP& app = AppGet();
	app.pdc->PushAxisAlignedClip(RcfGlobalFromLocal(rcfView), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	FillRcf(rcfView, pbrBack);
	DrawContent(rcfCont);
	app.pdc->PopAxisAlignedClip();
	DrawScrollBar();
}


void UIPS::DrawContent(RCF rcf)
{
}


void UIPS::DrawScrollBar(void)
{
	RCF rcfScroll = rcfView;
	rcfScroll.left = rcfView.right;
	rcfScroll.right = rcfScroll.left + dxyfScrollBarWidth;
	FillRcf(rcfScroll, pbrAltBack);
	RCF rcfThumb = rcfScroll;
	if (rcfCont.DyfHeight() > 0) {
		rcfThumb.top = rcfScroll.top + rcfScroll.DyfHeight() * (rcfView.top - rcfCont.top) / rcfCont.DyfHeight();
		rcfThumb.bottom = rcfScroll.top + rcfScroll.DyfHeight() * (rcfView.bottom - rcfCont.top) / rcfCont.DyfHeight();
	}
	if (rcfThumb.top < rcfScroll.top)
		rcfThumb.top = rcfScroll.top;
	if (rcfThumb.bottom > rcfScroll.bottom)
		rcfThumb.bottom = rcfScroll.bottom;
	FillRcf(rcfThumb, pbrBack);
	FillRcf(RCF(rcfThumb.left, rcfThumb.top - 1, rcfThumb.right, rcfThumb.top), pbrGridLine);
	FillRcf(RCF(rcfThumb.left, rcfThumb.bottom, rcfThumb.right, rcfThumb.bottom + 1), pbrGridLine);
	FillRcf(RCF(rcfThumb.left, rcfThumb.top, rcfThumb.left + 1, rcfThumb.bottom), pbrGridLine);

}


void UIPS::MouseHover(PTF ptf, MHT mht)
{
	if (RcfView().FContainsPtf(ptf))
		::SetCursor(AppGet().hcurUpDown);
	else
		::SetCursor(AppGet().hcurArrow);
}


void UIPS::ScrollWheel(PTF ptf, int dwheel)
{
	float dlifScroll = (float)dwheel / (float)WHEEL_DELTA;
	ScrollTo(DyfLine() * dlifScroll);
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

SIZF UIBB::SizfLayoutPreferred(void)
{
	return SIZF(-1.0f, 32.0f);
}

void UIBB::Draw(RCF rcfUpdate)
{
	FillRcf(rcfUpdate, pbrBack);
}

void UIBB::AdjustBtnRcfBounds(UI* pui, RCF& rcf, float dxfWidth)
{
	if (pui == NULL || !pui->FVisible())
		return;
	rcf.left = rcf.right + 10.0f;
	rcf.right = rcf.left + dxfWidth;
	pui->SetBounds(rcf);
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


void UITIP::Draw(RCF rcfUpdate)
{
	RCF rcf = RcfInterior();
	FillRcf(rcf, pbrText);
	rcf.Inflate(PTF(-1.0, -1.0));
	FillRcf(rcf, pbrTip);
	if (puiOwner) {
		wstring sz = puiOwner->SzTip();
		if (!sz.empty())
			DrawSz(sz, ptxTip, rcf.Inflate(PTF(-5.0f, -3.0f)), pbrText);
	}
}


void UITIP::AttachOwner(UI* pui)
{
	puiOwner = pui;
	if (!puiOwner)
		return;
	RCF rcfOwner = puiOwner->RcfInterior();
	rcfOwner = puiOwner->RcfGlobalFromLocal(rcfOwner);
	wstring szTip = puiOwner->SzTip();
	SIZF sizfTip = SizfSz(szTip, ptxTip, 1000.0f, 1000.0f);
	sizfTip.height += 8.0f;
	sizfTip.width += 14.0f;
	RCF rcfTip = RCF(PTF(rcfOwner.XCenter(), rcfOwner.top - sizfTip.height - 1.0f), sizfTip);
	RCF rcfDesk = puiParent->RcfInterior();
	if (rcfTip.top < rcfDesk.top)
		rcfTip.Offset(PTF(0.0f, sizfTip.height + rcfOwner.DyfHeight() + 1.0f));
	if (rcfTip.left < rcfDesk.left)
		rcfTip.Offset(PTF(sizfTip.width + rcfOwner.DxfWidth() + 1.0f, 0.0f));
	if (rcfTip.bottom > rcfDesk.bottom)
		rcfTip.Offset(PTF(0.0f, -(sizfTip.height + rcfOwner.DyfHeight() + 1.0f)));
	if (rcfTip.right > rcfDesk.right)
		rcfTip.Offset(PTF(-(sizfTip.width + rcfOwner.DxfWidth() + 1.0f), 0.0f));
	SetBounds(rcfTip);
}
