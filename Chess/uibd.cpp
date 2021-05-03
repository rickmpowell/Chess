/*
 *
 *	uibd.cpp
 * 
 *	User interface for the board panel
 * 
 */

#include "uibd.h"
#include "ga.h"
#include "Chess.h"


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
TX* UIBD::ptxLabel;
BMP* UIBD::pbmpPieces;
GEOM* UIBD::pgeomCross;
GEOM* UIBD::pgeomArrowHead;

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

	/* fonts */

	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		16.0f, L"",
		&ptxLabel);

	/* bitmaps */

	pbmpPieces = PbmpFromPngRes(idbPieces, pdc, pfactwic);

	/* geometries */

	/* capture X, which is created as a cross that is rotated later */
	pgeomCross = PgeomCreate(pfactd2, rgptfCross, CArray(rgptfCross));
	/* arrow head */
	pgeomArrowHead = PgeomCreate(pfactd2, rgptfArrowHead, CArray(rgptfArrowHead));
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
	SafeRelease(&ptxLabel);
	SafeRelease(&pbmpPieces);
	SafeRelease(&pgeomCross);
	SafeRelease(&pgeomArrowHead);
}


/*	UIBD::UIBD
 *
 *	Constructor for the board screen panel.
 */
UIBD::UIBD(GA* pga) : SPA(pga), sqDragInit(sqNil), sqHover(sqNil),
		cpcPointOfView(cpcWhite),
		dxyfSquare(80.0f), dxyfBorder(2.0f), dxyfMargin(50.0f),
		angle(0.0f),
		dyfLabel(18.0f)	// TODO: this is a font attribute
{
	pbtnRotateBoard = new BTNCH(this, cmdRotateBoard, RCF(0, 0, 0, 0), L'\x2b6f');
}


/*	UIBD::~UIBD
 *
 *	Destructor for the board screen panel
 */
UIBD::~UIBD(void)
{
	if (pbtnRotateBoard)
		delete pbtnRotateBoard;
}


/*	UIBD::Layout
 *
 *	Layout work for the board screen panel.
 */
void UIBD::Layout(void)
{
	rcfSquares = RcfInterior();
	pbtnRotateBoard->SetBounds(RCF(rcfSquares.left+3.0f, rcfSquares.bottom - 30.0f, rcfSquares.left + 33.0f, rcfSquares.bottom));
	dxyfMargin = rcfBounds.DyfHeight() / 12.0f;
	float dxyf = dxyfMargin + 3.0f * dxyfBorder;
	rcfSquares.Inflate(-dxyf, -dxyf);
	dxyfSquare = (rcfSquares.bottom - rcfSquares.top) / 8.0f;
}


/*	UIBD::NewGame
 *
 *	Starts a new game on the screen board
 */
void UIBD::NewGame(void)
{
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
}


/*	UIBD::MakeMv
 *
 *	Makes a move on the board in the screen panel
 */
void UIBD::MakeMv(MV mv, SPMV spmv)
{
	if (spmv == SPMV::Animate)
		AnimateMv(mv);
	ga.bdg.MakeMv(mv);
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
	ga.bdg.TestGameOver(rgmvDrag);
	if (ga.bdg.gs != GS::Playing)
		rgmvDrag.clear();
	if (spmv != SPMV::Hidden)
		Redraw();
}


void UIBD::UndoMv(SPMV spmv)
{
	ga.bdg.UndoMv();
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
	if (spmv != SPMV::Hidden)
		Redraw();
}


