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
 *	SPABD class
 *
 *	Screen panel that displays the board
 *
 */


BRS* SPABD::pbrLight;
BRS* SPABD::pbrDark;
BRS* SPABD::pbrBlack;
BRS* SPABD::pbrAnnotation;
BRS* SPABD::pbrHilite;
TF* SPABD::ptfLabel;
TF* SPABD::ptfControls;
TF* SPABD::ptfGameState;
BMP* SPABD::pbmpPieces;
GEOM* SPABD::pgeomCross;
GEOM* SPABD::pgeomArrowHead;

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


/*	SPABD::CreateRsrc
 *
 *	Creates the drawing resources necessary to draw the board.
 */
void SPABD::CreateRsrc(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
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
		&ptfLabel);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		32.0f, L"",
		&ptfGameState);
	pfactdwr->CreateTextFormat(L"Arial", NULL,
		DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		24.0f, L"",
		&ptfControls);

	/* bitmaps */

	pbmpPieces = PbmpFromPngRes(idbPieces, pdc, pfactwic);

	/* geometries */

	/* capture X, which is created as a cross that is rotated later */
	pgeomCross = PgeomCreate(pfactd2, rgptfCross, CArray(rgptfCross));
	/* arrow head */
	pgeomArrowHead = PgeomCreate(pfactd2, rgptfArrowHead, CArray(rgptfArrowHead));
}



/*	SPABD::DiscardRsrc
 *
 *	Cleans up the resources created by CreateRsrc
 */
void SPABD::DiscardRsrc(void)
{
	SafeRelease(&pbrLight);
	SafeRelease(&pbrDark);
	SafeRelease(&pbrBlack);
	SafeRelease(&pbrAnnotation);
	SafeRelease(&pbrHilite);
	SafeRelease(&ptfLabel);
	SafeRelease(&ptfControls);
	SafeRelease(&ptfGameState);
	SafeRelease(&pbmpPieces);
	SafeRelease(&pgeomCross);
	SafeRelease(&pgeomArrowHead);
}


/*	SPABD::SPABD
 *
 *	Constructor for the board screen panel.
 */
SPABD::SPABD(GA* pga) : SPA(pga), phtDragInit(NULL), phtCur(NULL), sqHover(sqNil), ictlHover(-1),
		cpcPointOfView(cpcWhite),
		dxyfSquare(80.0f), dxyfBorder(2.0f), dxyfMargin(50.0f),
		angle(0.0f),
		dyfLabel(18.0f)	// TODO: this is a font attribute
{
}


/*	SPABD::~SPABD
 *
 *	Destructor for the board screen panel
 */
SPABD::~SPABD(void)
{
}


/*	SPABD::Layout
 *
 *	Layout work for the board screen panel.
 */
void SPABD::Layout(const PTF& ptf, SPA* pspa, LL ll)
{
	SPA::Layout(ptf, pspa, ll);
	dxyfMargin = (rcfBounds.bottom - rcfBounds.top) / 12.0f;
	float dxyf = dxyfMargin + 3.0f * dxyfBorder;
	rcfSquares = RcfInterior();
	rcfSquares.Inflate(-dxyf, -dxyf);
	dxyfSquare = (rcfSquares.bottom - rcfSquares.top) / 8.0f;
}


/*	SPABD::DxWidth
 *
 *	Returns the width of the board screen panel. Used for screen layout.
 */
float SPABD::DxWidth(void) const
{
	return (ga.rcfBounds.bottom - ga.rcfBounds.top - 10.0f - 50.0f);
}


/*	SPABD::DyHeight
 *
 *	Returns the height of the board screen panel. Used for screen layout.
 */
float SPABD::DyHeight(void) const
{
	return DxWidth();
}


/*	SPABD::NewGame
 *
 *	Starts a new game on the screen board
 */
void SPABD::NewGame(void)
{
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
}


/*	SPABD::MakeMv
 *
 *	Makes a move on the board in the screen panel
 */
