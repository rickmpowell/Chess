/*
 *
 *	debug.cpp
 * 
 *	Debugging panels
 * 
 */

#include "debug.h"
#include "uiga.h"
#include "resources/Resource.h"

bool fValidate = true;


/*
 *
 *	SPINDEPTH control
 *
 *	Spin control to display the debug log depth
 *
 */


SPINDEPTH::SPINDEPTH(UIBBDBLOG* puiParent, int& depth, int cmdUp, int cmdDown) : SPIN(puiParent, cmdUp, cmdDown), depth(depth) 
{
}

wstring SPINDEPTH::SzValue(void) const
{
	return to_wstring(depth);
}


/*
 *
 *	UIDBBTNS
 * 
 *	Debug button bar
 * 
 */


UIBBDB::UIBBDB(UIDB* puiParent) : UIBB(puiParent), btnTest(this, cmdTest, L'\x2713')
{
}


void UIBBDB::Layout(void)
{
	RC rc = RcInterior().Inflate(-2.0f, -2.0f);
	rc.right = rc.left;
	AdjustBtnRcBounds(&btnTest, rc, rc.DyHeight());
}


/*
 *
 *	UIBBDBLOG
 * 
 *	Button bar for log controls
 * 
 */


UIBBDBLOG::UIBBDBLOG(UIDB* puiParent) : UIBB(puiParent), uidb(*puiParent),
		spindepth(this, puiParent->depthShow, cmdLogDepthUp, cmdLogDepthDown), 
		staticFile(this, L"Log file:"), 
		btnLogOnOff(this, cmdLogFileToggle, idbFloppyDisk), 
		spindepthFile(this, puiParent->depthFile, cmdLogFileDepthUp, cmdLogFileDepthDown)
{
	staticFile.SetTextSize(16.0f);
}


void UIBBDBLOG::Layout(void)
{
	RC rc = RcInterior().Inflate(-2.0f, -2.0f);
	rc.right = rc.left;
	AdjustBtnRcBounds(&spindepth, rc, 40.0f);
	rc.right += 20.0f;
	AdjustBtnRcBounds(&staticFile, rc, 60.0f);
	AdjustBtnRcBounds(&btnLogOnOff, rc, rc.DyHeight());
	AdjustBtnRcBounds(&spindepthFile, rc, 40.0f);
}


/*
 *
 *	UIDB
 * 
 *	Debug panel
 * 
 */


UIDB::UIDB(UIGA& uiga) : UIPS(uiga), uibbdb(this), uibbdblog(this), 
		ptxLog(nullptr), ptxLogBold(nullptr), ptxLogItalic(nullptr), ptxLogBoldItalic(nullptr), dyLine(12.0f),
		plgCur(&lgRoot), depthCur(0), depthFile(2), depthShow(2), posLog(nullptr)
{
}


UIDB::~UIDB()
{
	if (posLog) {
		posLog->close();
		delete posLog;
	}
	DiscardRsrc();
}


bool UIDB::FCreateRsrc(void)
{
	if (ptxLog)
		return false;

	ptxLog = PtxCreate(12.0f, false, false);
	ptxLogBold = PtxCreate(12.0f, true, false);
	ptxLogItalic = PtxCreate(12.0f, false, true);
	ptxLogBoldItalic = PtxCreate(12.0f, true, true);

	return true;
}


void UIDB::DiscardRsrc(void)
{
	SafeRelease(&ptxLog);
	SafeRelease(&ptxLogBold);
	SafeRelease(&ptxLogItalic);
	SafeRelease(&ptxLogBoldItalic);
}

void UIDB::ComputeMetrics(bool fStatic)
{
	if (!fStatic)
		dyLine = SizFromSz(L"0", ptxLog).height;
}


/*	UIDB::Layout
 *
 *	Layout the items in the debug panel.
 */
void UIDB::Layout(void)
{
	/* position top items */
	
	RC rc = RcInterior();
	RC rcView = rc;
	rc.bottom = rc.top;
	AdjustUIRcBounds(uibbdb, rc, true);
	rcView.top = rc.bottom;

	/* positon bottom items */
	
	rc = RcInterior();
	rc.top = rc.bottom;
	AdjustUIRcBounds(uibbdblog, rc, false);
	rcView.bottom = rc.top;

	/* move list content is whatever is left */

	AdjustRcView(rcView);
}


