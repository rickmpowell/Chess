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

wchar_t UI::szFontFamily[] = L"Arial";
BRS* UI::pbrBack;
BRS* UI::pbrScrollBack;
BRS* UI::pbrGridLine;
BRS* UI::pbrText;
BRS* UI::pbrHilite;
BRS* UI::pbrWhite;
BRS* UI::pbrBlack;
TX* UI::ptxText;
TX* UI::ptxTextBold;
TX* UI::ptxList;
TX* UI::ptxListBold;
TX* UI::ptxTip;


bool UI::FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrBack)
		return false;

	pdc->CreateSolidColorBrush(coStdBack, &pbrBack);
	pdc->CreateSolidColorBrush(coScrollBack, &pbrScrollBack);
	pdc->CreateSolidColorBrush(coGridLine, &pbrGridLine);
	pdc->CreateSolidColorBrush(coStdText, &pbrText);
	pdc->CreateSolidColorBrush(coHiliteBack, &pbrHilite);
	pdc->CreateSolidColorBrush(coWhite, &pbrWhite);
	pdc->CreateSolidColorBrush(coBlack, &pbrBlack);

	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   16.0f, L"",
							   &ptxText);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   16.0f, L"",
							   &ptxTextBold);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   12.0f, L"",
							   &ptxList);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   12.0f, L"",
							   &ptxListBold);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
							   DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
							   13.0f, L"",
							   &ptxTip);

	return true;
}


void UI::DiscardRsrcStatic(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrScrollBack);
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrText);
	SafeRelease(&pbrHilite);
	SafeRelease(&pbrWhite);
	SafeRelease(&pbrBlack);
	SafeRelease(&ptxList);
	SafeRelease(&ptxListBold);
	SafeRelease(&ptxText);
	SafeRelease(&ptxTextBold);
	SafeRelease(&ptxTip);
}


/*	UI::PgeomCreate
 *
 *	Creates a geometry object from the array of points.
 */
ID2D1PathGeometry* UI::PgeomCreate(const PT apt[], int cpt) const
{
	ID2D1PathGeometry* pgeom;
	App().pfactd2->CreatePathGeometry(&pgeom);
	ID2D1GeometrySink* psink;
	pgeom->Open(&psink);
	psink->BeginFigure(apt[0], D2D1_FIGURE_BEGIN_FILLED);
	psink->AddLines(&apt[1], cpt - 1);
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


TX* UI::PtxCreate(float dyHeight, bool fBold, bool fItalic) const
{
	TX* ptx = nullptr;
	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
									 fBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
									 fItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
									 DWRITE_FONT_STRETCH_NORMAL,
									 dyHeight, L"", 
									 &ptx);
	return ptx;
}


/*	UI::UI
 *
 *	Constructor for the UI elements. Adds the item to the UI tree structure
 *	by connecting it to the parent.
 */
UI::UI(UI* puiParent, bool fVisible) : puiParent(puiParent), rcBounds(0, 0, 0, 0), fVisible(fVisible),
		coText(coStdText), coBack(coStdBack)
{
	if (puiParent)
		puiParent->AddChild(this);
}


UI::UI(UI* puiParent, const RC& rcBounds, bool fVisible) : puiParent(puiParent), rcBounds(rcBounds), fVisible(fVisible),
		coText(coStdText), coBack(coStdBack)
{
	if (puiParent) {
		puiParent->AddChild(this);
		this->rcBounds.Offset(puiParent->rcBounds.left, puiParent->rcBounds.top);
	}
}


UI::~UI(void) 
{
	for (; !vpuiChild.empty(); vpuiChild.pop_back())
		delete vpuiChild.back();
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
	vpuiChild.resize(remove(vpuiChild.begin(), vpuiChild.end(), puiChild) - vpuiChild.begin());
}


/*	UI::PuiPrevSib
 *
 *	Returns the UI's previous sibling
 */
UI* UI::PuiPrevSib(void) const
{
	if (puiParent == nullptr)
		return nullptr;
	vector<UI*>::const_iterator itpui = find(puiParent->vpuiChild.begin(), puiParent->vpuiChild.end(), this);
	return itpui == puiParent->vpuiChild.begin() ? nullptr : *(itpui - 1);
}


