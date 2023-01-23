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


enum {
	cmdSetBoardWhite = 0,
	cmdSetBoardBlack = 1,
	cmdSetBoardInit = 2,
	cmdSetBoardEmpty = 3,
	cmdSetSquare = 4,
	cmdClearSquare = 5
};



/*
 *
 *	UICPC
 *
 *	The color picking button in the piece panel
 * 
 */


UICPC::UICPC(UI* puiParent, int cmd) : BTN(puiParent, cmd)
{
}


void UICPC::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	FillRc(rc, pbrText);
	rc.Inflate(-3.0f, -3.0f);
	FillRc(rc, cmd == cmdSetBoardWhite ? pbrBack : pbrText);
}


/*
 *
 *	UIDRAGPCP
 *
 *	Base class for the draggable controls in the piece placement palette
 *
 */


UIDRAGPCP::UIDRAGPCP(UIPCP& uipcp) : UI(&uipcp), uipcp(uipcp)
{
}


void UIDRAGPCP::StartLeftDrag(const PT& pt)
{
	SetCapt(this);
	ptDragInit = pt;
	ptDragCur = pt;
	Redraw();
	InvalOutsideRc(RcDrag(ptDragCur));
}


void UIDRAGPCP::EndLeftDrag(const PT& pt)
{
	ReleaseCapt();
	InvalOutsideRc(RcDrag(ptDragCur));
	ptDragCur = pt;
	uipcp.HiliteSq(sqNil);
	Redraw();

	/* hit test the mouse location */
	SQ sq = SqHitTest(pt);
	if (sq.fIsNil())
		return;
	Drop(sq);
}


void UIDRAGPCP::LeftDrag(const PT& pt)
{
	SQ sq = SqHitTest(pt);
	InvalOutsideRc(RcDrag(ptDragCur));
	ptDragCur = pt;
	InvalOutsideRc(RcDrag(ptDragCur));
	Redraw();
	uipcp.HiliteSq(sq);
}


void UIDRAGPCP::MouseHover(const PT& pt, MHT mht)
{
}


void UIDRAGPCP::Draw(const RC& rcUpdate)
{
	DrawInterior(this, RcInterior());
}


/*	UIDRAGPCP::DrawCursor
 *
 *	Draws the dragged sprite/cursor along with the mouse. Draws on the puiDraw element, which
 *	should be the top-level UI. 
 * 
 *	This function just delegates to DrawInterior.
 */
void UIDRAGPCP::DrawCursor(UI* puiDraw, const RC& rcUpdate)
{
	DrawInterior(puiDraw, puiDraw->RcLocalFromUiLocal(this, RcDrag(ptDragCur)));
}


/*	UIDRAGPCP:RcDrag
 *
 *	Given a mouse point in local coordinates, returns the rectangle of the dragged cursor
 *	in local coordinates.
 */
RC UIDRAGPCP::RcDrag(const PT& pt) const
{
	return RcInterior().Offset(pt.x - ptDragInit.x, pt.y - ptDragInit.y);
}


SQ UIDRAGPCP::SqHitTest(PT pt) const
{
	/* OK, this is a little funky, because we're allowing hit testing on one panel while we've
	   got capture on a different one. We only allow a limited number of places where we can
	   drag to, so this isn't that complicated, but it's easy to get confused with coordinate
	   sywstems. Be careful! */
	
	SQ sq;
	HTBD htbd = uipcp.uibd.HtbdHitTest(uipcp.uibd.PtLocalFromUiLocal(this, pt), &sq);
	switch (htbd) {
	case htbdMoveablePc:
	case htbdOpponentPc:
	case htbdUnmoveablePc:
	case htbdEmpty:
		return sq;
	default:
		break;
	}
	return sqNil;
}



/*
 *
 *	UIDRAGAPC
 *
 *	The piece drag elements in the piece panel
 *
 */


UIDRAGAPC::UIDRAGAPC(UIPCP& uipcp, APC apc) : UIDRAGPCP(uipcp), apc(apc)
{
}


/*	UIDRAGAPC::DrawInterior
 *
 *	Draws the interior part of the dragging elements in the PCP palette. 
 */
void UIDRAGAPC::DrawInterior(UI* pui, const RC& rcDraw)
{
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	BMP* pbmpPieces = uipcp.PbmpPieces();
	SIZ siz = pbmpPieces->GetSize();
	float dxPiece = siz.width / 6.0f;
	float dyPiece = siz.height / 2.0f;
	float xPiece = mpapcxBitmap[apc] * dxPiece;
	float yPiece = (int)uipcp.cpcShow * dyPiece;
	pui->DrawBmp(rcDraw, pbmpPieces, RC(xPiece, yPiece, xPiece + dxPiece, yPiece + dyPiece), 1.0f);
}


