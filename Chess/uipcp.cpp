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
	cmdClearSquare = 5,
	cmdToggleToMove = 6,
	cmdSetEpdFen = 7,
	cmdOpenEpdFile = 8,
	/* castle states or-together these values, so these commands actually take up 16
	   total slots */
	cmdToggleCsBase = 9,
	cmdToggleCsWhiteKing = cmdToggleCsBase + csWhiteKing,
	cmdToggleCsWhiteQueen = cmdToggleCsBase + csWhiteQueen,
	cmdToggleCsBlackKing = cmdToggleCsBase + csBlackKing,
	cmdToggleCsBlackQueen = cmdToggleCsBase + csBlackQueen,
	cmdToggleCsMost = cmdToggleCsBase + (csWhiteKing|csWhiteQueen|csBlackKing|csBlackQueen),
	cmdNextUnused = 25
};

static_assert(cmdNextUnused == cmdToggleCsMost + 1);


CHKCS::CHKCS(UI* puiParent, UIPCP& uipcp, int cmd) : BTN(puiParent, cmd), uipcp(uipcp)
{
	SetCoText(coBlack);
	SetCoBack(coWhite);
}


void CHKCS::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	rc.Inflate(-2, -2);
	FillRc(rc, pbrText);
	rc.Inflate(-1, -1);
	FillRc(rc, pbrBack);

	if (uipcp.uiga.ga.bdg.csCur & (cmd - cmdToggleCsBase))
		DrawSzCenter(L"\x2713", ptxList, RcInterior(), pbrText);
}

void CHKCS::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


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
	FillRc(RcInterior(), coBlack);
	FillRc(RcInterior().Inflate(-2.5f, -2.5f), cmd == cmdSetBoardWhite ? coWhite : coBlack);
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
	SetDrag(this);
	ptDragInit = pt;
	ptDragCur = pt;
	Redraw();
	InvalOutsideRc(RcDrag(ptDragCur));
}


void UIDRAGPCP::EndLeftDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
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


void UIDRAGPCP::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
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
	float dxPiece = siz.width / 6;
	float dyPiece = siz.height / 2;
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
	rc.Inflate(-6, -6);
	float dxyLine = 7;
	ELL ell(rc.PtCenter(), rc.DxWidth()/2-dxyLine);
	pui->DrawEll(ell, coDragDelCircle, dxyLine);

	/* taking an opponent piece - draw an X */

	const float dxyCrossFull = 10.0f;
	const float dxyCrossCenter = 3.0f;
	const PT aptCross[] = {
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

	rc.Inflate(-2*dxyLine-1, -2*dxyLine-1);
	GEOM* pgeomCross = PgeomCreate(aptCross, CArray(aptCross));
	pui->FillRotateGeom(pgeomCross, rc.PtCenter(), rc.DxWidth() / (2*dxyCrossFull), 45, coDragDelCross);
	SafeRelease(&pgeomCross);
}


void UIDRAGDEL::Drop(SQ sq)
{
	uipcp.sqDrop = sq;
	uipcp.DispatchCmd(cmdClearSquare);
}


/*
 *
 *	UICPCTOMOVE
 * 
 *	Side to move picker
 * 
 */

UICPCTOMOVE::UICPCTOMOVE(UIPCP& uipcp) : BTN(&uipcp, cmdToggleToMove), uipcp(uipcp)
{
}

void UICPCTOMOVE::Draw(const RC& rcUpdate)
{
	CPC cpc = uipcp.uiga.ga.bdg.cpcToMove;
	RC rc = RcInterior();
	rc.top += 8.0f;
	rc.bottom = rc.top + 32.0f;
	rc.Inflate(-12.0f, 0);
	FillRc(rc, pbrBlack);
	rc.Inflate(-2.5f, -2.5f);
	FillRc(rc, cpc == cpcWhite ? pbrWhite : pbrBlack);
	rc.top += 4.0f;
	if (cpc == cpcWhite)
		DrawSzCenter(L"White", ptxTextBold, rc, pbrBlack);
	else
		DrawSzCenter(L"Black", ptxTextBold, rc, pbrWhite);
	rc.Offset(0, rc.DyHeight() + 8.0f);
	rc.Inflate(12.0f, 0);
	DrawSzCenter(L"To Move", ptxTextBold, rc, pbrText);
}


void UICPCTOMOVE::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


/*
 *
 */

UICASTLESTATE::UICASTLESTATE(UIPCP& uipcp) : UI(&uipcp), uipcp(uipcp)
{
	vpchkcs.push_back(new CHKCS(this, uipcp, cmdToggleCsWhiteKing));
	vpchkcs.push_back(new CHKCS(this, uipcp, cmdToggleCsWhiteQueen));
	vpchkcs.push_back(new CHKCS(this, uipcp, cmdToggleCsBlackKing));
	vpchkcs.push_back(new CHKCS(this, uipcp, cmdToggleCsBlackQueen));
	SetCoText(coBlack);
}

