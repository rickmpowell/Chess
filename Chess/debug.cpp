/*
 *
 *	debug.cpp
 * 
 *	Debugging panels
 * 
 */

#include "debug.h"
#include "resources/Resource.h"

bool fValidate = true;


UIDBBTNS::UIDBBTNS(UI* puiParent) : UI(puiParent), btnTest(this, cmdTest, RCF(), L'\x2713')
{
}

void UIDBBTNS::Layout(void)
{
	btnTest.SetBounds(RCF(12.0f, 4.0f, 38.0f, 30.0f));
}

SIZF UIDBBTNS::SizfLayoutPreferred(void)
{
	return SIZF(-1.0f, 40.0f);
}

void UIDBBTNS::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);
}


UIDB::UIDB(GA* pga) : UIPS(pga), uidbbtns(this), dyfLine(12.0f), ptxLog(nullptr), ptxLogBold(nullptr), depthLog(0), depthShow(2)
{
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


void UIDB::ShowLog(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData)
{
	LGENTRY lgentry(lgt, lgf, szTag, szData);

	if (lgt == LGT::Close)
		depthLog--;
	assert(depthLog >= 0);
	lgentry.depth = depthLog;
	if (lgt == LGT::Open)
		depthLog++;

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


	rglgentry.push_back(lgentry);

	float dyfBot = lgentry.dyfTop + lgentry.dyfHeight;
	UpdateContSize(PTF(RcfContent().DxfWidth(), dyfBot));
	FMakeVis(RcfContent().top + dyfBot, lgentry.dyfHeight);
	Redraw();
}

void UIDB::ClearLog(void)
{
	rglgentry.clear();
	Redraw();
}

void UIDB::SetLogDepth(int depth)
{
	depthShow = depth;
}