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
	static BRS* pbrLight;
	static BRS* pbrDark;
	static BRS* pbrBlack;
	static BRS* pbrAnnotation;
	static BRS* pbrHilite;
	static TF* ptfLabel;
	static TF* ptfControls;
	static TF* ptfGameState;
	static BMP* pbmpPieces;
	static GEOM* pgeomCross;
	static GEOM* pgeomArrowHead;

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
	static void CreateRsrc(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
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
