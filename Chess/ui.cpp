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

wchar_t UI::szFontFamily[] = L"Arial"; // L"Segoe UI Symbol";
BRS* UI::pbrBack;
BRS* UI::pbrAltBack;
BRS* UI::pbrGridLine;
BRS* UI::pbrText;
BRS* UI::pbrTip;
BRS* UI::pbrHilite;
TX* UI::ptxText;
TX* UI::ptxList;
TX* UI::ptxTip;


void UI::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrBack)
		return;
	pdc->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
	pdc->CreateSolidColorBrush(ColorF(.95f, .95f, .95f), &pbrAltBack);
	pdc->CreateSolidColorBrush(ColorF(.8f, .8f, .8f), &pbrGridLine);
	pdc->CreateSolidColorBrush(ColorF(0.4f, 0.4f, 0.4f), &pbrText);
	pdc->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 0.85f), &pbrTip);
	pdc->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 0.5f), &pbrHilite);

	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptxText);
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxList);
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		13.0f, L"",
		&ptxTip);

	BTNCH::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
}


void UI::DiscardRsrcClass(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrAltBack);
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrText);
	SafeRelease(&pbrTip);
	SafeRelease(&pbrHilite);
	SafeRelease(&ptxList);
	SafeRelease(&ptxText);
	SafeRelease(&ptxTip);
}