/*	UI::PuiNextSib
 *
 *	Gets the next sibling window of the given UI element. Returns null if no more
 *	siblings.
 */
UI* UI::PuiNextSib(void) const
{
	if (puiParent == nullptr)
		return nullptr;
	vector<UI*>::const_iterator itpui = find(puiParent->vpuiChild.begin(), puiParent->vpuiChild.end(), this);
	return itpui == puiParent->vpuiChild.end() ? nullptr : *(itpui + 1);
}


bool UI::FCreateRsrc(void)
{
	return false;
}


void UI::DiscardRsrc(void)
{
}


void UI::DiscardAllRsrc(void)
{
	DiscardRsrc();
	for (UI*& pui : vpuiChild)
		pui->DiscardAllRsrc();
}


void UI::EnsureRsrc(bool fStatic)
{
	if (fStatic || FCreateRsrc())
		ComputeMetrics(fStatic);
	for (UI*& pui : vpuiChild)
		pui->EnsureRsrc(fStatic);
}

void UI::ComputeMetrics(bool fStatic)
{
}


void UI::Layout(void)
{
}

void UI::Relayout(void)
{
	EnsureRsrc(false);
	Layout();
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
	Relayout();
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
	Relayout();
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
		puiParent->Relayout();
		puiParent->Redraw();
	}
	else {
		Relayout();
		Redraw();
	}
}


void UI::ShowAll(bool fVisNew)
{
	for (UI* puiChild : vpuiChild)
		puiChild->ShowAll(fVisNew);
	Show(fVisNew);
}

/*	UI::PuiFromPt
 *
 *	Returns the UI element the point is over. Point is in global coordinates.
 *	Returns nullptr if the point is not within the UI.
 */
UI* UI::PuiFromPt(const PT& pt)
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
	SetDrag(this);
}

void UI::EndLeftDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
}

void UI::LeftDrag(const PT& pt)
{
}

void UI::StartRightDrag(const PT& pt)
{
	SetDrag(this);
}

void UI::EndRightDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
}

void UI::RightDrag(const PT& pt)
{
}

void UI::NoButtonDrag(const PT& pt)
{
}

void UI::MouseHover(const PT& pt, MHT mht)
{
}

void UI::ScrollWheel(const PT& pt, int dwheel)
{
}


/*	UI::SetDrag
 *
 *	Sets the mouse captured drag UI element. Note that capture is a global state, 
 *	and is not really an attribute tracked by individual UI element. By convention 
 *	we keep track of these types of global elements in the root UI element. These 
 *	child UI elements just propagate the capture towards the root of the tree, and 
 *	it is  the root elements responsibility to actually keep track of drag capture
 *	and do something with it. 
 */
void UI::SetDrag(UI* pui)
{
	if (puiParent)
		puiParent->SetDrag(pui);
}


/*	UI::ReleaseDrag
 *
 *	Releases drag capture, i.e., sets is to zero. Note that capture is not a 
 *	nestable attribute - release always completely clears the global capture 
 *	state, it does not restore it.
 */
void UI::ReleaseDrag(void)
{
	if (puiParent)
		puiParent->ReleaseDrag();
}


void UI::SetFocus(UI* pui)
{
	if (puiParent)
		puiParent->SetFocus(pui);
}


void UI::KeyDown(int vk)
{
}


void UI::KeyUp(int vk)
{
}


TID UI::StartTimer(DWORD dmsec)
{
	return StartTimer(this, dmsec);
}


void UI::StopTimer(TID tid)
{
	return StopTimer(this, tid);
}


TID UI::StartTimer(UI* pui, DWORD dmsec)
{
	if (puiParent)
		return puiParent->StartTimer(pui, dmsec);
	return 0;
}


void UI::StopTimer(UI* pui, TID tid)
{
	if (puiParent)
		puiParent->StopTimer(pui, tid);
}


void UI::TickTimer(TID tid, DWORD msecCur)
{
}


/*	UI::DispatchCmd
 *
 *	Dispatches a command. We simply propogate the command id to the parent. It is up to the
 *	top level UI element to intercept this command and do something with it.
 */
