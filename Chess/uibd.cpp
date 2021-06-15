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
 *	Screen panel that displays the board
 *
 */


BRS* UIBD::pbrLight;
BRS* UIBD::pbrDark;
BRS* UIBD::pbrBlack;
BRS* UIBD::pbrAnnotation;
BRS* UIBD::pbrHilite;

const float dxyfCrossFull = 20.0f;
const float dxyfCrossCenter = 4.0f;
PTF rgptfCross[] = {
	{-dxyfCrossCenter, -dxyfCrossFull},
	{dxyfCrossCenter, -dxyfCrossFull},
	{dxyfCrossCenter, -dxyfCrossCenter},
	{dxyfCrossFull, -dxyfCrossCenter},
	{dxyfCrossFull, dxyfCrossCenter},
	{dxyfCrossCenter, dxyfCrossCenter},
	{dxyfCrossCenter, dxyfCrossFull},
	{-dxyfCrossCenter, dxyfCrossFull},
	{-dxyfCrossCenter, dxyfCrossCenter},
	{-dxyfCrossFull, dxyfCrossCenter},
	{-dxyfCrossFull, -dxyfCrossCenter},
	{-dxyfCrossCenter, -dxyfCrossCenter},
	{-dxyfCrossCenter, -dxyfCrossFull} };

PTF rgptfArrowHead[] = {
	{0, 0},
	{dxyfCrossFull * 0.5f, dxyfCrossFull * 0.86f},
	{-dxyfCrossFull * 0.5f, dxyfCrossFull * 0.86f},
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

	pdc->CreateSolidColorBrush(ColorF(0.5f, 0.6f, 0.4f), &pbrDark);
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

	AppGet().pfactdwr->CreateTextFormat(szFontFamily, NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		dxyfSquare/4.0f, L"",
		&ptxLabel);

	/* bitmaps */

	pbmpPieces = PbmpFromPngRes(idbPieces);

	/* geometries */

	/* capture X, which is created as a cross that is rotated later */
	pgeomCross = PgeomCreate(rgptfCross, CArray(rgptfCross));
	/* arrow head */
	pgeomArrowHead = PgeomCreate(rgptfArrowHead, CArray(rgptfArrowHead));
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
UIBD::UIBD(GA* pga) : UIP(pga), 
		pbmpPieces(nullptr), pgeomCross(nullptr), pgeomArrowHead(nullptr), ptxLabel(nullptr),
		btnRotateBoard(this, cmdRotateBoard, L'\x2b6f'),
		cpcPointOfView(CPC::White), 
		rcfSquares(0, 0, 640.0f, 640.0f), dxyfSquare(80.0f), dxyfBorder(2.0f), dxyfMargin(50.0f), dxyfOutline(4.0f), dyfLabel(0), angle(0.0f),
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

	rcfSquares = RcfInterior();

	dxyfMargin = rcfBounds.DyfHeight() / 16.0f;
	if (dxyfMargin > 18.0f)
		dxyfBorder = dxyfMargin / 30.0f;
	else
		dxyfMargin = dxyfBorder = 0;

	/* these thin lines need to be integer values or they look like shit */
	dxyfOutline = roundf(2.0f * dxyfBorder);
	dxyfBorder = roundf(dxyfBorder);

	float dxyf = dxyfMargin + dxyfOutline + dxyfBorder;
	rcfSquares.Inflate(-dxyf, -dxyf);
	dxyfSquare = (rcfSquares.bottom - rcfSquares.top) / 8.0f;
	
	CreateRsrc();
	dyfLabel = SizfSz(L"8", ptxLabel).height;

	/* position the rotation button */

	RCF rcf = RcfInterior().Inflate(-dxyfBorder, -dxyfBorder);
	float dxyfBtn = dxyfSquare * 0.33f;
	rcf.top = rcf.bottom - dxyfBtn;
	rcf.right = rcf.left + dxyfBtn;
	btnRotateBoard.SetBounds(rcf);
}


/*	UIBD::NewGame
 *
 *	Starts a new game on the screen board
 */
void UIBD::NewGame(void)
{
	ga.bdg.GenRgmv(gmvDrag, RMCHK::Remove);
}


/*	UIBD::MakeMv
 *
 *	Makes a move on the board and echoes it to the screen 
 *	board panel. We also do end of game checking here.
 */
