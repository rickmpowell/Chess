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


const ColorF coBoardDark = ColorF(0.42f, 0.54f, 0.32f);
const ColorF coBoardLight = ColorF(1.0f, 1.0f, 0.95f);


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
	SQ sqDragHilite;

	vector<ANO> vano;	// annotations

public:
	UIBD(UIGA& uiga);
	~UIBD(void);
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void InitGame(void);
	void StartGame(void);
	void MakeMvu(MVU mvu, SPMV spmv);
	void UndoMvu(SPMV spmv);
	void RedoMvu(SPMV spmv);

	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	virtual void DrawCursor(UI* pui, const RC& rcUpdate);
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
	void DrawPc(UI* pui, const RC& rc, float opacity, PC pc);
	void AnimateMvu(MVU mvu, unsigned dframe);
	void AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe);
	RC RcGetDrag(void);
	void HiliteLegalMoves(SQ sq);
	RC RcFromSq(SQ sq) const;
	void SetDragHiliteSq(SQ sq);

	virtual ColorF CoFore(void) const { return coBoardDark; }
	virtual ColorF CoBack(void) const { return coBoardLight; }

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
public:
	UICPC(UI* puiParent, int cmd);

	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoBack(void) const { return ColorF::Black; }
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
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate);
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


class UICPCMOVE : public UI
{
	UIPCP& uipcp;
public:
	UICPCMOVE(UIPCP& uipcp);
	virtual void Draw(const RC& rcUpdate);
	virtual void Layout(void);
};


class UICASTLESTATE : public UI
{
	UIPCP& uipcp;
public:
	UICASTLESTATE(UIPCP& uipcp);
	virtual void Draw(const RC& rcUpdate);
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
};


class UISETFEN : public BTN
{
public:
	wstring szFen;
public:
	UISETFEN(UIPCP& uipcp, int cmd, const wstring& szFen);

	virtual void Draw(const RC& rcUpdate);
	RC RcFromSq(SQ sq) const;
	virtual ColorF CoFore(void) const { return coBoardDark; }
	virtual ColorF CoBack(void) const { return coBoardLight; }
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
	friend class UICPCMOVE;
	friend class UICASTLESTATE;

	UIBD& uibd;
	UITITLE uititle;
	map<CPC, UICPC*> mpcpcpuicpc;
	map<APC, UIDRAGAPC*> mpapcpuiapc;
	UIDRAGDEL uidragdel;
	UICPCMOVE uicpcmove;
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
	virtual ColorF CoBack(void) const { return coBoardLight; }

	RC RcFromApc(APC apc) const;
	BMP* PbmpPieces(void);
	void HiliteSq(SQ sq);

};