void UI::DispatchCmd(int cmd)
{
	if (puiParent)
		puiParent->DispatchCmd(cmd);;
}


/*	UI::FEnabledCmd
 *
 *	Returns true if the given command is enabled. This works by just propogating the command
 *	up through the parent chain until we find a UI elment tha thandles these commands.
 */
bool UI::FEnabledCmd(int cmd) const
{
	if (puiParent)
		return puiParent->FEnabledCmd(cmd);
	else
		return true;
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


/*	UI::RcLocalFromUiLocal
 *
 *	Converts local coordinates in pui to local coordinates in the current window.
 */
RC UI::RcLocalFromUiLocal(const UI* pui, const RC& rc) const
{
	return rc.Offset(pui->rcBounds.left-rcBounds.left, pui->rcBounds.top-rcBounds.top);
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

/*	UI::PtLocalFromUiLocal
 *
 *	Converts local coordinates in pui to local coordinates in the current window.
 */
PT UI::PtLocalFromUiLocal(const UI* pui, const PT& pt) const
{
	return PT(pt.x + pui->rcBounds.left - rcBounds.left, pt.y + pui->rcBounds.top - rcBounds.top);
}


PT UI::PtGlobalFromUiLocal(const UI* pui, const PT& pt) const
{
	return PT(pt.x + pui->rcBounds.left, pt.y + pui->rcBounds.top);
}


/*	UI::RedrawWithChildren
 *
 *	Updates the UI element and all child elements. rcUpdate is in global coordinates.
 *	Assumes the DC is already valid and ready to go, so can be done inside a
 *	BeginPaint and also inside regular redraws.
 */
void UI::RedrawWithChildren(const RC& rcUpdate, bool fParentDrawn)
{
	if (!fVisible)
		return;
	RC rc = rcUpdate & rcBounds;
	if (!rc)
		return;
	RedrawNoChildren(rc, fParentDrawn);
	for (UI* pui : vpuiChild)
		pui->RedrawWithChildren(rc, true);
}



/*	UI::RedrawNoChildren
 *
 *	Redraws the UI element without drawing child UI elements. Rectangle is in
 *	global coordinates.
 */
void UI::RedrawNoChildren(const RC& rc, bool fParentDrawn)
{
	App().pdc->PushAxisAlignedClip(rc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	
	EnsureRsrc(false);
	COLORBRS colorbrsBack(pbrBack, CoBack());
	COLORBRS colorbrsText(pbrText, CoText());
	
	RC rcDraw = RcLocalFromGlobal(rc);
	Erase(rcDraw, fParentDrawn);
	Draw(rcDraw);
	
	App().pdc->PopAxisAlignedClip();
}


/*	UI::Redraw
 *
 *	Does a full redraw of the UI element and all its children.
 */
void UI::Redraw(void)
{
	Redraw(RcInterior(), false);
}


/*	UI::Redraw
 *
 *	Redraws the area of the UI element, along with children elements. rcUpdate 
 *	is in local coordinates.
 * 
 *	fParentDrawn is true if the parent window has been drawn, which needs to be
 *	known for windows that have transparent backgrounds.
 */
void UI::Redraw(const RC& rcUpdate, bool fParentDrawn)
{
	if (!fVisible)
		return;
	RC rcGlobal = RcGlobalFromLocal(rcUpdate);
	BeginDraw();
	RedrawWithChildren(rcGlobal, fParentDrawn);
	RedrawOverlappedSiblings(rcGlobal);
	RedrawCursor(rcUpdate);
	EndDraw();
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
		if (fFoundUs)
			pui->RedrawWithChildren(pui->rcBounds & rcUpdate, false);
		else if (pui == this)
			fFoundUs = true;
	}
	puiParent->RedrawOverlappedSiblings(rcUpdate);
}


/*	UI::RedrawCursor
 *
 *	Redraws the cursor after screen update has finished. rcUpdate is the area that
 *	has been redrawn, so we don't need to redraw the cursor if it doesn't intersect
 *	with that rectangle. rcUpdate is in global coordinates.
 */
void UI::RedrawCursor(const RC& rcUpdate)
{
	if (puiParent == nullptr)
		return;
	puiParent->RedrawCursor(rcUpdate);
}


/*	UI::DrawCursor
 *
 *	Draws the cursor, if we've set one. rcUpdate
 */
void UI::DrawCursor(UI* puiDraw, const RC& rcUpdate)
{
}


/*	UI::InvalOutsideRc
 *
 *	While we're tracking piece dragging, it's possible for a piece to be drawn outside
 *	the bounding box of the board. Any drawing inside the board is taken care of by
 *	calling Draw directly, so we handle these outside parts by just invalidating the
 *	area so they'll get picked off eventually by normal update paints.
 * 
 *	Rectangle is in local coordinates
 */
void UI::InvalOutsideRc(const RC& rcInval) const
{
	RC rcInt = RcInterior();
	InvalRc(RC(rcInval.left, rcInval.top, rcInval.right, rcInt.top), false);	// area above
	InvalRc(RC(rcInval.left, rcInt.bottom, rcInval.right, rcInval.bottom), false);	// area below
	InvalRc(RC(rcInval.left, rcInt.top, rcInt.left, rcInt.bottom), false);	// area left
	InvalRc(RC(rcInt.right, rcInt.top, rcInval.right, rcInt.bottom), false);	// area right
}


/*	UI::Draw
 *
 *	Virtual function to implement the draw operation for a UI object. This is where
 *	the UI elmement draws itself. rcDraw is the rectangle that needs to be updated,
 *	in local coordinates.
 */
void UI::Draw(const RC& rcDraw)
{
}


void UI::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	FillRcBack(RcInterior());
}


/*	UI::TransparentErase
 *
 *	Call this helper function in the erase handler of UI elements that have transparent 
 *	backgrounds. It makes sure parent items have been redrawn so that the Draw operation
 *	has an up-to-date and consistent canvas to drawn on.
 * 
 *	rcUpdate is in local coordinates. if fParentDrawn is true, then we know the background
 *	has already been drawn so we don't actually have to do any work.
 */
void UI::TransparentErase(const RC& rcUpdate, bool fParentDrawn)
{
	if (fParentDrawn)
		return;
	if (puiParent)
		puiParent->RedrawNoChildren(RcGlobalFromLocal(rcUpdate), false);
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


void UI::SetCoText(ColorF co)
{
	coText = co;
}

void UI::SetCoBack(ColorF co)
{
	coBack = co;
}

ColorF UI::CoText(void) const
{
	return coText;
}


ColorF UI::CoBack(void) const
{
	return coBack;
}


/*	UI::FillRc
 *
 *	Graphics helper for filling a rectangle with a brush. The rectangle is in
 *	local UI coordinates
 */
void UI::FillRc(const RC& rc, BR* pbr) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	App().pdc->FillRectangle(&rcGlobal, pbr);
}


void UI::FillRc(const RC& rc, ColorF co) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	COLORBRS colorbrs(pbrBack, co);
	App().pdc->FillRectangle(&rcGlobal, pbrBack);
}


