/*
 *
 *	uibd.cpp
 * 
 *	User interface for the board panel
 * 
 */

#include "uibd.h"
#include "ga.h"
#include "Resources/Resource.h"


/*
 *
 *	UIBD class
 *
 *	Screen panel that displays the board and handles the mouse and keyaboard
 *	interface for it.
 *
 */


BRS* UIBD::pbrAnnotation;
BRS* UIBD::pbrHilite;

const float dxyCrossFull = 20.0f;
const float dxyCrossCenter = 4.0f;
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

PT rgptArrowHead[] = {
	{0, 0},
	{dxyCrossFull * 0.5f, dxyCrossFull * 0.86f},
	{-dxyCrossFull * 0.5f, dxyCrossFull * 0.86f},
	{0, 0}
};


/*	UIBD::CreateRsrcClass
 *
 *	Creates the drawing resources necessary to draw the board.
 */
void UIBD::CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
	if (pbrHilite)
		return;

	/* brushes */

	pdc->CreateSolidColorBrush(ColorF(ColorF::Red), &pbrHilite);
	pdc->CreateSolidColorBrush(ColorF(1.f, 0.15f, 0.0f), &pbrAnnotation);
}


/*	UIBD::DiscardRsrcClass
 *
 *	Cleans up the resources created by CreateRsrcClass
 */
void UIBD::DiscardRsrcClass(void)
{
	SafeRelease(&pbrHilite);
	SafeRelease(&pbrAnnotation);
}

void UIBD::CreateRsrc(void)
{
	if (ptxLabel)
		return;

	App().pfactdwr->CreateTextFormat(szFontFamily, nullptr,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		dxySquare/4.0f, L"",
		&ptxLabel);

	/* bitmaps */

	pbmpPieces = PbmpFromPngRes(idbPieces);

	/* geometries */

	/* capture X, which is created as a cross that is rotated later */
	pgeomCross = PgeomCreate(rgptCross, CArray(rgptCross));
	/* arrow head */
	pgeomArrowHead = PgeomCreate(rgptArrowHead, CArray(rgptArrowHead));
}

void UIBD::DiscardRsrc(void)
{
	SafeRelease(&ptxLabel);
	SafeRelease(&pbmpPieces);
	SafeRelease(&pgeomCross);
	SafeRelease(&pgeomArrowHead);
}


/*	UIBD::UIBD
 *
 *	Constructor for the board screen panel.
 */
UIBD::UIBD(UIGA& uiga) : UIP(uiga), 
		pbmpPieces(nullptr), pgeomCross(nullptr), pgeomArrowHead(nullptr), ptxLabel(nullptr),
		btnRotateBoard(this, cmdRotateBoard, L'\x2b6f'),
		cpcPointOfView(cpcWhite), 
		rcSquares(0, 0, 640.0f, 640.0f), dxySquare(80.0f), dxyBorder(2.0f), dxyMargin(50.0f), dxyOutline(4.0f), dyLabel(0), angle(0.0f),
		sqDragInit(sqNil), sqHover(sqNil), sqDragHilite(sqNil)
{
}


/*	UIBD::~UIBD
 *
 *	Destructor for the board screen panel
 */
UIBD::~UIBD(void)
{
}


/*	UIBD::Layout
 *
 *	Layout work for the board screen panel.
 */
void UIBD::Layout(void)
{
	DiscardRsrc();

	rcSquares = RcInterior();

	dxySquare = roundf(rcSquares.DxWidth() / 9.5f);

	dxyBorder = dxySquare / 32.0f;
	if (dxyBorder < 1.0f) {
		/* if board is too small don't display the margin of the board */
		dxyBorder = dxyOutline = dxyMargin = 0;
		dxySquare = rcSquares.DxWidth() / 8.0f;
	}
	else {
		dxyOutline = roundf(2.0f * dxyBorder);
		dxyBorder = roundf(dxyBorder);
		dxyMargin = (RcInterior().DxWidth() - 2 * (dxyBorder + dxyOutline) - 8 * dxySquare) / 2.0f;
		float dxy = dxyMargin + dxyOutline + dxyBorder;
		rcSquares.Inflate(-dxy, -dxy);
	}
	
	CreateRsrc();
	dyLabel = SizFromSz(L"8", ptxLabel).height;

	/* position the rotation button */

	RC rc = RcInterior().Inflate(-dxyBorder, -dxyBorder);
	float dxyBtn = dxySquare * 0.375f;
	rc.top = rc.bottom - dxyBtn;
	rc.right = rc.left + dxyBtn;
	btnRotateBoard.SetBounds(rc);
}


