/*
 *
 *	ti.cpp
 * 
 *	The title panel on the screen. Includes interfaces for starting new
 *	game, loading games, setting game type attributes, etc.
 * 
 */

#include "ti.h"
#include "ga.h"
#include "Resource.h"



UIICON::UIICON(UI* puiParent, int idb) : UI(puiParent), idb(idb), pbmp(NULL)
{
}

UIICON::~UIICON(void)
{
}


void UIICON::CreateRsrc(void)
{
	if (pbmp)
		return;
	APP& app = AppGet();
	pbmp = PbmpFromPngRes(idb, app.pdc, app.pfactwic);
}


void UIICON::DiscardRsrc(void)
{
	SafeRelease(&pbmp);
}


void UIICON::Draw(const RCF* prcfUpdate)
{
	D2D1_SIZE_F sizf = Sizf();
	DrawBmp(RcfInterior(), pbmp, RCF(0, 0, sizf.width, sizf.height), 1.0f);
}


SIZF UIICON::Sizf(void) const
{
	return pbmp->GetSize();
}


/*
 *
 *	UIGT class
 * 
 *	Game type widgit in the title block
 * 
 */


UIGT::UIGT(UI* puiParent) : UI(puiParent)
{
}


void UIGT::Draw(const RCF* prcfUpdate)
{
	DrawSz(L"Casual", ptxList, RcfInterior());
}


/*
 *
 *	UILOCALE class
 * 
 *	Game locale widgit in the title block
 * 
 */

UILOCALE::UILOCALE(UI* puiParent) : UI(puiParent)
{
}

void UILOCALE::Draw(const RCF* prcfUpdate)
{
	DrawSz(L"Local Machine", ptxList, RcfInterior());
}


/*
 *
 *	UIGTM
 * 
 *	Game time type widgit in the title block
 * 
 */

UIGTM::UIGTM(UI* puiParent) : UI(puiParent)
{
}

void UIGTM::Draw(const RCF* prcfUpdate)
{
	DrawSz(L"Rapid \x2022 10+0", ptxList, RcfInterior());
}


/*
 *
 *	UITI class implementation
 *
 *	The title screen panel
 *
 */


/*	SPATI::CreateRsrcClass
 *
 *	Creates the drawing resources for displaying the title screen
 *	panel. Note that this is a static routine working on global static
 *	resources that are shared by all instances of this class.
 */
void UITI::CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
}


/*	UITI::DiscardRsrcClass
 *
 *	Deletes all resources associated with this screen panel. This is a
 *	static routine, and works on static class globals.
 */
void UITI::DiscardRsrcClass(void)
{
}


UITI::UITI(GA* pga) : UIP(pga), uiicon(this, idbLogo), uilocale(this), uigt(this), uigtm(this),
		uiplWhite(this, cpcWhite), uiplBlack(this, cpcBlack), szText(L"")
{
}


void UITI::Layout(void)
{
	SIZF sizf = uiicon.Sizf();
	RCF rcf(20.0f, 20.0f, 0, 90.0f);
	rcf.right = rcf.left + sizf.width * 70.0f / sizf.height;
	uiicon.SetBounds(rcf);

	rcf.left = rcf.right + 20.0f;
	rcf.right = RcfInterior().right;
	rcf.bottom = rcf.top + 16.0f;
	uilocale.SetBounds(rcf);

	rcf.top = rcf.bottom;
	rcf.bottom = rcf.top + 16.0f;
	uigt.SetBounds(rcf);

	rcf.top = rcf.bottom;
	rcf.bottom = rcf.top + 16.0f;
	uigtm.SetBounds(rcf);

	rcf.top = 100.0f;
	rcf.left = 0;
	rcf.bottom = rcf.top + 28.0f;
	uiplWhite.SetBounds(rcf);

	rcf.top = rcf.bottom;
	rcf.bottom = rcf.top + 28.0f;
	uiplBlack.SetBounds(rcf);
}


void UITI::Draw(const RCF* prcfUpdate)
{
	FillRcf(*prcfUpdate, pbrBack);

	RCF rcf = RcfInterior();
	rcf.top = rcf.bottom - 36.0f;
	RCF rcfLine = rcf;
	rcfLine.bottom = rcfLine.top + 1;
	FillRcf(rcfLine, pbrGridLine);

	if (szText.size() > 0)
		DrawSz(szText, ptxTextSm, rcf);
}


void UITI::SetText(const wstring& sz)
{
	szText = sz;
	Redraw();
}


void UITI::SetPl(CPC cpc, PL* ppl)
{
	if (cpc == cpcWhite)
		uiplWhite.SetPl(ppl);
	else
		uiplBlack.SetPl(ppl);
}