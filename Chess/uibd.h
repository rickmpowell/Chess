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

	BTNCH btnRotateBoard;

	CPC cpcPointOfView;
	RC rcSquares;
	float dxySquare, dxyBorder, dxyMargin, dxyOutline;
	float dyLabel;
	float angle;	// angle for rotation animation

	SHF shfDragInit;
	PT ptDragInit;
	PT ptDragCur;
	SHF shfHover;
	RC rcDragPc;	// rectangle the dragged piece was last drawn in
	GMV gmvDrag;	// legal moves in the UI

	vector<ANO> vano;	// annotations

public:
	UIBD(GA* pga);
	~UIBD(void);
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void InitGame(void);
	void MakeMv(MV mv, SPMV spmv);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);

	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	void DrawMargins(const RC& rcUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawSquares(int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawFileLabels(int fileFirst, int fileLast);
	void DrawRankLabels(int rankFirst, int rankLast);
	bool FHoverShf(SHF shf, MV& mv);
	void DrawHoverMv(MV mv);
	void DrawPieceShf(SHF shf);
	void DrawAnnotations(void);
	void DrawSquareAnnotation(SHF shf);
	void DrawArrowAnnotation(SHF shfFrom, SHF shfTo);

	void DrawHilites(void);
	void DrawGameState(void);
	void DrawPc(const RC& rc, float opacity, IPC ipc);
	void AnimateMv(MV mv, unsigned dframe);
	void AnimateShfToShf(SHF shfFrom, SHF shfTo, unsigned dframe);
	void DrawDragPc(const RC& rc);
	RC RcGetDrag(void);
	void InvalOutsideRc(const RC& rc) const;
	void HiliteLegalMoves(SHF shf);
	RC RcFromShf(SHF shf) const;

	virtual void FillRcBack(const RC& rc) const;

	void FlipBoard(CPC cpcNew);

	HTBD HtbdHitTest(const PT& pt, SHF* pshf) const;
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);

	bool FMoveablePc(SHF shf) const;
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

	virtual void Draw(const RC& rcUpdate);
};