/*	UIDB::SizLayoutPreferred
 *
 *	Tell our owner what our preferred size is.
 */
SIZ UIDB::SizLayoutPreferred(void)
{
	return SIZ(240.0f, -1.0f);
}


/*	UIDB::Draw
 *
 *	Draws the debug UI element
 */
void UIDB::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrGridLine);
	UIPS::Draw(rcUpdate); // draws content area of the scrollable area
}


/*	UIDB::DrawContent
 *
 *	Displays the content area of the debug UI element, which is the log information
 */
void UIDB::DrawContent(const RC& rcUpdate)
{
	if (lgRoot.vplgChild.empty())
		return;

	for (LG* plgChild : lgRoot.vplgChild)
		DrawLg(*plgChild, rcUpdate.top, rcUpdate.bottom);
}

bool FRangesOverlap(float fl1First, float fl1Lim, float fl2First, float fl2Lim)
{
	assert(fl1First <= fl1Lim && fl2First <= fl2Lim);
	return !(fl2Lim <= fl1First || fl2First >= fl1Lim);
}

/*	DrawLg
 *
 *	Draws the log block rooted at lg, constrained by the bounds yTop and yBot, which are in
 *	local view coordinates.
 */
void UIDB::DrawLg(LG& lg, float yTop, float yBot)
{
	if (lg.depth > DepthShow() || lg.dyLineOpen == 0)
		return;
	float yLgTop = RcContent().top + 4.0f + lg.yTop;
	if (!FRangesOverlap(yLgTop, yLgTop + lg.dyBlock, yTop, yBot))
		return;

	/* if an open with immediate close, we may combine into a single line, otherwise draw everythign 
	   we've received. */

	RC rc;
	if (!FLgAnyChildVisible(lg)) {
		if (FRcOfLgOpen(lg, rc, yTop, yBot)) {
			if (lg.lgt == lgtClose && FCollapsedLg(lg))
				DrawItem(wjoin(lg.tagOpen.sz, lg.szDataOpen, lg.szDataClose), lg.depth, lg.lgfClose, rc);
			else
				DrawItem(wjoin(lg.tagOpen.sz, lg.szDataOpen, lg.tagOpen[L"TEMP"]), lg.depth, lg.lgfOpen, rc);
		}
		if (lg.lgt == lgtClose && !FCollapsedLg(lg) && FRcOfLgClose(lg, rc, yTop, yBot))
			DrawItem(wjoin(lg.tagClose.sz, lg.szDataOpen, lg.szDataClose), lg.depth, lg.lgfClose, rc);
	}
	else {
		if (FRcOfLgOpen(lg, rc, yTop, yBot))
			DrawItem(wjoin(lg.tagOpen.sz, lg.szDataOpen, lg.tagOpen[L"TEMP"]), lg.depth, lg.lgfOpen, rc);
		for (LG* plgChild : lg.vplgChild)
			DrawLg(*plgChild, yTop, yBot);
		if (lg.lgt == lgtClose && FRcOfLgClose(lg, rc, yTop, yBot))
			DrawItem(wjoin(lg.tagClose.sz, lg.szDataClose), lg.depth, lg.lgfClose, rc);
	}
}


/*	UIDB::FRcOfLgOpen
 *
 *	Returns the rectangle we draw open log items into. This also works for leaf nodes.
 *	The Rectangle is returned in local view coordinates. Returns false if the rectangle
 *	is not visible in the range
 */
bool UIDB::FRcOfLgOpen(const LG& lg, RC& rc, float yTop, float yBot) const
{
	rc = RcContent();
	rc.top += 4.0f + lg.yTop;
	rc.bottom = rc.top + lg.dyLineOpen;
	rc.right = rc.left + 1000.0f;
	return FRangesOverlap(rc.top, rc.bottom, yTop, yBot);
}


/*	UIDB::FRcOfLogClose
 *
 *	For closed log nodes, returns the rectangle of the close text, in local view
 *	coordinates. Returns false if the rectrangel is not visible in the range.
 */