ID2D1PathGeometry* UI::PgeomCreate(PTF rgptf[], int cptf)
{
	/* capture X, which is created as a cross that is rotated later */
	ID2D1PathGeometry* pgeom;
	App().pfactd2->CreatePathGeometry(&pgeom);
	ID2D1GeometrySink* psink;
	pgeom->Open(&psink);
	psink->BeginFigure(rgptf[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&rgptf[1], cptf - 1);
	psink->EndFigure(D2D1_FIGURE_END_CLOSED);
	psink->Close();
	SafeRelease(&psink);
	return pgeom;
}


BMP* UI::PbmpFromPngRes(int idb)
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
	hr = App().pfactwic->CreateStream(&pstm);
	hr = pstm->InitializeFromMemory(pbRes, cbRes);
	IWICBitmapDecoder* pdec = NULL;
	hr = App().pfactwic->CreateDecoderFromStream(pstm, NULL, WICDecodeMetadataCacheOnLoad, &pdec);
	IWICBitmapFrameDecode* pframe = NULL;
	hr = pdec->GetFrame(0, &pframe);
	IWICFormatConverter* pconv = NULL;
	hr = App().pfactwic->CreateFormatConverter(&pconv);
	hr = pconv->Initialize(pframe, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
	ID2D1Bitmap1* pbmp;
	hr = App().pdc->CreateBitmapFromWicBitmap(pconv, NULL, &pbmp);

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
	while (vpuiChild.size() > 0)
		delete vpuiChild[0];
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
	vpuiChild.push_back(puiChild);
}


/*	UI::RemoveChild
 *
 *	Removes the child UI from its parent's child list.
 */
void UI::RemoveChild(UI* puiChild) 
{
	unsigned ipuiTo = 0, ipuiFrom;
	for (ipuiFrom = 0; ipuiFrom < vpuiChild.size(); ipuiFrom++) {
		if (vpuiChild[ipuiFrom] != puiChild)
			vpuiChild[ipuiTo++] = vpuiChild[ipuiFrom];
	}
	assert(ipuiTo < ipuiFrom);
	vpuiChild.resize(ipuiTo);
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
	for (UI* pui : puiParent->vpuiChild) {
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
	for (int ipui = 0; ipui < (int)puiParent->vpuiChild.size() - 1; ipui++)
		if (puiParent->vpuiChild[ipui] == this)
			return puiParent->vpuiChild[ipui + 1];
	return NULL;
}


void UI::CreateRsrc(void)
{
	for (UI* puiChild : vpuiChild)
		puiChild->CreateRsrc();
}


void UI::DiscardRsrc(void)
{
	for (UI* puiChild : vpuiChild)
		puiChild->DiscardRsrc();
}


void UI::Layout(void)
{
}


SIZF UI::SizfLayoutPreferred(void)
{
	return SIZF(-1.0f, -1.0f);
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


/*	UI::RcfBounds
 *
 *	Returns the bounds of the item, in parent coorrdinates.
 */
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
	if (ptfNew.x == rcfBounds.DxfWidth() && ptfNew.y == rcfBounds.DyfHeight())
		return;
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
	if (dxf == 0 && dyf == 0)
		return;
	rcfBounds.Offset(dxf, dyf);
	for (UI* pui : vpuiChild)
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
	if (puiParent) {
		puiParent->Layout();
		puiParent->Redraw();
	}
	else {
		Layout();
		Redraw();
	}
}


/*	UI::PuiFromPtf
 *
 *	Returns the UI element the point is over. Point is in global coordinates.
 *	Returns NULL if the point is not within the UI.
 */
UI* UI::PuiFromPtf(PTF ptf)
{
	if (!fVisible || !rcfBounds.FContainsPtf(ptf))
		return NULL;
	for (UI* puiChild : vpuiChild) {
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


void UI::ScrollWheel(PTF ptf, int dwheel)
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


void UI::SetFocus(UI* pui)
{
	if (puiParent)
		puiParent->ReleaseCapt();
}


void UI::KeyDown(int vk)
{
}


void UI::KeyUp(int vk)
{
}


void UI::DispatchCmd(int cmd)
{
	if (puiParent)
		puiParent->DispatchCmd(cmd);;
}


void UI::ShowTip(UI* puiAttach, bool fShow)
{
	if (puiParent)
		puiParent->ShowTip(puiAttach, fShow);
}


wstring UI::SzTip(void) const
{
	return L"";
}


wstring UI::SzTipFromCmd(int cmd) const
{
	if (puiParent)
		return puiParent->SzTipFromCmd(cmd);
	else
		return L"";
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
 *	Updates the UI element and all child elements. rcfUpdate is in
 *	global coordinates.
 */
void UI::Update(RCF rcfUpdate)
{
	if (!fVisible)
		return;
	RCF rcf = rcfUpdate & rcfBounds;
	if (!rcf)
		return;
	App().pdc->PushAxisAlignedClip(rcf, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	RCF rcfDraw = RcfLocalFromGlobal(rcf);
	Draw(rcfDraw);
	App().pdc->PopAxisAlignedClip();
	for (UI* pui : vpuiChild)
		pui->Update(rcf);
}


/*	UI::Redraw
 *
 *	Redraws the UI element and all children
 */
void UI::Redraw(void)
{
	Redraw(RcfInterior());
}


/*	UI::Redraw
 *
 *	Redraws the area of the UI element. rcfUpdate is in local coordinates.
 */
void UI::Redraw(RCF rcfUpdate)
{
	if (!fVisible)
		return;
	BeginDraw();
	Update(RcfGlobalFromLocal(rcfUpdate));
	EndDraw();
}


void UI::Draw(RCF rcfDraw)
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


APP& UI::App(void) const
{
	assert(puiParent);
	return puiParent->App();
}


const GA& UI::Ga(void) const
{
	return *App().pga;
}

GA& UI::Ga(void)
{
	return *App().pga;
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

bool UI::FDepthLog(LGT lgt, int& depth)
{
	assert(puiParent);
	return puiParent->FDepthLog(lgt, depth);
}

void UI::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	assert(puiParent);
	puiParent->AddLog(lgt, lgf, depth, tag, szData);
}


/*	UI::FillRcf
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRcf(RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	App().pdc->FillRectangle(&rcf, pbr);
}


void UI::FillRcfBack(RCF rcf) const
{
	if (puiParent == NULL)
		FillRcf(rcf, pbrBack);
	else
		puiParent->FillRcfBack(RcfParentFromLocal(rcf));
}


/*	UI::FillEllf
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(PtfGlobalFromLocal(PTF(0, 0)));
	App().pdc->FillEllipse(&ellf, pbr);
}


/*	UI::DrawEllf
 *
 *	Helper function for drawing an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::DrawEllf(ELLF ellf, ID2D1Brush* pbr) const
{
	ellf.Offset(PtfGlobalFromLocal(PTF(0, 0)));
	App().pdc->DrawEllipse(&ellf, pbr);
}


/*	UI::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local UI coordinates.
 */
void UI::DrawSz(const wstring& sz, TX* ptx, RCF rcf, ID2D1Brush* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	App().pdc->DrawText(sz.c_str(), (UINT32)sz.size(), ptx, &rcf, pbr==NULL ? pbrText : pbr);
}


void UI::DrawSzCenter(const wstring& sz, TX* ptx, RCF rcf, ID2D1Brush* pbr) const
{
	TATX tatxSav(ptx, DWRITE_TEXT_ALIGNMENT_CENTER);
	DrawSz(sz, ptx, rcf, pbr);
}


SIZF UI::SizfSz(const wstring& sz, TX* ptx, float dxf, float dyf) const
{
	IDWriteTextLayout* ptxl;
	App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(), ptx, dxf, dyf, &ptxl);
	DWRITE_TEXT_METRICS dtm;
	ptxl->GetMetrics(&dtm);
	SafeRelease(&ptxl);
	return SIZF(dtm.width, dtm.height);
}



/*	UI::DrawRgch
 *
 *	Helper function for drawing text on the screen panel. Rectangle is in local
 *	UI coordinates
 */
void UI::DrawRgch(const WCHAR* rgch, int cch, TX* ptx, RCF rcf, BR* pbr) const
{
	rcf = RcfGlobalFromLocal(rcf);
	App().pdc->DrawText(rgch, (UINT32)cch, ptx, &rcf, pbr == NULL ? pbrText : pbr);
}




/*	UI::DrawSzFit
 *
 *	Draws the text in the rectangle, sizing the font smaller to make it
 *	fit. Uses ptx as the font style to base dependent fonts on. Text
 *	is baseline aligned, which should be constant no matter what size
 *	font we eventually end up using.
 */
void UI::DrawSzFit(const wstring& sz, TX* ptxBase, RCF rcfFit, BR* pbrText) const
{
	if (pbrText == nullptr)
		pbrText = UI::pbrText;

	/* if the text fits, just blast it out */

	IDWriteTextLayout* play = nullptr;
	App().pfactdwr->CreateTextLayout(sz.c_str(), sz.size(), ptxBase, 1.0e+6, rcfFit.DyfHeight(), &play);
	DWRITE_TEXT_METRICS tm;
	play->GetMetrics(&tm);
	if (tm.width <= rcfFit.DxfWidth() && tm.height <= rcfFit.DyfHeight()) {
		DrawSz(sz, ptxBase, rcfFit, pbrText);
		SafeRelease(&play);
		return;
	}

	RCF rcf = RcfGlobalFromLocal(rcfFit);

	/* figure out where our baseline is */

	DWRITE_LINE_METRICS lm;
	uint32_t clm;
	play->GetLineMetrics(&lm, 1, &clm);
	float yfBaseline = rcf.top + lm.baseline;

	/* and loop using progressively smaller fonts until the text fits */

	IDWriteTextFormat* ptx = nullptr;
	for (float dyfFont = ptxBase->GetFontSize() - 1.0f; dyfFont > 6.0f; dyfFont--) {
		SafeRelease(&play);
		SafeRelease(&ptx);
		App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
			DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			dyfFont, L"", &ptx);
		App().pfactdwr->CreateTextLayout(sz.c_str(), sz.size(),
			ptx, rcf.DxfWidth(), rcf.DyfHeight(), &play);
		DWRITE_TEXT_METRICS tm;
		play->GetMetrics(&tm);
		if (tm.width <= rcfFit.DxfWidth() && tm.height <= rcfFit.DyfHeight())
			break;
	}

	play->GetLineMetrics(&lm, 1, &clm);
	App().pdc->DrawTextLayout(Point2F(rcf.left, yfBaseline - lm.baseline), play,
			pbrText, D2D1_DRAW_TEXT_OPTIONS_NONE);

	SafeRelease(&play);
	SafeRelease(&ptx);
}


/*	UI::DrawBmp
 *
 *	Helper function for drawing part of a bitmap on the screen panel. Destination
 *	coordinates are in local UI coordinates.
 */
void UI::DrawBmp(RCF rcfTo, BMP* pbmp, RCF rcfFrom, float opacity) const
{
	App().pdc->DrawBitmap(pbmp, rcfTo.Offset(rcfBounds.PtfTopLeft()), 
			opacity, D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR, rcfFrom);
}


/*
 *
 *	BTN classes
 * 
 */


BTN::BTN(UI* puiParent, int cmd) : UI(puiParent), cmd(cmd)
{
	this->fHilite = false;
	this->fTrack = false;
}

void BTN::Track(bool fTrackNew) 
{
	fTrack = fTrackNew;
	Redraw();
}

void BTN::Hilite(bool fHiliteNew) 
{
	fHilite = fHiliteNew;
	Redraw();
}

void BTN::Draw(RCF rcfUpdate) 
{
}

void BTN::StartLeftDrag(PTF ptf)
{
	SetCapt(this);
	Track(true);
}

void BTN::EndLeftDrag(PTF ptf)
{
	ReleaseCapt();
	Track(false);
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
		ShowTip(this, true);
	}
	else if (mht == MHT::Exit) {
		Hilite(false);
		ShowTip(this, false);
	}
}


wstring BTN::SzTip(void) const
{
	return SzTipFromCmd(cmd);
}


TX* BTNCH::ptxButton;
BRS* BTNCH::pbrsButton;

void BTNCH::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxButton)
		return;

	pdc->CreateSolidColorBrush(ColorF(0.0, 0.0, 0.0), &pbrsButton);
	pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		20.0f, L"",
		&ptxButton);
}


