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


UIDB::UIDB(GA* pga) : UIPS(pga), uidbbtns(this), dyfLine(12.0f)
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

}

void UIDB::DiscardRsrc(void)
{
	SafeRelease(&ptxLog);
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
	SetView(rcfView);
	SetContent(rcfView);
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
	for (wstring sz : rgszLog) {
		SIZF sizf = SizfSz(sz, ptxLog, rcf.DxfWidth());
		rcf.bottom = rcf.top + dyfLine;
		DrawSz(sz, ptxLog, rcf, pbrText);
		rcf.top = rcf.bottom;
	}
}


void UIDB::ShowLog(LGT lgt, const wstring& sz)
{
#ifdef NOPE
	if (sz.size() == 0)
		return;
#endif
	rgszLog.push_back(sz);
	UpdateContSize(PTF(RcfContent().DxfWidth(), (rgszLog.size() + 1) * dyfLine));
	FMakeVis(RcfContent().top + rgszLog.size() * dyfLine, dyfLine);
	Redraw();
}

void UIDB::ClearLog(void)
{
	rgszLog.clear();
	Redraw();
}