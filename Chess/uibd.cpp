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


BRS* UIBD::pbrLight;
BRS* UIBD::pbrDark;
BRS* UIBD::pbrBlack;
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
	if (pbrLight)
		return;

	/* brushes */

	pdc->CreateSolidColorBrush(ColorF(0.42f, 0.54f, 0.32f), &pbrDark);
	pdc->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 0.95f), &pbrLight);
	pdc->CreateSolidColorBrush(ColorF(ColorF::Black), &pbrBlack);
	pdc->CreateSolidColorBrush(ColorF(ColorF::Red), &pbrHilite);
	pdc->CreateSolidColorBrush(ColorF(1.f, 0.15f, 0.0f), &pbrAnnotation);
}


/*	UIBD::DiscardRsrcClass
 *
 *	Cleans up the resources created by CreateRsrcClass
 */
void UIBD::DiscardRsrcClass(void)
{
	SafeRelease(&pbrLight);
	SafeRelease(&pbrDark);
	SafeRelease(&pbrBlack);
	SafeRelease(&pbrAnnotation);
	SafeRelease(&pbrHilite);
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
		sqDragInit(sqNil), sqHover(sqNil)
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
	dyLabel = SizSz(L"8", ptxLabel).height;

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
	uiga.ga.bdg.GenVemv(vemvDrag, ggLegal);
}


/*	UIBD::MakeMv
 *
 *	Makes a move on the board and echoes it on the screen.
 * 
 *	Throws an exception if the move is not valid.
 */
void UIBD::MakeMv(MV mv, SPMV spmv)
{
	for (EMV emvDrag : vemvDrag) {
		if (emvDrag.sqFrom() == mv.sqFrom() && emvDrag.sqTo() == mv.sqTo())
			goto FoundMove;
	}
	throw 1;

FoundMove:
	if (FSpmvAnimate(spmv))
		AnimateMv(mv, DframeFromSpmv(spmv));
	uiga.ga.bdg.MakeMv(mv);
	uiga.ga.bdg.GenVemv(vemvDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vemvDrag, *uiga.ga.prule);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::UndoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imvCurLast >= 0) {
		MV mv = uiga.ga.bdg.vmvGame[uiga.ga.bdg.imvCurLast];
		AnimateSqToSq(mv.sqTo(), mv.sqFrom(), DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.UndoMv();
	uiga.ga.bdg.GenVemv(vemvDrag, ggLegal);
	uiga.ga.bdg.SetGs(gsPlaying);
	if (spmv != spmvHidden)
		Redraw();
}


void UIBD::RedoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && uiga.ga.bdg.imvCurLast < (int)uiga.ga.bdg.vmvGame.size()) {
		MV mv = uiga.ga.bdg.vmvGame[uiga.ga.bdg.imvCurLast+1];
		AnimateMv(mv, DframeFromSpmv(spmv));
	}
	uiga.ga.bdg.RedoMv();
	uiga.ga.bdg.GenVemv(vemvDrag, ggLegal);
	uiga.ga.bdg.SetGameOver(vemvDrag, *uiga.ga.prule);
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

	if (!sqDragInit.fIsNil())
		DrawDragPc(rcDragPc);
}


/*	UIBD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid. Also draws
 *	the file and rank labels along the edge of the board.
 */
