/*
 *
 *	ui.cpp
 * 
 *	Our lightweight user interface items.
 * 
 */

#include "ui.h"


/*
 *	UI static drawing objects
 */

ID2D1SolidColorBrush* UI::pbrBack;
ID2D1SolidColorBrush* UI::pbrText;
IDWriteTextFormat* UI::ptfText;


void UI::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (pbrBack)
		return;
	prt->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
	prt->CreateSolidColorBrush(ColorF(0.4f, 0.4f, 0.4f), &pbrText);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptfText);
}


void UI::DiscardRsrc(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrText);
	SafeRelease(&ptfText);
}


/*	UI::UI
 *
 *	Constructor for the UI elements. Adds the item to the UI tree structure
 *	by connecting it to the parent.
 */
UI::UI(UI* puiParent) : puiParent(puiParent), rcfBounds(0, 0, 0, 0) 
{
	if (puiParent)
		puiParent->AddChild(this);
}


UI::UI(UI* puiParent, RCF rcfBounds) : puiParent(puiParent), rcfBounds(rcfBounds) 
{
	if (puiParent)
		puiParent->AddChild(this);
}


UI::~UI(void) 
{
	while (rgpuiChild.size() > 0)
		delete rgpuiChild[0];
	if (puiParent) {
		puiParent->RemoveChild(this);
		puiParent = NULL;
	}
}


/*	UI::AddChild
 *
 *	Adds a child UI to the UI element.
 */
void UI::AddChild(UI* puiChild) 
{
	rgpuiChild.push_back(puiChild);
}


/*	UI::RemoveChild
 *
 *	Removes the child UI from its parent's child list.
 */
void UI::RemoveChild(UI* puiChild) 
{
	unsigned ipuiTo = 0, ipuiFrom;
	for (ipuiFrom = 0; ipuiFrom < rgpuiChild.size(); ipuiFrom++) {
		if (rgpuiChild[ipuiFrom] != puiChild)
			rgpuiChild[ipuiTo++] = rgpuiChild[ipuiFrom];
	}
	assert(ipuiTo < ipuiFrom);
	rgpuiChild.resize(ipuiTo);
}


/*	UI::RcfInterior
 *
 *	Returns the interior of the given UI element, in local coordinates.
 */
RCF UI::RcfInterior(void) const 
{
	RCF rcf = rcfBounds;
	return rcf.Offset(-rcfBounds.left, -rcfBounds.top);
}


void UI::SetBounds(RCF rcfNew) 
{
	rcfBounds = rcfNew;
}


void UI::Resize(PTF ptfNew) 
{
	rcfBounds.right = rcfBounds.left + ptfNew.x;
	rcfBounds.bottom = rcfBounds.top + ptfNew.y;
}


void UI::Move(PTF ptfNew) 
{
	rcfBounds.Offset(ptfNew.x - rcfBounds.left, ptfNew.y - rcfBounds.top);
}


RCF UI::RcfToParent(RCF rcf) const
{
	if (puiParent == NULL)
		return rcf;
	return rcf.Offset(rcfBounds.left, rcfBounds.top);
}


RCF UI::RcfToGlobal(RCF rcf) const
{
	if (puiParent == NULL)
		return rcf;
	return puiParent->RcfToGlobal(RcfToParent(rcf));
}


PTF UI::PtfToParent(PTF ptf) const
{
	if (puiParent == NULL)
		return ptf;
	return ptf.Offset(rcfBounds.left, rcfBounds.top);
}


PTF UI::PtfToGlobal(PTF ptf) const
{
	if (puiParent == NULL)
		return ptf;
	return puiParent->PtfToGlobal(PtfToParent(ptf));
}


void UI::Draw(void)
{
}


ID2D1RenderTarget* UI::PrtGet(void) const
{
	if (puiParent == NULL)
		return NULL;
	return puiParent->PrtGet();
}


/*	UI::FillRcf
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRcf(RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfToGlobal(rcf);
	PrtGet()->FillRectangle(&rcf, pbr);
}


/*	UI::FillEllf
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(PtfToGlobal(PTF(0, 0)));
	PrtGet()->FillEllipse(&ellf, pbr);
}


/*	UI::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local UI coordinates.
 */
void UI::DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfToGlobal(rcf);
	PrtGet()->DrawText(sz.c_str(), (UINT32)sz.size(), ptf, &rcf, pbr==NULL ? pbrText : pbr);
}


/*	UI::DrawBmp
 *
 *	Helper function for drawing part of a bitmap on the screen panel. Destination
 *	coordinates are in local UI coordinates.
 */
void UI::DrawBmp(RCF rcfTo, ID2D1Bitmap* pbmp, RCF rcfFrom, float opacity) const
{
	PrtGet()->DrawBitmap(pbmp, rcfTo.Offset(rcfBounds.PtfTopLeft()), 
		opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rcfFrom);
}