void UI::FillRcBack(const RC& rc) const
{
	FillRc(rc, pbrBack);
}


void UI::DrawRr(const RR& rr, BR* pbr) const
{
	RR rrGlobal(RcGlobalFromLocal(RC(rr.rect)));
	rrGlobal.radiusX = rr.radiusX;
	rrGlobal.radiusY = rr.radiusY;
	App().pdc->DrawRoundedRectangle(rrGlobal, pbr);
}


void UI::FillRr(const RR& rr, BR* pbr) const
{
	RR rrGlobal(RcGlobalFromLocal(rr.rect));
	rrGlobal.radiusX = rr.radiusX;
	rrGlobal.radiusY = rr.radiusY;
	App().pdc->FillRoundedRectangle(rrGlobal, pbr);
}


/*	UI::FillEll
 *
 *	Helper function for filling an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::FillEll(const ELL& ell, BR* pbr) const
{
	ELL ellGlobal = ell.Offset(PtGlobalFromLocal(PT(0, 0)));
	App().pdc->FillEllipse(&ellGlobal, pbr);
}


/*	UI::DrawEll
 *
 *	Helper function for drawing an ellipse with a brush. Rectangle is in
 *	local UI coordinates
 */
void UI::DrawEll(const ELL& ell, ID2D1Brush* pbr, float dxyWidth) const
{
	ELL ellGlobal = ell.Offset(PtGlobalFromLocal(PT(0, 0)));
	App().pdc->DrawEllipse(&ellGlobal, pbr, dxyWidth);
}


