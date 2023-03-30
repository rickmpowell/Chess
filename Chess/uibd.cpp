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
 *	BMPPC
 * 
 *	Bitmap piece drawing code
 *
 */


BMPPC* pbmppc;


BMPPC::BMPPC(void) : pbmpPieces(nullptr)
{
}


BMPPC::~BMPPC(void)
{
	SafeRelease(&pbmpPieces);
}


void BMPPC::Draw(UI& ui, const RC& rc, PC pc, float opacity)
{
	if (pbmpPieces == nullptr)
		pbmpPieces = ui.PbmpFromPngRes(idbPieces);
	if (pc.apc()  == apcNull)
		return;

	/* the piece png has the 12 different chess pieces oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0 , -1};
	static const int mpcpcyBitmap[] = { 0, 1 };
	SIZ siz = pbmpPieces->GetSize();
	float dxPiece = siz.width / 6.0f;
	float dyPiece = siz.height / 2.0f;
	float xPiece = mpapcxBitmap[pc.apc()] * dxPiece;
	float yPiece = mpcpcyBitmap[pc.cpc()] * dyPiece;
	ui.DrawBmp(rc, pbmpPieces, RC(xPiece, yPiece, xPiece + dxPiece, yPiece + dyPiece), opacity);
}


/*
 *
 *	UIBD class
 *
 *	Screen panel that displays the board and handles the mouse and keyaboard
 *	interface for it.
 *
 */


const float dxyCrossFull = 20.0f;
const float dxyCrossCenter = 4.0f;
PT aptCross[] = {
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
	{-dxyCrossCenter, -dxyCrossFull} 
};



bool UIBD::FCreateRsrc(void)
{
	if (ptxLabel)
		return false;

	App().pdc->CreateSolidColorBrush(coAnnotation, &pbrAnnotation);
	ptxLabel = PtxCreate(dxySquare/4.0f, false, false);
	pgeomCross = PgeomCreate(aptCross, CArray(aptCross));

	return true;
}

void UIBD::DiscardRsrc(void)
{
	SafeRelease(&pbrAnnotation);
	SafeRelease(&ptxLabel);
	SafeRelease(&pgeomCross);
}


/*	UIBD::UIBD
 *
 *	Constructor for the board screen panel.
 */
UIBD::UIBD(UIGA& uiga) : UIP(uiga), 
		pgeomCross(nullptr), ptxLabel(nullptr), pbrAnnotation(nullptr), pgeomArrowHead(nullptr),
		btnRotateBoard(this, cmdRotateBoard, L'\x2b6f'),
		cpcPointOfView(cpcWhite), 
		rcSquares(0, 0, 640.0f, 640.0f), 
	    dxySquare(80.0f), dxyBorder(2.0f), dxyMargin(50.0f), dxyOutline(4.0f), dyLabel(0), angle(0.0f),
		sqDragInit(sqNil), sqHover(sqNil), sqDragHilite(sqNil), panoDrag(nullptr), fClickClick(false), mveHilite(mveNil)
{
}


/*	UIBD::~UIBD
 *
 *	Destructor for the board screen panel
 */
UIBD::~UIBD(void)
{
	DiscardRsrc();
}


void UIBD::SetEpd(const char* sz)
{
	mpszvepdp.clear();
	uiga.ga.bdg.SetFen(sz);
	SetEpdProperties(sz);
}


void UIBD::SetEpdProperties(const char* szEpd)
{
	uiga.ga.bdg.InitEpdProperties(szEpd, mpszvepdp);
}


/*	UIBD::Layout
 *
 *	Layout work for the board screen panel.
 */
void UIBD::Layout(void)
{
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
	uiga.ga.bdg.GenMoves(vmveDrag, ggLegal);
	SetMoveHilite(mveNil);
}


void UIBD::StartGame(void)
{
	uiga.ga.bdg.GenMoves(vmveDrag, ggLegal);
	SetMoveHilite(mveNil);
}


/*	UIBD::MakeMvu
 *
 *	Makes a move on the board and echoes it on the screen.
 * 
 *	Throws an exception if the move is not valid.
 */
void UIBD::MakeMv(MVE mve, SPMV spmv)
{
	if (!FInVmveDrag(mve.sqFrom(), mve.sqTo(), mve))
		throw 1;

	if (FSpmvAnimate(spmv))
		AnimateMv(mve, DframeFromSpmv(spmv));
	uiga.ga.bdg.MakeMv(mve);
	SetMoveHilite(mve);
	uiga.ga.bdg.GenMoves(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vmveDrag, *uiga.ga.prule);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::UndoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imveCurLast >= 0) {
		MVE mve = uiga.ga.bdg.vmveGame[uiga.ga.bdg.imveCurLast];
		AnimateSqToSq(mve.sqTo(), mve.sqFrom(), DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.UndoMv();
	SetMoveHilite(mveNil);
	uiga.ga.bdg.GenMoves(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGs(gsPlaying);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::RedoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imveCurLast < uiga.ga.bdg.vmveGame.size()) {
		MVE mve = uiga.ga.bdg.vmveGame[uiga.ga.bdg.imveCurLast+1];
		AnimateMv(mve, DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.RedoMv();
	SetMoveHilite(mveNil);
	uiga.ga.bdg.GenMoves(vmveDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vmveDrag, *uiga.ga.prule);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::SetMoveHilite(MVE mve)
{
	mveHilite = mve;
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
	
	DC* pdc = App().pdc;
	TRANSDC transdc(pdc, Matrix3x2F::Rotation(angle, rcBounds.PtCenter()));
	DrawMargins(rcDraw, rankFirst, rankLast, fileFirst, fileLast);
	DrawSquares(rankFirst, rankLast, fileFirst, fileLast);
	DrawDragHilite();
	DrawAnnotations();
	DrawGameState();
}


ColorF UIBD::CoText(void) const
{
	return uiga.FInBoardSetup() ? coBoardBWDark : coBoardDark; 
}


ColorF UIBD::CoBack(void) const
{
	return uiga.FInBoardSetup() ? coBoardBWLight : coBoardLight; 
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
			RC rcSq = RcFromSq(sq);
			if ((rank + file) % 2 == 0)
				FillRc(rcSq, pbrText);
			MVE mve;
			if (FHoverSq(sqDragInit.fIsNil() ? sqHover : sqDragInit, sq, mve))
				DrawHoverMove(mve);
			if (!mveHilite.fIsNil() && (mveHilite.sqFrom() == sq || mveHilite.sqTo() == sq)) {
				OPACITYBR opacitybr(pbrBlack, (rank+file) % 2 ? opacityBoardMoveHilite : 0.2f);
				COLORBRS colorbrs(pbrBlack, coBoardMoveHilite);
				FillRc(rcSq, pbrBlack);
			}
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
	wchar_t szLabel[2] = { 0 };
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
	wchar_t szLabel[2] = { 0, 0 };
	szLabel[0] = '1' + rankFirst;
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
#ifdef UNUSED
	RC rc = RcInterior();
	rc.bottom = rc.top + 12;
	switch (uiga.ga.bdg.gs) {
	case gsNotStarted:
		DrawSz(L"Move piece to start game", ptxList, rc);
		break;
	case gsPlaying:
		break;
	case gsWhiteCheckMated: 
	case gsWhiteTimedOut:
	case gsWhiteResigned:
		DrawSz(L"Black has won", ptxList, rc);
		break;
	case gsBlackCheckMated:
	case gsBlackTimedOut:
	case gsBlackResigned:
		DrawSz(L"White has won", ptxList, rc);
		break;
	case gsStaleMate: 
	case gsDrawDead:
	case gsDrawAgree:
	case gsDraw3Repeat:
	case gsDraw50Move:
		DrawSz(L"Game ended in a draw", ptxList, rc);
		break;
	case gsCanceled:
		DrawSz(L"Game canceled", ptxList, rc);
		break;
	case gsPaused:
		DrawSz(L"Game Paused", ptxList, rc);
		break;
	default: 
		assert(false);
		break;
	}
#endif
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
bool UIBD::FHoverSq(SQ sqFrom, SQ sq, MVE& mve)
{
	if (sqFrom.fIsNil() || !(uiga.ga.bdg.FGsPlaying() || uiga.ga.bdg.FGsNotStarted()))
		return false;
	return FInVmveDrag(sqFrom, sq, mve);
}


/*	UIBD::DrawHoverMove
 *
 *	Draws the move hints over destination squares that we display while the 
 *	user is hovering the mouse over a moveable piece.
 *
 *	We draw a circle over every square you can move to, and an X over captures.
 */
void UIBD::DrawHoverMove(MVE mve)
{
	OPACITYBR opacityBrSav(pbrBlack, 0.33f);

	RC rc = RcFromSq(mve.sqTo());
	if (!uiga.ga.bdg.FMvIsCapture(mve)) {
		/* moving to an empty square - draw a circle */
		ELL ell(rc.PtCenter(), dxySquare / 5);
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
	pbmppc->Draw(*this, RcFromSq(sq), uiga.ga.bdg.PcFromSq(sq), opacity);
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


void UIBD::AnimateMv(MVE mve, unsigned dframe)
{
	AnimateSqToSq(mve.sqFrom(), mve.sqTo(), dframe);
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
		Redraw(rcFrame|rcDragPc, false);
		rcFrame = rcDragPc;
	}
	sqDragInit = sqNil;
}


void UIBD::DrawDragHilite(void)
{
	if (sqDragHilite.fIsNil())
		return;

	RC rc = RcFromSq(sqDragHilite);
	OPACITYBR opacitybr(pbrAnnotation, 0.125f);
	FillRc(rc, pbrAnnotation);
}


void UIBD::DrawAnnotations(void)
{
	OPACITYBR opacitybr(pbrAnnotation, 0.5f);
	for (ANO& ano : vano) {
		if (ano.sqFrom == ano.sqTo)
			DrawSquareAnnotation(ano.sqFrom);
		else
			DrawArrowAnnotation(ano.sqFrom, ano.sqTo);
	}
}


void UIBD::DrawSquareAnnotation(SQ sq)
{
	RC rc = RcFromSq(sq);
	float dxy = rc.DxWidth() / 20;
	ELL ell(rc.PtCenter(), rc.DxWidth() / 2 - dxy);
	OPACITYBR opacitybr(pbrText, opacityAnnotation);
	DrawEll(ell, coAnnotation, dxy);
}


float DxyDistance(PT ptFrom, PT ptTo)
{
	float dx = ptTo.x - ptFrom.x;
	float dy = ptTo.y - ptFrom.y;
	return sqrt(dx * dx + dy * dy);
}


void UIBD::DrawArrowAnnotation(SQ sqFrom, SQ sqTo)
{
	/* create an unrotated arrow geometry using the distance between squares */

	RC rcFrom = RcFromSq(sqFrom);
	RC rcTo = RcFromSq(sqTo);
	PT ptFrom = rcFrom.PtCenter();
	PT ptTo = rcTo.PtCenter();

	/* create the arrow geometry pointing directly to the right */

	float dxy = DxyDistance(ptFrom, ptTo);
	float dxArrow = rcFrom.DxWidth() / 3;
	float dyArrow = rcFrom.DyHeight() / 3;
	float dyShaft = dxArrow / 3;

	/* if multiple annotations point at the same square, back the arrows off a tiny
	   amount so the arrow heads don't overlap too much */

	int cano = 0;
	for (const ANO& ano : vano)
		cano += ano.sqTo == sqTo;
	if (cano > 1)
		dxy -= dyShaft;
	if (cano > 4) {
		dxy -= dyShaft/2;
		dxArrow -= dyShaft / 2;
		dyArrow -= dyShaft / 2;
	}

	PT aptArrow[8] = {  {0,             dyShaft/2},
						 {dxy - dxArrow, dyShaft/2},
						 {dxy - dxArrow, dyArrow/2},
						 {dxy,           0.0f},
						 {dxy - dxArrow, -dyArrow/2},
						 {dxy - dxArrow, -dyShaft/2},
						 {0,			 -dyShaft/2},
						 {0,             dyShaft/2} };
	GEOM* pgeom = PgeomCreate(aptArrow, CArray(aptArrow));

	/* figure out the angle to rotate the arrow by */

	float angleArrow;
	float dx = ptTo.x - ptFrom.x, dy = ptTo.y - ptFrom.y;
	if (dy == 0)
		angleArrow = dx < 0 ? 180.0f : 0.0f;
	else {
		angleArrow = 90.0f - atan(dx/dy)*180.0f/(float)M_PI;
		if (dy < 0)	
			angleArrow += 180.0f;
	}

	/* and draw */

	OPACITYBR opacitybr(pbrText, opacityAnnotation);
	COLORBRS colorbr(pbrText, coAnnotation);
	FillRotateGeom(pgeom, ptFrom, 1, angleArrow, pbrText);
	SafeRelease(&pgeom);
}


void UIBD::FlipBoard(CPC cpcNew)
{
	for (UI* pui : vpuiChild) {
		assert(pui->FVisible());
		pui->Show(false);
	}

	for (angle = 0.0f; angle > -180.0f; angle -= 4.0f)
		uiga.Redraw();
	angle = 0.0f;
	cpcPointOfView = cpcNew;
	uiga.Relayout();
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
	MVE mve;
	return FInVmveDrag(sq, sqNil, mve);
}


/*	UIBD::StartLeftDrag
 *
 *	Starts the mouse left button down drag operation on the board panel.
 *	Left dragging is used for piece moving; in board edit mode, it's used
 *	to place pieces on the board.
 */
void UIBD::StartLeftDrag(const PT& pt)
{
	vano.clear();
	SQ sq;
	HTBD htbd = HtbdHitTest(pt, &sq);
	sqHover = sqNil;

	if (fClickClick) {
		fClickClick = false;
		EndLeftDrag(pt, false);
		return;
	}
	SetDrag(this);

	if (htbd == htbdMoveablePc) {
		ptDragInit = pt;
		sqDragInit = sq;
		ptDragCur = pt;
		rcDragPc = RcGetDrag();
	}
	else {
		::MessageBeep(0);
	}
	Redraw();
}


/*	UIBD::EndLeftDrag
 *
 *	Stops the left mouse button dragging. If we're over a legal square, 
 *	initiates the move. In board setup mode, this is used to place the piece
 *	on the board.
 */
void UIBD::EndLeftDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
	if (sqDragInit.fIsNil())
		return;
	SQ sqFrom = sqDragInit;
	sqDragInit = sqNil;
	SetDragHiliteSq(sqNil);
	SQ sqTo;
	HtbdHitTest(pt, &sqTo);
	if (!sqTo.fIsNil()) {
		if (uiga.FInBoardSetup()) {
			PC pc = uiga.ga.bdg.PcFromSq(sqFrom);
			uiga.ga.bdg.ClearSq(sqFrom);
			uiga.ga.bdg.SetSq(sqTo, pc);
		}
		else {
			MVE mve;
			if (fClick && sqFrom == sqTo) {
				if (FInVmveDrag(sqFrom, sqNil, mve)) {
					SetDrag(this);
					fClickClick = true;
					sqDragInit = sqFrom;
					ptDragInit = ptDragCur = pt;
					rcDragPc = RcGetDrag();
				}
			}
			else {
				if (FInVmveDrag(sqFrom, sqTo, mve))
					uiga.ga.PplFromCpc(uiga.ga.bdg.cpcToMove)->ReceiveMv(mve, spmvFast);
			}
		}
	}
	Redraw();
	InvalOutsideRc(rcDragPc);
}


/*	UIBD::LeftDrag
 *
 *	Notification while the mouse button is down and dragging around. Th
 */
void UIBD::LeftDrag(const PT& pt)
{
	SQ sq;
	HtbdHitTest(pt, &sq);
	if (sqDragInit.fIsNil()) {
		EndLeftDrag(pt, false);
		return;
	}
	MVE mve;
	SetDragHiliteSq(uiga.FInBoardSetup() ||
			FInVmveDrag(sqDragInit, sq, mve) ? sq : sqNil);
	ptDragCur = pt;
	InvalOutsideRc(rcDragPc);
	rcDragPc = RcGetDrag();
	Redraw();
}


void UIBD::NoButtonDrag(const PT& pt)
{
	if (fClickClick)
		LeftDrag(pt);
}

void UIBD::StartRightDrag(const PT& pt)
{
	CancelClickClick();
	SetDrag(this);

	SQ sqHit;
	HTBD htbd = HtbdHitTest(pt, &sqHit);
	
	if (sqHit.fIsNil())
		vano.clear();
	else {
		vano.push_back(ANO(sqHit));
		panoDrag = &vano.back();
	}
	Redraw();
}


void UIBD::EndRightDrag(const PT& pt, bool fClick)
{
	ReleaseDrag();
	SQ sqHit;
	HTBD htbd = HtbdHitTest(pt, &sqHit);
	
	/* set the end point of the last annotation, or if dragged off, just delete
	   the annotation */

	if (panoDrag) {
		if (!sqHit.fIsNil()) {
			panoDrag->sqTo = SqToNearestMove(panoDrag->sqFrom, pt);
			assert(panoDrag == &vano.back());
			/* if annotation already exists, this is a delete operation, so
			   delete both the old one and the new one */
			for (vector<ANO>::iterator iano = vano.begin(); iano < vano.end()-1; iano++) {
				if (*iano == *panoDrag) {
					vano.erase(iano);
					vano.pop_back();
					break;
				}
			}
		}
		else
			vano.pop_back();
		panoDrag = nullptr;
	}
	Redraw();
}


void UIBD::RightDrag(const PT& pt)
{
	SQ sqHit;
	HTBD htbd = HtbdHitTest(pt, &sqHit);
	
	/* modify the end point of the dragged annotation */
	if (panoDrag) {
		if (!sqHit.fIsNil())
			panoDrag->sqTo = SqToNearestMove(panoDrag->sqFrom, pt);
		else
			panoDrag->sqTo = panoDrag->sqFrom;
	}
	Redraw();
}


void UIBD::CancelClickClick(void)
{
	if (!fClickClick)
		return;
	fClickClick = false;
	sqDragInit = sqNil;
}


/*	UIBD::SqToNearestMove
 *
 *	Returns the square that is the nearest to sqHit that is a square that a piece
 *	could conceivably move to. Basically removes squares that are not on a line
 *	(row or column), diagonal, or knight-like L.
 *	
 *	If the hit point is the initial square, returns the initial square.
 */
SQ UIBD::SqToNearestMove(SQ sqFrom, PT ptHit) const
{
	/* this is brute force algorithm and could probably be more efficient, but it's
	   probably not too bad. Use bitboards to generate all possible destination
	   squares */
	
	BB bbHit = BB(sqFrom);
	BB bb = bbHit | Ga().bdg.BbKnightAttacked(bbHit);
	for (DIR dir = dirMin; dir < dirMax; ++dir)
		bb |= mpbb.BbSlideTo(sqFrom, dir);

	/* go through each potential destination square and find the one that's closest
	   to the hit square */

	SQ sqBest = sqNil;
	float dxyBest = DxyDistance(PT(0, 0), RcInterior().PtBotRight());
	for (; bb; bb.ClearLow()) {
		SQ sq = bb.sqLow();
		float dxy = DxyDistance(ptHit, RcFromSq(sq).PtCenter());
		if (dxy < dxyBest) {
			dxyBest = dxy;
			sqBest = sq;
		}
	}
	return sqBest;
}


/*	UIBD::FInVmveDrag
 *
 *	Returns true if the source and destination squares line up with a legal move
 *	in the drag move list. if sqTo is sqNil, searches for the first move that 
 *	matches just the source square; if sqFrom is true searches for first move
 *	that matches the destination square.
 */
bool UIBD::FInVmveDrag(SQ sqFrom, SQ sqTo, MVE& mve) const
{
	for (const MVE& mveDrag : vmveDrag) {
		if ((sqFrom.fIsNil() || mveDrag.sqFrom() == sqFrom) && (sqTo.fIsNil() || mveDrag.sqTo() == sqTo)) {
			mve = mveDrag;
			return true;
		}
	}
	return false;
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
	if (!uiga.FInBoardSetup())
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
	pbmppc->Draw(*pui, 
				 pui->RcLocalFromUiLocal(this, rcDragPc), 
				 uiga.ga.bdg.PcFromSq(sqDragInit));
}