void UIBD::RedoMv(SPMV spmv)
{
	ga.bdg.RedoMv();
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
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
void UIBD::Draw(const RCF* prcfUpdate)
{
	DC* pdc = AppGet().pdc;
	pdc->SetTransform(Matrix3x2F::Rotation(angle, Point2F((rcfBounds.left + rcfBounds.right) / 2, (rcfBounds.top + rcfBounds.bottom) / 2)));
	DrawMargins();
	DrawLabels();
	DrawSquares();
	DrawHover();
	DrawPieces();
	DrawHilites();
	DrawAnnotations();
	DrawGameState();
	pdc->SetTransform(Matrix3x2F::Identity());
}


/*	UIBD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid.
 */
void UIBD::DrawMargins(void)
{
	RCF rcf = RcfInterior();
	FillRcf(rcf, pbrLight);
	rcf.Inflate(PTF(-dxyfMargin, -dxyfMargin));
	FillRcf(rcf, pbrDark);
	rcf.Inflate(PTF(-2.0f * dxyfBorder, -2.0f * dxyfBorder));
	FillRcf(rcf, pbrLight);
}


/*	UIBD::DrawSquares
 *
 *	Draws the squares of the chessboard. Assumes the light color
 *	has already been used to fill the board area, so all we need to
 *	do is draw the dark colors on top
 */
void UIBD::DrawSquares(void)
{
	for (SQ sq = 0; sq < sqMax; sq++)
		if ((sq.rank() + sq.file()) % 2 == 0)
			FillRcf(RcfFromSq(sq), pbrDark);
}


/*	UIBD::DrawLabels
 *
 *	Draws the square labels in the margin area of the green and cream
 *	board.
 */
void UIBD::DrawLabels(void)
{
	DrawFileLabels();
	DrawRankLabels();
}


/*	UIBD::DrawFileLabels
 *
 *	Draws the file letter labels along the bottom of the board.
 */
void UIBD::DrawFileLabels(void)
{
	TCHAR szLabel[2];
	szLabel[0] = L'a';
	szLabel[1] = 0;
	RCF rcf;
	rcf.top = rcfSquares.bottom + 3.0f * dxyfBorder + dxyfBorder;
	rcf.bottom = rcf.top + dyfLabel;
	ptxLabel->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (int file = 0; file < fileMax; file++) {
		RCF rcfT(RcfFromSq(SQ(0, file)));
		rcf.left = rcfT.left;
		rcf.right = rcfT.right;
		DrawSz(wstring(szLabel), ptxLabel, rcf, pbrDark);
		szLabel[0]++;
	}
}


/*	UIBD::DrawRankLabels
 *
 *	Draws the numerical rank labels along the side of the board
 */
void UIBD::DrawRankLabels(void)
{
	RCF rcf(dxyfMargin / 2, rcfSquares.top, dxyfMargin, 0);
	TCHAR szLabel[2];
	szLabel[0] = cpcPointOfView == cpcBlack ? '1' : '8';
	szLabel[1] = 0;
	for (int rank = 0; rank < rankMax; rank++) {
		rcf.bottom = rcf.top + dxyfSquare;
		DrawSz(wstring(szLabel), ptxLabel,
			RCF(rcf.left, (rcf.top + rcf.bottom - dyfLabel) / 2, rcf.right, rcf.bottom),
			pbrDark);
		rcf.top = rcf.bottom;
		if (cpcPointOfView == cpcBlack)
			szLabel[0]++;
		else
			szLabel[0]--;
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
	const WCHAR* szState = L"";
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
	assert(sq >= 0 && sq < sqMax);
	int rank = sq.rank(), file = sq.file();
	if (cpcPointOfView == cpcWhite)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	RCF rcf(0, 0, dxyfSquare, dxyfSquare);
	rcf.Offset(rcfSquares.left + dxyfSquare * file, rcfSquares.top + dxyfSquare * rank);
	return rcf;
}


/*	UIBD::DrawHover
 *
 *	Draws the move hints that we display while the user is hovering
 *	the mouse over a moveable piece.
 *
 *	We draw a circle over every square you can move to.
 */
void UIBD::DrawHover(void)
{
	if (sqHover == sqNil)
		return;
	pbrBlack->SetOpacity(0.33f);
	unsigned long grfDrawn = 0L;
	for (MV mv : rgmvDrag) {
		if (mv.SqFrom() != sqHover)
			continue;
		SQ sqTo = mv.SqTo();
		if (grfDrawn & (1L << sqTo))
			continue;
		grfDrawn |= 1L << sqTo;
		RCF rcf = RcfFromSq(sqTo);
		if (ga.bdg.mpsqtpc[sqTo] == tpcEmpty && !ga.bdg.FMvEnPassant(mv)) {
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
	}
	pbrBlack->SetOpacity(1.0f);
}


/*	UIBD::DrawPieces
 *
 *	Draws pieces on the board. Includes support for the trackingn display
 *	while we're dragging pieces.
 *
 *	Direct2D is fast enough for us to do full screen redraws on just about
 *	any user-initiated change, so there isn't much point in optimizing.
 */
void UIBD::DrawPieces(void)
{
	for (SQ sq = 0; sq < sqMax; sq++) {
		float opacity = sqDragInit == sq ? 0.2f : 1.0f;
		DrawPc(RcfFromSq(sq), opacity, ga.bdg.mpsqtpc[sq]);
	}
	if (!sqDragInit.FIsNil())
		DrawDragPc(rcfDragPc);
}


/*	UIBD::DrawDragPc
 *
 *	Draws the piece we're currently dragging, as specified by
 *	phtDragInit, on the screen, in the location at rcf. rcf should
 *	track the mouse location.
 */
void UIBD::DrawDragPc(const RCF& rcf)
{
	assert(!sqDragInit.FIsNil());
	DrawPc(rcf, 1.0f, ga.bdg.mpsqtpc[sqDragInit]);
}


void UIBD::FillRcfBack(RCF rcf) const
{
	/* TODO: should probably draw the entire board (squares and labels) here */
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
void UIBD::DrawPc(RCF rcf, float opacity, BYTE tpc)
{
	/* the piece png has the 12 different chess pieces oriented like:
	 *   WK WQ WN WR WB WP
	 *   BK BQ BN BR BB BP
	 */
	static const int mpapcxBitmap[] = { -1, 5, 3, 2, 4, 1, 0, -1, -1 };
	D2D1_SIZE_F ptf = pbmpPieces->GetSize();
	float dxfPiece = ptf.width / 6.0f;
	float dyfPiece = ptf.height / 2.0f;
	float xfPiece = mpapcxBitmap[ApcFromTpc(tpc)] * dxfPiece;
	float yfPiece = CpcFromTpc(tpc) * dyfPiece;
	DrawBmp(rcf, pbmpPieces, RCF(xfPiece, yfPiece, xfPiece + dxfPiece, yfPiece + dyfPiece), opacity);
}


void UIBD::AnimateMv(MV mv)
{
	int frameMax = 40;
	sqDragInit = mv.SqFrom();
	RCF rcfFrom = RcfFromSq(sqDragInit);
	ptfDragInit = rcfFrom.PtfTopLeft();
	RCF rcfTo = RcfFromSq(mv.SqTo());
	for (int frame = 0; frame < frameMax; frame++) {
		rcfDragPc = rcfFrom;
		rcfDragPc.Offset((rcfFrom.left * (float)(frameMax - frame) + rcfTo.left * (float)frame) / (float)frameMax - rcfFrom.left,
			(rcfFrom.top * (float)(frameMax - frame) + rcfTo.top * (float)frame) / (float)frameMax - rcfFrom.top);
		Redraw();
	}
	sqDragInit = sqNil;
}


void UIBD::DrawHilites(void)
{
}


void UIBD::DrawAnnotations(void)
{
	pbrAnnotation->SetOpacity(0.5f);
	for (ANO ano : rgano) {
		if (ano.sqTo.FIsNil())
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
	for (UI* pui : rgpuiChild)
		pui->Show(false);
	for (angle = 0.0f; angle > -180.0f; angle -= 4.0f)
		ga.Redraw();
	angle = 0.0f;
	cpcPointOfView = cpcNew;
	ga.Layout();
	ga.Redraw();
	for (UI* pui : rgpuiChild)
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
	if (cpcPointOfView == cpcWhite)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	*psq = SQ(rank, file);
	if (ga.bdg.mpsqtpc[*psq] == tpcEmpty)
		return HTBD::Empty;
	if (CpcFromTpc(ga.bdg.mpsqtpc[*psq]) != ga.bdg.cpcToMove)
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
	assert(CpcFromTpc(ga.bdg.mpsqtpc[sq]) == ga.bdg.cpcToMove);
	for (MV mv : rgmvDrag)
		if (mv.SqFrom() == sq)
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
	if (sqDragInit.FIsNil())
		return;
	SQ sq;
	HTBD htbd = HtbdHitTest(ptf, &sq);
	if (!sq.FIsNil()) {
		for (MV mv : rgmvDrag) {
			if (mv.SqFrom() == sqDragInit && mv.SqTo() == sq) {
				ga.MakeMv(mv, SPMV::Fast);
				break;
			}
		}
	}

	InvalOutsideRcf(rcfDragPc);
	sqDragInit = sqNil;
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
	HTBD htbd = HtbdHitTest(ptf, &sq);
	if (sqDragInit.FIsNil()) {
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


