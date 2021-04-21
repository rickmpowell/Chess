/*
 *
 *	ui.cpp
 * 
 *	Our lightweight user interface items.
 * 
 */

#include "ui.h"


WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch)
{
	if (imv / 10 != 0)
		pch = PchDecodeInt(imv / 10, pch);
	*pch++ = imv % 10 + L'0';
	*pch = '\0';
	return pch;
}


/*
 *	UI static drawing objects
 */

ID2D1SolidColorBrush* UI::pbrBack;
ID2D1SolidColorBrush* UI::pbrAltBack;
ID2D1SolidColorBrush* UI::pbrGridLine;
ID2D1SolidColorBrush* UI::pbrText;
IDWriteTextFormat* UI::ptfText;


void UI::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	if (pbrBack)
		return;
	prt->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
	prt->CreateSolidColorBrush(ColorF(.95f, .95f, .95f), &pbrAltBack);
	prt->CreateSolidColorBrush(ColorF(.8f, .8f, .8f), &pbrGridLine);
	prt->CreateSolidColorBrush(ColorF(0.4f, 0.4f, 0.4f), &pbrText);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptfText);
}


void UI::DiscardRsrc(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrAltBack);
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrText);
	SafeRelease(&ptfText);
}

ID2D1PathGeometry* UI::PgeomCreate(ID2D1Factory* pfactd2d, PTF rgptf[], int cptf)
{
	/* capture X, which is created as a cross that is rotated later */
	ID2D1PathGeometry* pgeom;
	pfactd2d->CreatePathGeometry(&pgeom);
	ID2D1GeometrySink* psink;
	pgeom->Open(&psink);
	psink->BeginFigure(rgptf[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&rgptf[1], cptf - 1);
	psink->EndFigure(D2D1_FIGURE_END_CLOSED);
	psink->Close();
	SafeRelease(&psink);
	return pgeom;
}


ID2D1Bitmap* UI::PbmpFromPngRes(int idb, ID2D1RenderTarget* prt, IWICImagingFactory* pfactwic)
{
	HRSRC hres = ::FindResource(NULL, MAKEINTRESOURCE(idb), L"IMAGE");
	if (hres == NULL)
		return NULL;
	ULONG cbRes = ::SizeofResource(NULL, hres);
	HGLOBAL hresLoad = ::LoadResource(NULL, hres);
	if (hresLoad == NULL)
		return NULL;
	BYTE* pbRes = (BYTE*)::LockResource(hresLoad);
	if (pbRes == NULL)
		return NULL;

	HRESULT hr;
	IWICStream* pstm = NULL;
	hr = pfactwic->CreateStream(&pstm);
	hr = pstm->InitializeFromMemory(pbRes, cbRes);
	IWICBitmapDecoder* pdec = NULL;
	hr = pfactwic->CreateDecoderFromStream(pstm, NULL, WICDecodeMetadataCacheOnLoad, &pdec);
	IWICBitmapFrameDecode* pframe = NULL;
	hr = pdec->GetFrame(0, &pframe);
	IWICFormatConverter* pconv = NULL;
	hr = pfactwic->CreateFormatConverter(&pconv);
	hr = pconv->Initialize(pframe, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
	ID2D1Bitmap* pbmp;
	hr = prt->CreateBitmapFromWicBitmap(pconv, NULL, &pbmp);

	SafeRelease(&pframe);
	SafeRelease(&pconv);
	SafeRelease(&pdec);
	SafeRelease(&pstm);

	return pbmp;
}


/*	UI::UI
 *
 *	Constructor for the UI elements. Adds the item to the UI tree structure
 *	by connecting it to the parent.
 */
UI::UI(UI* puiParent, bool fVisible) : puiParent(puiParent), rcfBounds(0, 0, 0, 0), fVisible(fVisible)
{
	if (puiParent)
		puiParent->AddChild(this);
}


UI::UI(UI* puiParent, RCF rcfBounds, bool fVisible) : puiParent(puiParent), rcfBounds(rcfBounds), fVisible(fVisible)
{
	if (puiParent) {
		puiParent->AddChild(this);
		this->rcfBounds.Offset(puiParent->rcfBounds.left, puiParent->rcfBounds.top);
	}
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


/*	UI::FVisible
 *
 *	Returns true if the UI element is visible
 */
bool UI::FVisible(void) const
{
	return fVisible;
}


/*	UI::SetBounds
 *
 *	Sets the bounding box for the UI element. Coordinates are relative
 *	to the parent's coordinate system
 */
void UI::SetBounds(RCF rcfNew) 
{
	if (puiParent)
		rcfNew.Offset(puiParent->rcfBounds.left, puiParent->rcfBounds.top);
	rcfBounds = rcfNew;
}


/*	UI::Resizes
 *
 *	Resizes the UI element.
 */
void UI::Resize(PTF ptfNew) 
{
	rcfBounds.right = rcfBounds.left + ptfNew.x;
	rcfBounds.bottom = rcfBounds.top + ptfNew.y;
}


/*	UI::Move
 *
 *	Moves the UI element to the new upper left position. In parent coordinates.
 */
void UI::Move(PTF ptfNew) 
{
	if (puiParent) {
		/* convert to global coordinates */
		ptfNew = PTF(ptfNew.x + puiParent->rcfBounds.left, 
			ptfNew.y + puiParent->rcfBounds.top);
	}
	rcfBounds.Offset(ptfNew.x - rcfBounds.left, ptfNew.y - rcfBounds.top);
}


/*	UI::Show
 *
 *	Shows or hides a UI element and does the appropriate redraw
 */
void UI::Show(bool fVisNew)
{
	if (fVisible == fVisNew)
		return;
	fVisible = fVisNew;
	if (puiParent)
		puiParent->Redraw();
}


/*	UI::RcfParentFromLocal
 *
 *	Converts a rectangle from local coordinates to parent coordinates
 */
RCF UI::RcfParentFromLocal(RCF rcf) const
{
	if (puiParent == NULL)
		return rcf;
	return rcf.Offset(rcfBounds.left-puiParent->rcfBounds.left, 
		rcfBounds.top-puiParent->rcfBounds.top);
}


RCF UI::RcfGlobalFromLocal(RCF rcf) const
{
	return rcf.Offset(rcfBounds.left, rcfBounds.top);
}


RCF UI::RcfLocalFromParent(RCF rcf) const
{
	if (puiParent == NULL)
		return rcf;
	return rcf.Offset(puiParent->rcfBounds.left - rcfBounds.left,
		puiParent->rcfBounds.top - rcfBounds.top);
}


RCF UI::RcfLocalFromGlobal(RCF rcf) const
{
	return rcf.Offset(-rcfBounds.left, -rcfBounds.top);
}


/*	UI::PtfParentFromLocal
 *
 *	Converts a point from loal coordinates to local coordinates of the
 *	parent UI element.
 */
PTF UI::PtfParentFromLocal(PTF ptf) const
{
	if (puiParent == NULL)
		return ptf;
	return PTF(ptf.x + rcfBounds.left - puiParent->rcfBounds.left,
		ptf.y + rcfBounds.top - puiParent->rcfBounds.left);

}


/*	UI::PtfGlobalFromLocal
 *
 *	Converts a point from local coordinates to global (relative to the 
 *	main window) coordinates.
 */
PTF UI::PtfGlobalFromLocal(PTF ptf) const
{
	return PTF(ptf.x + rcfBounds.left, ptf.y + rcfBounds.top);
}


/*	UI::Update
 *
 *	Updates the UI element and all child elements. prcfUpdate is in
 *	global coordinates.
 */
void UI::Update(const RCF* prcfUpdate)
{
	if (!fVisible)
		return;
	if (prcfUpdate == NULL)
		prcfUpdate = &rcfBounds;
	RCF rcf = *prcfUpdate & rcfBounds;
	if (!rcf)
		return;
	RCF rcfDraw = RcfLocalFromGlobal(rcf);
	Draw(&rcfDraw);
	for (UI* pui : rgpuiChild)
		pui->Update(&rcf);
}


/*	UI::Redraw
 *
 *	Redraws the UI element and all children
 */
void UI::Redraw(void)
{
	if (!fVisible)
		return;
	ID2D1RenderTarget* prt = PrtGet();
	BeginDraw();
	Update(&rcfBounds);
	EndDraw();
}


void UI::Draw(const RCF* prcfDraw)
{
}


/*	UI::PrtGet
 *
 *	Gets the render target we need to draw in for all the UI elements.
 *	The render target will be in global coordinates.
 */
ID2D1RenderTarget* UI::PrtGet(void) const
{
	if (puiParent == NULL)
		return NULL;
	return puiParent->PrtGet();
}


void UI::BeginDraw(void)
{
	if (puiParent)
		puiParent->BeginDraw();
	else
		PrtGet()->BeginDraw();
}


void UI::EndDraw(void)
{
	if (puiParent)
		puiParent->EndDraw();
	else
		PrtGet()->EndDraw();
}

/*	UI::FillRcf
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRcf(RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	PrtGet()->FillRectangle(&rcf, pbr);
}


/*	UI::FillEllf
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(PtfGlobalFromLocal(PTF(0, 0)));
	PrtGet()->FillEllipse(&ellf, pbr);
}


/*	UI::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local UI coordinates.
 */
void UI::DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	PrtGet()->DrawText(sz.c_str(), (UINT32)sz.size(), ptf, &rcf, pbr==NULL ? pbrText : pbr);
}


void UI::DrawSzCenter(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	DWRITE_TEXT_ALIGNMENT taSav = ptf->GetTextAlignment();
	ptf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	DrawSz(sz, ptf, rcf, pbr);
	ptf->SetTextAlignment(taSav);
}


/*	UI::DrawRgch
 *
 *	Helper function for drawing text on the screen panel. Rectangle is in local
 *	UI coordinates
 */
void UI::DrawRgch(const WCHAR* rgch, int cch, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	PrtGet()->DrawText(rgch, (UINT32)cch, ptf, &rcf, pbr == NULL ? pbrText : pbr);

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
