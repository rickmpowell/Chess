/*
 *
 *	uibd.h
 * 
 *	UI for board panel
 * 
 */

#pragma once
#include "panel.h"
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



class UIBD : public UIP
{
public:
	static BRS* pbrLight;
	static BRS* pbrDark;
	static BRS* pbrBlack;
	static BRS* pbrAnnotation;
	static BRS* pbrHilite;
	BMP* pbmpPieces;
	GEOM* pgeomCross;
	GEOM* pgeomArrowHead;
	TX* ptxLabel;

	CPC cpcPointOfView;
	RCF rcfSquares;
	float dxyfSquare, dxyfBorder, dxyfMargin, dxyfOutline;
	float dyfLabel;

	BTN* pbtnRotateBoard;

	float angle;	// angle for rotation animation
	SQ sqDragInit;
	PTF ptfDragInit;
	PTF ptfDragCur;
	SQ sqHover;
	RCF rcfDragPc;	// rectangle the dragged piece was last drawn in
	GMV gmvDrag;	// legal moves in the UI

	vector<ANO> vano;	// annotations

public:
	UIBD(GA* pga);
	~UIBD(void);
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

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
	void DrawPc(RCF rcf, float opacity, IPC ipc);
	void AnimateMv(MV mv, unsigned dframe);
	void AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe);
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