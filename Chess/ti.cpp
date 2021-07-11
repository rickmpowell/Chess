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
#include "resources/Resource.h"



UIICON::UIICON(UI* puiParent, int idb) : UI(puiParent), idb(idb), pbmp(nullptr)
{
}

UIICON::~UIICON(void)
{
}


void UIICON::CreateRsrc(void)
{
	if (pbmp)
		return;
	pbmp = PbmpFromPngRes(idb);
}


void UIICON::DiscardRsrc(void)
{
	SafeRelease(&pbmp);
}


void UIICON::Draw(const RC& rcUpdate)
{
	D2D1_SIZE_F siz = Siz();
	DrawBmp(RcInterior(), pbmp, RC(0, 0, siz.width, siz.height), 1.0f);
}


SIZ UIICON::Siz(void) const
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


void UIGT::Draw(const RC& rcUpdate)
{
	DrawSz(L"Casual", ptxList, RcInterior());
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

void UILOCALE::Draw(const RC& rcUpdate)
{
	DrawSz(L"Local Machine", ptxList, RcInterior());
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

void UIGTM::Draw(const RC& rcUpdate)
{
	DrawSz(L"Rapid \x2022 10+0", ptxList, RcInterior());
}


/*
 *
 *	UITI class implementation
 *
 *	The title screen panel
 *
 */


UITI::UITI(GA* pga) : UIP(pga), uiicon(this, idbSqChessLogo), uilocale(this), uigt(this), uigtm(this),
		uiplWhite(this, CPC::White), uiplBlack(this, CPC::Black)
{
}


void UITI::Layout(void)
{
	SIZ siz = uiicon.Siz();
	RC rc(20.0f, 20.0f, 0, 90.0f);
	rc.right = rc.left + siz.width * 70.0f / siz.height;
	uiicon.SetBounds(rc);

	rc.left = rc.right + 20.0f;
	rc.right = RcInterior().right;
	rc.bottom = rc.top + 16.0f;
	uilocale.SetBounds(rc);

	rc.top = rc.bottom;
	rc.bottom = rc.top + 16.0f;
	uigt.SetBounds(rc);

	rc.top = rc.bottom;
	rc.bottom = rc.top + 16.0f;
	uigtm.SetBounds(rc);

	rc.top = 100.0f;
	rc.left = 0;
	rc.bottom = rc.top + 28.0f;
	uiplWhite.SetBounds(rc);

	rc.top = rc.bottom;
	rc.bottom = rc.top + 28.0f;
	uiplBlack.SetBounds(rc);
}


void UITI::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrBack);
}


void UITI::SetPl(CPC cpc, PL* ppl)
{
	if (cpc == CPC::White)
		uiplWhite.SetPl(ppl);
	else
		uiplBlack.SetPl(ppl);
}