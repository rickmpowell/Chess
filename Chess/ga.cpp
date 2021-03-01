/*
 *
 *	ga.cpp
 * 
 *	Game implementation. A game is the whole big collection of
 *	everything that goes into playing the game, including the
 *	board, players, and display.
 * 
 */

#include "Chess.h"
#include "ga.h"


/*
 * 
 *	SPA class implementation
 *
 *	Screen panel implementation. Base class for the pieces of stuff
 *	that gets displayed on the screen
 *
 */


ID2D1SolidColorBrush* SPA::pbrBack;
ID2D1SolidColorBrush* SPA::pbrText;
ID2D1SolidColorBrush* SPA::pbrGridLine;
ID2D1SolidColorBrush* SPA::pbrAltBack;
IDWriteTextFormat* SPA::ptfText;


/*	SPA::CreateRsrc
 *
 *	Static routine for creating the drawing objects necessary to draw the various
 *	screen panels.
 */
void SPA::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr)
{
	if (!pbrBack) {
		prt->CreateSolidColorBrush(ColorF(ColorF::White), &pbrBack);
		prt->CreateSolidColorBrush(ColorF(ColorF::Black), &pbrText);
		prt->CreateSolidColorBrush(ColorF(.8f, .8f, .8f), &pbrGridLine);
		prt->CreateSolidColorBrush(ColorF(.95f, .95f, .95f), &pbrAltBack);
		pfactdwr->CreateTextFormat(L"Arial", NULL,
			DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			16.0f, L"",
			&ptfText);

	}
}


void SPA::DiscardRsrc(void)
{
	SafeRelease(&pbrBack);
	SafeRelease(&pbrText);
	SafeRelease(&pbrGridLine);
	SafeRelease(&pbrAltBack);
	SafeRelease(&ptfText);
}


void SPA::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	switch (ll) {
	case LL::Absolute:
		rcfBounds.left = ptf.x;
		rcfBounds.top = ptf.y;
		rcfBounds.right = rcfBounds.left + DxWidth();
		rcfBounds.bottom = rcfBounds.top + DyHeight();
		break;
	case LL::Right:
		rcfBounds.left = pspa->rcfBounds.right + ptf.x;
		rcfBounds.top = pspa->rcfBounds.top;
		rcfBounds.right = rcfBounds.left + DxWidth();
		rcfBounds.bottom = rcfBounds.top + DyHeight();
		break;
	default:
		assert(false);
		break;
	}
	SetShadow(NULL);
}


void SPA::SetShadow(ID2D1RenderTarget* prt)
{
#ifdef LATER
	ID2D1DeviceContext* pdc;
	ID2D1Effect* peffShadow;
	pdc->CreateEffect(CLSID_D2D1Shadow, &peffShadow);
	peffShadow->SetInput(0, bitmap);

	// Shadow is composited on top of a white surface to show opacity.
	ID2D1Effect* peffFlood;
	pdc->CreateEffect(CLSID_D2D1Flood, &peffFlood);
	peffFlood->SetValue(D2D1_FLOOD_PROP_COLOR, Vector4F(1.0f, 1.0f, 1.0f, 1.0f));
	ID2D1Effect* peffAffineTrans;
	pdc->CreateEffect(CLSID_D2D12DAffineTransform, &peffAffineTrans);
	peffAffineTrans->SetInputEffect(0, peffShadow);
	D2D1_MATRIX_3X2_F mx = Matrix3x2F::Translation(20, 20));
	peffAffineTrans->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, mx);
	ID2D1Effect* peffComposite;
	pdc->CreateEffect(CLSID_D2D1Composite, &peffComposite);

	peffComposite->SetInputEffect(0, peffFlood);
	peffComposite->SetInputEffect(1, peffAffineTrans);
	peffComposite->SetInput(2, bitmap);

	pdc->DrawImage(peffComposite);
#endif
}


SPA::SPA(GA& ga) : ga(ga)
{
}


SPA::~SPA(void)
{
}


void SPA::Draw(ID2D1RenderTarget* prt)
{
	prt->FillRectangle(&rcfBounds, pbrBack);
}


/*	SPA::Redraw
 *
 *	Simple helper to redraw panel.
 */
void SPA::Redraw(void)
{
	ga.app.prth->BeginDraw();
	Draw(ga.app.prth);
	ga.app.prth->EndDraw();
}


float SPA::DxWidth(void) const
{
	return 100.0f;
}


float SPA::DyHeight(void) const
{
	return 200.0f;
}


