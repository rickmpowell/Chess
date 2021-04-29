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

BRS* UI::pbrBack;
BRS* UI::pbrAltBack;
BRS* UI::pbrGridLine;
BRS* UI::pbrText;
TF* UI::ptfText;


void UI::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrBack)
		return;
	pdc->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
	pdc->CreateSolidColorBrush(ColorF(.95f, .95f, .95f), &pbrAltBack);
	pdc->CreateSolidColorBrush(ColorF(.8f, .8f, .8f), &pbrGridLine);
	pdc->CreateSolidColorBrush(ColorF(0.4f, 0.4f, 0.4f), &pbrText);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptfText);
	BTNCH::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
}


void UI::DiscardRsrcClass(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrAltBack);
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrText);
	SafeRelease(&ptfText);
}


ID2D1PathGeometry* UI::PgeomCreate(FACTD2* pfactd2, PTF rgptf[], int cptf)
{
	/* capture X, which is created as a cross that is rotated later */
	ID2D1PathGeometry* pgeom;
	pfactd2->CreatePathGeometry(&pgeom);
	ID2D1GeometrySink* psink;
	pgeom->Open(&psink);
	psink->BeginFigure(rgptf[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&rgptf[1], cptf - 1);
	psink->EndFigure(D2D1_FIGURE_END_CLOSED);
	psink->Close();
	SafeRelease(&psink);
	return pgeom;
}


BMP* UI::PbmpFromPngRes(int idb, DC* pdc, FACTWIC* pfactwic)
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
	ID2D1Bitmap1* pbmp;
	hr = pdc->CreateBitmapFromWicBitmap(pconv, NULL, &pbmp);

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


/*	UI::PuiPrevSib
 *
 *	Returns the UI's previous sibling
 */
UI* UI::PuiPrevSib(void) const
{
	if (puiParent == NULL)
		return NULL;
	UI* puiPrev = NULL;
	for (UI* pui : puiParent->rgpuiChild) {
		if (pui == this)
			return puiPrev;
		puiPrev = pui;
	}
	assert(false);
	return NULL;
}


UI* UI::PuiNextSib(void) const
{
	if (puiParent == NULL)
		return NULL;
	for (int ipui = 0; ipui < puiParent->rgpuiChild.size() - 1; ipui++)
		if (puiParent->rgpuiChild[ipui] == this)
			return puiParent->rgpuiChild[ipui + 1];
	return NULL;
}


void UI::CreateRsrc(void)
{
	for (UI* puiChild : rgpuiChild)
		puiChild->CreateRsrc();
}


void UI::DiscardRsrc(void)
{
	for (UI* puiChild : rgpuiChild)
		puiChild->DiscardRsrc();
}


void UI::Layout(void)
{
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


RCF UI::RcfBounds(void) const
{
	RCF rcf = rcfBounds;
	if (puiParent)
		rcf.Offset(-puiParent->rcfBounds.left, -puiParent->rcfBounds.top);
	return rcf;
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
	rcfBounds.right = rcfBounds.left + rcfNew.DxfWidth();
	rcfBounds.bottom = rcfBounds.top + rcfNew.DyfHeight();
	OffsetBounds(rcfNew.left - rcfBounds.left, rcfNew.top - rcfBounds.top);
	Layout();
}


/*	UI::Resize
 *
 *	Resizes the UI element.
 */
void UI::Resize(PTF ptfNew) 
{
	rcfBounds.right = rcfBounds.left + ptfNew.x;
	rcfBounds.bottom = rcfBounds.top + ptfNew.y;
	Layout();
}


/*	UI::Move
 *
 *	Moves the UI element to the new upper left position. In parent coordinates.
 */
void UI::Move(PTF ptfNew) 
{
	if (puiParent) {
		/* convert to global coordinates */
		ptfNew.x += puiParent->rcfBounds.left;
		ptfNew.y += puiParent->rcfBounds.top;
	}
	float dxf = ptfNew.x - rcfBounds.left;
	float dyf = ptfNew.y - rcfBounds.top;
	OffsetBounds(dxf, dyf);
}


void UI::OffsetBounds(float dxf, float dyf)
{
	rcfBounds.Offset(dxf, dyf);
	for (UI* pui : rgpuiChild)
		pui->OffsetBounds(dxf, dyf);
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
	Layout();
	if (puiParent)
		puiParent->Redraw();
}


/*	UI::PuiFromPtf
 *
 *	Returns the UI element the point is over. Point is in global coordinates.
 *	Returns NULL if the point is not within the UI.
 */
UI* UI::PuiFromPtf(PTF ptf)
{
	if (!rcfBounds.FContainsPtf(ptf))
		return NULL;
	for (UI* puiChild : rgpuiChild) {
		UI* pui = puiChild->PuiFromPtf(ptf);
		if (pui)
			return pui;
	}
	return this;
}


void UI::StartLeftDrag(PTF ptf)
{
	SetCapt(this);
}


void UI::EndLeftDrag(PTF ptf)
{
	ReleaseCapt();
}


void UI::LeftDrag(PTF ptf)
{
}


void UI::MouseHover(PTF ptf, MHT mht)
{
}


void UI::SetCapt(UI* pui)
{
	if (puiParent)
		puiParent->SetCapt(pui);
}


void UI::ReleaseCapt(void)
{
	if (puiParent)
		puiParent->ReleaseCapt();
}


void UI::DispatchCmd(int cmd)
{
	if (puiParent)
		puiParent->DispatchCmd(cmd);;
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


/*	UI::PtfLocalFromGlobal
 *
 *	Converts a point from global (relative to the main top-level window) to
 *	local coordinates.
 */
PTF UI::PtfLocalFromGlobal(PTF ptf) const
{
	return PTF(ptf.x - rcfBounds.left, ptf.y - rcfBounds.top);
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
	BeginDraw();
	Update(&rcfBounds);
	EndDraw();
}


void UI::Draw(const RCF* prcfDraw)
{
}


void UI::InvalRcf(RCF rcf, bool fErase) const
{
	if (rcf.top >= rcf.bottom || rcf.left >= rcf.right || puiParent == NULL)
		return;
	puiParent->InvalRcf(RcfParentFromLocal(rcf), fErase);
}


void UI::PresentSwch(void) const
{
	if (puiParent == NULL)
		return;
	puiParent->PresentSwch();
}


APP& UI::AppGet(void) const
{
	assert(puiParent);
	return puiParent->AppGet();
}


void UI::BeginDraw(void)
{
	assert(puiParent);
	puiParent->BeginDraw();
}


void UI::EndDraw(void)
{
	assert(puiParent);
	puiParent->EndDraw();
}


/*	UI::FillRcf
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRcf(RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	AppGet().pdc->FillRectangle(&rcf, pbr);
}


/*	UI::FillEllf
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(PtfGlobalFromLocal(PTF(0, 0)));
	AppGet().pdc->FillEllipse(&ellf, pbr);
}


/*	UI::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local UI coordinates.
 */
void UI::DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	AppGet().pdc->DrawText(sz.c_str(), (UINT32)sz.size(), ptf, &rcf, pbr==NULL ? pbrText : pbr);
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
void UI::DrawRgch(const WCHAR* rgch, int cch, TF* ptf, RCF rcf, BR* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	AppGet().pdc->DrawText(rgch, (UINT32)cch, ptf, &rcf, pbr == NULL ? pbrText : pbr);
}


/*	UI::DrawBmp
 *
 *	Helper function for drawing part of a bitmap on the screen panel. Destination
 *	coordinates are in local UI coordinates.
 */
void UI::DrawBmp(RCF rcfTo, BMP* pbmp, RCF rcfFrom, float opacity) const
{
	AppGet().pdc->DrawBitmap(pbmp, rcfTo.Offset(rcfBounds.PtfTopLeft()), 
		opacity, D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR, rcfFrom);
}


/*
 *
 *	BTN classes
 * 
 */


BTN::BTN(UI* puiParent, int cmd, RCF rcf) : UI(puiParent, rcf), cmd(cmd)
{
	this->fHilite = false;
	this->fTrack = false;
}

void BTN::Track(bool fTrackNew) 
{
	fTrack = fTrackNew;
}

void BTN::Hilite(bool fHiliteNew) 
{
	fHilite = fHiliteNew;
}

void BTN::Draw(DC* pdc) 
{
}

void BTN::StartLeftDrag(PTF ptf)
{
	Track(true);
	Redraw();
}

void BTN::EndLeftDrag(PTF ptf)
{
	Track(false);
	Redraw();
	if (RcfInterior().FContainsPtf(ptf))
		DispatchCmd(cmd);
}

void BTN::LeftDrag(PTF ptf)
{
	Hilite(RcfInterior().FContainsPtf(ptf));
}

void BTN::MouseHover(PTF ptf, MHT mht)
{
	if (mht == MHT::Enter) {
		Hilite(true);
		Redraw();
	}
	else if (mht == MHT::Exit) {
		Hilite(false);
		Redraw();
	}
}


TF* BTNCH::ptfButton;
BRS* BTNCH::pbrsButton;

void BTNCH::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptfButton)
		return;

	pdc->CreateSolidColorBrush(ColorF(0.0, 0.0, 0.0), &pbrsButton);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		24.0f, L"",
		&ptfButton);
}