void UI::DrawEll(const ELL& ell, ColorF co, float dxyWidth) const
{
	COLORBRS colorbrs(pbrText, co);
	DrawEll(ell, pbrText, dxyWidth);
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


/*	UI::DrawSzBaseline
 *
 *	Draws text, like DrawSz, but baseline aligned. dyBaseline is the distance from the
 *	top of the box to the baseline.
 */
void UI::DrawSzBaseline(const wstring& sz, TX* ptx, const RC& rc, float dyBaseline, BR* pbr) const
{
	RC rcText = RcGlobalFromLocal(rc);
	float dyBaselineAct = DyBaselineSz(sz, ptx);
	rcText.top += dyBaseline - dyBaselineAct;
	App().pdc->DrawText(sz.c_str(), (UINT32)sz.size(), ptx, &rcText, pbr ? pbr : pbrText);
}


/*	UI::DyBaselineSz
 *
 *	Returns the height of the baseline of the text drawn with the given font.
 */
float UI::DyBaselineSz(const wstring& sz, TX* ptx) const
{
	IDWriteTextLayout* play = nullptr;
	App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(), ptx, 1.0e+6, 1.0e+6, &play);
	if (play == nullptr)
		throw 1;
	DWRITE_TEXT_METRICS tm;
	play->GetMetrics(&tm);
	DWRITE_LINE_METRICS lm;
	uint32_t clm;
	play->GetLineMetrics(&lm, 1, &clm);
	SafeRelease(&play);
	return lm.baseline;
}


void UI::DrawSzCenter(const wstring& sz, TX* ptx, const RC& rc, ID2D1Brush* pbr) const
{
	TATX tatxSav(ptx, DWRITE_TEXT_ALIGNMENT_CENTER);
	DrawSz(sz, ptx, rc, pbr);
}


SIZ UI::SizFromSz(const wstring& sz, TX* ptx, float dx, float dy) const
{
	IDWriteTextLayout* ptxl;
	App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(), ptx, dx, dy, &ptxl);
	DWRITE_TEXT_METRICS dtm;
	ptxl->GetMetrics(&dtm);
	SafeRelease(&ptxl);
	return SIZ(dtm.width, dtm.height);
}


SIZ UI::SizFromAch(const wchar_t* ach, int cch, TX* ptx, float dx, float dy) const
{
	IDWriteTextLayout* ptxl;
	App().pfactdwr->CreateTextLayout(ach, cch, ptx, dx, dy, &ptxl);
	DWRITE_TEXT_METRICS dtm;
	ptxl->GetMetrics(&dtm);
	SafeRelease(&ptxl);
	return SIZ(dtm.width, dtm.height);
}


/*	UI::DrawAch
 *
 *	Helper function for drawing text on the screen panel. Rectangle is in local
 *	UI coordinates
 */
void UI::DrawAch(const wchar_t* ach, int cch, TX* ptx, const RC& rc, BR* pbr) const
{
	RC rcGlobal = RcGlobalFromLocal(rc);
	App().pdc->DrawText(ach, (UINT32)cch, ptx, &rcGlobal, pbr ? pbr : pbrText);
}


/*	UI::DrawSzFitBaseline
 *
 *	Draws the text in the rectangle, sizing the font smaller to make it
 *	fit. Uses ptx as the font style to base dependent fonts on. Text
 *	is baseline aligned, which should be constant no matter what size
 *	font we eventually end up using. If dyBaseline is 0, align the baseline 
 *	to where the baseline of the original ptxBase is.
 */
