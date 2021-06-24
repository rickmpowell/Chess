/*
 *
 *	debug.cpp
 * 
 *	Debugging panels
 * 
 */

#include "debug.h"
#include "ga.h"
#include "resources/Resource.h"

bool fValidate = true;



SPINDEPTH::SPINDEPTH(UIBBDBLOG* puiParent) : SPIN(puiParent, cmdLogDepthUp, cmdLogDepthDown), 
		ga(puiParent->uidb.ga)
{
}

wstring SPINDEPTH::SzValue(void) const
{
	return to_wstring(ga.uidb.DepthLog());
}


/*
 *
 *	UIDBBTNS
 * 
 *	Debug button bar
 * 
 */


UIBBDB::UIBBDB(UIDB* puiParent) : UIBB(puiParent), 
	btnTest(this, cmdTest, L'\x2713')
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
		btnLogOnOff(this, cmdLogFileToggle, idbFloppyDisk), spindepth(this)
{
}


void UIBBDBLOG::Layout(void)
{
	RC rc = RcInterior().Inflate(-2.0f, -2.0f);
	rc.right = rc.left;
	AdjustBtnRcBounds(&btnLogOnOff, rc, rc.DyHeight());
	AdjustBtnRcBounds(&spindepth, rc, 40.0f);
}


/*
 *
 *	UIDB
 * 
 *	Debug panel
 * 
 */


UIDB::UIDB(GA* pga) : UIPS(pga), uibbdb(this), uibbdblog(this), 
		ptxLog(nullptr), ptxLogBold(nullptr), ptxLogItalic(nullptr), ptxLogBoldItalic(nullptr), dyLine(12.0f),
		depthCur(0), depthShowSet(-1), depthShowDefault(2), posLog(nullptr)
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
	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLog);
	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLogBold);
	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLogItalic);
	App().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLogBoldItalic);
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
	AdjustUIRcBounds(&uibbdb, rc, true);
	rcView.top = rc.bottom;

	/* positon bottom items */
	rc = RcInterior();
	rc.top = rc.bottom;
	AdjustUIRcBounds(&uibbdblog, rc, false);
	rcView.bottom = rc.top;

	/* move list content is whatever is left */

	dyLine = SizSz(L"0", ptxLog).height;
	AdjustRcView(rcView);
}


void UIDB::Draw(RC rcUpdate)
{
	FillRc(rcUpdate, pbrGridLine);
	UIPS::Draw(rcUpdate); // draws content area of the scrollable area
}