void BTNCH::DiscardRsrcClass(void)
{
	SafeRelease(&ptfButton);
	SafeRelease(&pbrsButton);
}

BTNCH::BTNCH(UI* puiParent, int cmd, RCF rcf, WCHAR ch) : BTN(puiParent, cmd, rcf), ch(ch)
{
}

void BTNCH::Draw(const RCF* prcfUpdate)
{
	WCHAR sz[2];
	sz[0] = ch;
	sz[1] = 0;
	pbrsButton->SetColor(ColorF((fHilite+fTrack)*0.5f, 0.0, 0.0));
	DrawSz(sz, ptfButton, RcfInterior(), pbrsButton);
}

BTNIMG::BTNIMG(UI* puiParent, int cmd, RCF rcf, int idb) : BTN(puiParent, cmd, rcf), idb(idb), pbmp(NULL)
{
}

BTNIMG::~BTNIMG(void)
{
	DiscardRsrc();
}

void BTNIMG::Draw(const RCF* prcfUpdate)
{
	DrawBmp(RcfInterior(), pbmp, RCF(PTF(0, 0), pbmp->GetSize()));
}

void BTNIMG::CreateRsrc(void)
{
	if (pbmp)
		return;
	APP& app = AppGet();
	pbmp = PbmpFromPngRes(idb, app.pdc, app.pfactwic);
}

void BTNIMG::DiscardRsrc(void)
{
	SafeRelease(&pbmp);
}

SIZF BTNIMG::SizfImg(void) const
{
	return pbmp == NULL ? SIZF(0, 0) : pbmp->GetSize();
}