void UIDRAGAPC::Drop(SQ sq)
{
	uipcp.apcDrop = apc;
	uipcp.sqDrop = sq;
	uipcp.DispatchCmd(cmdSetSquare);
}


/*
 *
 *	UIDRAGDEL
 * 
 *	The delete piece control
 * 
 */


UIDRAGDEL::UIDRAGDEL(UIPCP& uipcp) : UIDRAGPCP(uipcp)
{
}


void UIDRAGDEL::DrawInterior(UI* pui, const RC& rcDraw)
{
	RC rc = rcDraw;
	rc.Inflate(-6.0f, -6.0f);
	float dxyLine = 7.0f;
	ELL ell(rc.PtCenter(), PT(rc.DxWidth()/2.0f-dxyLine, rc.DyHeight()/2.0f-dxyLine));
	COLORBRS colorbrsSav(pbrText, ColorF(0.65f, 0.15f, 0.25f));
	pui->DrawEll(ell, pbrText, dxyLine);

	/* taking an opponent piece - draw an X */

	/* TODO: can we share this with the cross in UIBD? */

	const float dxyCrossFull = 10.0f;
	const float dxyCrossCenter = 3.0f;
	const PT rgptCross[] = {
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
	PT ptCenter = PtGlobalFromUiLocal(pui, rc.PtCenter());
	TRANSDC transdc(pdc,
			Matrix3x2F::Rotation(45.0f, PT(0, 0)) * 
			Matrix3x2F::Scale(SizeF(rc.DxWidth() / (2*dxyCrossFull), rc.DyHeight()/ (2*dxyCrossFull)), PT(0, 0)) * 
			Matrix3x2F::Translation(ptCenter.x, ptCenter.y));

	pbrText->SetColor(ColorF(0, 0, 0));
	GEOM* pgeomCross = PgeomCreate(rgptCross, CArray(rgptCross));
	pdc->FillGeometry(pgeomCross, pbrText);
	SafeRelease(&pgeomCross);
}


void UIDRAGDEL::Drop(SQ sq)
{
	uipcp.sqDrop = sq;
	uipcp.DispatchCmd(cmdClearSquare);
}


/*
 *
 *
 *
 */


UISETFEN::UISETFEN(UIPCP& uipcp, int cmd, const wstring& szFen) : BTN(&uipcp, cmd), szFen(szFen)
{
}


RC UISETFEN::RcFromSq(SQ sq) const
{
	RC rc = RcInterior();
	float dx = rc.DxWidth() / 8;
	float dy = rc.DyHeight() / 8;
	rc.left += sq.file() * dx;
	rc.right = rc.left + dx;
	rc.top += (rankMax - sq.rank() - 1) * dy;
	rc.bottom = rc.top + dy;
	return rc;
}


void UISETFEN::Draw(const RC& rcUpdate)
{
	/* create a little dummy board with the FEN string */
	BDG bdg(szFen.c_str());

	for (int rank = 0; rank < rankMax; rank++) {
		for (int file = 0; file < fileMax; file++) {
			SQ sq(rank, file);
			RC rc(RcFromSq(sq));
			COLORBRS colorbrsBack(pbrBack, (rank + file) & 1 ? coBoardLight : coBoardDark);
			FillRc(rc, pbrBack);
			if (!bdg.FIsEmpty(sq)) {
				CPC cpc = bdg.CpcFromSq(sq);
				rc.Inflate(-2, -2);
				COLORBRS colorbrsFore(pbrText, cpc == cpcWhite ? ColorF(ColorF::White) : ColorF(ColorF::Black));
				FillRc(rc, pbrText);
			}
		}
	}
}


UIFEN::UIFEN(UIPCP& uipcp) : UI(&uipcp), uipcp(uipcp)
{
}


void UIFEN::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior().Inflate(-2, -2);
	const wchar_t szTitle[] = L"FEN:";
	RC rcTitle = rc;
	DrawSz(szTitle, ptxText, rcTitle.Inflate(0, -2));
	rc.left += SizSz(szTitle, ptxText).width + 4.0f;
	FillRc(rc, pbrText);
	FillRc(rc.Inflate(-1, -1), pbrBack);

	rc.Inflate(-6, -1);
	DrawSz(uipcp.uiga.ga.bdg.SzFEN(), ptxText, rc);
}


/*
 *
 *	UIPCP
 *
 *	The top-level piece panel
 *
 */


