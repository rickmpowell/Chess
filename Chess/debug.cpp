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


/*
 *
 *	UIDBBTNS
 * 
 *	Debug button bar
 * 
 */


UIDBBTNS::UIDBBTNS(UI* puiParent) : UI(puiParent),
	btnTest(this, cmdTest, RCF(), L'\x2713'),
	btnDnDepth(this, cmdLogDepthDown, RCF(), L'\x21e4'),
	btnUpDepth(this, cmdLogDepthUp, RCF(), L'\x21e5'),
	btnLogOnOff(this, cmdLogFileToggle, RCF(), L'\x25bc')
{
}

void UIDBBTNS::Layout(void)
{
	RCF rcf(10.0f, 2.0f, 34.0f, 26.0f);;
	btnTest.SetBounds(rcf);
	btnDnDepth.SetBounds(rcf.Offset(34.0f, 0));
	btnUpDepth.SetBounds(rcf.Offset(34.0f, 0));
	btnLogOnOff.SetBounds(rcf.Offset(34.0f, 0));
}

SIZF UIDBBTNS::SizfLayoutPreferred(void)
{
	return SIZF(-1.0f, 28.0f);
}

void UIDBBTNS::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);
}


UIDB::UIDB(GA* pga) : UIPS(pga), uidbbtns(this), dyfLine(12.0f), ptxLog(nullptr), ptxLogBold(nullptr), 
		depthCur(0), depthShow(2), posLog(nullptr)
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
	AppGet().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLog);
	AppGet().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		12.0f, L"",
		&ptxLogBold);

}

void UIDB::DiscardRsrc(void)
{
	SafeRelease(&ptxLog);
	SafeRelease(&ptxLogBold);
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
	AdjustUIRcfBounds(&uidbbtns, rcf, true);
	rcfView.top = rcf.bottom;

	/* positon bottom items */
	rcf = RcfInterior();
	rcf.top = rcf.bottom;
	// no items yet
	rcfView.bottom = rcf.top;

	/* move list content is whatever is left */

	dyfLine = SizfSz(L"0", ptxLog).height;
	AdjustRcfView(rcfView);
}


void UIDB::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrGridLine);
	UIPS::Draw(prcfUpdate); // draws content area of the scrollable area
}


void UIDB::DrawContent(const RCF& rcfCont)
{
	RCF rcf = RcfContent();
	rcf.left += 4.0f;
	rcf.right = rcf.left + 1000.0f;
	
	size_t ilgentryFirst;
	for (ilgentryFirst = 0; ilgentryFirst < rglgentry.size(); ilgentryFirst++) {
		if (rcf.top + rglgentry[ilgentryFirst].dyfTop + rglgentry[ilgentryFirst].dyfHeight > RcfView().top)
			break;
		rcf.top = RcfContent().top + rglgentry[ilgentryFirst].dyfTop;
	}

	for (size_t ilgentry = ilgentryFirst; ilgentry < rglgentry.size() && rcf.top < RcfView().bottom; ilgentry++) {
		
		/* 0 heights are those that were combined into another line */
		
		if (rglgentry[ilgentry].dyfHeight == 0)
			continue;
		
		/* get string and formatting */

		wstring sz = rglgentry[ilgentry].szTag + L" " + rglgentry[ilgentry].szData;
		rcf.bottom = rcf.top + dyfLine;
		LGF lgf = rglgentry[ilgentry].lgf;

		/* if matching open and close are next to each other, then combine them to a single line */

		if (ilgentry + 1 < rglgentry.size() && FCombineLogEntries(rglgentry[ilgentry], rglgentry[ilgentry+1])) {
			sz += wstring(L" ") + rglgentry[ilgentry+1].szData;
			lgf = rglgentry[ilgentry + 1].lgf;
		}

		DrawSz(sz, lgf == LGF::Bold ? ptxLogBold : ptxLog, 
			RCF(rcf.left+12.0f*rglgentry[ilgentry].depth, rcf.top, rcf.right, rcf.bottom), 
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


void UIDB::AddLog(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData)
{
	LGENTRY lgentry(lgt, lgf, szTag, szData);

	if (lgt == LGT::Close)
		depthCur--;
	assert(depthCur >= 0);
	lgentry.depth = depthCur;
	if (lgt == LGT::Open)
		depthCur++;

	if (lgentry.depth > depthShow)
		return;

	if (rglgentry.size() > 0 && rglgentry.back().lgt == LGT::Temp)
		rglgentry.pop_back();
	
	if (rglgentry.size() > 0 && FCombineLogEntries(rglgentry.back(), lgentry))
		lgentry.dyfHeight = 0;
	else
		lgentry.dyfHeight = dyfLine;
	if (rglgentry.size() == 0)
		lgentry.dyfTop = 0;
	else
		lgentry.dyfTop = rglgentry.back().dyfTop + rglgentry.back().dyfHeight;

	if (posLog && lgentry.lgt != LGT::Temp) {
		*posLog << string(4 * lgentry.depth, ' ');
		*posLog << SzFlattenWsz(lgentry.szTag);
		*posLog << ' ';
		*posLog << SzFlattenWsz(lgentry.szData);
		*posLog << endl;
	}
	rglgentry.push_back(lgentry);

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
	rglgentry.clear();
	UpdateContSize(SIZF(0,0));
	Redraw();
}


void UIDB::SetDepthLog(int depth)
{
	depthShow = depth;
}


void UIDB::InitLog(int depth)
{
	ClearLog();
	if (depth > 0)
		SetDepthLog(depth);
}


int UIDB::DepthLog(void) const
{
	return depthShow;
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