void UICASTLESTATE::Layout(void)
{
	RC rcInt = RcInterior();
	RC rc(0, 0, 17.0f, 17.0f);
	vpchkcs[0]->SetBounds(rc + PT(8.0f, 32.0f));
	vpchkcs[1]->SetBounds(rc + PT(8.0f, 50.0f));
	vpchkcs[2]->SetBounds(rc + PT(rcInt.right - rc.DxWidth() - 6.0f, 32.0f));
	vpchkcs[3]->SetBounds(rc + PT(rcInt.right - rc.DxWidth() - 6.0f, 50.0f));
}

void UICASTLESTATE::Draw(const RC& rcUpdate)
{
	RC rc = RcInterior();
	DrawSzCenter(L"Castle State", ptxListBold, rc);
	rc.top += 16.0f;
	RC rcWhite = rc, rcBlack = rc;
	rcWhite.right = rc.XCenter();
	rcBlack.left = rc.XCenter();
	DrawSz(L"White", ptxList, rcWhite);
	{
	TATX tatxSav(ptxList, DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawSz(L"Black", ptxList, rcBlack);
	}


	rc.top = rc.top + 18.0f;
	rc.bottom = rc.top + 16.0f;
	DrawSzCenter(L"O-O", ptxList, rc);
	rc.Offset(0, 18.0f);
	DrawSzCenter(L"O-O-O", ptxList, rc);
}

void UICASTLESTATE::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


/*
 *
 *	UISETFEN
 * 
 *	The editor for displaying/setting the FEN string of the board being edited.
 *
 */


UISETFEN::UISETFEN(UI* pui, int cmd, const wstring& szFen) : BTN(pui, cmd), szFen(szFen)
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
			if (((rank + file) & 1) == 0)
				FillRc(rc, pbrText);
			if (!bdg.FIsEmpty(sq)) {
				CPC cpc = bdg.CpcFromSq(sq);
				rc.Inflate(-1.5, -1.5);
				FillRc(rc, cpc == cpcWhite ? pbrWhite : pbrBlack);
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
	DrawSz(szTitle, ptxTextBold, rcTitle.Inflate(0, -3));
	rc.left += SizFromSz(szTitle, ptxTextBold).width + 4.0f;
	FillRc(rc, pbrBlack);
	FillRc(rc.Inflate(-1, -1), pbrWhite);
	rc.Inflate(-6, -3);
	DrawSz(uipcp.uiga.ga.bdg.SzFEN(), ptxTextBold, rc);
}


void UIFEN::Erase(const RC& rcUpdate, bool fParentErase)
{
	TransparentErase(rcUpdate, fParentErase);
}


/*
 *
 *	UIEPD control container
 *
 */


UIEPD::UIEPD(UIPCP& uipcp, const wstring& szFile) : UI(&uipcp), uipcp(uipcp),
	szFile(szFile),
	uisetfen(this, cmdSetEpdFen, L"r1bq1r1k/p1pnbpp1/1p2p3/6p1/3PB3/5N2/PPPQ1PPP/2KR3R w - -"), 
	btnFile(this, cmdOpenEpdFile, szFile)
{
}


void UIEPD::Erase(const RC& rcUpdate, bool fParentDrawn)
{
	TransparentErase(rcUpdate, fParentDrawn);
}


void UIEPD::Draw(const RC& rcUpdate)
{
}


void UIEPD::Layout(void)
{
	RC rcInt = RcInterior();

	RC rc = rcInt;
	rc.bottom -= 8.0f;
	rc.top = rc.bottom - 20.0f;
	btnFile.SetBounds(rc);
	
	rc = RC(0, 0, rc.top, rc.top);
	rc.Offset(rcInt.XCenter()-rc.XCenter(), 0);
	rc.Inflate(-15, -15);
	uisetfen.SetBounds(rc);
}


/*
 *
 *	UIPCP
 *
 *	The top-level piece panel
 *
 */


UIPCP::UIPCP(UIGA& uiga) : UIP(uiga), uibd(uiga.uibd), 
			uititle(this, L"Setup Board: Move Pieces and Drag onto Board. Press 'Done' When Finished."), 
			uidragdel(*this), uicpctomove(*this), uicastlestate(*this), uifen(*this), uiepd(*this, L"arsan21.epd"),
			cpcShow(cpcWhite), sqDrop(sqNil), apcDrop(apcNull)
{
	for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc)
		mpcpcpuicpc[cpc] = new UICPC(this, (int)cmdSetBoardWhite+cpc);
	for (APC apc = apcPawn; apc < apcMax; ++apc)
		mpapcpuiapc[apc] = new UIDRAGAPC(*this, apc);
	vpuisetfen.push_back(new UISETFEN(this, cmdSetBoardInit, BDG::szFENInit));
	vpuisetfen.push_back(new UISETFEN(this, cmdSetBoardEmpty, L"8/8/8/8/8/8/8/8 w - - 0 1"));
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
	for (; !vpuisetfen.empty(); vpuisetfen.pop_back())
		delete vpuisetfen.back();
}