void BTNCH::DiscardRsrcClass(void)
{
	SafeRelease(&ptxButton);
	SafeRelease(&pbrsButton);
}


BTNCH::BTNCH(UI* puiParent, int cmd, WCHAR ch) : BTN(puiParent, cmd), ch(ch)
{
}


void BTNCH::Draw(RCF rcfUpdate)
{
	CreateRsrc();
	WCHAR sz[2];
	sz[0] = ch;
	sz[1] = 0;
	RCF rcfChar(PTF(0, 0), SizfSz(sz, ptxButton));
	RCF rcfTo = RcfInterior();
	FillRcfBack(rcfTo);
	COLORBRS colorbrsSav(pbrsButton, ColorF((fHilite + fTrack) * 0.5f, 0.0, 0.0));
	rcfChar += rcfTo.PtfCenter();
	rcfChar.Offset(-rcfChar.DxfWidth() / 2.0f, -rcfChar.DyfHeight() / 2.0f);
	DrawSzCenter(sz, ptxButton, rcfTo, pbrsButton);
}


BTNIMG::BTNIMG(UI* puiParent, int cmd, int idb) : BTN(puiParent, cmd), idb(idb), pbmp(NULL)
{
}


BTNIMG::~BTNIMG(void)
{
	DiscardRsrc();
}


