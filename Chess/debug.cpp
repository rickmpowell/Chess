/*
 *
 *	debug.cpp
 * 
 *	Debugging panels
 * 
 */

#include "debug.h"
#include "Resource.h"

bool fValidate = false;


UIDBBTNS::UIDBBTNS(UI* puiParent) : UI(puiParent), btnTest(this, cmdTest, RCF(), L'\x2713')
{
}

void UIDBBTNS::Layout(void)
{
	btnTest.SetBounds(RCF(4.0f, 4.0f, 30.0f, 30.0f));
}

void UIDBBTNS::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);
}


UIDB::UIDB(GA* pga) : UIPS(pga), uidbbtns(this)
{
}


void UIDB::Layout(void)
{
	/* position top items */
	RCF rcf = RcfInterior();
	RCF rcfCont = rcf;
	rcf.bottom = rcf.top;
	AdjustUIRcfBounds(&uidbbtns, rcf, true, 40.0f);
	rcfCont.top = rcf.bottom;

	/* positon bottom items */
	rcf = RcfInterior();
	rcf.top = rcf.bottom;
	// no items yet
	rcfCont.bottom = rcf.top;

	/* move list content is whatever is left */

	SetView(rcfCont);
	SetContent(rcfCont);
}


void UIDB::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrGridLine);
	UIPS::Draw(prcfUpdate); // draws content area of the scrollable area

}


void UIDB::AddEval(const BDG& bdg, MV mv, float eval)
{
}