void UIPCP::Layout(void)
{
	RC rcUnused = RcInterior();

	RC rc = rcUnused;
	rc.bottom = rc.top + 24.0f;
	uititle.SetBounds(rc);
	rcUnused.top = rc.bottom + 16.0f;

	/* get the area where the draggable icons go */

	RC rcDrags = rcUnused;
	rcDrags.bottom = rcDrags.top + 72.0f;

	/* position the black/white choose buttons */

	rc = rcDrags;
	rc.left += 24.0f;
	rc.top = rc.YCenter() + 1.0f;
	rc.right = rc.left + rc.DyHeight();
	mpcpcpuicpc[cpcBlack]->SetBounds(rc);
	rc.Offset(0, -rc.DyHeight()-2.0f);
	mpcpcpuicpc[cpcWhite]->SetBounds(rc);

	/* position the piece drag sources and the delete drag item */

	rc = RC(rc.right, rcDrags.top, rc.right + rcDrags.DyHeight(), rcDrags.bottom);
	rc.Inflate(-2.0f, -2.0f);
	rc.Offset(12.0f, 0);
	for (APC apc = apcPawn; apc < apcMax; ++apc) {
		mpapcpuiapc[apc]->SetBounds(rc);
		rc.Offset(rc.DxWidth() + 8.0f, 0);
	}
	uidragdel.SetBounds(rc);

	/* side to move */

	rc.left = rc.right + 32.0f;
	rc.right = rc.left + 100.0f;
	uicpctomove.SetBounds(rc);

	/* castle state */

	rc.left = rc.right + 32.0f;
	rc.right += 140.0f;
	uicastlestate.SetBounds(rc);

	/* full board shortcuts */

	rc = RC(rc.right + 32.0f, rcDrags.top, rc.right+28.0f+rcDrags.DyHeight(), rcDrags.bottom);
	float xLast = rc.left;
	for (UISETFEN* puisetfen : vpuisetfen) {
		puisetfen->SetBounds(rc);
		xLast = rc.right;
		rc.Offset(rc.DxWidth()+12.0f, 0);
	}

	rcUnused.top = rcDrags.bottom + 12.0f;

	/* FEN string */

	rc = rcUnused;
	rc.bottom = rc.top + 30.0f;
	rc.right = xLast;
	rc.Inflate(-12.0f, 0);
	uifen.SetBounds(rc);

	/* EPD chooser */

	rc = RcInterior();
	rc.top += 24.0f;
	rc.left = xLast + 20.0f;
	rc.bottom -= 12.0f;
	rc.right -= 12.0f;
	uiepd.SetBounds(rc);
}


void UIPCP::DispatchCmd(int cmd)
{
	switch (cmd) {
	case cmdClosePanel:
		Show(false);
		uiga.InitGame();
		return;
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
				uiga.ga.bdg.InitGame();
				uiga.ga.bdg.SetFen(puisetfen->szFen.c_str());
				break;
			}
		break;
	case cmdSetEpdFen:
		uiga.ga.bdg.InitGame();
		uiga.ga.bdg.SetFen(uiepd.uisetfen.szFen.c_str());
		break;
	case cmdSetSquare:
		uibd.Ga().bdg.SetSq(sqDrop, PC(cpcShow, apcDrop));
		break;
	case cmdClearSquare:
		uibd.Ga().bdg.ClearSq(sqDrop);
		break;
	case cmdToggleToMove:
		uibd.Ga().bdg.ToggleToMove();
		break;
	case cmdToggleCsWhiteQueen:
	case cmdToggleCsWhiteKing:
	case cmdToggleCsBlackQueen:
	case cmdToggleCsBlackKing:
		uibd.Ga().bdg.ToggleCsCur(cmd - cmdToggleCsBase);
		break;
	default:
		break;
	}
	uiga.Redraw();
}


void UIPCP::HiliteSq(SQ sq)
{
	uibd.SetDragHiliteSq(sq);
	uibd.Redraw();
}


bool UIPCP::FEnabledCmd(int cmd) const
{
	return true;
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
	case cmdClosePanel:
		return L"Exit Board Edit Mode";
	case cmdToggleToMove:
		return L"Switch Player to Move";
	case cmdToggleCsBlackKing:
		return L"Black King-Side Castle";
	case cmdToggleCsBlackQueen:
		return L"Black Queen-Side Castle";
	case cmdToggleCsWhiteKing:
		return L"White King-Side Castle";
	case cmdToggleCsWhiteQueen:
		return L"White Queen-Side Castle";
	default:
		return L"";
	}
}


BMP* UIPCP::PbmpPieces(void)
{
	return uibd.pbmpPieces;
}