void UIDB::DrawContent(RC rcCont)
{
	if (vlgentry.size() == 0)
		return;

	RC rc = RcContent();
	rc.left += 4.0f;
	rc.right = rc.left + 1000.0f;
	
	size_t ilgentryFirst = IlgentryFromY(RcView().top);
	rc.top = RcContent().top + vlgentry[ilgentryFirst].dyTop;

	for (size_t ilgentry = ilgentryFirst; ilgentry < vlgentry.size() && rc.top < RcView().bottom; ilgentry++) {
		LGENTRY& lgentry = vlgentry[ilgentry];

		/* 0 heights are those that were combined into another line */
		
		if (lgentry.dyHeight == 0)
			continue;
		
		/* get string and formatting */

		wstring sz;
		if (lgentry.tag.sz.size() > 0)
			sz = lgentry.tag.sz + L" ";
		sz += lgentry.szData;
		rc.bottom = rc.top + dyLine;
		LGF lgf = lgentry.lgf;

		/* if matching open and close are next to each other, then combine them to a single line */

		if (ilgentry + 1 < vlgentry.size() && FCombineLogEntries(lgentry, vlgentry[ilgentry+1])) {
			sz += wstring(L" ") + vlgentry[ilgentry+1].szData;
			lgf = vlgentry[ilgentry + 1].lgf;
		}

		TX* ptx;
		switch (lgf) {
		case LGF::Italic: ptx = ptxLogItalic; break;
		case LGF::Bold: ptx = ptxLogBold; break;
		case LGF::BoldItalic: ptx = ptxLogBoldItalic; break;
		default: ptx = ptxLog; break;
		}
		DrawSz(sz, ptx, 
			RC(rc.left+12.0f*lgentry.depth, rc.top, rc.right, rc.bottom), 
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
	if (vlgentry.size() == 0)
		return 0;
	size_t ilgentryFirst = 0; 
	size_t ilgentryLast = vlgentry.size()-1;
	if (y < yContentTop + vlgentry[ilgentryFirst].dyTop)
	if (y >= yContentTop + vlgentry[ilgentryLast].dyTop + vlgentry[ilgentryLast].dyHeight)
		return ilgentryLast;

	for (;;) {
		size_t ilgentryMid = (ilgentryFirst + ilgentryLast) / 2;
		if (y < yContentTop + vlgentry[ilgentryMid].dyTop)
			ilgentryLast = ilgentryMid-1;
		else if (y >= yContentTop + vlgentry[ilgentryMid].dyTop + vlgentry[ilgentryMid].dyHeight)
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
bool UIDB::FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const
{
	if (lgentry2.lgt == LGT::Temp)
		return true;
	if (lgentry1.lgt == LGT::Open && lgentry2.lgt == LGT::Close &&  
			lgentry1.depth == lgentry2.depth && lgentry1.tag.sz == lgentry2.tag.sz)
		return true;
	return false;
}


float UIDB::DyLine(void) const
{
	return dyLine;
}


bool UIDB::FDepthLog(LGT lgt, int& depth)
{
	if (lgt == LGT::Close)
		depthCur--;
	assert(depthCur >= 0);
	depth = depthCur;
	if (lgt == LGT::Open)
		depthCur++;
	return depth <= DepthLog();
}


void UIDB::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	LGENTRY lgentry(lgt, lgf, depth, tag, szData);

	assert(depth <= DepthLog());

	if (vlgentry.size() > 0 && vlgentry.back().lgt == LGT::Temp)
		vlgentry.pop_back();
	
	if (vlgentry.size() > 0 && FCombineLogEntries(vlgentry.back(), lgentry))
		lgentry.dyHeight = 0;
	else
		lgentry.dyHeight = dyLine;
	if (vlgentry.size() == 0)
		lgentry.dyTop = 0;
	else
		lgentry.dyTop = vlgentry.back().dyTop + vlgentry.back().dyHeight;

	if (posLog && lgentry.lgt != LGT::Temp) {
		*posLog << string(4 * lgentry.depth, ' ');
		if (lgentry.tag.sz.size() > 0) {
			*posLog << SzFlattenWsz(lgentry.tag.sz);
			*posLog << ' ';
		}
		*posLog << SzFlattenWsz(lgentry.szData);
		*posLog << endl;
	}
	vlgentry.push_back(lgentry);

	float dyBot = lgentry.dyTop + lgentry.dyHeight;
	UpdateContSize(SIZ(RcContent().DxWidth(), dyBot));
	FMakeVis(RcContent().top + dyBot, lgentry.dyHeight);
	Redraw();
}


/*	UIDB::ClearLog
 *
 *	Clears the current log
 */
void UIDB::ClearLog(void)
{
	vlgentry.clear();
	UpdateContSize(SIZ(0,0));
	FMakeVis(RcContent().top, RcContent().left);
}


void UIDB::SetDepthLog(int depth)
{
	depthShowSet = depth;
}

void UIDB::SetDepthLogDefault(int depth)
{
	depthShowDefault = depth;
}

void UIDB::InitLog(int depth)
{
	ClearLog();
	if (depth < 0)
		depth = -1;
	SetDepthLogDefault(depth);
	Redraw();
}


int UIDB::DepthLog(void) const
{
	if (depthShowSet == -1)
		return depthShowDefault;
	else
		return depthShowSet;
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
		wstring szAppData = ga.app.SzAppDataPath();
		posLog = new ofstream(szAppData + L"\\chess.log");
	}
}

bool UIDB::FLogFileEnabled(void) const
{
	return posLog != nullptr;
}
