#pragma once
/*
 *
 *	uibd.h
 * 
 *	UI for board panel
 * 
 */


 /*
  *
  *	SPABD class
  *
  *	Class that keeps and displays the game board on the screen inside
  *	the board panel
  *
  */

#include "framework.h"
#include "ui.h"
#include "bd.h"


class HTBD;

class SPABD : public SPA
{
public:
	static ID2D1SolidColorBrush* pbrLight;
	static ID2D1SolidColorBrush* pbrDark;
	static ID2D1SolidColorBrush* pbrBlack;
	static ID2D1SolidColorBrush* pbrAnnotation;
	static ID2D1SolidColorBrush* pbrHilite;
	static IDWriteTextFormat* ptfLabel;
	static IDWriteTextFormat* ptfControls;
	static IDWriteTextFormat* ptfGameState;
	static ID2D1Bitmap* pbmpPieces;
	static ID2D1PathGeometry* pgeomCross;
	static ID2D1PathGeometry* pgeomArrowHead;

	CPC cpcPointOfView;
	RCF rcfSquares;
	float dxyfSquare, dxyfBorder, dxyfMargin;
	float dyfLabel;
	float angle;	// angle for rotation animation
	HTBD* phtDragInit;
	HTBD* phtCur;
	SQ sqHover;
	int ictlHover;
	RCF rcfDragPc;	// rectangle the dragged piece was last drawn in
	vector<MV> rgmvDrag;	// legal moves in the UI
	vector<ANO> rgano;	// annotations

public:
	SPABD(GA* pga);
	~SPABD(void);
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

	void NewGame(void);
	void MakeMv(MV mv, bool fRedraw);
	void UndoMv(bool fRedraw);
	void RedoMv(bool fRedraw);

	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);

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
	void DrawControls(void);

	void DrawHilites(void);
	void DrawGameState(void);
	void DrawPc(RCF rcf, float opacity, TPC tpc);
	void DrawDragPc(const RCF& rcf);
	RCF RcfGetDrag(void);
	void InvalOutsideRcf(RCF rcf) const;
	void HiliteLegalMoves(SQ sq);
	void HiliteControl(int ictl);
	RCF RcfFromSq(SQ sq) const;
	RCF RcfControl(int ictl) const;
	void DrawControl(WCHAR ch, int ictl, HTT htt) const;

	void Resign(void);
	void FlipBoard(CPC cpcNew);

	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;

	virtual HT* PhtHitTest(PTF ptf);
	virtual void StartLeftDrag(HT* pht);
	virtual void EndLeftDrag(HT* pht);
	virtual void LeftDrag(HT* pht);
	virtual void MouseHover(HT* pht);

	bool FMoveablePc(SQ sq);
};
