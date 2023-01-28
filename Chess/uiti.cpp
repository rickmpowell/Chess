/*
 *
 *	uiti.cpp
 * 
 *	The title panel on the screen. Includes interfaces for starting new
 *	game, loading games, setting game type attributes, etc.
 * 
 */

#include "uiti.h"
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


UIGT::UIGT(UI* puiParent, UIGA& uiga) : UI(puiParent), uiga(uiga)
{
}


void UIGT::Draw(const RC& rcUpdate)
{
	DrawSz(uiga.ga.szEvent, ptxList, RcInterior());
}


/*
 *
 *	UILOCALE class
 * 
 *	Game locale widgit in the title block
 * 
 */

UILOCALE::UILOCALE(UI* puiParent, UIGA& uiga) : UI(puiParent), uiga(uiga)
{
}

void UILOCALE::Draw(const RC& rcUpdate)
{
	DrawSz(uiga.ga.szSite, ptxList, RcInterior());
}


/*
 *
 *	UIGTM
 * 
 *	Game time control type widgit in the title block
 * 
 */

UIGTM::UIGTM(UI* puiParent, UIGA& uiga) : UI(puiParent), uiga(uiga)
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


UITI::UITI(UIGA& uiga) : UIP(uiga), uiicon(this, idbSqChessLogo), uilocale(this, uiga), uigt(this, uiga), uigtm(this, uiga),
		uiplWhite(*this, uiga, cpcWhite), uiplBlack(*this, uiga, cpcBlack)
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
	uigt.SetBounds(rc);

	rc.top = rc.bottom;
	rc.bottom = rc.top + 16.0f;
	uilocale.SetBounds(rc);

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


/*
 *
 *	UIPVTPL
 * 
 *	Displays the piece value table for the given player
 * 
 */


UIPVTPL::UIPVTPL(UI* puiParent, UIGA& uiga, CPC cpc, GPH gph) : UI(puiParent), 
		uiga(uiga), cpc(cpc), gph(gph)
{
}


ColorF CoGradient(ColorF co1, ColorF co2, float pct)
{
	if (pct <= 0.0f)
		pct = 0.0f;
	else if (pct > 1.0f)
		pct = 1.0f;
	return ColorF(co1.r * (1.0f - pct) + co2.r * pct,
		co1.g * (1.0f - pct) + co2.g * pct,
		co1.b * (1.0f - pct) + co2.b * pct);
}


ColorF UIPVTPL::CoFromApcSq(APC apc, SQ sq) const
{
	PL* ppl = uiga.ga.PplFromCpc(cpc);
	EV dev = ppl->EvFromGphApcSq(gph, apc, sq) - ppl->EvBaseApc(apc);
	if (dev < 0)
		return ColorF(CoGradient(ColorF::White, ColorF::Red, -(float)dev/100.0f));
	else
		return ColorF(CoGradient(ColorF::White, ColorF::Green, (float)dev/100.0f));
}


void UIPVTPL::Draw(const RC& rcUpdate)
{
	float dxyGutter = 5.0;
	float dxyPiece = (RcInterior().DyHeight() - dxyGutter) / 6 - dxyGutter;
	dxyPiece = min(dxyPiece, RcInterior().DxWidth());

	RC rc;
	rc.left = RcInterior().PtCenter().x - dxyPiece / 2;
	rc.right = rc.left + dxyPiece;
	rc.top = dxyGutter;
	rc.bottom = rc.top + dxyPiece;

	BRS* pbrVal = nullptr;
	App().pdc->CreateSolidColorBrush(ColorF(ColorF::Blue), &pbrVal);

	float dxySq = rc.DxWidth() / 8;
	for (APC apc = apcPawn; apc <= apcKing; ++apc) {
		RC rcSq(rc.left, rc.top, rc.left + dxySq, rc.top + dxySq);
		for (int rank = rankMax-1; rank >= 0; rank--) {
			for (int file = 0; file < fileMax; file++) {
				pbrVal->SetColor(CoFromApcSq(apc, SQ(rank, file).sqFlip()));
				FillRc(rcSq, pbrVal);
				rcSq.Offset(dxySq, 0);
			}
			rcSq.Offset(-8*dxySq, dxySq);
		}
		rc.Offset(0, dxyPiece + dxyGutter);
	}
	SafeRelease(&pbrVal);
}


void UIPVTPL::SetGph(GPH gph)
{
	this->gph = gph;
}

/*
 *	UIPVT
 *
 *	Panel for displaying piece value tables of the current players.
 *	Only displays something useful for PLAI derived players. Since
 *	piece tables are static, this is a very dull panel.
 *
 */


UIPVT::UIPVT(UIGA& uiga, CPC cpc) : UIP(uiga), cpc(cpc),
	uipvtplOpening(this, uiga, cpc, gphMidMin), 
	uipvtplMidGame(this, uiga, cpc, gphMidMid), 
	uipvtplEndGame(this, uiga, cpc, gphMidMax)
{
}


void UIPVT::Layout(void)
{
	float dxyGutter = 5.0;
	RC rc = RcInterior();
	rc.left += dxyGutter;
	float dxyWidth = rc.DxWidth() / 3 - dxyGutter;
	rc.right = rc.left + dxyWidth;
	uipvtplOpening.SetBounds(rc);
	uipvtplMidGame.SetBounds(rc.Offset(dxyWidth+dxyGutter, 0));
	uipvtplEndGame.SetBounds(rc.Offset(dxyWidth+dxyGutter, 0));
}


void UIPVT::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrGridLine);
}
