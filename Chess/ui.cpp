/*
 *
 *	ui.cpp
 * 
 *	Our lightweight user interface items.
 * 
 */

#include "ui.h"


wchar_t* PchDecodeInt(unsigned imv, wchar_t* pch)
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

	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptxText);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxList);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
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


/*	UI::PgeomCreate
 *
 *	Creates a geometry object from the array of points.
 */
ID2D1PathGeometry* UI::PgeomCreate(PT rgpt[], int cpt)
{
	/* capture X, which is created as a cross that is rotated later */
	ID2D1PathGeometry* pgeom;
	App().pfactd2->CreatePathGeometry(&pgeom);
	ID2D1GeometrySink* psink;
	pgeom->Open(&psink);
	psink->BeginFigure(rgpt[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&rgpt[1], cpt - 1);
	psink->EndFigure(D2D1_FIGURE_END_CLOSED);
	psink->Close();
	SafeRelease(&psink);
	return pgeom;
}


/*	UI::PbmpFromPngRes
 *
 *	Creates a bitmap object from a resource file in PNG format. The PNG format resources
 *	will use the "IMAGE" resource type.
 */
BMP* UI::PbmpFromPngRes(int idb)
{
	HRSRC hres = ::FindResource(nullptr, MAKEINTRESOURCE(idb), L"IMAGE");
	if (!hres)
		return nullptr;
	ULONG cbRes = ::SizeofResource(nullptr, hres);
	HGLOBAL hresLoad = ::LoadResource(nullptr, hres);
	if (!hresLoad)
		return nullptr;
	BYTE* pbRes = (BYTE*)::LockResource(hresLoad);
	if (!pbRes)
		return nullptr;

	HRESULT hr;
	IWICStream* pstm = nullptr;
	hr = App().pfactwic->CreateStream(&pstm);
	hr = pstm->InitializeFromMemory(pbRes, cbRes);
	IWICBitmapDecoder* pdec = nullptr;
	hr = App().pfactwic->CreateDecoderFromStream(pstm, nullptr, WICDecodeMetadataCacheOnLoad, &pdec);
	IWICBitmapFrameDecode* pframe = nullptr;
	hr = pdec->GetFrame(0, &pframe);
	IWICFormatConverter* pconv = nullptr;
	hr = App().pfactwic->CreateFormatConverter(&pconv);
	hr = pconv->Initialize(pframe, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
	ID2D1Bitmap1* pbmp;
	hr = App().pdc->CreateBitmapFromWicBitmap(pconv, nullptr, &pbmp);

	SafeRelease(&pframe);
	SafeRelease(&pconv);
	SafeRelease(&pdec);
	SafeRelease(&pstm);

	return pbmp;
}


TX* UI::PtxCreate(float dyHeight, bool fBold, bool fItalic)
{
	TX* ptx = nullptr;
	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		fBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
		fItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		dyHeight, L"", &ptx);
	return ptx;
}


/*	UI::UI
 *
 *	Constructor for the UI elements. Adds the item to the UI tree structure
 *	by connecting it to the parent.
 */
UI::UI(UI* puiParent, bool fVisible) : puiParent(puiParent), rcBounds(0, 0, 0, 0), fVisible(fVisible)
{
	if (puiParent)
		puiParent->AddChild(this);
}


UI::UI(UI* puiParent, RC rcBounds, bool fVisible) : puiParent(puiParent), rcBounds(rcBounds), fVisible(fVisible)
{
	if (puiParent) {
		puiParent->AddChild(this);
		this->rcBounds.Offset(puiParent->rcBounds.left, puiParent->rcBounds.top);
	}
}


UI::~UI(void) 
{
	while (vpuiChild.size() > 0)
		delete vpuiChild[0];
	if (puiParent) {
		puiParent->RemoveChild(this);
		puiParent = nullptr;
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
	if (puiParent == nullptr)
		return nullptr;
	UI* puiPrev = nullptr;
	for (UI* pui : puiParent->vpuiChild) {
		if (pui == this)
			return puiPrev;
		puiPrev = pui;
	}
	assert(false);
	return nullptr;
}


/*	UI::PuiNextSib
 *
 *	Gets the next sibling window of the given UI element.
 */
UI* UI::PuiNextSib(void) const
{
	if (puiParent == nullptr)
		return nullptr;
	for (int ipui = 0; ipui < (int)puiParent->vpuiChild.size() - 1; ipui++)
		if (puiParent->vpuiChild[ipui] == this)
			return puiParent->vpuiChild[ipui + 1];
	return nullptr;
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


SIZ UI::SizLayoutPreferred(void)
{
	return SIZ(-1.0f, -1.0f);
}


/*	UI::RcInterior
 *
 *	Returns the interior of the given UI element, in local coordinates.
 */
RC UI::RcInterior(void) const 
{
	RC rc = rcBounds;
	return rc.Offset(-rcBounds.left, -rcBounds.top);
}


/*	UI::RcBounds
 *
 *	Returns the bounds of the item, in parent coorrdinates.
 */
RC UI::RcBounds(void) const
{
	RC rc = rcBounds;
	if (puiParent)
		rc.Offset(-puiParent->rcBounds.left, -puiParent->rcBounds.top);
	return rc;
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
void UI::SetBounds(const RC& rcNew) 
{
	RC rc = rcNew;
	if (puiParent)
		rc.Offset(puiParent->rcBounds.left, puiParent->rcBounds.top);
	rcBounds.right = rcBounds.left + rc.DxWidth();
	rcBounds.bottom = rcBounds.top + rc.DyHeight();
	OffsetBounds(rc.left - rcBounds.left, rc.top - rcBounds.top);
	Layout();
}


/*	UI::Resize
 *
 *	Resizes the UI element.
 */
void UI::Resize(const PT& ptNew) 
{
	if (ptNew.x == rcBounds.DxWidth() && ptNew.y == rcBounds.DyHeight())
		return;
	rcBounds.right = rcBounds.left + ptNew.x;
	rcBounds.bottom = rcBounds.top + ptNew.y;
	Layout();
}


/*	UI::Move
 *
 *	Moves the UI element to the new upper left position. In parent coordinates.
 */
void UI::Move(const PT& ptNew) 
{
	PT pt = ptNew;
	if (puiParent) {
		/* convert to global coordinates */
		pt.x += puiParent->rcBounds.left;
		pt.y += puiParent->rcBounds.top;
	}
	float dx = pt.x - rcBounds.left;
	float dy = pt.y - rcBounds.top;
	OffsetBounds(dx, dy);
}


void UI::OffsetBounds(float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;
	rcBounds.Offset(dx, dy);
	for (UI* pui : vpuiChild)
		pui->OffsetBounds(dx, dy);
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


/*	UI::PuiFromPt
 *
 *	Returns the UI element the point is over. Point is in global coordinates.
 *	Returns nullptr if the point is not within the UI.
 */
UI* UI::PuiFromPt(PT pt)
{
	if (!fVisible || !rcBounds.FContainsPt(pt))
		return nullptr;
	for (UI* puiChild : vpuiChild) {
		UI* pui = puiChild->PuiFromPt(pt);
		if (pui)
			return pui;
	}
	return this;
}


void UI::StartLeftDrag(const PT& pt)
{
	SetCapt(this);
}


void UI::EndLeftDrag(const PT& pt)
{
	ReleaseCapt();
}


void UI::LeftDrag(const PT& pt)
{
}


void UI::MouseHover(const PT& pt, MHT mht)
{
}


void UI::ScrollWheel(const PT& pt, int dwheel)
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


/*	UI::RcParentFromLocal
 *
 *	Converts a rectangle from local coordinates to parent coordinates
 */
RC UI::RcParentFromLocal(const RC& rc) const
{
	if (puiParent == nullptr)
		return rc;
	return rc.Offset(rcBounds.left-puiParent->rcBounds.left, 
		rcBounds.top-puiParent->rcBounds.top);
}


RC UI::RcGlobalFromLocal(const RC& rc) const
{
	return rc.Offset(rcBounds.left, rcBounds.top);
}


RC UI::RcLocalFromParent(const RC& rc) const
{
	if (puiParent == nullptr)
		return rc;
	return rc.Offset(puiParent->rcBounds.left - rcBounds.left,
		puiParent->rcBounds.top - rcBounds.top);
}


RC UI::RcLocalFromGlobal(const RC& rc) const
{
	return rc.Offset(-rcBounds.left, -rcBounds.top);
}


/*	UI::PtParentFromLocal
 *
 *	Converts a point from loal coordinates to local coordinates of the
 *	parent UI element.
 */
PT UI::PtParentFromLocal(const PT& pt) const
{
	if (puiParent == nullptr)
		return pt;
	return PT(pt.x + rcBounds.left - puiParent->rcBounds.left,
		pt.y + rcBounds.top - puiParent->rcBounds.left);

}


/*	UI::PtGlobalFromLocal
 *
 *	Converts a point from local coordinates to global (relative to the 
 *	main window) coordinates.
 */
PT UI::PtGlobalFromLocal(const PT& pt) const
{
	return PT(pt.x + rcBounds.left, pt.y + rcBounds.top);
}


/*	UI::PtLocalFromGlobal
 *
 *	Converts a point from global (relative to the main top-level window) to
 *	local coordinates.
 */
PT UI::PtLocalFromGlobal(const PT& pt) const
{
	return PT(pt.x - rcBounds.left, pt.y - rcBounds.top);
}


/*	UI::Update
 *
 *	Updates the UI element and all child elements. rcUpdate is in
 *	global coordinates.
 */
void UI::Update(const RC& rcUpdate)
{
	if (!fVisible)
		return;
	RC rc = rcUpdate & rcBounds;
	if (!rc)
		return;
	App().pdc->PushAxisAlignedClip(rc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	RC rcDraw = RcLocalFromGlobal(rc);
	Draw(rcDraw);
	App().pdc->PopAxisAlignedClip();
	for (UI* pui : vpuiChild)
		pui->Update(rc);
}


/*	UI::Redraw
 *
 *	Redraws the UI element and all children
 */
void UI::Redraw(void)
{
	Redraw(RcInterior());
}


/*	UI::Redraw
 *
 *	Redraws the area of the UI element. rcUpdate is in local coordinates.
 */
void UI::Redraw(const RC& rcUpdate)
{
	if (!fVisible)
		return;
	BeginDraw();
	Update(RcGlobalFromLocal(rcUpdate));
	EndDraw();
	RedrawOverlappedSiblings(RcGlobalFromLocal(rcUpdate));
}


/*	UI::RedrawOverlappedSiblings
 *
 *	When doing a partial redraw, we may need to redraw some sibling UI elements, like
 *	tool tips, which overlap the update area. rcUpdate is in global coordinates. Note
 *	that we only need to check siblings after the current UI element in the child lists,
 *	since UI elements earlier than us will be under and are guaranteed to be overwritten.
 */
void UI::RedrawOverlappedSiblings(const RC& rcUpdate)
{
	if (puiParent == nullptr)
		return;
	bool fFoundUs = false;
	for (UI* pui : puiParent->vpuiChild) {
		if (fFoundUs) {
			if (pui->rcBounds & rcUpdate)
				pui->Redraw(RcLocalFromGlobal(pui->rcBounds & rcUpdate));
		}
		else if (pui == this)
			fFoundUs = true;
	}
	puiParent->RedrawOverlappedSiblings(rcUpdate);
}


void UI::Draw(const RC& rcDraw)
{
}


void UI::InvalRc(const RC& rc, bool fErase) const
{
	if (rc.top >= rc.bottom || rc.left >= rc.right || puiParent == nullptr)
		return;
	puiParent->InvalRc(RcParentFromLocal(rc), fErase);
}


void UI::PresentSwch(void) const
{
	if (puiParent == nullptr)
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


/*	UI::FillRc
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRc(const RC& rc, ID2D1Brush* pbr) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	App().pdc->FillRectangle(&rcGlobal, pbr);
}


void UI::FillRcBack(const RC& rc) const
{
	if (puiParent == nullptr)
		FillRc(rc, pbrBack);
	else
		puiParent->FillRcBack(RcParentFromLocal(rc));
}


/*	UI::FillEll
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEll(const ELL& ell, ID2D1Brush* pbr) const
{
	ELL ellGlobal = ell.Offset(PtGlobalFromLocal(PT(0, 0)));
	App().pdc->FillEllipse(&ellGlobal, pbr);
}


/*	UI::DrawEll
 *
 *	Helper function for drawing an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::DrawEll(const ELL& ell, ID2D1Brush* pbr) const
{
	ELL ellGlobal = ell.Offset(PtGlobalFromLocal(PT(0, 0)));
	App().pdc->DrawEllipse(&ellGlobal, pbr);
}


/*	UI::DrawSz
 *
 *	Helper function for writing text on the screen panel. Rectangle is in
 *	local UI coordinates.
 */
void UI::DrawSz(const wstring& sz, TX* ptx, const RC& rc, ID2D1Brush* pbr) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	App().pdc->DrawText(sz.c_str(), (UINT32)sz.size(), ptx, &rcGlobal, pbr ? pbr : pbrText);
}


void UI::DrawSzCenter(const wstring& sz, TX* ptx, const RC& rc, ID2D1Brush* pbr) const
{
	TATX tatxSav(ptx, DWRITE_TEXT_ALIGNMENT_CENTER);
	DrawSz(sz, ptx, rc, pbr);
}


SIZ UI::SizSz(const wstring& sz, TX* ptx, float dx, float dy) const
{
	IDWriteTextLayout* ptxl;
	App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(), ptx, dx, dy, &ptxl);
	DWRITE_TEXT_METRICS dtm;
	ptxl->GetMetrics(&dtm);
	SafeRelease(&ptxl);
	return SIZ(dtm.width, dtm.height);
}


/*	UI::DrawRgch
 *
 *	Helper function for drawing text on the screen panel. Rectangle is in local
 *	UI coordinates
 */
void UI::DrawRgch(const wchar_t* rgch, int cch, TX* ptx, const RC& rc, BR* pbr) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	App().pdc->DrawText(rgch, (UINT32)cch, ptx, &rcGlobal, pbr ? pbr : pbrText);
}




/*	UI::DrawSzFit
 *
 *	Draws the text in the rectangle, sizing the font smaller to make it
 *	fit. Uses ptx as the font style to base dependent fonts on. Text
 *	is baseline aligned, which should be constant no matter what size
 *	font we eventually end up using.
 */
void UI::DrawSzFit(const wstring& sz, TX* ptxBase, const RC& rcFit, BR* pbrText) const
{
	if (pbrText == nullptr)
		pbrText = UI::pbrText;

	/* if the text fits, just blast it out */

	IDWriteTextLayout* play = nullptr;
	App().pfactdwr->CreateTextLayout(sz.c_str(), sz.size(), ptxBase, 1.0e+6, rcFit.DyHeight(), &play);
	DWRITE_TEXT_METRICS tm;
	play->GetMetrics(&tm);
	if (tm.width <= rcFit.DxWidth() && tm.height <= rcFit.DyHeight()) {
		DrawSz(sz, ptxBase, rcFit, pbrText);
		SafeRelease(&play);
		return;
	}

	RC rcGlobal = RcGlobalFromLocal(rcFit);

	/* figure out where our baseline is */

	DWRITE_LINE_METRICS lm;
	uint32_t clm;
	play->GetLineMetrics(&lm, 1, &clm);
	float yBaseline = rcGlobal.top + lm.baseline;

	/* and loop using progressively smaller fonts until the text fits */

	IDWriteTextFormat* ptx = nullptr;
	for (float dyFont = ptxBase->GetFontSize() - 1.0f; dyFont > 6.0f; dyFont--) {
		SafeRelease(&play);
		SafeRelease(&ptx);
		App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
			DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			dyFont, L"", &ptx);
		App().pfactdwr->CreateTextLayout(sz.c_str(), sz.size(),
			ptx, rcGlobal.DxWidth(), rcGlobal.DyHeight(), &play);
		DWRITE_TEXT_METRICS tm;
		play->GetMetrics(&tm);
		if (tm.width <= rcFit.DxWidth() && tm.height <= rcFit.DyHeight())
			break;
	}

	play->GetLineMetrics(&lm, 1, &clm);
	App().pdc->DrawTextLayout(Point2F(rcGlobal.left, yBaseline - lm.baseline), play,
			pbrText, D2D1_DRAW_TEXT_OPTIONS_NONE);

	SafeRelease(&play);
	SafeRelease(&ptx);
}


/*	UI::DrawBmp
 *
 *	Helper function for drawing part of a bitmap on the screen panel. Destination
 *	coordinates are in local UI coordinates.
 */
void UI::DrawBmp(const RC& rcTo, BMP* pbmp, const RC& rcFrom, float opacity) const
{
	App().pdc->DrawBitmap(pbmp, rcTo.Offset(rcBounds.PtTopLeft()), 
			opacity, D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR, rcFrom);
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

void BTN::Draw(const RC& rcUpdate) 
{
}

void BTN::StartLeftDrag(const PT& pt)
{
	SetCapt(this);
	Track(true);
}

void BTN::EndLeftDrag(const PT& pt)
{
	ReleaseCapt();
	Track(false);
	if (RcInterior().FContainsPt(pt))
		DispatchCmd(cmd);
}

void BTN::LeftDrag(const PT& pt)
{
	Hilite(RcInterior().FContainsPt(pt));
}

void BTN::MouseHover(const PT& pt, MHT mht)
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
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		20.0f, L"",
		&ptxButton);
}


void BTNCH::DiscardRsrcClass(void)
{
	SafeRelease(&ptxButton);
	SafeRelease(&pbrsButton);
}


BTNCH::BTNCH(UI* puiParent, int cmd, wchar_t ch) : BTN(puiParent, cmd), ch(ch)
{
}


void BTNCH::Draw(const RC& rcUpdate)
{
	CreateRsrc();
	wchar_t sz[2];
	sz[0] = ch;
	sz[1] = 0;
	RC rcChar(PT(0, 0), SizSz(sz, ptxButton));
	RC rcTo = RcInterior();
	FillRcBack(rcTo);
	COLORBRS colorbrsSav(pbrsButton, ColorF((fHilite + fTrack) * 0.5f, 0.0, 0.0));
	rcChar += rcTo.PtCenter();
	rcChar.Offset(-rcChar.DxWidth() / 2.0f, -rcChar.DyHeight() / 2.0f);
	DrawSzCenter(sz, ptxButton, rcTo, pbrsButton);
}


BTNIMG::BTNIMG(UI* puiParent, int cmd, int idb) : BTN(puiParent, cmd), idb(idb), pbmp(nullptr)
{
}


BTNIMG::~BTNIMG(void)
{
	DiscardRsrc();
}


void BTNIMG::Draw(const RC& rcUpdate)
{
	CreateRsrc();
	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_ALIASED);
	RC rcTo = RcInterior();
	FillRcBack(rcTo);
	rcTo.Offset(rcBounds.PtTopLeft());
	rcTo.Inflate(PT(-2.0f, -2.0f));
	RC rcFrom = RC(PT(0, 0), pbmp->GetSize());
	if (rcFrom.DxWidth() < rcTo.DxWidth()) {
		float dxy = (rcTo.DxWidth() - rcFrom.DxWidth()) / 2.0f;
		rcTo.Inflate(-dxy, -dxy);
	}
	if (rcFrom.DyHeight() < rcTo.DyHeight()) {
		float dxy = (rcTo.DyHeight() - rcFrom.DyHeight()) / 2.0f;
		rcTo.Inflate(-dxy, -dxy);
	}
	COLORBRS colorbrsSav(pbrText, ColorF((fHilite + fTrack) * 0.5f, 0.0f, 0.0f));
	pdc->FillOpacityMask(pbmp, pbrText, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, rcTo, rcFrom);
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


SIZ BTNIMG::SizImg(void) const
{
	return pbmp ? pbmp->GetSize() : SIZ(0, 0);
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
	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
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


void STATIC::Draw(const RC& rcUpdate)
{
	CreateRsrc();
	RC rcChar(PT(0, 0), SizSz(SzText(), ptxStatic));
	RC rcTo = RcInterior();
	FillRcBack(rcTo);
	rcChar += rcTo.PtCenter();
	rcChar.Offset(-rcChar.DxWidth() / 2.0f, -rcChar.DyHeight() / 2.0f);
	DrawSzCenter(SzText(), ptxStatic, rcTo, pbrsStatic);
}


/*
 *
 *	BTNGEOM
 * 
 *	Button with a geometry to fill for the visual
 * 
 */


BTNGEOM::BTNGEOM(UI* puiParent, int cmd, PT rgpt[], int cpt) : BTN(puiParent, cmd), pgeom(nullptr)
{
	pgeom = PgeomCreate(rgpt, cpt);
}

BTNGEOM::~BTNGEOM(void)
{
	SafeRelease(&pgeom);
}

void BTNGEOM::Draw(const RC& rcUpdate)
{
	float dxyScale = RcInterior().DxWidth() / 2.0f;
	FillRc(RcInterior(), pbrBack);
	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	PT ptCenter = rcBounds.PtCenter();
	TRANSDC transdc(pdc,
		Matrix3x2F::Scale(SizeF(dxyScale, dxyScale), PT(0, 0)) *
		Matrix3x2F::Translation(SizeF(ptCenter.x, ptCenter.y)));
	COLORBRS colorbrsSav(pbrText, ColorF((fHilite + fTrack) * 0.5f, 0.0, 0.0));
	pdc->FillGeometry(pgeom, pbrText);
}


/*
 *
 *	SPIN ui control
 * 
 */
static PT rgptLeft[3] = { {0.5f, 0.75f}, {0.5f, -0.75f}, {-0.5f, 0.0f} };
static PT rgptRight[3] = { {-0.5, 0.75f}, {-0.5f, -0.75f}, {0.5f, 0.0f} };


SPIN::SPIN(UI* puiParent, int cmdUp, int cmdDown) : UI(puiParent, true), ptxSpin(nullptr),
		btngeomDown(this, cmdDown, rgptLeft, 3), btngeomUp(this, cmdUp, rgptRight, 3)
		
{
}


void SPIN::CreateRsrc(void)
{
	if (ptxSpin)
		return;

	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
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
	float dxyScale = RcInterior().DyHeight() / 2.0f;
	RC rc = RcInterior();
	rc.right = rc.left + dxyScale;
	btngeomDown.SetBounds(rc);
	rc = RcInterior();
	rc.left = rc.right - dxyScale;
	btngeomUp.SetBounds(rc);
}


void SPIN::Draw(const RC& rcUpdate)
{
	wstring szValue = SzValue();
	SIZ siz = SizSz(szValue, ptxSpin);
	RC rc = RcInterior();
	rc.Offset(PT(0, (rc.DyHeight() - siz.height) / 2));
	DrawSzCenter(szValue, ptxSpin, rc, pbrText);
}
