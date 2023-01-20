/*
 *
 *	uibd.h
 * 
 *	UI for board panel
 * 
 */

#pragma once
#include "uipa.h"
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
enum HTBD
{
	htbdNone,
	htbdStatic,
	htbdFlipBoard,
	htbdEmpty,
	htbdOpponentPc,
	htbdMoveablePc,
	htbdUnmoveablePc
};



class UIBD : public UIP
{
	friend class UIPCP;
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

	SQ sqDragInit;
	PT ptDragInit;
	PT ptDragCur;
	SQ sqHover;
	RC rcDragPc;	// rectangle the dragged piece was last drawn in
	VMVE vmveDrag;	// legal moves in the UI

	vector<ANO> vano;	// annotations

public:
	UIBD(UIGA& uiga);
	~UIBD(void);
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void InitGame(void);
	void MakeMvu(MVU mvu, SPMV spmv);
	void UndoMvu(SPMV spmv);
	void RedoMvu(SPMV spmv);

	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	void DrawMargins(const RC& rcUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawSquares(int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawFileLabels(int fileFirst, int fileLast);
	void DrawRankLabels(int rankFirst, int rankLast);
	bool FHoverSq(SQ sq, MVU& mvu);
	void DrawHoverMvu(MVU mvu);
	void DrawPieceSq(SQ sq);
	void DrawAnnotations(void);
	void DrawSquareAnnotation(SQ sq);
	void DrawArrowAnnotation(SQ sqFrom, SQ sqTo);

	void DrawHilites(void);
	void DrawGameState(void);
	void DrawPc(const RC& rc, float opacity, CPC cpc, APC apc);
	void AnimateMvu(MVU mvu, unsigned dframe);
	void AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe);
	void DrawDragPc(const RC& rc);
	RC RcGetDrag(void);
	void InvalOutsideRc(const RC& rc) const;
	void HiliteLegalMoves(SQ sq);
	RC RcFromSq(SQ sq) const;

	virtual void FillRcBack(const RC& rc) const;

	void FlipBoard(CPC cpcNew);

	HTBD HtbdHitTest(const PT& pt, SQ* psq) const;
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);

	bool FMoveablePc(SQ sq) const;
};


/*
 *
 *	Controls for the piece set up panel
 *
 */

class UIPCP;


class UICPC : public BTN
{
	CPC cpc;
public:
	UICPC(UI* puiParent, CPC cpc);

	virtual void Draw(const RC& rcUpdate);
};


class UIAPC : public UI
{
	APC apc;
	UIPCP& uipcp;
public:
	UIAPC(UIPCP& uipcp, APC apc);

	virtual void Draw(const RC& rcUpdate);
};


class UIPCDEL : public UI
{
	UIPCP& uipcp;
public:
	UIPCDEL(UIPCP& uipcp);

	virtual void Draw(const RC& rcUpdate);
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
	friend class UIAPC;
	friend class UICPC;

	UIBD& uibd;
	map<CPC, UICPC*> mpcpcpuicpc;
	map<APC, UIAPC*> mpapcpuiapc;
	UIPCDEL uipcdel;
	CPC cpcShow;
public:
	UIPCP(UIGA& uiga);
	~UIPCP(void);

	virtual void Layout(void);
	virtual void Draw(const RC& rcUpdate);
	virtual void DispatchCmd(int cmd);
	virtual wstring SzTipFromCmd(int cmd) const;

	RC RcFromApc(APC apc) const;
	BMP* PbmpPieces(void);

};