bool UIDB::FRcOfLgClose(const LG& lg, RC& rc, float yTop, float yBot) const
{
	rc = RcContent();
	rc.bottom = rc.top + 4.0f + lg.yTop + lg.dyBlock;
	rc.top = rc.bottom - lg.dyLineClose;
	rc.right = rc.left + 1000.0f;
	return FRangesOverlap(rc.top, rc.bottom, yTop, yBot);
}


void UIDB::DrawItem(const wstring& sz, int depth, LGF lgf, RC rc)
{
	TX* ptx;
	switch (lgf) {
	case lgfItalic: 
		ptx = ptxLogItalic; 
		break;
	case lgfBold: 
		ptx = ptxLogBold; 
		break;
	case lgfBoldItalic: 
		ptx = ptxLogBoldItalic; 
		break;
	default: 
		ptx = ptxLog; 
		break;
	}
	rc.left += 12.0f * depth;
	DrawSz(sz, ptx, rc, pbrText);
}


float UIDB::DyLine(void) const
{
	return dyLine;
}


/*	UIDB::AddLog
 *
 *	Adds a log entry to the log. lgt is the type (open, close, or data), lgf is formatting,
 *	depth is the depth of the structured log (count of open opens), tag is the name of the
 *	open item (with attributes), and szData is the data in the entry.
 *
 *	Log entries correspond to the features of an XML document, and logs should easily be
 *	transformed into XML.
 */
void UIDB::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept
{
	assert(depth <= DepthLog());

	LG* plgUpdate = plgCur;

	assert(lgt != lgtNil);
	switch (lgt) {
	case lgtTemp:
		plgCur->tagOpen[L"TEMP"] = szData;
		break;
	case lgtClose:
		plgCur->lgt = lgt;
		plgCur->lgfClose = lgf;
		plgCur->tagClose = tag;
		plgCur->szDataClose = szData;
		plgCur = plgCur->plgParent;
		break;
	default:
		plgUpdate = new LG(lgt, lgf, depth, tag, szData);
		plgCur->AddChild(plgUpdate);
		if (lgt == lgtOpen)
			plgCur = plgUpdate;
		break;
	}

	/* relayout and redraw - we go to some effort to do minimum redraw here */

	float dyLast = DyComputeLgPos(*plgUpdate);
	if (depth <= DepthShow()) {
		float yBot = plgUpdate->yTop + plgUpdate->dyBlock;
		UpdateContHeight(yBot);
		RC rc = RC(0, yBot - dyLast, RcView().DxWidth(), yBot).Offset(0, RcContent().top + 4.0f);;
		if (!FMakeVis(rc.top, rc.DyHeight()))
			RedrawContent(rc);
	}

	/* logging to file */

	if (posLog && depth <= DepthFile()) {
		*posLog << string(4 * depth, ' ');
		if (!tag.sz.empty())
			*posLog << SzFlattenWsz(tag.sz) << ' ';
		*posLog << SzFlattenWsz(szData) << endl;
	}
}


/*	UIDB::DyComputeLgPos
 *
 *	Comptes the position and size of every item that may need to change after the
 *	given item was added to the log tree.
 * 
 *	Returns the height of last itme added, or zero if the item is hidden.
 */
float UIDB::DyComputeLgPos(LG& lg)
{
	/* compute heights of lines and blocks */

	float dyLast = 0;
	lg.dyLineOpen = lg.dyLineClose = 0;
	lg.dyBlock = 0;
	if (lg.depth <= DepthShow() && lg.lgt != lgtNil) {
		dyLast = lg.dyLineOpen = dyLine;
		if (lg.lgt == lgtClose && !FCollapsedLg(lg))
			dyLast = lg.dyLineClose = dyLine;
		/* block height must be recomputed for this and all parent blocks - we don't have to relayout
			sibling items here because we only *append* items to the log, so only the last sibling
			can ever change. */
		for (LG* plgWalk = &lg; plgWalk; plgWalk = plgWalk->plgParent) {
			plgWalk->dyBlock = DyBlock(*plgWalk);
			assert(plgWalk->FIsLastSibling());
		}
	}

	/* figure out position of this item */

	lg.yTop = 0;
	LG* plgPrev = PlgPrev(&lg);
	if (plgPrev) {
		switch (plgPrev->lgt) {
		default:
			break;
		case lgtClose:
			lg.yTop = plgPrev->yTop + plgPrev->dyBlock;
			break;
		case lgtOpen:
		case lgtData:
			lg.yTop = plgPrev->yTop + plgPrev->dyLineOpen;
			break;
		}
	}

	/* we're done! */

	return dyLast;
}