void UIBD::MakeMv(MV mv, SPMV spmv)
{
	assert(!mv.fIsNil());
#ifndef NDEBUG
	for (int imv = 0; imv < gmvDrag.cmv(); imv++) {
		MV mvDrag = gmvDrag[imv];
		if (mvDrag.sqFrom() == mv.sqFrom() && mvDrag.sqTo() == mv.sqTo())
			goto FoundMove;
	}
	assert(false);
FoundMove:
#endif
	if (FSpmvAnimate(spmv))
		AnimateMv(mv, DframeFromSpmv(spmv));
	ga.bdg.MakeMv(mv);
	ga.bdg.GenRgmv(gmvDrag, RMCHK::Remove);
	ga.bdg.SetGameOver(gmvDrag, *ga.prule);
	if (spmv != SPMV::Hidden)
		Redraw();
}


void UIBD::UndoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && ga.bdg.imvCur >= 0) {
		MV mv = ga.bdg.vmvGame[ga.bdg.imvCur];
		AnimateSqToSq(mv.sqTo(), mv.sqFrom(), DframeFromSpmv(spmv));
	}
	ga.bdg.UndoMv();
	ga.bdg.GenRgmv(gmvDrag, RMCHK::Remove);
	if (spmv != SPMV::Hidden)
		Redraw();
}