void UI::DrawSzFitBaseline(const wstring& sz, TX* ptxBase, const RC& rcFit, float dyBaseline, BR* pbrText) const
{
	if (pbrText == nullptr)
		pbrText = UI::pbrText;

	/* if the text fits, just blast it out */

	IDWriteTextLayout* play = nullptr;
	App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(), ptxBase, 1.0e+6, rcFit.DyHeight(), &play);
	if (play == nullptr)
		throw 1;
	DWRITE_TEXT_METRICS tm;
	DWRITE_LINE_METRICS lm;
	uint32_t clm;
	play->GetMetrics(&tm);
	if (dyBaseline <= 0.0) {
		play->GetLineMetrics(&lm, 1, &clm);
		dyBaseline = lm.baseline;
	}
	if (tm.width <= rcFit.DxWidth() && tm.height <= rcFit.DyHeight()) {
		DrawSzBaseline(sz, ptxBase, rcFit, dyBaseline, pbrText);
		SafeRelease(&play);
		return;
	}

	RC rcGlobal = RcGlobalFromLocal(rcFit);

	/* and loop using progressively smaller fonts until the text fits */

	IDWriteTextFormat* ptx = nullptr;
	for (float dyFont = ptxBase->GetFontSize() - 1.0f; dyFont > 6.0f; dyFont--) {
		SafeRelease(&play);
		SafeRelease(&ptx);
		ptx = PtxCreate(dyFont, false, false);
		if (App().pfactdwr->CreateTextLayout(sz.c_str(), (UINT32)sz.size(),
											 ptx,
											 rcGlobal.DxWidth(), rcGlobal.DyHeight(),
											 &play) != S_OK)
			throw 1;
		play->GetMetrics(&tm);
		if (tm.width <= rcFit.DxWidth() && tm.height <= rcFit.DyHeight())
			break;
	}

	/* finally, draw the text baseline aligned */

	play->GetLineMetrics(&lm, 1, &clm);
	App().pdc->DrawTextLayout(Point2F(rcGlobal.left, rcGlobal.top + dyBaseline - lm.baseline), 
							  play,
							  pbrText, 
							  D2D1_DRAW_TEXT_OPTIONS_NONE);

	/* cleanup and return */
	
	SafeRelease(&play);
	SafeRelease(&ptx);
}


void UI::DrawSzFit(const wstring& sz, TX* ptxBase, const RC& rcFit, BR* pbrText) const
{
	DrawSzFitBaseline(sz, ptxBase, rcFit, 0, pbrText);
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


/*	UI::FillGeom
 *
 *	Fills the geometry with the given color. Offsets the geometry to the point
 *	and scales it by the size.
 */
void UI::FillGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, BR* pbrFill) const
{
	if (pbrFill == nullptr)
		pbrFill = pbrText;

	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	PT ptOrigin = rcBounds.PtTopLeft();
	TRANSDC transdc(pdc,
		Matrix3x2F::Scale(SizeF(sizScale.width, sizScale.height), PT(0, 0)) *
		Matrix3x2F::Translation(SizeF(ptOrigin.x + ptOffset.x, ptOrigin.y + ptOffset.y)));
	pdc->FillGeometry(pgeom, pbrFill);
}

void UI::FillGeom(GEOM* pgeom, PT ptOffset, float dxyScale, BR* pbrFill) const
{
	return FillGeom(pgeom, ptOffset, SIZ(dxyScale, dxyScale), pbrFill);
}


void UI::FillGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, ColorF coFill) const
{
	COLORBRS colorbrs(pbrText, coFill);
	FillGeom(pgeom, ptOffset, sizScale, pbrText);
}

void UI::FillGeom(GEOM* pgeom, PT ptOffset, float dxyScale, ColorF coFill) const
{
	return FillGeom(pgeom, ptOffset, SIZ(dxyScale, dxyScale), coFill);
}


void UI::FillRotateGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, float angle, BR* pbrFill) const
{
	if (pbrFill == nullptr)
		pbrFill = pbrText;

	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	PT ptOrigin = rcBounds.PtTopLeft();
	TRANSDC transdc(pdc,
		Matrix3x2F::Rotation(angle, PT(0, 0)) *
		Matrix3x2F::Scale(SizeF(sizScale.width, sizScale.height), PT(0, 0)) *
		Matrix3x2F::Translation(SizeF(ptOrigin.x + ptOffset.x, ptOrigin.y + ptOffset.y)));
	pdc->FillGeometry(pgeom, pbrFill);
}