float UIDB::DyBlock(LG& lg) const
{
	float dy = lg.dyLineOpen;
	for (LG* plgChild : lg.vplgChild)
		dy += plgChild->dyBlock;
	dy += lg.dyLineClose;
	return dy;
}


LG* UIDB::PlgPrev(const LG* plg) const
{
	LG* plgParent = plg->plgParent;
	if (plgParent == nullptr)
		return nullptr;
	LG* plgPrev = nullptr;
	for (LG* plgSib : plgParent->vplgChild) {
		if (plgSib == plg)
			break;
		plgPrev = plgSib;
	}
	if (plgPrev == nullptr)
		return plgParent;
	else
		return plgPrev;
}


void UIDB::RelayoutLog(void)
{
	/* TODO: try to scroll to keep the current visible area on the screen */
	lgRoot.yTop = 0;
	FullRelayoutLg(lgRoot, lgRoot.yTop);
	UpdateContHeight(lgRoot.dyBlock);
	if (!FMakeVis(RcContent().top, RcContent().left))
		RedrawContent();
}


void UIDB::FullRelayoutLg(LG& lg, float& yTop)
{
	lg.yTop = yTop;
	lg.dyLineOpen = lg.dyLineClose = 0;
	if (lg.depth <= DepthShow()) {
		lg.dyLineOpen = dyLine;
		if (lg.lgt == lgtClose && !FCollapsedLg(lg))
			lg.dyLineClose = dyLine;
	}
	yTop += lg.dyLineOpen;
	for (LG* plg : lg.vplgChild)
		FullRelayoutLg(*plg, yTop);
	lg.dyBlock = DyBlock(lg);
	yTop += lg.dyLineClose;
}


bool UIDB::FLgAnyChildVisible(const LG& lg) const
{
	return !lg.vplgChild.empty() && lg.depth+1 <= DepthShow();
}


bool UIDB::FCollapsedLg(const LG& lg) const
{
	if (FLgAnyChildVisible(lg))
		return false;
	return lg.tagOpen.sz == lg.tagClose.sz;
}


/*	UIDB::ClearLog
 *
 *	Clears the current log
 */
void UIDB::ClearLog(void) noexcept
{
	for ( ; !lgRoot.vplgChild.empty(); lgRoot.vplgChild.pop_back())
		delete lgRoot.vplgChild.back();
	lgRoot.yTop = 0;
	UpdateContHeight(0);
	FMakeVis(RcContent().top, dyLine);
}


/*	UIDB::SetDepthShow
 *
 *	User-set depth of the log that we show/save. If this value is not set, then
 *	the default log depth is used.
 */
void UIDB::SetDepthShow(int depth) noexcept
{
	depthShow = max(0, depth);
}


void UIDB::SetDepthFile(int depth) noexcept
{
	depthFile = max(0, depth);
}


void UIDB::InitLog(void) noexcept
{
	ClearLog();
	Redraw();
}


int UIDB::DepthShow(void) const noexcept
{
	return depthShow;
}


int UIDB::DepthFile(void) const noexcept
{
	return depthFile;
}


void UIDB::EnableLogFile(bool fEnable)
{
	if (fEnable == (posLog != nullptr))
		return;
	if (posLog) {
		posLog->close();
		delete posLog;
		posLog = nullptr;
	}
	else {
		wstring szAppData = uiga.app.SzAppDataPath();
		posLog = new ofstream(szAppData + L"\\sqchess.log");
	}
}


/*	UIDB::FLogFIleEnabled
 *
 *	Returns true if we're logging stuff to a file.
 */
bool UIDB::FLogFileEnabled(void) const noexcept
{
	return posLog != nullptr;
}