/*	UIBD::InitGame
 *
 *	Starts a new game on the screen board
 */
void UIBD::InitGame(void)
{
	uiga.ga.bdg.GenVmve(vmveDrag, ggLegal);
}


void UIBD::StartGame(void)
{
	uiga.ga.bdg.GenVmve(vmveDrag, ggLegal);
}


/*	UIBD::MakeMvu
 *
 *	Makes a move on the board and echoes it on the screen.
 * 
 *	Throws an exception if the move is not valid.
 */
void UIBD::MakeMvu(MVU mvu, SPMV spmv)
{
	for (MVE mveDrag : vmveDrag) {
		if (mveDrag.sqFrom() == mvu.sqFrom() && mveDrag.sqTo() == mvu.sqTo())
			goto FoundMove;
	}
	throw 1;

FoundMove:
	if (FSpmvAnimate(spmv))
		AnimateMvu(mvu, DframeFromSpmv(spmv));
	uiga.ga.bdg.MakeMvu(mvu);
	uiga.ga.bdg.GenVmve(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vmveDrag, *uiga.ga.prule);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::UndoMvu(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imvuCurLast >= 0) {
		MVU mvu = uiga.ga.bdg.vmvuGame[uiga.ga.bdg.imvuCurLast];
		AnimateSqToSq(mvu.sqTo(), mvu.sqFrom(), DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.UndoMvu();
	uiga.ga.bdg.GenVmve(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGs(gsPlaying);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::RedoMvu(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imvuCurLast < (int)uiga.ga.bdg.vmvuGame.size()) {
		MVU mvu = uiga.ga.bdg.vmvuGame[uiga.ga.bdg.imvuCurLast+1];
		AnimateMvu(mvu, DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.RedoMvu();
	uiga.ga.bdg.GenVmve(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vmveDrag, *uiga.ga.prule);
	if (spmv != spmvHidden)
		Redraw();
}


/*	UIBD::Draw
 *
 *	Draws the board panel on the screen. This particular implementation
 *	does a green and cream chess board with square labels written in the
 *	board margin.
 *
 *	Note that mouse tracking feedback is drawn by our generic Draw. It
 *	turns out Direct2D drawing is fast enough to redraw the entire board
 *	in response to user input, which simplifies drawing quite a bit. We
 *	just handle all dragging drawing in here.
 */
void UIBD::Draw(const RC& rcDraw)
{
	DC* pdc = App().pdc;
	int rankFirst = (int)floor((rcDraw.top - rcSquares.top) / dxySquare);
	float dy = (rcDraw.bottom - rcSquares.top) / dxySquare;
	int rankLast = (int)floor(dy) - (int)(floor(dy) == dy);
	int fileFirst = (int)floor((rcDraw.left - rcSquares.left) / dxySquare);
	float dx = (rcDraw.right - rcSquares.left) / dxySquare;
	int fileLast = (int)floor(dx) - (int)(floor(dx) == dx);
	rankFirst = clamp(rankFirst, 0, rankMax - 1);
	rankLast = clamp(rankLast, 0, rankMax - 1);
	fileFirst = clamp(fileFirst, 0, fileMax - 1);
	fileLast = clamp(fileLast, 0, fileMax - 1);
	if (cpcPointOfView == cpcWhite) {
		rankFirst = rankMax - rankFirst - 1;
		rankLast = rankMax - rankLast - 1;
		swap(rankFirst, rankLast);
	}
	else {
		fileFirst = fileMax - fileFirst - 1;
		fileLast = fileMax - fileLast - 1;
		swap(fileFirst, fileLast);
	}

	{
	TRANSDC transdc(pdc, Matrix3x2F::Rotation(angle, rcBounds.PtCenter()));
	DrawMargins(rcDraw, rankFirst, rankLast, fileFirst, fileLast);
	DrawSquares(rankFirst, rankLast, fileFirst, fileLast);
	DrawHilites();
	DrawAnnotations();
	DrawGameState();
	}
}

ColorF UIBD::CoFore(void) const
{
	return uiga.FInBoardSetup() ? ColorF(0.4f, 0.4f, 0.4f) : coBoardDark; 
}

ColorF UIBD::CoBack(void) const
{
	return uiga.FInBoardSetup() ? ColorF(0.9f, 0.9f, 0.9f) : coBoardLight; 
}

/*	UIBD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid. Also draws
 *	the file and rank labels along the edge of the board.
 */
void UIBD::DrawMargins(const RC& rcUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast)
{
	FillRcBack(rcUpdate);
	if (dxyMargin <= 0.0f)
		return;
	RC rc = RcInterior();
	rc.Inflate(PT(-dxyMargin, -dxyMargin));
	FillRc(rc & rcUpdate, pbrText);
	rc.Inflate(PT(-dxyOutline, -dxyOutline));
	FillRcBack(rc & rcUpdate);
	DrawFileLabels(rankFirst, rankLast);
	DrawRankLabels(fileFirst, fileLast);
}


/*	UIBD::DrawSquares
 *
 *	Draws the squares of the chessboard. Assumes the light color
 *	has already been used to fill the board area, so all we need to
 *	do is draw the dark colors on top.
 * 
 *	Once the background is laid down, we draw the piece and any
 *	move hints that need to be done on the square.
 */
void UIBD::DrawSquares(int rankFirst, int rankLast, int fileFirst, int fileLast)
{
	for (int rank = rankFirst; rank <= rankLast; rank++)
		for (int file = fileFirst; file <= fileLast; file++) {
			SQ sq(rank, file);
			if ((rank + file) % 2 == 0)
				FillRc(RcFromSq(sq), pbrText);
			MVU mvu;
			if (FHoverSq(sq, mvu))
				DrawHoverMvu(mvu);
			DrawPieceSq(sq);
		}
}


/*	UIBD::DrawFileLabels
 *
 *	Draws the file letter labels along the bottom of the board.
 */
void UIBD::DrawFileLabels(int fileFirst, int fileLast)
{
	assert(fileLast >= fileFirst);
	wchar_t szLabel[2];
	szLabel[0] = L'a' + fileFirst;
	szLabel[1] = 0;
	float yTop = rcSquares.bottom + dxyBorder+dxyOutline + dxyBorder;
	float yBot = yTop + dyLabel;
	for (int file = 0; file <= fileLast; file++) {
		RC rc(RcFromSq(SQ(0, file)));
		DrawSzCenter(wstring(szLabel), ptxLabel, RC(rc.left, yTop, rc.right, yBot));
		szLabel[0]++;
	}
}


/*	UIBD::DrawRankLabels
 *
 *	Draws the numerical rank labels along the side of the board
 */
void UIBD::DrawRankLabels(int rankFirst, int rankLast)
{
	assert(rankLast >= rankFirst);
	wchar_t szLabel[2];
	szLabel[0] = L'1' + rankFirst;
	szLabel[1] = 0;
	float dxLabel = SizFromSz(L"8", ptxLabel).width;
	float dxRight = rcSquares.left - (dxyBorder + dxyOutline + dxyBorder) - dxLabel/2.0f;
	float dxLeft = dxRight - dxLabel;
	for (int rank = rankFirst; rank <= rankLast; rank++) {
		RC rc = RcFromSq(SQ(rank, 0));
		DrawSzCenter(wstring(szLabel), ptxLabel, RC(dxLeft, (rc.top + rc.bottom - dyLabel) / 2, dxRight, rc.bottom));
		szLabel[0]++;
	}
}


/*	UIBD::DrawGameState
 *
 *	When the game is over, this displays the game state on the board
 */
void UIBD::DrawGameState(void)
{
	if (uiga.ga.bdg.gs == gsPlaying)
		return;
	switch (uiga.ga.bdg.gs) {
		/* TODO: show checkmate */
	case gsWhiteCheckMated: break;
	case gsBlackCheckMated: break;
	case gsWhiteTimedOut: break;
	case gsBlackTimedOut: break;
	case gsStaleMate: break;
	default: break;
	}
}


/*	UIBD::RcFromSq
 *
 *	Returns the rectangle of the given square on the screen
 */
RC UIBD::RcFromSq(SQ sq) const
{
	int rank = sq.rank(), file = sq.file();
	if (cpcPointOfView == cpcWhite)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	RC rc(0, 0, dxySquare, dxySquare);
	return rc.Offset(rcSquares.left + dxySquare * file, rcSquares.top + dxySquare * rank);
}


void UIBD::SetDragHiliteSq(SQ sq)
{
	sqDragHilite = sq;
}


/*	UIBD::FHoverSq
 *
 *	Returns true if the square is the destination of move that originates in the
 *	tracking square sqHover. Returns the move itself in mv. 
 */
bool UIBD::FHoverSq(SQ sq, MVU& mvu)
{
	if (sqHover.fIsNil() || uiga.ga.bdg.gs != gsPlaying)
		return false;
	for (MVE mveDrag : vmveDrag) {
		if (mveDrag.sqFrom() == sqHover && mveDrag.sqTo() == sq) {
			mvu = (MVU)mveDrag;
			return true;
		}
	}
	return false;
}


/*	UIBD::DrawHoverMvu
 *
 *	Draws the move hints over destination squares that we display while the 
 *	user is hovering the mouse over a moveable piece.
 *
 *	We draw a circle over every square you can move to, and an X over captures.
 */
void UIBD::DrawHoverMvu(MVU mvu)
{
	OPACITYBR opacityBrSav(pbrBlack, 0.33f);

	RC rc = RcFromSq(mvu.sqTo());
	if (!uiga.ga.bdg.FMvuIsCapture(mvu)) {
		/* moving to an empty square - draw a circle */
		ELL ell(rc.PtCenter(), PT(dxySquare / 5, dxySquare / 5));
		FillEll(ell, pbrBlack);
	}
	else {
		/* taking an opponent piece - draw an X */
		FillRotateGeom(pgeomCross, rc.PtCenter(), dxySquare/(2*dxyCrossFull), 45, pbrBlack);
	}
}


/*	UIBD::DrawPieceSq
 *
 *	Draws pieces on the board. 
 */
void UIBD::DrawPieceSq(SQ sq)
{
	if (sq.fIsNil())
		return;
	float opacity = sqDragInit == sq ? 0.2f : 1.0f;
	DrawPc(this, RcFromSq(sq), opacity, uiga.ga.bdg.PcFromSq(sq));
}


/*	UIBD::RcGetDrag
 *
 *	Gets the current screen rectangle the piece we're dragging
 */
RC UIBD::RcGetDrag(void)
{
	RC rcInit = RcFromSq(sqDragInit);
	RC rc(0, 0, dxySquare, dxySquare);
	float dxInit = ptDragInit.x - rcInit.left;
	float dyInit = ptDragInit.y - rcInit.top;
	return rc.Offset(ptDragCur.x - dxInit, ptDragCur.y - dyInit);
}


/*	UIBD::DrawPc
 *
 *	Draws the chess piece on the UI element pui at rc.
 */
void UIBD::DrawPc(UI* pui, const RC& rcPc, float opacity, PC pc)
{
	if (pc.apc()  == apcNull)
		return;

	/* the piece png has the 12 different chess pieces oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0 };
	SIZ siz = pbmpPieces->GetSize();
	float dxPiece = siz.width / 6.0f;
	float dyPiece = siz.height / 2.0f;
	float xPiece = mpapcxBitmap[pc.apc()] * dxPiece;
	float yPiece = (int)pc.cpc() * dyPiece;
	pui->DrawBmp(rcPc, pbmpPieces, RC(xPiece, yPiece, xPiece + dxPiece, yPiece + dyPiece), opacity);
}


void UIBD::AnimateMvu(MVU mvu, unsigned dframe)
{
	AnimateSqToSq(mvu.sqFrom(), mvu.sqTo(), dframe);
}


void UIBD::AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned framefMax)
{
	sqDragInit = sqFrom;
	RC rcFrom = RcFromSq(sqDragInit);
	RC rcTo = RcFromSq(sqTo);
	ptDragInit = rcFrom.PtTopLeft();
	RC rcFrame = rcFrom;
	for (unsigned framef = 0; framef < framefMax; framef++) {
		rcDragPc = rcFrom;
		unsigned framefRem = framefMax - framef;
		rcDragPc.Move(PT((rcFrom.left*(float)framefRem + rcTo.left*(float)framef) / (float)framefMax,
						 (rcFrom.top*(float)framefRem + rcTo.top*(float)framef) / (float)framefMax));
		Redraw(rcFrame|rcDragPc);
		rcFrame = rcDragPc;
	}
	sqDragInit = sqNil;
}


void UIBD::DrawHilites(void)
{
	if (sqDragHilite.fIsNil())
		return;

	RC rc = RcFromSq(sqDragHilite);
	OPACITYBR opacitybr(pbrAnnotation, 0.125f);
	FillRc(rc, pbrAnnotation);
}


void UIBD::DrawAnnotations(void)
{
	OPACITYBR oopacitybr(pbrAnnotation, 0.5f);
	for (ANO& ano : vano) {
		if (ano.sqTo.fIsNil())
			DrawSquareAnnotation(ano.sqFrom);
		else
			DrawArrowAnnotation(ano.sqFrom, ano.sqTo);
	}
}


void UIBD::DrawSquareAnnotation(SQ sq)
{
}


void UIBD::DrawArrowAnnotation(SQ sqFrom, SQ sqTo)
{
}


void UIBD::FlipBoard(CPC cpcNew)
{
	/* assumes all children are visible */
	for (UI* pui : vpuiChild)
		pui->Show(false);

	for (angle = 0.0f; angle > -180.0f; angle -= 4.0f)
		uiga.Redraw();
	angle = 0.0f;
	cpcPointOfView = cpcNew;
	uiga.Layout();
	uiga.Redraw();

	for (UI* pui : vpuiChild)
		pui->Show(true);
}


/*	UIBD::HtbdHitTest
 *
 *	Hit tests the given mouse coordinate and returns a HT implementation
 *	if it belongs to this panel. Returns None if there is no hit.
 *
 *	If we return non-null, we are guaranteed to return a HTBD hit test
 *	structure from here, which means other mouse tracking notifications
 *	can safely cast to a HTBD to get access to board hit testing info.
 *
 *	The point is in local coordinates.
 */
HTBD UIBD::HtbdHitTest(const PT& pt, SQ* psq) const
{
	if (!RcInterior().FContainsPt(pt))
		return htbdNone;
	if (!rcSquares.FContainsPt(pt))
		return htbdStatic;
	int rank = (int)((pt.y - rcSquares.top) / dxySquare);
	int file = (int)((pt.x - rcSquares.left) / dxySquare);
	if (cpcPointOfView == cpcWhite)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	*psq = SQ(rank, file);
	if (uiga.ga.bdg.FIsEmpty(*psq))
		return htbdEmpty;
	
	/* in setup mode, all pieces are moveable */
	
	if (uiga.FInBoardSetup())
		return htbdMoveablePc;

	/* otherwise be more specific about the type of piece we're hit testing */

	if (uiga.ga.bdg.CpcFromSq(*psq) != uiga.ga.bdg.cpcToMove)
		return htbdOpponentPc;

	if (FMoveablePc(*psq))
		return htbdMoveablePc;
	else
		return htbdUnmoveablePc;
}


/*	UIBD::FMoveablePc
 *
 *	Returns true if the square contains a piece that has a legal move
 */
bool UIBD::FMoveablePc(SQ sq) const
{
	assert(uiga.ga.bdg.CpcFromSq(sq) == uiga.ga.bdg.cpcToMove);
	for (MVE mveDrag : vmveDrag)
		if (mveDrag.sqFrom() == sq)
			return true;
	return false;
}


/*	UIBD::StartLeftDrag
 *
 *	Starts the mouse left button down drag operation on the board panel.
 */
void UIBD::StartLeftDrag(const PT& pt)
{
	SQ sq;
	HTBD htbd = HtbdHitTest(pt, &sq);
	sqHover = sqNil;
	SetCapt(this);

	if (htbd == htbdMoveablePc) {
		ptDragInit = pt;
		sqDragInit = sq;
		ptDragCur = pt;
		rcDragPc = RcGetDrag();
		Redraw();
	}
	else {
		MessageBeep(0);
	}
}


void UIBD::EndLeftDrag(const PT& pt)
{
	ReleaseCapt();
	if (sqDragInit.fIsNil())
		return;
	SQ sqFrom = sqDragInit;
	sqDragInit = sqNil;
	SQ sqTo;
	HtbdHitTest(pt, &sqTo);
	if (!sqTo.fIsNil()) {
		if (uiga.FInBoardSetup()) {
			PC pc = uiga.ga.bdg.PcFromSq(sqFrom);
			uiga.ga.bdg.ClearSq(sqFrom);
			uiga.ga.bdg.SetSq(sqTo, pc);
		}
		else {
			for (MVE mve : vmveDrag) {
				if (mve.sqFrom() == sqFrom && mve.sqTo() == sqTo) {
					uiga.ga.PplFromCpc(uiga.ga.bdg.cpcToMove)->ReceiveMvu(mve, spmvFast);
					goto Done;
				}
			}
		}
	}
	/* need to force redraw if it wasn't a legal  move to remove vestiges of the
	   screen dragging */
	Redraw();
Done:
	InvalOutsideRc(rcDragPc);
}


/*	UIBD::LeftDrag
 *
 *	Notification while the mouse button is down and dragging around. The hit test for 
 *	the captured mouse position is in pht.
 *
 *	We use this for users to drag pieces around while they are trying to move.
 */
void UIBD::LeftDrag(const PT& pt)
{
	SQ sq;
	HtbdHitTest(pt, &sq);
	if (sqDragInit.fIsNil()) {
		EndLeftDrag(pt);
		return;
	}
	ptDragCur = pt;
	InvalOutsideRc(rcDragPc);
	rcDragPc = RcGetDrag();
	Redraw();
}


/*	UIBD::MouseHover
 *
 *	Notification that tells us the mouse is hovering over the board. The mouse button 
 *	will not be down. the pht is the this test class for the mouse location. It is 
 *	guaranteed to be a HTBD	for hit testing over the UIBD.
 */
void UIBD::MouseHover(const PT& pt, MHT mht)
{
	SQ sq;
	HTBD htbd = HtbdHitTest(pt, &sq);
	HiliteLegalMoves(htbd == htbdMoveablePc ? sq : sqNil);
	switch (htbd) {
	case htbdMoveablePc:
		::SetCursor(uiga.app.hcurHand);
		break;
	case htbdEmpty:
	case htbdOpponentPc:
	case htbdUnmoveablePc:
		::SetCursor(uiga.app.hcurArrow);
		break;
	default:
		::SetCursor(uiga.app.hcurArrow);
		break;
	}
}


/*	UIBD::HiliteLegalMoves
 *
 *	When the mouse is hovering over square sq, causes the screen to be redrawn with 
 *	squares that the piece can move to with a hilight. If sq is sqNil, no hilights are 
 *	drawn.
 */
void UIBD::HiliteLegalMoves(SQ sq)
{
	if (sq == sqHover)
		return;
	sqHover = sq;
	Redraw();
}


void UIBD::DrawCursor(UI* pui, const RC& rcUpdate)
{
	if (sqDragInit.fIsNil())
		return;
	PC pc = uiga.ga.bdg.PcFromSq(sqDragInit);
	DrawPc(pui, pui->RcLocalFromUiLocal(this, rcDragPc), 1.0f, pc);
}