HT* SPA::PhtHitTest(const PTF& ptf)
{
	if (!rcfBounds.FContainsPtf(ptf))
		return NULL;
	return new HT(ptf, HTT::Static, this);
}


void SPA::StartLeftDrag(HT* pht)
{
}


void SPA::EndLeftDrag(HT* pht)
{
}


void SPA::LeftDrag(HT* pht)
{
}

void SPA::MouseHover(HT* pht)
{
}


/*
 *
 *	SPARGMV class implementation
 * 
 *	Move list implementation
 * 
 */


SPARGMV::SPARGMV(GA& ga) : SPAS(ga)
{
}


SPARGMV::~SPARGMV(void)
{
}


IDWriteTextFormat* SPARGMV::ptfList;


/*	SPARGMV::CreateRsrc
 *
 *	Creates the drawing resources for displaying the move list screen
 *	panel.
 */
void SPARGMV::CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr)
{
	if (ptfList)
		return;

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		14.0f, L"",
		&ptfList);

}


void SPARGMV::DiscardRsrc(void)
{
	SafeRelease(&ptfList);
}


float SPARGMV::mpcoldxf[] = { 40.0f, 70.0f, 70.0f, 20.0f };
float SPARGMV::dyfList = 20.0f;


void SPARGMV::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	SPAS::Layout(ptf, pspa, ll);
	RCF rcf = rcfBounds;
	rcf.Offset(-rcf.left, -rcf.top);
	rcf.top = 4.0f * dyfList;
	SetView(rcf);
	SetContent(rcf);
}


void SPARGMV::Draw(ID2D1RenderTarget* prt)
{
	SPAS::Draw(prt);

	/* draw header box */

	RCF rcf = rcfBounds;
	rcf.bottom = rcfView.top - 1;
	prt->FillRectangle(&rcf, pbrAltBack);
	rcf.top = rcf.bottom + 1.0f;
	rcf.bottom = rcf.top + 1.0f;
	prt->FillRectangle(&rcf, pbrGridLine);
}


void SPARGMV::DrawContent(ID2D1RenderTarget* prt, const RCF& rcfCont)
{
	BDG bdgT(bdgInit);
	float yf = rcfCont.top + dyfList;
	for (unsigned imv = 0; imv < ga.bdg.rgmvGame.size(); imv++) {
		MV mv = ga.bdg.rgmvGame[imv];
		if (imv % 2 == 0) {
			DrawMoveNumber(prt, RCF(0, yf, mpcoldxf[0], yf+dyfList), imv / 2 + 1);
			DrawMv(prt, RCF(mpcoldxf[0], yf, mpcoldxf[0]+mpcoldxf[1], yf+dyfList), 
					bdgT, mv);
		}
		else {
			DrawMv(prt, RCF(mpcoldxf[0]+mpcoldxf[1], yf, mpcoldxf[0]+mpcoldxf[1]+mpcoldxf[2], yf+dyfList), 
					bdgT, mv);
			yf += dyfList;
		}
		bdgT.MakeMv(mv);
	}
}


void SPARGMV::DrawMv(ID2D1RenderTarget* prt, RCF rcf, const BDG& bdg, MV mv)
{
	rcf.Offset(rcfBounds.left, rcfBounds.top);
	rcf.left += 4.0f;
	wstring sz = bdg.SzDecodeMv(mv);
	ptfList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	prt->DrawText(sz.c_str(), sz.size(), ptfList, rcf, pbrText);
}


void SPARGMV::DrawMoveNumber(ID2D1RenderTarget* prt, RCF rcf, int imv)
{
	rcf.Offset(rcfBounds.left, rcfBounds.top);
	rcf.right -= 4.0f;
	WCHAR sz[8];
	WCHAR* pch = PchDecodeInt(imv, sz);
	*pch++ = L'.';
	ptfList->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	prt->DrawText(sz, pch-sz, ptfList, rcf, pbrText);
}


WCHAR* SPARGMV::PchDecodeInt(unsigned imv, WCHAR* pch)
{
	if (imv / 10 != 0)
		pch = PchDecodeInt(imv / 10, pch);
	*pch++ = imv % 10 + L'0';
	*pch = '\0';
	return pch;
}


float SPARGMV::DxWidth(void) const
{
	float dxf = 0;
	for (int col = 0; col < CArray(mpcoldxf); col++)
		dxf += mpcoldxf[col];
	return dxf;
}


float SPARGMV::DyHeight(void) const
{
	return ga.spabd.DyHeight();
}


