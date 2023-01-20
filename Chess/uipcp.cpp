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
	cmdSetBoardEmpty = 3
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
	rc.Inflate(-1.0f, -1.0f);
	FillRc(rc, cmd == cmdSetBoardWhite ? pbrBack : pbrText);
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
	COLORBRS colorbrsSav(pbrText, ColorF(0.65f, 0.15f, 0.25f));
	DrawEll(ell, pbrText, dxyLine);

	/* taking an opponent piece - draw an X */

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
	TRANSDC transdc(pdc,
			Matrix3x2F::Rotation(45.0f, PT(0, 0)) * 
			Matrix3x2F::Scale(SizeF(rc.DxWidth() / (2*dxyCrossFull), rc.DyHeight()/ (2*dxyCrossFull)), PT(0, 0)) * 
			Matrix3x2F::Translation(SizeF(rcBounds.left + rc.XCenter(), rcBounds.top + rc.YCenter())));

	pbrText->SetColor(ColorF(0, 0, 0));
	GEOM* pgeomCross = PgeomCreate(rgptCross, CArray(rgptCross));
	pdc->FillGeometry(pgeomCross, pbrText);
	SafeRelease(&pgeomCross);

}


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

UIPCP::UIPCP(UIGA& uiga) : UIP(uiga), uibd(uiga.uibd), uipcdel(*this), uifen(*this), cpcShow(cpcWhite)
{
	for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc)
		mpcpcpuicpc[cpc] = new UICPC(this, (int)cmdSetBoardWhite+cpc);
	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc] = new UIAPC(*this, apc);
	vpuisetfen.push_back(new UISETFEN(*this, cmdSetBoardInit, BDG::szFENInit));
	vpuisetfen.push_back(new UISETFEN(*this, cmdSetBoardEmpty, L"8/8/8/8/8/8/8/8 w - - 0 1"));
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
	uipcdel.SetBounds(rc);

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
	default:
		break;
	}
	Redraw();
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