void UI::FillRotateGeom(GEOM* pgeom, PT ptOffset, float dxyScale, float angle, BR* pbrFill) const
{
	FillRotateGeom(pgeom, ptOffset, SIZ(dxyScale, dxyScale), angle, pbrFill);
}

void UI::FillRotateGeom(GEOM* pgeom, PT ptOffset, float dxyScale, float angle, ColorF coFill) const
{
	COLORBRS colorbrs(pbrText, coFill);
	FillRotateGeom(pgeom, ptOffset, dxyScale, angle, pbrText);
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


void BTN::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


ColorF BTN::CoText(void) const
{
	return FEnabledCmd(cmd) ?
		CoBlend(coBtnText, coBtnHilite, (float)(fHilite + fTrack) / 2.0f) :
		coBtnDisabled;
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


void BTN::StartLeftDrag(const PT& pt)
{
	if (!FEnabledCmd(cmd))
		return;
	SetDrag(this);
	Track(true);
}


void BTN::EndLeftDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
	Track(false);
	if (!FEnabledCmd(cmd))
		return;
	if (RcInterior().FContainsPt(pt))
		DispatchCmd(cmd);
}


void BTN::LeftDrag(const PT& pt)
{
	Hilite(RcInterior().FContainsPt(pt));
}


void BTN::MouseHover(const PT& pt, MHT mht)
{
	if (!FEnabledCmd(cmd))
		return;
	if (mht == mhtEnter) {
		Hilite(true);
		ShowTip(this, true);
	}
	else if (mht == mhtExit) {
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

bool BTNCH::FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxButton)
		return false;

	pdc->CreateSolidColorBrush(coBtnText, &pbrsButton);
	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		20.0f, L"",
		&ptxButton);
	
	return true;
}


void BTNCH::DiscardRsrcStatic(void)
{
	SafeRelease(&ptxButton);
	SafeRelease(&pbrsButton);
}


BTNCH::BTNCH(UI* puiParent, int cmd, wchar_t ch) : BTN(puiParent, cmd), ch(ch)
{
}


void BTNCH::Draw(const RC& rcUpdate)
{
	wchar_t sz[2];
	sz[0] = ch;
	sz[1] = 0;
	RC rcChar(PT(0, 0), SizFromSz(sz, ptxButton));
	RC rcTo = RcInterior();
	COLORBRS colorbrsSav(pbrsButton, CoText());
	rcChar += rcTo.PtCenter();
	rcChar.Offset(-rcChar.DxWidth() / 2.0f, -rcChar.DyHeight() / 2.0f);
	DrawSzCenter(sz, ptxButton, rcChar, pbrsButton);
}


void BTNCH::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


float BTNCH::DxWidth(void)
{
	wchar_t szText[2];
	szText[0] = ch;
	szText[1] = 0;
	return SizFromSz(szText, ptxButton).width;
}


/*
 *
 *	BTNTEXT
 * 
 *	A button with text as the button face
 * 
 */


TX* BTNTEXT::ptxButton;

bool BTNTEXT::FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (ptxButton)
		return false;

	pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxButton);

	return true;
}


void BTNTEXT::DiscardRsrcStatic(void)
{
	SafeRelease(&ptxButton);
}


BTNTEXT::BTNTEXT(UI* puiParent, int cmd, const wstring& sz) : BTNCH(puiParent, cmd, ' '), szText(sz)
{
}


void BTNTEXT::Draw(const RC& rcUpdate)
{
	RC rcText(PT(0, 0), SizFromSz(szText, ptxButton));
	RC rcTo = RcInterior();
	COLORBRS colorbrsSav(pbrsButton, CoText());
	rcText += rcTo.PtCenter() - rcText.PtCenter();
	DrawSzCenter(szText, ptxButton, rcText, pbrsButton);
}


float BTNTEXT::DxWidth(void)
{
	return SizFromSz(szText, ptxButton).width;
}