void SPABD::MakeMv(MV mv, bool fRedraw)
{
	ga.bdg.MakeMv(mv);
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
	ga.bdg.TestGameOver(rgmvDrag);
	if (ga.bdg.gs != GS::Playing)
		rgmvDrag.clear();
	if (fRedraw)
		Redraw();
}


void SPABD::UndoMv(bool fRedraw)
{
	ga.bdg.UndoMv();
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
	if (fRedraw)
		Redraw();
}


void SPABD::RedoMv(bool fRedraw)
{
	ga.bdg.RedoMv();
	ga.bdg.GenRgmv(rgmvDrag, RMCHK::Remove);
	if (fRedraw)
		Redraw();
}


/*	SPABD::Draw
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
void SPABD::Draw(const RCF* prcfUpdate)
{
	DC* pdc = PdcGet();
	pdc->SetTransform(Matrix3x2F::Rotation(angle, Point2F((rcfBounds.left + rcfBounds.right) / 2, (rcfBounds.top + rcfBounds.bottom) / 2)));
	DrawMargins();
	DrawLabels();
	DrawSquares();
	DrawPieces();
	DrawHover();
	DrawHilites();
	DrawAnnotations();
	DrawGameState();
	DrawControls();
	pdc->SetTransform(Matrix3x2F::Identity());
}


/*	SPABD::DrawMargins
 *
 *	For the green and cream board, the margins are the cream color
 *	with a thin green line just outside the square grid.
 */
void SPABD::DrawMargins(void)
{
	RCF rcf = RcfInterior();
	FillRcf(rcf, pbrLight);
	rcf.Inflate(PTF(-dxyfMargin, -dxyfMargin));
	FillRcf(rcf, pbrDark);
	rcf.Inflate(PTF(-2.0f * dxyfBorder, -2.0f * dxyfBorder));
	FillRcf(rcf, pbrLight);
}


/*	SPABD::DrawSquares
 *
 *	Draws the squares of the chessboard. Assumes the light color
 *	has already been used to fill the board area, so all we need to
 *	do is draw the dark colors on top
 */
void SPABD::DrawSquares(void)
{
	for (SQ sq = 0; sq < sqMax; sq++)
		if ((sq.rank() + sq.file()) % 2 == 0)
			FillRcf(RcfFromSq(sq), pbrDark);
}


/*	SPABD::DrawLabels
 *
 *	Draws the square labels in the margin area of the green and cream
 *	board.
 */
void SPABD::DrawLabels(void)
{
	DrawFileLabels();
	DrawRankLabels();
}


/*	SPABD::DrawFileLabels
 *
 *	Draws the file letter labels along the bottom of the board.
 */
void SPABD::DrawFileLabels(void)
{
	TCHAR szLabel[2];
	szLabel[0] = L'a';
	szLabel[1] = 0;
	RCF rcf;
	rcf.top = rcfSquares.bottom + 3.0f * dxyfBorder + dxyfBorder;
	rcf.bottom = rcf.top + dyfLabel;
	ptfLabel->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	for (int file = 0; file < fileMax; file++) {
		RCF rcfT(RcfFromSq(SQ(0, file)));
		rcf.left = rcfT.left;
		rcf.right = rcfT.right;
		DrawSz(wstring(szLabel), ptfLabel, rcf, pbrDark);
		szLabel[0]++;
	}
}


/*	SPABD::DrawRankLabels
 *
 *	Draws the numerical rank labels along the side of the board
 */
void SPABD::DrawRankLabels(void)
{
	RCF rcf(dxyfMargin / 2, rcfSquares.top, dxyfMargin, 0);
	TCHAR szLabel[2];
	szLabel[0] = cpcPointOfView == cpcBlack ? '1' : '8';
	szLabel[1] = 0;
	for (int rank = 0; rank < rankMax; rank++) {
		rcf.bottom = rcf.top + dxyfSquare;
		DrawSz(wstring(szLabel), ptfLabel,
			RCF(rcf.left, (rcf.top + rcf.bottom - dyfLabel) / 2, rcf.right, rcf.bottom),
			pbrDark);
		rcf.top = rcf.bottom;
		if (cpcPointOfView == cpcBlack)
			szLabel[0]++;
		else
			szLabel[0]--;
	}
}