UIPCP::UIPCP(UIGA& uiga) : UIP(uiga), uibd(uiga.uibd), uidragdel(*this), uifen(*this), cpcShow(cpcWhite), sqDrop(sqNil), apcDrop(apcNull)
{
	for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc)
		mpcpcpuicpc[cpc] = new UICPC(this, (int)cmdSetBoardWhite+cpc);
	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc] = new UIDRAGAPC(*this, apc);
	vpuisetfen.push_back(new UISETFEN(*this, cmdSetBoardInit, BDG::szFENInit));
	vpuisetfen.push_back(new UISETFEN(*this, cmdSetBoardEmpty, L"8/8/8/8/8/8/8/8 w - - 0 1"));
}


UIPCP::~UIPCP(void)
{
	for (map<CPC,UICPC*>::iterator it = mpcpcpuicpc.begin(); it != mpcpcpuicpc.end(); it++) {
		delete it->second;
		mpcpcpuicpc[it->first] = nullptr;
	}
	for (map<APC,UIDRAGAPC*>::iterator it = mpapcpuiapc.begin(); it != mpapcpuiapc.end(); it++) {
		delete it->second;
		mpapcpuiapc[it->first] = nullptr;
	}
	while (vpuisetfen.size() > 0) {
		UISETFEN* puisetfen = vpuisetfen.back();
		vpuisetfen.pop_back();
		delete puisetfen;
	}
}


RC UIPCP::RcFromApc(APC apc) const
{
	RC rc = RcInterior();
	rc.Inflate(-4.0f, -4.0f);
	rc.bottom -= 32.0f;
	rc.left += rc.DyHeight() / 2.0f + 20.0f;
	rc.right = rc.left + rc.DyHeight();
	rc.Offset((rc.DxWidth() + 4.0f) * (apc - apcPawn), 0);
	return rc;
}


void UIPCP::Layout(void)
{
	/* position the color buttons */

	RC rc = RcInterior();
	rc.Inflate(-12.0f, -12.0f);
	rc.left += 8.0f;
	rc.bottom -= 32.0f;
	rc.top = rc.YCenter() + 1.0f;
	rc.right = rc.left + rc.DyHeight();
	mpcpcpuicpc[cpcBlack]->SetBounds(rc);
	rc.Offset(0, -rc.DyHeight()-2.0f);
	mpcpcpuicpc[cpcWhite]->SetBounds(rc);

	/* position the piece drag sources */

	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc]->SetBounds(RcFromApc(apc));

	/* delete piece */

	rc = RcFromApc(apcKing);
	rc.Offset(rc.DxWidth() + 8.0f, 0);
	uidragdel.SetBounds(rc);

	/* castle and en passant state */
	/* player to move */

	/* full board shortcuts */

	rc = RcInterior();
	rc.Inflate(-12.0, -4.0f);
	rc.bottom -= 32.0f;
	rc.left = rc.right - rc.DyHeight();
	for (UISETFEN* puisetfen : vpuisetfen) {
		puisetfen->SetBounds(rc);
		rc.Offset(-rc.DxWidth()-8.0f, 0);
	}

	/* FEN string */

	rc = RcInterior().Inflate(-8.0, -4.0);
	rc.top = rc.bottom - 26.0f;
	uifen.SetBounds(rc);
}


void UIPCP::Draw(const RC& rcUpdate)
{
	COLORBRS colorbrs(pbrBack, coBoardLight);
	FillRc(rcUpdate, pbrBack);
}


void UIPCP::DispatchCmd(int cmd)
{
	switch (cmd) {
	case cmdSetBoardBlack:
		cpcShow = cpcBlack;
		break;
	case cmdSetBoardWhite:
		cpcShow = cpcWhite;
		break;
	case cmdSetBoardEmpty:
	case cmdSetBoardInit:
		for (UISETFEN* puisetfen : vpuisetfen)
			if (puisetfen->cmd == cmd) {
				uiga.ga.bdg.InitGame(puisetfen->szFen.c_str());
				uiga.Redraw();
				break;
			}
		break;
	case cmdSetSquare:
		break;
	case cmdClearSquare:
		break;
	default:
		break;
	}
	Redraw();
}


void UIPCP::HiliteSq(SQ sq)
{
	uibd.SetDragHiliteSq(sq);
	uibd.Redraw();
}


wstring UIPCP::SzTipFromCmd(int cmd) const
{
	switch (cmd) {
	case cmdSetBoardBlack:
		return L"Place Black Pieces";
	case cmdSetBoardWhite:
		return L"Place White Pieces";
	case cmdSetBoardEmpty:
		return L"Make Empty Board";
	case cmdSetBoardInit:
		return L"Starting Position";
	default:
		return L"";
	}
}


BMP* UIPCP::PbmpPieces(void)
{
	return uibd.pbmpPieces;
}