/*
 *
 *	BTNIMG class
 * 
 *	Class for drawing a button with an image as the button face
 * 
 */


BTNIMG::BTNIMG(UI* puiParent, int cmd, int idb) : BTN(puiParent, cmd), idb(idb), pbmp(nullptr)
{
}


BTNIMG::~BTNIMG(void)
{
	DiscardRsrc();
}


void BTNIMG::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}

void BTNIMG::Draw(const RC& rcUpdate)
{
	DC* pdc = App().pdc;
	AADC aadcSav(pdc, D2D1_ANTIALIAS_MODE_ALIASED);
	RC rcTo = RcInterior();
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
	COLORBRS colorbrsSav(pbrText, CoText());
	pdc->FillOpacityMask(pbmp, pbrText, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, rcTo, rcFrom);
}


bool BTNIMG::FCreateRsrc(void)
{
	if (pbmp)
		return false;

	pbmp = PbmpFromPngRes(idb);

	return true;
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
		ptxStatic(nullptr), pbrsStatic(nullptr), szText(sz), dyFont(20.0f)
{
}


bool STATIC::FCreateRsrc(void)
{
	if (ptxStatic)
		return false;

	App().pdc->CreateSolidColorBrush(coStaticText, &pbrsStatic);
	ptxStatic = PtxCreate(dyFont, false, false);
	
	return true;
}


void STATIC::DiscardRsrc(void)
{
	SafeRelease(&ptxStatic);
	SafeRelease(&pbrsStatic);
}


void STATIC::SetTextSize(float dyFontNew)
{
	dyFont = dyFontNew;
	DiscardRsrc();
}


void STATIC::SetText(const wstring& sz)
{
	szText = sz;
}


wstring STATIC::SzText(void) const
{
	return szText;
}


void STATIC::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


void STATIC::Draw(const RC& rcUpdate)
{
	RC rcText(PT(0, 0), SizFromSz(SzText(), ptxStatic));
	RC rcTo = RcInterior();
	rcText += rcTo.PtCenter() - rcText.PtCenter();;
	DrawSzCenter(SzText(), ptxStatic, rcText, pbrsStatic);
}


/*
 *
 *	BTNGEOM
 * 
 *	Button with a geometry to fill for the visual
 * 
 */


BTNGEOM::BTNGEOM(UI* puiParent, int cmd, PT apt[], int cpt) : BTN(puiParent, cmd), pgeom(nullptr)
{
	pgeom = PgeomCreate(apt, cpt);
}

BTNGEOM::~BTNGEOM(void)
{
	SafeRelease(&pgeom);
}

void BTNGEOM::Draw(const RC& rcUpdate)
{
	float dxyScale = RcInterior().DxWidth() / 2.0f;
	FillGeom(pgeom, RcInterior().PtCenter(), dxyScale,
			 CoBlend(CoText(), coBtnHilite, (fHilite + fTrack) * 0.5f));
}


/*
 *
 *	SPIN ui control
 * 
 */
static PT aptLeft[3] = { {0.5f, 0.75f}, {0.5f, -0.75f}, {-0.5f, 0.0f} };
static PT aptRight[3] = { {-0.5, 0.75f}, {-0.5f, -0.75f}, {0.5f, 0.0f} };


SPIN::SPIN(UI* puiParent, int cmdUp, int cmdDown) : UI(puiParent, true), ptxSpin(nullptr),
		btngeomDown(this, cmdDown, aptLeft, 3), btngeomUp(this, cmdUp, aptRight, 3)
		
{
}


bool SPIN::FCreateRsrc(void)
{
	if (ptxSpin)
		return false;

	ptxSpin = PtxCreate(14.0, false, false);

	return true;
}


void SPIN::DiscardRsrc(void)
{
	SafeRelease(&ptxSpin);
}


void SPIN::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
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
	SIZ siz = SizFromSz(szValue, ptxSpin);
	RC rc = RcInterior();
	rc.Offset(PT(0, (rc.DyHeight() - siz.height) / 2));
	COLORBRS colorbrs(pbrText, btngeomDown.CoText());
	DrawSzCenter(szValue, ptxSpin, rc);
}
