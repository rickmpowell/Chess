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

	map<string, vector<EPDP>> mpszvepdp;	/* EPD properties */


public:
	UIBD(UIGA& uiga);
	virtual ~UIBD(void);

	/* EPD lines have a FEN string along with EPD properties; EPD properties are saved
	   in the UIBD because they are not necessary to actual game play, from the end of */

	void SetEpd(const char* sz);
	void SetEpdProperties(const char* szEpd);

	/* Game control */

	void InitGame(void);
	void StartGame(void);
	void MakeMv(MVE mve, SPMV spmv);
	void UndoMv(SPMV spmv);
	void RedoMv(SPMV spmv);
	void SetMoveHilite(MVE mve);

	/*	layout */

	virtual void Layout(void);

	/* drawing */

	virtual void Draw(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual ColorF CoText(void) const;
	virtual ColorF CoBack(void) const;
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

	void DrawPc(UI& ui, const RC& rc, float opacity, PC pc);
	void AnimateMv(MVE mve, unsigned dframe);
	void AnimateSqToSq(SQ sqFrom, SQ sqTo, unsigned dframe);
	void HiliteLegalMoves(SQ sq);
	void SetDragHiliteSq(SQ sq);
	void CancelClickClick(void);
	RC RcGetDrag(void);
	RC RcFromSq(SQ sq) const;

	void FlipBoard(CPC cpcNew);

	/* mouse ui */

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



/*
 *
 *	UICPC control
 * 
 *	Piece color picker
 * 
 */


class UICPC : public BTN
{
public:
	UICPC(UI* puiParent, int cmd);

	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoBack(void) const { return coBlack; }
};



/*
 *
 *	UIDRAGPCP control
 * 
 *	Base class for draggable controls int he board setup panel
 * 
 */


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
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void DrawCursor(UI* pui, const RC& rcUpdate);
	virtual void DrawInterior(UI* pui, const RC& rc) = 0;
	virtual void Drop(SQ sq) = 0;
	RC RcDrag(const PT& pt) const;
	SQ SqHitTest(PT pt) const;
	virtual void SetDefCursor(void);
};



/*
 *
 *	UIDRAGAPC
 * 
 *	Draggable chess piece control for the board setup panel
 * 
 */


class UIDRAGAPC : public UIDRAGPCP
{
	APC apc;
public:
	UIDRAGAPC(UIPCP& uipcp, APC apc);

	virtual void DrawInterior(UI* pui, const RC& rc);
	virtual void Drop(SQ sq);
};


/*
 *
 *	UIDRAGDEL control
 * 
 *	Draggable delete piece icon on the board setup panel
 * 
 */


class UIDRAGDEL : public UIDRAGPCP
{
public:
	UIDRAGDEL(UIPCP& uipcp);

	virtual void DrawInterior(UI* pui, const RC& rc);
	virtual void Drop(SQ sq);
};


/*
 *
 *	UICPCTOMOVE control
 * 
 *	Color to be moved control in the board setup panel.
 * 
 */


class UICPCTOMOVE : public BTN
{
	UIPCP& uipcp;
public:
	UICPCTOMOVE(UIPCP& uipcp);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
};


/*
 *
 *	CHKCS control
 * 
 *	Check boxes for the castle state in the board setup panel
 *
 */


class CHKCS : public BTN
{
	UIPCP& uipcp;
public:
	CHKCS(UI* puiParent, UIPCP& uipcp, int cmd);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
};


/*
 *
 *	UICASTLESTATE control
 * 
 *	Container control for the group of castle state checkboxes in the board setup
 *	panel.
 *
 */


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
 *	The FEN edit box in the board setup panel. Currently this is just static
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


/*
 *
 *	UISETFEN control
 * 
 *	The mini-board picker button control in the board setup panel. They hold a FEN string,
 *	which is used to make a mini-board
 * 
 */


class UISETFEN : public BTN
{
public:
	string szEpd;
public:
	UISETFEN(UI* pui, int cmd, const string& szFen);
	void SetEpd(const string& sz);

	virtual void Draw(const RC& rcUpdate);
	void DrawBdg(UI& ui, BDG& bdg, const RC& rcBox);
	RC RcFromSq(const RC& rcBox, SQ sq) const;
	virtual ColorF CoText(void) const { return coBoardBWDark; }
	virtual ColorF CoBack(void) const { return coBoardBWLight; }

	virtual SIZ SizOfTip(UITIP& uitip) const;
	virtual void DrawTip(UITIP& uitip);
};


/*
 *
 *	BTNFILE
 *
 *	A button that displays a filename, designed to be clicked on and bring up an Open
 *	dialog.
 * 
 */


class BTNFILE : public BTNTEXT
{
public:
	BTNFILE(UI* puiParent, int cmd, const wstring& szPath);
	void SetFile(const wstring& szNew);
};


/*
 *
 *	UIEPD container control
 * 
 *	A UI element that contains the various pieces for implementing an EPD file enumerator
 *	and picker. 
 *
 */


class UIEPD : public UI
{
	friend class UIPCP;
	
	UIPCP& uipcp;
	wstring szFile;
	UISETFEN uisetfen;
	BTNUP btnup;
	BTNDOWN btndown;
	BTNFILE btnfile;
	int iliCur;

public:
	UIEPD(UIPCP& uipcp, const wstring& szFile);
	void SetLine(int ili);
	void SetFile(const wstring& szFileNew);
	void GetFile(wchar_t szFile[], int cch);

	string SzFindLine(ifstream& ifs, int ili);

	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Draw(const RC& rcUpdate);
	virtual void Layout(void);
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
	UIEPD uiepd;
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
	virtual bool FEnabledCmd(int cmd) const;
	virtual ColorF CoBack(void) const { return coBoardBWLight; }

	RC RcFromApc(APC apc) const;
	BMP* PbmpPieces(void);
	void HiliteSq(SQ sq);

	void OpenEpdFile(void);
};