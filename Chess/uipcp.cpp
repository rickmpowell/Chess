/*
 *
 *	uipcp.cpp
 *
 *	Piece palette user interface panel for setting up a board. Includes drag-droppable
 *	pieces that are dragged to the board, and various other controls for initializing
 *	a game state.
 * 
 *	Works closely with the uibd, including leveraging graphical elements.
 *
 */

#include "uibd.h"
#include "uiga.h"


/*
 *
 *	UICPC
 *
 *	The color picking button in the piece panel
 * 
 */


UICPC::UICPC(UI* puiParent, CPC cpc) : BTN(puiParent, cpc), cpc(cpc)
{
}


void UICPC::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	FillRc(rc, pbrText);
	rc.Inflate(-1.0f, -1.0f);
	FillRc(rc, cpc == cpcWhite ? pbrBack : pbrText);
}



/*
 *
 *	UIAPC
 *
 *	The piece drag elements in the piece panel
 *
 */


UIAPC::UIAPC(UIPCP& uipcp, APC apc) : UI(&uipcp), uipcp(uipcp), apc(apc)
{
}



void UIAPC::Draw(const RC& rcUpdate)
{
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	BMP* pbmpPieces = uipcp.PbmpPieces();
	SIZ siz = pbmpPieces->GetSize();
	float dxPiece = siz.width / 6.0f;
	float dyPiece = siz.height / 2.0f;
	float xPiece = mpapcxBitmap[apc] * dxPiece;
	float yPiece = (int)uipcp.cpcShow * dyPiece;
	DrawBmp(RcInterior(), pbmpPieces, RC(xPiece, yPiece, xPiece + dxPiece, yPiece + dyPiece), 1.0f);
}


/*
 *
 *	THe delete piece control
 * 
 */


UIPCDEL::UIPCDEL(UIPCP& uipcp) : UI(&uipcp), uipcp(uipcp)
{
}


void UIPCDEL::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	rc.Inflate(-6.0f, -6.0f);
	float dxyLine = 7.0f;
	ELL ell(rc.PtCenter(), PT(rc.DxWidth()/2.0f-dxyLine, rc.DyHeight()/2.0f-dxyLine));
	COLORBRS colorbrsSav(pbrText, ColorF(0.75f, 0.05f, 0.15f));
	DrawEll(ell, pbrText, dxyLine);

	/* taking an opponent piece - draw an X */

	const float dxyCrossFull = 20.0f;
	const float dxyCrossCenter = 6.0f;
	PT rgptCross[] = {
		{-dxyCrossCenter, -dxyCrossFull},
		{dxyCrossCenter, -dxyCrossFull},
		{dxyCrossCenter, -dxyCrossCenter},
		{dxyCrossFull, -dxyCrossCenter},
		{dxyCrossFull, dxyCrossCenter},
		{dxyCrossCenter, dxyCrossCenter},
		{dxyCrossCenter, dxyCrossFull},
		{-dxyCrossCenter, dxyCrossFull},
		{-dxyCrossCenter, dxyCrossCenter},
		{-dxyCrossFull, dxyCrossCenter},
		{-dxyCrossFull, -dxyCrossCenter},
		{-dxyCrossCenter, -dxyCrossCenter},
		{-dxyCrossCenter, -dxyCrossFull} };

	DC* pdc = App().pdc;
	rc.Inflate(-2.0f*dxyLine-1.0f, -2.0f*dxyLine-1.0f);
	TRANSDC transdc(pdc,
		Matrix3x2F::Rotation(45.0f, PT(0.0f, 0.0f)) *
		Matrix3x2F::Scale(SizeF(rc.DxWidth() / (2.0f * dxyCrossFull),
			rc.DyHeight()/ (2.0f * dxyCrossFull)),
			PT(0.0, 0.0)) *
		Matrix3x2F::Translation(SizeF(rcBounds.left + rc.XCenter(),
			rcBounds.top + rc.YCenter())));

	pbrText->SetColor(ColorF(0, 0, 0));
	GEOM* pgeomCross = PgeomCreate(rgptCross, CArray(rgptCross));
	pdc->FillGeometry(pgeomCross, pbrText);
	SafeRelease(&pgeomCross);

}


/*
 *
 *	UIPCP
 *
 *	The top-level piece panel
 *
 */


UIPCP::UIPCP(UIGA& uiga) : UIP(uiga), uibd(uiga.uibd), uipcdel(*this), cpcShow(cpcWhite)
{
	for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc)
		mpcpcpuicpc[cpc] = new UICPC(this, cpc);
	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc] = new UIAPC(*this, apc);
}


UIPCP::~UIPCP(void)
{
	for (map<CPC,UICPC*>::iterator it = mpcpcpuicpc.begin(); it != mpcpcpuicpc.end(); it++) {
		delete it->second;
		mpcpcpuicpc[it->first] = nullptr;
	}
	for (map<APC,UIAPC*>::iterator it = mpapcpuiapc.begin(); it != mpapcpuiapc.end(); it++) {
		delete it->second;
		mpapcpuiapc[it->first] = nullptr;
	}
}



RC UIPCP::RcFromApc(APC apc) const
{
	RC rc = RcInterior();
	rc.Inflate(-4.0f, -4.0f);
	rc.top += 32.0f;
	rc.left += rc.DyHeight() / 2.0f + 24.0f;
	rc.right = rc.left + rc.DyHeight();
	rc.Offset((rc.DxWidth() + 4.0f) * (apc - apcPawn), 0);
	return rc;
}


void UIPCP::Layout(void)
{
	/* position the color buttons */

	RC rc = RcInterior();
	rc.Inflate(-4.0f, -4.0f);
	rc.left += 12.0f;
	rc.top += 32.0f;
	rc.bottom = rc.YCenter();
	rc.right = rc.left + rc.DyHeight();
	mpcpcpuicpc[cpcWhite]->SetBounds(rc);
	rc.Offset(0, rc.DyHeight());
	mpcpcpuicpc[cpcBlack]->SetBounds(rc);

	/* position the piece drag sources */

	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc]->SetBounds(RcFromApc(apc));

	/* delete piece */

	rc = RcFromApc(apcKing);
	rc.Offset(rc.DxWidth() + 8.0f, 0);
	uipcdel.SetBounds(rc);

	/* castle and en passant state */

	/* player to move */

	/* full board shortcuts for starting position and empty board */
}


void UIPCP::Draw(const RC& rcUpdate)
{
	FillRc(rcUpdate, pbrBack);
}


void UIPCP::DispatchCmd(int cmd)
{
	cpcShow = (CPC)cmd;
	Redraw();
}


wstring UIPCP::SzTipFromCmd(int cmd) const
{
	return cmd == cpcWhite ? L"White" : L"Black";
}


BMP* UIPCP::PbmpPieces(void)
{
	return uibd.pbmpPieces;
}