void SPARGMV::NewGame(void)
{
	bdgInit = ga.bdg;
}


/*
 *
 *	SPATI class implementation
 * 
 *	The title screen panel
 * 
 */


SPATI::SPATI(GA& ga) : SPA(ga)
{
}

void SPATI::Draw(ID2D1RenderTarget* prt)
{
	SPA::Draw(prt);
	PL* pplWhite = ga.PplFromTpc(tpcWhite);
	PL* pplBlack = ga.PplFromTpc(tpcBlack);
	wstring sz = pplWhite->SzName() + L"\n v. \n" + pplBlack->SzName();
	unsigned cch = sz.length();
	ptfText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	RCF rcf = rcfBounds;
	rcf.top += 40.0f;
	prt->DrawText(sz.c_str(), cch, ptfText, rcf,
		pbrText, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);}


/*
 *
 *	GA class
 * 
 *	Chess game implementation
 * 
 */


void GA::CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic)
{
	SPA::CreateRsrc(prt, pfactdwr);
	SPABD::CreateRsrc(prt, pfactd2d, pfactdwr, pfactwic);
	SPARGMV::CreateRsrc(prt, pfactdwr);
}


void GA::DiscardRsrc(void)
{
	SPA::DiscardRsrc();
	SPABD::DiscardRsrc();
	SPARGMV::DiscardRsrc();
}


GA::GA(APP& app) : app(app), spati(*this), spabd(*this), spargmv(*this),
	phtCapt(NULL)
{
	PTF ptfMargin = PTF(10.0f, 10.0f);
	spati.Layout(ptfMargin, NULL, LL::Absolute);
	spabd.Layout(ptfMargin, &spati, LL::Right);
	spargmv.Layout(ptfMargin, &spabd, LL::Right);
	rgppl[0] = rgppl[1] = NULL;
}


GA::~GA(void)
{
}


void GA::SetPl(BYTE tpc, PL* ppl)
{
	rgppl[CpcFromTpc(tpc)] = ppl;
}


void GA::Draw(ID2D1RenderTarget* prt)
{
	spati.Draw(prt);
	spabd.Draw(prt);
	spargmv.Draw(prt);
}


void GA::NewGame(void)
{
	bdg.NewGame();
	spabd.NewGame();
	spargmv.NewGame();
	app.Redraw(NULL);
}


HT* GA::PhtHitTest(const PTF& ptf)
{
	if (phtCapt) 
		return phtCapt->pspa->PhtHitTest(ptf);

	HT* pht;
	if ((pht = spati.PhtHitTest(ptf)) != NULL)
		return pht;
	if ((pht = spabd.PhtHitTest(ptf)) != NULL)
		return pht;
	if ((pht = spargmv.PhtHitTest(ptf)) != NULL)
		return pht;
	return NULL;
}


void GA::MouseMove(HT* pht)
{
	if (phtCapt)
		phtCapt->pspa->LeftDrag(pht);
	else if (pht == NULL)
		::SetCursor(app.hcurArrow);
	else {
		switch (pht->htt) {
		case HTT::Static:
			::SetCursor(app.hcurArrow);
			break;
		case HTT::MoveablePc:
			::SetCursor(app.hcurMove);
			break;
		case HTT::UnmoveablePc:
			::SetCursor(app.hcurNo);
			break;
		case HTT::EmptyPc:
		case HTT::OpponentPc:
			::SetCursor(app.hcurNo);
			break;
		default:
			::SetCursor(app.hcurArrow);
			break;
		}
		pht->pspa->MouseHover(pht);
	}
}


void GA::SetCapt(HT* pht)
{
	ReleaseCapt();
	if (pht) {
		::SetCapture(app.hwnd);
		phtCapt = pht->PhtClone();
	}
}


void GA::ReleaseCapt(void)
{
	if (phtCapt == NULL)
		return;
	delete phtCapt;
	phtCapt = NULL;
	::ReleaseCapture();
}


void GA::LeftDown(HT* pht)
{
	SetCapt(pht);
	if (pht)
		pht->pspa->StartLeftDrag(pht);
}


void GA::LeftUp(HT* pht)
{
	if (phtCapt) {
		phtCapt->pspa->EndLeftDrag(pht);
		ReleaseCapt();
	}
	else if (pht)
		pht->pspa->EndLeftDrag(pht);
}


void GA::MakeMv(MV mv)
{
	spabd.MakeMv(mv);
	spargmv.Redraw();
}