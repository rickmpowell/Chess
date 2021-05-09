/*
 *
 *	uibd.h
 * 
 *	UI for board panel
 * 
 */

#pragma once
#include "framework.h"
#include "ui.h"
#include "bd.h"


/*
 *
 *	UIBD class
 *
 *	Class that keeps and displays the game board on the screen inside
 *	the board panel
 *
 */


/*
 *	Hit test codes
 */
enum class HTBD
{
	None,
	Static,
	FlipBoard,
	Empty,
	OpponentPc,
	MoveablePc,
	UnmoveablePc
};



/*
 *	Move speeds
 */
enum class SPMV
{
	Hidden,
	Fast,
	Animate
};


class UIBD : public UIP
{
public:
	static BRS* pbrLight;
	static BRS* pbrDark;
	static BRS* pbrBlack;
	static BRS* pbrAnnotation;
	static BRS* pbrHilite;
	static TX* ptxLabel;
	static BMP* pbmpPieces;
	static GEOM* pgeomCross;
	static GEOM* pgeomArrowHead;

	CPC cpcPointOfView;
	RCF rcfSquares;
	float dxyfSquare, dxyfBorder, dxyfMargin;
	float dyfLabel;

	BTN* pbtnRotateBoard;

	float angle;	// angle for rotation animation
	SQ sqDragInit;
	PTF ptfDragInit;
	PTF ptfDragCur;
	SQ sqHover;
	RCF rcfDragPc;	// rectangle the dragged piece was last drawn in
	vector<MV> rgmvDrag;	// legal moves in the UI

	vector<ANO> rgano;	// annotations

public:
	UIBD(GA* pga);
	~UIBD(void);
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

	void NewGame(void);
	void MakeMv(MV mv, SPMV spmv);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);

	virtual void Layout(void);

	virtual void Draw(const RCF* prcfUpdate = NULL);
	void DrawMargins(void);
	void DrawSquares(void);
	void DrawLabels(void);
	void DrawFileLabels(void);
	void DrawRankLabels(void);
	void DrawHover(void);
	void DrawPieces(void);
	void DrawAnnotations(void);
	void DrawSquareAnnotation(SQ sq);
	void DrawArrowAnnotation(SQ sqFrom, SQ sqTo);

	void DrawHilites(void);
	void DrawGameState(void);
	void DrawPc(RCF rcf, float opacity, TPC tpc);
	void AnimateMv(MV mv);
	void DrawDragPc(const RCF& rcf);
	RCF RcfGetDrag(void);
	void InvalOutsideRcf(RCF rcf) const;
	void HiliteLegalMoves(SQ sq);
	RCF RcfFromSq(SQ sq) const;

	virtual void FillRcfBack(RCF rcf) const;

	void FlipBoard(CPC cpcNew);

	HTBD HtbdHitTest(PTF ptf, SQ* psq) const;
	virtual void StartLeftDrag(PTF ptf);
	virtual void EndLeftDrag(PTF ptf);
	virtual void LeftDrag(PTF ptf);
	virtual void MouseHover(PTF ptf, MHT mht);

	bool FMoveablePc(SQ sq) const;
};


/*
 *
 *	UIPCP screen panel
 * 
 *	A screen panel that holds a chess piece toolbar for building boards. User drags
 *	pieces from the panel to the board. Includes buttons for deleting and OK/Cancel.
 * 
 */


class UIPCP : public UIP
{
public:
	UIPCP(GA* pga);
	~UIPCP(void);

	virtual void Draw(const RCF* prcfUpdate = NULL);
};