void BTNIMG::Draw(RCF rcfUpdate)
{
	CreateRsrc();
	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_ALIASED);
	RCF rcfTo = RcfInterior();
	FillRcfBack(rcfTo);
	rcfTo.Offset(rcfBounds.PtfTopLeft());
	rcfTo.Inflate(PTF(-2.0f, -2.0f));
	RCF rcfFrom = RCF(PTF(0, 0), pbmp->GetSize());
	if (rcfFrom.DxfWidth() < rcfTo.DxfWidth()) {
		float dxyf = (rcfTo.DxfWidth() - rcfFrom.DxfWidth()) / 2.0f;
		rcfTo.Inflate(-dxyf, -dxyf);
	}
	if (rcfFrom.DyfHeight() < rcfTo.DyfHeight()) {
		float dxyf = (rcfTo.DyfHeight() - rcfFrom.DyfHeight()) / 2.0f;
		rcfTo.Inflate(-dxyf, -dxyf);
	}
	COLORBRS colorbrsSav(pbrText, ColorF((fHilite + fTrack) * 0.5f, 0.0f, 0.0f));
	pdc->FillOpacityMask(pbmp, pbrText, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, rcfTo, rcfFrom);
}


void BTNIMG::CreateRsrc(void)
{
	if (pbmp)
		return;
	pbmp = PbmpFromPngRes(idb);
}


void BTNIMG::DiscardRsrc(void)
{
	SafeRelease(&pbmp);
}


SIZF BTNIMG::SizfImg(void) const
{
	return pbmp == NULL ? SIZF(0, 0) : pbmp->GetSize();
}