void UIBD::DrawMargins(const RC& rcUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast)
{
	FillRc(rcUpdate, pbrLight);
	if (dxyMargin <= 0.0f)
		return;
	RC rc = RcInterior();
	rc.Inflate(PT(-dxyMargin, -dxyMargin));
	FillRc(rc & rcUpdate, pbrDark);
	rc.Inflate(PT(-dxyOutline, -dxyOutline));
	FillRc(rc & rcUpdate, pbrLight);
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
				FillRc(RcFromSq(sq), pbrDark);
			MV mv;
			if (FHoverSq(sq, mv))
				DrawHoverMv(mv);
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
		DrawSzCenter(wstring(szLabel), ptxLabel, RC(rc.left, yTop, rc.right, yBot), pbrDark);
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
	float dxLabel = SizSz(L"8", ptxLabel).width;
	float dxRight = rcSquares.left - (dxyBorder + dxyOutline + dxyBorder) - dxLabel/2.0f;
	float dxLeft = dxRight - dxLabel;
	for (int rank = rankFirst; rank <= rankLast; rank++) {
		RC rc = RcFromSq(SQ(rank, 0));
		DrawSzCenter(wstring(szLabel), ptxLabel, RC(dxLeft, (rc.top + rc.bottom - dyLabel) / 2, dxRight, rc.bottom), pbrDark);
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


/*	UIBD::FHoverSq
 *
 *	Returns true if the square is the destination of move that originates in the
 *	tracking square sqHover. Returns the move itself in mv. 
 */
bool UIBD::FHoverSq(SQ sq, MV& mv)
{
	if (sqHover.fIsNil() || uiga.ga.bdg.gs != gsPlaying)
		return false;
	for (EMV emvDrag : vemvDrag) {
		if (emvDrag.sqFrom() == sqHover && emvDrag.sqTo() == sq) {
			mv = emvDrag;
			return true;
		}
	}
	return false;
}


/*	UIBD::DrawHoverMv
 *
 *	Draws the move hints over destination squares that we display while the 
 *	user is hovering the mouse over a moveable piece.
 *
 *	We draw a circle over every square you can move to, and an X over captures.
 */
void UIBD::DrawHoverMv(MV mv)
{
	OPACITYBR opacityBrSav(pbrBlack, 0.33f);

	RC rc = RcFromSq(mv.sqTo());
	if (!uiga.ga.bdg.FMvIsCapture(mv)) {
		/* moving to an empty square - draw a circle */
		ELL ell(rc.PtCenter(), PT(dxySquare / 5, dxySquare / 5));
		FillEll(ell, pbrBlack);
	}
	else {
		/* taking an opponent piece - draw an X */
		DC* pdc = App().pdc;
		TRANSDC transdc(pdc, 	
			Matrix3x2F::Rotation(45.0f, PT(0.0f, 0.0f)) *
			Matrix3x2F::Scale(SizeF(dxySquare / (2.0f * dxyCrossFull),
				dxySquare / (2.0f * dxyCrossFull)),
				PT(0.0, 0.0)) *
			Matrix3x2F::Translation(SizeF(rcBounds.left + (rc.right + rc.left) / 2,
				rcBounds.top + (rc.top + rc.bottom) / 2)));
		pdc->FillGeometry(pgeomCross, pbrBlack);
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
	DrawPc(RcFromSq(sq), opacity, uiga.ga.bdg.CpcFromSq(sq), uiga.ga.bdg.ApcFromSq(sq));
}


/*	UIBD::DrawDragPc
 *
 *	Draws the piece we're currently dragging, as specified by
 *	phtDragInit, on the screen, in the location at rc. rc should
 *	track the mouse location.
 */
void UIBD::DrawDragPc(const RC& rc)
{
	assert(!sqDragInit.fIsNil());
	DrawPc(rc, 1.0f, uiga.ga.bdg.CpcFromSq(sqDragInit), uiga.ga.bdg.ApcFromSq(sqDragInit));
}


void UIBD::FillRcBack(const RC& rc) const
{
	FillRc(rc, pbrLight);
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
 *	Draws the chess piece on the square at rc.
 */
void UIBD::DrawPc(const RC& rcPc, float opacity, CPC cpc, APC apc)
{
	if (apc == apcNull)
		return;

	/* the piece png has the 12 different chess pieces oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	SIZ siz = pbmpPieces->GetSize();
	float dxPiece = siz.width / 6.0f;
	float dyPiece = siz.height / 2.0f;
	float xPiece = mpapcxBitmap[apc] * dxPiece;
	float yPiece = (int)cpc * dyPiece;
	DrawBmp(rcPc, pbmpPieces, RC(xPiece, yPiece, xPiece + dxPiece, yPiece + dyPiece), opacity);
}


void UIBD::AnimateMv(MV mv, unsigned dframe)
{
	AnimateSqToSq(mv.sqFrom(), mv.sqTo(), dframe);
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
 *	The point is in global coordinates.
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
	if (uiga.ga.bdg.CpcFromSq(*psq) != uiga.ga.bdg.cpcToMove)
		return htbdOpponentPc;

	if (FMoveablePc(*psq))
		return htbdMoveablePc;
	else
		return htbdUnmoveablePc;
}


/*	UIBD::FMoveablePc
 *
 *	Returns true if the square contains a piece that has a
 *	legal move
 */
bool UIBD::FMoveablePc(SQ sq) const
{
	assert(uiga.ga.bdg.CpcFromSq(sq) == uiga.ga.bdg.cpcToMove);
	for (EMV emvDrag : vemvDrag)
		if (emvDrag.sqFrom() == sq)
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
		for (EMV emv : vemvDrag) {
			if (emv.sqFrom() == sqFrom && emv.sqTo() == sqTo) {
				uiga.ga.PplFromCpc(uiga.ga.bdg.cpcToMove)->ReceiveMv(emv, spmvFast);
				goto Done;
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
 *	Notification while the mouse button is down and dragging around. The
 *	hit test for the captured mouse position is in pht.
 *
 *	We use this for users to drag pieces around while they are trying to
 *	move.
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
 *	Notification that tells us the mouse is hovering over the board.
 *	The mouse button will not be down. the pht is the this test
 *	class for the mouse location. It is guaranteed to be a HTBD
 *	for hit testing over the UIBD.
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
 *	When the mouse is hovering over square sq, causes the screen to
 *	be redrawn with squares that the piece can move to with a
 *	hilight. If sq is sqNil, no hilights are drawn.
 */
void UIBD::HiliteLegalMoves(SQ sq)
{
	if (sq == sqHover)
		return;
	sqHover = sq;
	Redraw();
}


/*	UIBD::InvalOutsideRc
 *
 *	While we're tracking piece dragging, it's possible for a piece
 *	to be drawn outside the bounding box of the board. Any drawing
 *	inside the board is taken care of by calling Draw directly, so
 *	we handle these outside parts by just invalidating the area so
 *	they'll get picked off eventually by normal update paints.
 */
void UIBD::InvalOutsideRc(const RC& rcInval) const
{
	RC rcInt = RcInterior();
	InvalRc(RC(rcInval.left, rcInval.top, rcInval.right, rcInt.top), false);
	InvalRc(RC(rcInval.left, rcInt.bottom, rcInval.right, rcInval.bottom), false);
	InvalRc(RC(rcInval.left, rcInval.top, rcInt.left, rcInval.bottom), false);
	InvalRc(RC(rcInt.right, rcInval.top, rcInval.right, rcInval.bottom), false);
}


