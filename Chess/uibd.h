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
	BRS* pbrAnnotation;
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

	VMVE vmveDrag;	// legal moves in the UI
	vector<ANO> vano;	// annotations shown on the board

	PT ptDragInit;	// mouse point we initially hit when drag moving
	SQ sqDragInit;	// square of ptDragInit
	PT ptDragCur;	// current mouse position when dragging
	RC rcDragPc;	// rectangle the dragged piece was last drawn in
	SQ sqDragHilite;	// square we're hiliting during dragging
	MVE mveHilite;		// hilite last move
	bool fClickClick;	// true after click-click move selection mode started
	SQ sqHover;

	ANO* panoDrag;		// during right mouse drag, we drag out this annotation; points into vano

public:
	UIBD(UIGA& uiga);
	~UIBD(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void InitGame(void);
	void StartGame(void);
	void MakeMv(MVE mve, SPMV spmv);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);
	void SetMoveHilite(MVE mve);

	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	virtual void DrawCursor(UI* pui, const RC& rcUpdate);
	void DrawMargins(const RC& rcUpdate, int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawSquares(int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawHovers(int rankFirst, int rankLast, int fileFirst, int fileLast);
	void DrawFileLabels(int fileFirst, int fileLast);
	void DrawRankLabels(int rankFirst, int rankLast);
	bool FHoverSq(SQ sqFrom, SQ sq, MVE& mve);
	void DrawHoverMove(MVE mve);
	void DrawPieceSq(SQ sq);
	void DrawAnnotations(void);
	void DrawSquareAnnotation(SQ sq);
	void DrawArrowAnnotation(SQ sqFrom, SQ sqTo);
	SQ SqToNearestMove(SQ sqFrom, PT ptHit) const;
	void DrawDragHilite(void);
	void DrawGameState(void);

	void DrawPc(UI* pui, const RC& rc, float opacity, PC pc);
	void AnimateMv(MVE mve, unsigned dframe);
	void AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe);
	void HiliteLegalMoves(SQ sq);
	void SetDragHiliteSq(SQ sq);
	void CancelClickClick(void);
	RC RcGetDrag(void);
	RC RcFromSq(SQ sq) const;

	virtual ColorF CoText(void) const; 
	virtual ColorF CoBack(void) const; 

	void FlipBoard(CPC cpcNew);

	HTBD HtbdHitTest(const PT& pt, SQ* psq) const;
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);
	virtual void NoButtonDrag(const PT& pt);
	virtual void StartRightDrag(const PT& pt);
	virtual void EndRightDrag(const PT& pt, bool fClick);
	virtual void RightDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);

	bool FInVmveDrag(SQ sqSrc, SQ sqDest, MVE& mveFound) const;
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
public:
	UICPC(UI* puiParent, int cmd);

	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoBack(void) const { return coBlack; }
};


class UIDRAGPCP : public UI
{
protected:
	UIPCP& uipcp;
	PT ptDragInit;
	PT ptDragCur;

public:
	UIDRAGPCP(UIPCP& uipcp);

	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void DrawCursor(UI* pui, const RC& rcUpdate);
	virtual void DrawInterior(UI* pui, const RC& rc) = 0;
	virtual void Drop(SQ sq) = 0;
	RC RcDrag(const PT& pt) const;
	SQ SqHitTest(PT pt) const;
};


class UIDRAGAPC : public UIDRAGPCP
{
	APC apc;
public:
	UIDRAGAPC(UIPCP& uipcp, APC apc);

	virtual void DrawInterior(UI* pui, const RC& rc);
	virtual void Drop(SQ sq);
};


class UIDRAGDEL : public UIDRAGPCP
{
public:
	UIDRAGDEL(UIPCP& uipcp);

	virtual void DrawInterior(UI* pui, const RC& rc);
	virtual void Drop(SQ sq);
};


class UICPCTOMOVE : public BTN
{
	UIPCP& uipcp;
public:
	UICPCTOMOVE(UIPCP& uipcp);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
};


class CHKCS : public BTN
{
	UIPCP& uipcp;
public:
	CHKCS(UI* puiParent, UIPCP& uipcp, int cmd);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
};


class UICASTLESTATE : public UI
{
	UIPCP& uipcp;
	vector<CHKCS*> vpchkcs;
public:
	UICASTLESTATE(UIPCP& uipcp);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Layout(void);
};


/*
 *
 *	UIFEN
 *
 */


class UIFEN : public UI
{
	UIPCP& uipcp;
public:
	UIFEN(UIPCP& uipcp);

	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
};


class UISETFEN : public BTN
{
public:
	wstring szFen;
public:
	UISETFEN(UIPCP& uipcp, int cmd, const wstring& szFen);

	virtual void Draw(const RC& rcUpdate);
	RC RcFromSq(SQ sq) const;
	virtual ColorF CoText(void) const { return coBoardBWDark; }
	virtual ColorF CoBack(void) const { return coBoardBWLight; }
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
	friend class UICPC;
	friend class UIDRAGPCP;
	friend class UIDRAGAPC;
	friend class UIDRAGDEL;
	friend class UISETFEN;
	friend class UIFEN;
	friend class UICPCTOMOVE;
	friend class UICASTLESTATE;
	friend class CHKCS;

	UIBD& uibd;
	UITITLE uititle;
	map<CPC, UICPC*> mpcpcpuicpc;
	map<APC, UIDRAGAPC*> mpapcpuiapc;
	UIDRAGDEL uidragdel;
	UICPCTOMOVE uicpctomove;
	UICASTLESTATE uicastlestate;
	vector<UISETFEN*> vpuisetfen;
	UIFEN uifen;
	CPC cpcShow;
	SQ sqDrop;
	APC apcDrop;

public:
	UIPCP(UIGA& uiga);
	~UIPCP(void);

	virtual void Layout(void);
	virtual void DispatchCmd(int cmd);
	virtual wstring SzTipFromCmd(int cmd) const;
	virtual ColorF CoBack(void) const { return coBoardBWLight; }

	RC RcFromApc(APC apc) const;
	BMP* PbmpPieces(void);
	void HiliteSq(SQ sq);

};