/*
 *
 *	STATIC ui elements
 * 
 *	Static controls
 * 
 */


STATIC::STATIC(UI* puiParent, const wstring& sz) : UI(puiParent),  
		ptxStatic(nullptr), pbrsStatic(nullptr), szText(sz)
{
}


void STATIC::CreateRsrc(void)
{
	if (ptxStatic)
		return;

	App().pdc->CreateSolidColorBrush(ColorF(0.0, 0.0, 0.0), &pbrsStatic);
	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		20.0f, L"",
		&ptxStatic);
}


void STATIC::DiscardRsrc(void)
{
	SafeRelease(&ptxStatic);
	SafeRelease(&pbrsStatic);
}


void STATIC::SetText(const wstring& sz)
{
	szText = sz;
}


wstring STATIC::SzText(void) const
{
	return szText;
}


void STATIC::Draw(RCF rcfUpdate)
{
	CreateRsrc();
	RCF rcfChar(PTF(0, 0), SizfSz(SzText(), ptxStatic));
	RCF rcfTo = RcfInterior();
	FillRcfBack(rcfTo);
	rcfChar += rcfTo.PtfCenter();
	rcfChar.Offset(-rcfChar.DxfWidth() / 2.0f, -rcfChar.DyfHeight() / 2.0f);
	DrawSzCenter(SzText(), ptxStatic, rcfTo, pbrsStatic);
}


/*
 *
 *	BTNGEOM
 * 
 *	Button with a geometry to fill for the visual
 * 
 */


BTNGEOM::BTNGEOM(UI* puiParent, int cmd, PTF rgptf[], int cptf) : BTN(puiParent, cmd), pgeom(nullptr)
{
	pgeom = PgeomCreate(rgptf, cptf);
}

BTNGEOM::~BTNGEOM(void)
{
	SafeRelease(&pgeom);
}

void BTNGEOM::Draw(RCF rcfUpdate)
{
	float dxyfScale = RcfInterior().DxfWidth() / 2.0f;
	FillRcf(RcfInterior(), pbrBack);
	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	PTF ptfCenter = rcfBounds.PtfCenter();
	TRANSDC transdc(pdc,
		Matrix3x2F::Scale(SizeF(dxyfScale, dxyfScale), PTF(0, 0)) *
		Matrix3x2F::Translation(SizeF(ptfCenter.x, ptfCenter.y)));
	COLORBRS colorbrsSav(pbrText, ColorF((fHilite + fTrack) * 0.5f, 0.0, 0.0));
	pdc->FillGeometry(pgeom, pbrText);
}


/*
 *
 *	SPIN ui control
 * 
 */
static PTF rgptfLeft[3] = { {0.5f, 0.75f}, {0.5f, -0.75f}, {-0.5f, 0.0f} };
static PTF rgptfRight[3] = { {-0.5, 0.75f}, {-0.5f, -0.75f}, {0.5f, 0.0f} };


SPIN::SPIN(UI* puiParent, int cmdUp, int cmdDown) : UI(puiParent, true), ptxSpin(nullptr),
		btngeomDown(this, cmdDown, rgptfLeft, 3), btngeomUp(this, cmdUp, rgptfRight, 3)
		
{
}


void SPIN::CreateRsrc(void)
{
	if (ptxSpin)
		return;

	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		14.0f, L"",
		&ptxSpin);
}


void SPIN::DiscardRsrc(void)
{
	SafeRelease(&ptxSpin);
}

void SPIN::Layout(void)
{
	float dxyfScale = RcfInterior().DyfHeight() / 2.0f;
	RCF rcf = RcfInterior();
	rcf.right = rcf.left + dxyfScale;
	btngeomDown.SetBounds(rcf);
	rcf = RcfInterior();
	rcf.left = rcf.right - dxyfScale;
	btngeomUp.SetBounds(rcf);
}


void SPIN::Draw(RCF rcfUpdate)
{
	wstring szValue = SzValue();
	SIZF sizf = SizfSz(szValue, ptxSpin);
	RCF rcf = RcfInterior();
	rcf.Offset(PTF(0, (rcf.DyfHeight() - sizf.height) / 2));
	DrawSzCenter(szValue, ptxSpin, rcf, pbrText);
}