void SPABD::Resign(void)
{
	MessageBeep(0);
}


/*	SPABD::DrawGameState
 *
 *	When the game is over, this displays the game state on the board
 */
void SPABD::DrawGameState(void)
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


/*	SPABD::RcfFromSq
 *
 *	Returns the rectangle of the given square on the screen
 */
RCF SPABD::RcfFromSq(SQ sq) const
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


/*	SPABD::DrawHover
 *
 *	Draws the move hints that we display while the user is hovering
 *	the mouse over a moveable piece.
 *
 *	We draw a circle over every square you can move to.
 */
void SPABD::DrawHover(void)
{
	if (sqHover == sqNil)
		return;
	pbrBlack->SetOpacity(0.33f);
	for (MV mv : rgmvDrag) {
		if (mv.SqFrom() != sqHover)
			continue;
		RCF rcf = RcfFromSq(mv.SqTo());
		if (ga.bdg.mpsqtpc[mv.SqTo()] == tpcEmpty) {
			ELLF ellf(PTF((rcf.right + rcf.left) / 2, (rcf.top + rcf.bottom) / 2),
				PTF(dxyfSquare / 5, dxyfSquare / 5));
			FillEllf(ellf, pbrBlack);
		}
		else {
			DC* pdc = PdcGet();
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


/*	SPABD::DrawPieces
 *
 *	Draws pieces on the board. Includes support for the trackingn display
 *	while we're dragging pieces.
 *
 *	Direct2D is fast enough for us to do full screen redraws on just about
 *	any user-initiated change, so there isn't much point in optimizing.
 */
void SPABD::DrawPieces(void)
{
	for (SQ sq = 0; sq < sqMax; sq++) {
		float opacity = (phtDragInit && phtDragInit->sq == sq) ? 0.2f : 1.0f;
		DrawPc(RcfFromSq(sq), opacity, ga.bdg.mpsqtpc[sq]);
	}
	if (phtDragInit && phtCur)
		DrawDragPc(rcfDragPc);
}


/*	SPABD::DrawDragPc
 *
 *	Draws the piece we're currently dragging, as specified by
 *	phtDragInit, on the screen, in the location at rcf. rcf should
 *	track the mouse location.
 */
void SPABD::DrawDragPc(const RCF& rcf)
{
	assert(phtDragInit);
	DrawPc(rcf, 1.0f, ga.bdg.mpsqtpc[phtDragInit->sq]);
}


/*	SPABD::RcfGetDrag
 *
 *	Gets the current screen rectangle the piece we're dragging
 */
RCF SPABD::RcfGetDrag(void)
{
	RCF rcfInit = RcfFromSq(phtDragInit->sq);
	RCF rcf(0, 0, dxyfSquare, dxyfSquare);
	float dxfInit = phtDragInit->ptf.x - rcfInit.left;
	float dyfInit = phtDragInit->ptf.y - rcfInit.top;
	return rcf.Offset(phtCur->ptf.x - dxfInit, phtCur->ptf.y - dyfInit);
}


/*	SPABD::DrawPc
 *
 *	Draws the chess piece on the square at rcf.
 */
void SPABD::DrawPc(RCF rcf, float opacity, BYTE tpc)
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


void SPABD::DrawHilites(void)
{
}


void SPABD::DrawAnnotations(void)
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


void SPABD::DrawSquareAnnotation(SQ sq)
{
}


void SPABD::DrawArrowAnnotation(SQ sqFrom, SQ sqTo)
{
}


/*	SPABD::DrawControls
 *
 *	Draws controls for manipulating the board
 */
void SPABD::DrawControls(void)
{
	ptfControls->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	DrawControl(L'\x2b6f', 0, HTT::FlipBoard);
}


void SPABD::DrawControl(WCHAR ch, int ictl, HTT htt) const
{
	ID2D1Brush* pbr = pbrText;
	if (phtCur && phtCur->htt == htt)
		pbr = pbrTextSel;
	else if (ictlHover == ictl)
		pbr = pbrBlack;
	WCHAR sz[2];
	sz[0] = ch;
	sz[1] = 0;
	DrawSz(wstring(sz), ptfControls, RcfControl(ictl), pbr);
}


/*	SPABD::RcfControl
 *
 *	Returns the local coordinates of the ictl control on the playing
 *	board.
 */
RCF SPABD::RcfControl(int ictl) const
{
	RCF rcf(0, 0, 35.0f, 35.0f);
	rcf.Offset(12.0f + ictl * 35.0f, rcfBounds.DyfHeight() - 35.0f);
	return rcf;
}


void SPABD::FlipBoard(CPC cpcNew)
{
	for (angle = 0.0f; angle > -180.0f; angle -= 4.0f)
		ga.Redraw();
	angle = 0.0f;
	cpcPointOfView = cpcNew;
	ga.Layout();
	ga.Redraw();
}


/*	SPABD::PhtHitTest
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
HT* SPABD::PhtHitTest(PTF ptf)
{
	if (!rcfBounds.FContainsPtf(ptf)) {
		if (phtDragInit)
			return new HTBD(ptf, HTT::Miss, this, sqNil);
		return NULL;
	}
	ptf.x -= rcfBounds.left;
	ptf.y -= rcfBounds.top;
	if (!rcfSquares.FContainsPtf(ptf)) {
		if (RcfControl(0).FContainsPtf(ptf))
			return new HTBD(ptf, HTT::FlipBoard, this, sqNil);
		return new HTBD(ptf, HTT::Static, this, sqMax);
	}
	int rank = (int)((ptf.y - rcfSquares.top) / dxyfSquare);
	int file = (int)((ptf.x - rcfSquares.left) / dxyfSquare);
	if (cpcPointOfView == cpcWhite)
		rank = rankMax - 1 - rank;
	else
		file = fileMax - 1 - file;
	SQ sq = SQ(rank, file);
	if (ga.bdg.mpsqtpc[sq] == tpcEmpty)
		return new HTBD(ptf, HTT::EmptyPc, this, sq);
	if (CpcFromTpc(ga.bdg.mpsqtpc[sq]) != ga.bdg.cpcToMove)
		return new HTBD(ptf, HTT::OpponentPc, this, sq);

	if (FMoveablePc(sq))
		return new HTBD(ptf, HTT::MoveablePc, this, sq);
	else
		return new HTBD(ptf, HTT::UnmoveablePc, this, sq);
}


/*	SPABD::FMoveablePc
 *
 *	Returns true if the square contains a piece that has a
 *	legal move
 */
bool SPABD::FMoveablePc(SQ sq)
{
	assert(CpcFromTpc(ga.bdg.mpsqtpc[sq]) == ga.bdg.cpcToMove);
	for (MV mv : rgmvDrag)
		if (mv.SqFrom() == sq)
			return true;
	return false;
}


/*	SPABD::StartLeftDrag
 *
 *	Starts the mouse left button down drag operation on the board panel.
 */
void SPABD::StartLeftDrag(HT* pht)
{
	sqHover = sqNil;

	switch (pht->htt) {
	case HTT::MoveablePc:
	case HTT::FlipBoard:
		assert(phtDragInit == NULL);
		assert(phtCur == NULL);
		phtDragInit = (HTBD*)pht->PhtClone();
		phtCur = (HTBD*)pht->PhtClone();
		if (pht->htt == HTT::MoveablePc)
			rcfDragPc = RcfGetDrag();
		break;
	default:
		MessageBeep(0);
		return;
	}

	Redraw();
}


void SPABD::EndLeftDrag(HT* pht)
{
	if (phtDragInit == NULL)
		return;
	HTBD* phtbd = (HTBD*)pht;
	if (phtbd) {
		switch (phtDragInit->htt) {
		case HTT::MoveablePc:
			if (phtbd->sq == sqNil)
				break;
			/* find the to/from square in the move list  */
			for (MV mv : rgmvDrag) {
				if (mv.SqFrom() == phtDragInit->sq && mv.SqTo() == phtbd->sq) {
					ga.MakeMv(mv, true);
					break;
				}
			}
			break;
		case HTT::FlipBoard:
			FlipBoard(cpcPointOfView ^ 1);
			break;
		default:
			break;
		}
	}

	/* clean up drag */

	delete phtDragInit;
	phtDragInit = NULL;
	if (phtCur)
		delete phtCur;
	phtCur = NULL;

	InvalOutsideRcf(rcfDragPc);
	Redraw();
}


/*	SPABD::LeftDrag
 *
 *	Notification while the mouse button is down and dragging around. The
 *	hit test for the captured mouse position is in pht.
 *
 *	We use this for users to drag pieces around while they are trying to
 *	move.
 */
void SPABD::LeftDrag(HT* pht)
{
	if (phtDragInit == NULL || pht == NULL || pht->htt == HTT::Miss) {
		EndLeftDrag(pht);
		return;
	}

	if (phtCur)
		delete phtCur;
	phtCur = (HTBD*)pht->PhtClone();

	switch (phtDragInit->htt) {
	case HTT::FlipBoard:
		if (pht->htt != phtDragInit->htt)
			HiliteControl(-1);
		break;
	default:
		InvalOutsideRcf(rcfDragPc);
		rcfDragPc = RcfGetDrag();
		break;
	}
	Redraw();
}


/*	SPABD::MouseHover
 *
 *	Notification that tells us the mouse is hovering over the board.
 *	The mouse button will not be down. the pht is the this test
 *	class for the mouse location. It is guaranteed to be a HTBD
 *	for hit testing over the SPABD.
 */
void SPABD::MouseHover(HT* pht)
{
	if (pht == NULL)
		return;
	HTBD* phtbd = (HTBD*)pht;
	switch (phtbd->htt) {
	case HTT::MoveablePc:
		HiliteLegalMoves(phtbd->sq);
		break;
	case HTT::FlipBoard:
		HiliteControl(0);
		break;
	default:
		HiliteLegalMoves(sqNil);
		break;
	}
}


/*	SPABD::HiliteLegalMoves
 *
 *	When the mouse is hovering over square sq, causes the screen to
 *	be redrawn with squares that the piece can move to with a
 *	hilight. If sq is sqNil, no hilights are drawn.
 */
void SPABD::HiliteLegalMoves(SQ sq)
{
	if (sq == sqHover)
		return;
	ictlHover = -1;
	sqHover = sq;
	Redraw();
}


void SPABD::HiliteControl(int ictl)
{
	if (ictl == ictlHover)
		return;
	sqHover = sqNil;
	ictlHover = ictl;
	Redraw();
}


/*	SPABD::InvalOutsideRcf
 *
 *	While we're tracking piece dragging, it's possible for a piece
 *	to be drawn outside the bounding box of the board. Any drawing
 *	inside the board is taken care of by calling Draw directly, so
 *	we handle these outside parts by just invalidating the area so
 *	they'll get picked off eventually by normal update paints.
 */
void SPABD::InvalOutsideRcf(RCF rcf) const
{
	RCF rcfInt = RcfInterior();
	InvalRcf(RCF(rcf.left, rcf.top, rcf.right, rcfInt.top), false);
	InvalRcf(RCF(rcf.left, rcfInt.bottom, rcf.right, rcf.bottom), false);
	InvalRcf(RCF(rcf.left, rcf.top, rcfInt.left, rcf.bottom), false);
	InvalRcf(RCF(rcfInt.right, rcf.top, rcf.right, rcf.bottom), false);
}


