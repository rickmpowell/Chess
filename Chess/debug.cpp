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
		depthCur(0), depthFile(2), depthShow(2), posLog(nullptr)
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


void UIDB::CreateRsrc(void)
{
	if (ptxLog)
		return;
	ptxLog = PtxCreate(12.0f, false, false);
	ptxLogBold = PtxCreate(12.0f, true, false);
	ptxLogItalic = PtxCreate(12.0f, false, true);
	ptxLogBoldItalic = PtxCreate(12.0f, true, true);
	dyLine = SizFromSz(L"0", ptxLog).height;
	UI::CreateRsrc();
}


void UIDB::DiscardRsrc(void)
{
	SafeRelease(&ptxLog);
	SafeRelease(&ptxLogBold);
	SafeRelease(&ptxLogItalic);
	SafeRelease(&ptxLogBoldItalic);
	UI::DiscardRsrc();
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
void UIDB::DrawContent(const RC& rcCont)
{
	if (lgRoot.vplgChild.size() == 0)
		return;

	RC rc = RcContent();
	rc.left += 4.0f;
	rc.right = rc.left + 1000.0f;
	
	size_t ilgentryFirst = IlgentryFromY((int)RcView().top);
	rc.top = RcContent().top + lgRoot.vplgChild[ilgentryFirst]->dyTop;

	for (size_t ilgentry = ilgentryFirst; ilgentry < lgRoot.vplgChild.size() && rc.top < RcView().bottom; ilgentry++) {
		LG& lg = *lgRoot.vplgChild[ilgentry];

		/* 0 heights are those that were combined into another line */
		
		if (lg.dyHeight == 0)
			continue;
		
		/* get string and formatting */

		wstring sz;
		if (lg.tag.sz.size() > 0)
			sz = lg.tag.sz + L" ";
		sz += lg.szData;
		rc.bottom = rc.top + dyLine;
		LGF lgf = lg.lgf;

		/* if matching open and close are next to each other, then combine them to a single line */

		if (ilgentry + 1 < lgRoot.vplgChild.size() && FCombineLogEntries(lg, *lgRoot.vplgChild[ilgentry+1])) {
			sz += wstring(L" ") + lgRoot.vplgChild[ilgentry+1]->szData;
			lgf = lgRoot.vplgChild[ilgentry + 1]->lgf;
		}

		TX* ptx;
		switch (lgf) {
		case lgfItalic: ptx = ptxLogItalic; break;
		case lgfBold: ptx = ptxLogBold; break;
		case lgfBoldItalic: ptx = ptxLogBoldItalic; break;
		default: ptx = ptxLog; break;
		}
		DrawSz(sz, ptx, 
			RC(rc.left+12.0f*lg.depth, rc.top, rc.right, rc.bottom), 
			pbrText);
		rc.top = rc.bottom;
	}
}


/*	UIDB::IlgentryFromY
 *
 *	Finds the log entry in the content area that is located at the vertical location y, which 
 *	is in UIDB coordinates
 */
size_t UIDB::IlgentryFromY(int y) const
{
	float yContentTop = RcContent().top;
	if (lgRoot.vplgChild.size() == 0)
		return 0;
	size_t ilgentryFirst = 0; 
	size_t ilgentryLast = lgRoot.vplgChild.size()-1;
	if (y < yContentTop + lgRoot.vplgChild[ilgentryFirst]->dyTop)
		return ilgentryFirst;
	if (y >= yContentTop + lgRoot.vplgChild[ilgentryLast]->dyTop + lgRoot.vplgChild[ilgentryLast]->dyHeight)
		return ilgentryLast;

	for (;;) {
		size_t ilgentryMid = (ilgentryFirst + ilgentryLast) / 2;
		if (y < yContentTop + lgRoot.vplgChild[ilgentryMid]->dyTop)
			ilgentryLast = ilgentryMid-1;
		else if (y >= yContentTop + lgRoot.vplgChild[ilgentryMid]->dyTop + lgRoot.vplgChild[ilgentryMid]->dyHeight)
			ilgentryFirst = ilgentryMid+1;
		else
			return ilgentryMid;
	}	
}


/*	UIDB::FCombineLogEntries
 *
 *	Returns true if the two log entries should be combined into a single line. Basically, if they
 *	are the corresponding open/close of the same tag.
 */
bool UIDB::FCombineLogEntries(const LG& lg1, const LG& lg2) const noexcept
{
	if (lg2.lgt == lgtTemp)
		return true;
	if (lg1.lgt == lgtOpen && lg2.lgt == lgtClose &&  
			lg1.depth == lg2.depth && lg1.tag.sz == lg2.tag.sz)
		return true;
	return false;
}


float UIDB::DyLine(void) const
{
	return dyLine;
}


/*	UIDB::FDepthLog
 *
 *	Returns true if the item should be logged. Keeps track of the depth of our
 *	structured log in the Open/Close. This function is used as an optimization
 *	so we don't construct logging data if the data isn't going to be logged.
 */
bool UIDB::FDepthLog(LGT lgt, int& depth) noexcept
{
	if (lgt == lgtClose)
		depthCur--;
	assert(depthCur >= 0);
	depth = depthCur;
	if (lgt == lgtOpen)
		depthCur++;
	return depth <= DepthLog();
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
	LG lg(lgt, lgf, depth, tag, szData);

	assert(depth <= DepthLog());

	if (lgRoot.vplgChild.size() > 0 && lgRoot.vplgChild.back()->lgt == lgtTemp) {
		LG* plg = lgRoot.vplgChild.back();
		lgRoot.vplgChild.pop_back();
		delete plg;
	}
	
	/* compute the top and height of the item */

	if ((lgRoot.vplgChild.size() > 0 && FCombineLogEntries(*lgRoot.vplgChild.back(), lg)) || lg.depth > DepthShow())
		lg.dyHeight = 0;
	else
		lg.dyHeight = dyLine;
	if (lgRoot.vplgChild.size() == 0)
		lg.dyTop = 0;
	else
		lg.dyTop = lgRoot.vplgChild.back()->dyTop + lgRoot.vplgChild.back()->dyHeight;

	/* logging a file */

	if (posLog && lg.lgt != lgtTemp) {
		*posLog << string(4 * (int64_t)lg.depth, ' ');
		if (lg.tag.sz.size() > 0) {
			*posLog << SzFlattenWsz(lg.tag.sz);
			*posLog << ' ';
		}
		*posLog << SzFlattenWsz(lg.szData);
		*posLog << endl;
	}
	lgRoot.AddChild(new LG(lg));

	if (lg.depth <= DepthShow()) {
		float dyBot = lg.dyTop + lg.dyHeight;
		UpdateContSize(SIZ(RcContent().DxWidth(), dyBot));
		FMakeVis(RcContent().top + dyBot, lg.dyHeight);
		Redraw();
	}
}


/*	UIDB::ClearLog
 *
 *	Clears the current log
 */
void UIDB::ClearLog(void) noexcept
{
	while (lgRoot.vplgChild.size()) {
		LG* plg = lgRoot.vplgChild.back();
		lgRoot.vplgChild.pop_back();
		delete plg;
	}
	UpdateContSize(SIZ(0, 0));
	FMakeVis(RcContent().top, RcContent().left);
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


/*	UIDB::DepthLog
 *
 *	Returns the depth we're currently logging to. 
 */
int UIDB::DepthLog(void) const noexcept
{
	return max(depthShow, depthFile);
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
