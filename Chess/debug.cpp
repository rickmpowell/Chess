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
	RCF rcf = RcfInterior().Inflate(-2.0f, -2.0f);
	rcf.right = rcf.left;
	AdjustBtnRcfBounds(&btnTest, rcf, rcf.DyfHeight());
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
	RCF rcf = RcfInterior().Inflate(-2.0f, -2.0f);
	rcf.right = rcf.left;
	AdjustBtnRcfBounds(&spindepth, rcf, 40.0f);
	AdjustBtnRcfBounds(&btnLogOnOff, rcf, rcf.DyfHeight());
}


/*
 *
 *	UIDB
 * 
 *	Debug panel
 * 
 */


UIDB::UIDB(GA* pga) : UIPS(pga), uibbdb(this), uibbdblog(this), 
		ptxLog(nullptr), ptxLogBold(nullptr), dyfLine(12.0f),
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
	UI::CreateRsrc();
}


void UIDB::DiscardRsrc(void)
{
	SafeRelease(&ptxLog);
	SafeRelease(&ptxLogBold);
	UI::DiscardRsrc();
}


/*	UIDB::Layout
 *
 *	Layout the items in the debug panel.
 */
void UIDB::Layout(void)
{
	/* position top items */
	RCF rcf = RcfInterior();
	RCF rcfView = rcf;
	rcf.bottom = rcf.top;
	AdjustUIRcfBounds(&uibbdb, rcf, true);
	rcfView.top = rcf.bottom;

	/* positon bottom items */
	rcf = RcfInterior();
	rcf.top = rcf.bottom;
	AdjustUIRcfBounds(&uibbdblog, rcf, false);
	rcfView.bottom = rcf.top;

	/* move list content is whatever is left */

	dyfLine = SizfSz(L"0", ptxLog).height;
	AdjustRcfView(rcfView);
}


void UIDB::Draw(RCF rcfUpdate)
{
	FillRcf(rcfUpdate, pbrGridLine);
	UIPS::Draw(rcfUpdate); // draws content area of the scrollable area
}


void UIDB::DrawContent(RCF rcfCont)
{
	RCF rcf = RcfContent();
	rcf.left += 4.0f;
	rcf.right = rcf.left + 1000.0f;
	
	size_t ilgentryFirst;
	for (ilgentryFirst = 0; ilgentryFirst < vlgentry.size(); ilgentryFirst++) {
		if (rcf.top + vlgentry[ilgentryFirst].dyfTop + vlgentry[ilgentryFirst].dyfHeight > RcfView().top)
			break;
		rcf.top = RcfContent().top + vlgentry[ilgentryFirst].dyfTop;
	}

	for (size_t ilgentry = ilgentryFirst; ilgentry < vlgentry.size() && rcf.top < RcfView().bottom; ilgentry++) {
		
		/* 0 heights are those that were combined into another line */
		
		if (vlgentry[ilgentry].dyfHeight == 0)
			continue;
		
		/* get string and formatting */

		wstring sz = vlgentry[ilgentry].szTag + L" " + vlgentry[ilgentry].szData;
		rcf.bottom = rcf.top + dyfLine;
		LGF lgf = vlgentry[ilgentry].lgf;

		/* if matching open and close are next to each other, then combine them to a single line */

		if (ilgentry + 1 < vlgentry.size() && FCombineLogEntries(vlgentry[ilgentry], vlgentry[ilgentry+1])) {
			sz += wstring(L" ") + vlgentry[ilgentry+1].szData;
			lgf = vlgentry[ilgentry + 1].lgf;
		}

		DrawSz(sz, lgf == LGF::Bold ? ptxLogBold : ptxLog, 
			RCF(rcf.left+12.0f*vlgentry[ilgentry].depth, rcf.top, rcf.right, rcf.bottom), 
			pbrText);
		rcf.top = rcf.bottom;
	}
}


bool UIDB::FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const
{
	return lgentry1.lgt == LGT::Open && 
		lgentry1.szTag == lgentry2.szTag &&
		((lgentry2.lgt == LGT::Close && lgentry1.depth == lgentry2.depth) ||
		 (lgentry2.lgt == LGT::Temp && lgentry1.depth+1 == lgentry2.depth));
}


float UIDB::DyfLine(void) const
{
	return dyfLine;
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


void UIDB::AddLog(LGT lgt, LGF lgf, int depth, const wstring& szTag, const wstring& szData)
{
	LGENTRY lgentry(lgt, lgf, depth, szTag, szData);

	assert(depth <= DepthLog());

	if (vlgentry.size() > 0 && vlgentry.back().lgt == LGT::Temp)
		vlgentry.pop_back();
	
	if (vlgentry.size() > 0 && FCombineLogEntries(vlgentry.back(), lgentry))
		lgentry.dyfHeight = 0;
	else
		lgentry.dyfHeight = dyfLine;
	if (vlgentry.size() == 0)
		lgentry.dyfTop = 0;
	else
		lgentry.dyfTop = vlgentry.back().dyfTop + vlgentry.back().dyfHeight;

	if (posLog && lgentry.lgt != LGT::Temp) {
		*posLog << string(4 * lgentry.depth, ' ');
		*posLog << SzFlattenWsz(lgentry.szTag);
		*posLog << ' ';
		*posLog << SzFlattenWsz(lgentry.szData);
		*posLog << endl;
	}
	vlgentry.push_back(lgentry);

	float dyfBot = lgentry.dyfTop + lgentry.dyfHeight;
	UpdateContSize(SIZF(RcfContent().DxfWidth(), dyfBot));
	FMakeVis(RcfContent().top + dyfBot, lgentry.dyfHeight);
	Redraw();
}


/*	UIDB::ClearLog
 *
 *	Clears the current log
 */
void UIDB::ClearLog(void)
{
	vlgentry.clear();
	UpdateContSize(SIZF(0,0));
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