void UIBD::RedoMv(SPMV spmv)
{
	if (FSpmvAnimate(spmv) && ga.bdg.imvCur < (int)ga.bdg.vmvGame.size()) {
		MV mv = ga.bdg.vmvGame[ga.bdg.imvCur+1];
		AnimateMv(mv, DframeFromSpmv(spmv));
	}
	ga.bdg.RedoMv();
	ga.bdg.GenRgmv(gmvDrag, RMCHK::Remove);
	if (spmv != SPMV::Hidden)
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
void UIBD::Draw(RCF rcfUpdate)
{
	DC* pdc = AppGet().pdc;
	int rankFirst = (int)floor((rcfUpdate.top - rcfSquares.top) / dxyfSquare);
	float dyf = (rcfUpdate.bottom - rcfSquares.top) / dxyfSquare;
	int rankLast = (int)floor(dyf) - (int)(floor(dyf) == dyf);
	int fileFirst = (int)floor((rcfUpdate.left - rcfSquares.left) / dxyfSquare);
	float dxf = (rcfUpdate.right - rcfSquares.left) / dxyfSquare;
	int fileLast = (int)floor(dxf) - (int)(floor(dxf) == dxf);
	rankFirst = peg(rankFirst, 0, rankMax - 1);
	rankLast = peg(rankLast, 0, rankMax - 1);
	fileFirst = peg(fileFirst, 0, fileMax - 1);
	fileLast = peg(fileLast, 0, fileMax - 1);
	if (cpcPointOfView == CPC::White) {
		rankFirst = rankMax - rankFirst - 1;
		rankLast = rankMax - rankLast - 1;
		swap(rankFirst, rankLast);
	}

	pdc->SetTransform(Matrix3x2F::Rotation(angle, Point2F((rcfBounds.left + rcfBounds.right) / 2, (rcfBounds.top + rcfBounds.bottom) / 2)));
	DrawMargins(rcfUpdate, rankFirst, rankLast, fileFirst, fileLast);
	DrawSquares(rankFirst, rankLast, fileFirst, fileLast);
	DrawHilites();
	DrawAnnotations();
	DrawGameState();
	pdc->SetTransform(Matrix3x2F::Identity());

	if (!sqDragInit.fIsNil())
		DrawDragPc(rcfDragPc);
}


/*	UIBD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid. Also draws
 *	the file and rank labels along the edge of the board.
 */
void UIBD::DrawMargins(RCF rcfUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast)
{
	FillRcf(rcfUpdate, pbrLight);
	if (dxyfMargin <= 0.0f)
		return;
	RCF rcf = RcfInterior();
	rcf.Inflate(PTF(-dxyfMargin, -dxyfMargin));
	FillRcf(rcf & rcfUpdate, pbrDark);
	rcf.Inflate(PTF(-dxyfOutline, -dxyfOutline));
	FillRcf(rcf & rcfUpdate, pbrLight);
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
				FillRcf(RcfFromSq(sq), pbrDark);
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
	float yfTop = rcfSquares.bottom + dxyfBorder+dxyfOutline + dxyfBorder;
	float yfBot = yfTop + dyfLabel;
	for (int file = 0; file <= fileLast; file++) {
		RCF rcf(RcfFromSq(SQ(0, file)));
		DrawSzCenter(wstring(szLabel), ptxLabel, RCF(rcf.left, yfTop, rcf.right, yfBot), pbrDark);
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
	float dxfLabel = SizfSz(L"8", ptxLabel).width;
	float dxfRight = rcfSquares.left - (dxyfBorder + dxyfOutline + dxyfBorder) - dxfLabel/2.0f;
	float dxfLeft = dxfRight - dxfLabel;
	for (int rank = rankFirst; rank <= rankLast; rank++) {
		RCF rcf = RcfFromSq(SQ(rank, 0));
		DrawSzCenter(wstring(szLabel), ptxLabel,
			RCF(dxfLeft, (rcf.top + rcf.bottom - dyfLabel) / 2, dxfRight, rcf.bottom),
			pbrDark);
		szLabel[0]++;
	}
}


/*	UIBD::DrawGameState
 *
 *	When the game is over, this displays the game state on the board
 */
void UIBD::DrawGameState(void)
{
	if (ga.bdg.gs == GS::Playing)
		return;
	switch (ga.bdg.gs) {
		/* TODO: show checkmate */
	case GS::WhiteCheckMated: break;
	case GS::BlackCheckMated: break;
	case GS::WhiteTimedOut: break;
	case GS::BlackTimedOut: break;
	case GS::StaleMate: break;
	default: break;
	}
}


/*	UIBD::RcfFromSq
 *
 *	Returns the rectangle of the given square on the screen
 */
RCF UIBD::RcfFromSq(SQ sq) const
{
	assert(!sq.fIsOffBoard());
	int rank = sq.rank(), file = sq.file();
	if (cpcPointOfView == CPC::White)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	RCF rcf(0, 0, dxyfSquare, dxyfSquare);
	rcf.Offset(rcfSquares.left + dxyfSquare * file, rcfSquares.top + dxyfSquare * rank);
	return rcf;
}


/*	UIBD::FHoverSq
 *
 *	Returns true if the square is the destination of move that originates in the
 *	tracking square sqHover. Returns the move itself in mv. 
 */
bool UIBD::FHoverSq(SQ sq, MV& mv)
{
	if (sqHover == sqNil || ga.bdg.gs != GS::Playing)
		return false;
	for (int imv = 0; imv < gmvDrag.cmv(); imv++) {
		if (gmvDrag[imv].sqFrom() == sqHover && gmvDrag[imv].sqTo() == sq) {
			mv = gmvDrag[imv];
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
	pbrBlack->SetOpacity(0.33f);

	RCF rcf = RcfFromSq(mv.sqTo());
	if (!ga.bdg.FMvIsCapture(mv)) {
		/* moving to an empty square - draw a circle */
		ELLF ellf(PTF((rcf.right + rcf.left) / 2, (rcf.top + rcf.bottom) / 2),
			PTF(dxyfSquare / 5, dxyfSquare / 5));
		FillEllf(ellf, pbrBlack);
	}
	else {
		/* taking an opponent piece - draw an X */
		DC* pdc = AppGet().pdc;
		pdc->SetTransform(
			Matrix3x2F::Rotation(45.0f, PTF(0.0f, 0.0f)) *
			Matrix3x2F::Scale(SizeF(dxyfSquare / (2.0f * dxyfCrossFull),
				dxyfSquare / (2.0f * dxyfCrossFull)),
				PTF(0.0, 0.0)) *
			Matrix3x2F::Translation(SizeF(rcfBounds.left + (rcf.right + rcf.left) / 2,
				rcfBounds.top + (rcf.top + rcf.bottom) / 2)));
		pdc->FillGeometry(pgeomCross, pbrBlack);
		pdc->SetTransform(Matrix3x2F::Identity());
	}

	pbrBlack->SetOpacity(1.0f);
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
	DrawPc(RcfFromSq(sq), opacity, ga.bdg(sq));
}


/*	UIBD::DrawDragPc
 *
 *	Draws the piece we're currently dragging, as specified by
 *	phtDragInit, on the screen, in the location at rcf. rcf should
 *	track the mouse location.
 */
void UIBD::DrawDragPc(const RCF& rcf)
{
	assert(!sqDragInit.fIsNil());
	DrawPc(rcf, 1.0f, ga.bdg(sqDragInit));
}


void UIBD::FillRcfBack(RCF rcf) const
{
	FillRcf(rcf, pbrLight);
}


/*	UIBD::RcfGetDrag
 *
 *	Gets the current screen rectangle the piece we're dragging
 */
RCF UIBD::RcfGetDrag(void)
{
	RCF rcfInit = RcfFromSq(sqDragInit);
	RCF rcf(0, 0, dxyfSquare, dxyfSquare);
	float dxfInit = ptfDragInit.x - rcfInit.left;
	float dyfInit = ptfDragInit.y - rcfInit.top;
	return rcf.Offset(ptfDragCur.x - dxfInit, ptfDragCur.y - dyfInit);
}


/*	UIBD::DrawPc
 *
 *	Draws the chess piece on the square at rcf.
 */
void UIBD::DrawPc(RCF rcf, float opacity, IPC ipc)
{
	if (ipc == ipcEmpty)
		return;

	/* the piece png has the 12 different chess pieces oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	D2D1_SIZE_F ptf = pbmpPieces->GetSize();
	float dxfPiece = ptf.width / 6.0f;
	float dyfPiece = ptf.height / 2.0f;
	float xfPiece = mpapcxBitmap[ipc.apc()] * dxfPiece;
	float yfPiece = (int)ipc.cpc() * dyfPiece;
	DrawBmp(rcf, pbmpPieces, RCF(xfPiece, yfPiece, xfPiece + dxfPiece, yfPiece + dyfPiece), opacity);
}


void UIBD::AnimateMv(MV mv, unsigned dframe)
{
	AnimateSqToSq(mv.sqFrom(), mv.sqTo(), dframe);
}


void UIBD::AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe)
{
	float framefMax = (float)dframe;
	sqDragInit = sqFrom;
	RCF rcfFrom = RcfFromSq(sqDragInit);
	ptfDragInit = rcfFrom.PtfTopLeft();
	RCF rcfTo = RcfFromSq(sqTo);
	RCF rcfFrame = rcfFrom;
	for (float framef = 0.0f; framef < framefMax; framef++) {
		rcfDragPc = rcfFrom;
		float framefRem = framefMax - framef;
		rcfDragPc.Offset((rcfFrom.left*framefRem + rcfTo.left*framef) / framefMax - rcfFrom.left,
			(rcfFrom.top*framefRem + rcfTo.top*framef) / framefMax - rcfFrom.top);
		Redraw(rcfFrame|rcfDragPc);
		rcfFrame = rcfDragPc;
	}
	sqDragInit = sqNil;
}


void UIBD::DrawHilites(void)
{
}


void UIBD::DrawAnnotations(void)
{
	pbrAnnotation->SetOpacity(0.5f);
	for (ANO& ano : vano) {
		if (ano.sqTo.fIsNil())
			DrawSquareAnnotation(ano.sqFrom);
		else
			DrawArrowAnnotation(ano.sqFrom, ano.sqTo);
	}
	pbrAnnotation->SetOpacity(1.0f);
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
		ga.Redraw();
	angle = 0.0f;
	cpcPointOfView = cpcNew;
	ga.Layout();
	ga.Redraw();
	for (UI* pui : vpuiChild)
		pui->Show(true);
}


/*	UIBD::HtbdHitTest
 *
 *	Hit tests the given mouse coordinate and returns a HT implementation
 *	if it belongs to this panel. Returns NULL if there is no hit.
 *
 *	If we return non-null, we are guaranteed to return a HTBD hit test
 *	structure from here, which means other mouse tracking notifications
 *	can safely cast to a HTBD to get access to board hit testing info.
 *
 *	The point is in global coordinates.
 */
HTBD UIBD::HtbdHitTest(PTF ptf, SQ* psq) const
{
	if (!RcfInterior().FContainsPtf(ptf))
		return HTBD::None;
	if (!rcfSquares.FContainsPtf(ptf))
		return HTBD::Static;
	int rank = (int)((ptf.y - rcfSquares.top) / dxyfSquare);
	int file = (int)((ptf.x - rcfSquares.left) / dxyfSquare);
	if (cpcPointOfView == CPC::White)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	*psq = SQ(rank, file);
	if (ga.bdg.FIsEmpty(*psq))
		return HTBD::Empty;
	if (ga.bdg.CpcFromSq(*psq) != ga.bdg.cpcToMove)
		return HTBD::OpponentPc;

	if (FMoveablePc(*psq))
		return HTBD::MoveablePc;
	else
		return HTBD::UnmoveablePc;
}


/*	UIBD::FMoveablePc
 *
 *	Returns true if the square contains a piece that has a
 *	legal move
 */
bool UIBD::FMoveablePc(SQ sq) const
{
	assert(ga.bdg.CpcFromSq(sq) == ga.bdg.cpcToMove);
	for (int imv = 0; imv < gmvDrag.cmv(); imv++)
		if (gmvDrag[imv].sqFrom() == sq)
			return true;
	return false;
}


/*	UIBD::StartLeftDrag
 *
 *	Starts the mouse left button down drag operation on the board panel.
 */
void UIBD::StartLeftDrag(PTF ptf)
{
	SQ sq;
	HTBD htbd = HtbdHitTest(ptf, &sq);
	sqHover = sqNil;
	SetCapt(this);

	if (htbd == HTBD::MoveablePc) {
		ptfDragInit = ptf;
		sqDragInit = sq;
		ptfDragCur = ptf;
		rcfDragPc = RcfGetDrag();
		Redraw();
	}
	else {
		MessageBeep(0);
	}
}


void UIBD::EndLeftDrag(PTF ptf)
{
	ReleaseCapt();
	if (sqDragInit.fIsNil())
		return;
	SQ sqFrom = sqDragInit;
	sqDragInit = sqNil;
	SQ sqTo;
	HtbdHitTest(ptf, &sqTo);
	if (!sqTo.fIsNil()) {
		for (int imv = 0; imv < gmvDrag.cmv(); imv++) {
			MV mv = gmvDrag[imv];
			if (mv.sqFrom() == sqFrom && mv.sqTo() == sqTo) {
				ga.MakeMv(mv, SPMV::Fast);
				goto Done;
			}
		}
	}
	/* need to force redraw if it wasn't a legal  move to remove vestiges of the
	   screen dragging */
	Redraw();
Done:
	InvalOutsideRcf(rcfDragPc);
}


/*	UIBD::LeftDrag
 *
 *	Notification while the mouse button is down and dragging around. The
 *	hit test for the captured mouse position is in pht.
 *
 *	We use this for users to drag pieces around while they are trying to
 *	move.
 */
void UIBD::LeftDrag(PTF ptf)
{
	SQ sq;
	HtbdHitTest(ptf, &sq);
	if (sqDragInit.fIsNil()) {
		EndLeftDrag(ptf);
		return;
	}
	ptfDragCur = ptf;
	InvalOutsideRcf(rcfDragPc);
	rcfDragPc = RcfGetDrag();
	Redraw();
}


/*	UIBD::MouseHover
 *
 *	Notification that tells us the mouse is hovering over the board.
 *	The mouse button will not be down. the pht is the this test
 *	class for the mouse location. It is guaranteed to be a HTBD
 *	for hit testing over the UIBD.
 */
void UIBD::MouseHover(PTF ptf, MHT mht)
{
	SQ sq;
	HTBD htbd = HtbdHitTest(ptf, &sq);
	HiliteLegalMoves(htbd == HTBD::MoveablePc ? sq : sqNil);
	switch (htbd) {
	case HTBD::MoveablePc:
		::SetCursor(ga.app.hcurMove);
		break;
	case HTBD::Empty:
	case HTBD::OpponentPc:
	case HTBD::UnmoveablePc:
		::SetCursor(ga.app.hcurNo);
		break;
	default:
		::SetCursor(ga.app.hcurArrow);
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


/*	UIBD::InvalOutsideRcf
 *
 *	While we're tracking piece dragging, it's possible for a piece
 *	to be drawn outside the bounding box of the board. Any drawing
 *	inside the board is taken care of by calling Draw directly, so
 *	we handle these outside parts by just invalidating the area so
 *	they'll get picked off eventually by normal update paints.
 */
void UIBD::InvalOutsideRcf(RCF rcf) const
{
	RCF rcfInt = RcfInterior();
	InvalRcf(RCF(rcf.left, rcf.top, rcf.right, rcfInt.top), false);
	InvalRcf(RCF(rcf.left, rcfInt.bottom, rcf.right, rcf.bottom), false);
	InvalRcf(RCF(rcf.left, rcf.top, rcfInt.left, rcf.bottom), false);
	InvalRcf(RCF(rcfInt.right, rcf.top, rcf.right, rcf.bottom), false);
}


