/*
 *
 *	ui.h
 * 
 *	A light-weight UI element. Supports simple rectangular areas
 *	in a parent/child heirarchy with mouse dispatching.
 * 
 *	Screen panels are build on top of UI elements.
 * 
 */

#pragma once
#include "framework.h"
#include "app.h"

class GA;


/*
 *	Move speeds
 */
enum SPMV
{
	spmvHidden,
	spmvFast,
	spmvAnimateVeryFast,
	spmvAnimateFast,
	spmvAnimate
};

inline bool FSpmvAnimate(SPMV spmv)
{
	return spmv == spmvAnimate || spmv == spmvAnimateFast || spmv == spmvAnimateVeryFast;
}

inline unsigned DframeFromSpmv(SPMV spmv)
{
	switch (spmv) {
	case spmvAnimateVeryFast:
		return 5;
	case spmvAnimateFast:
		return 10;
	case spmvAnimate:
		return 30;
	default:
		break;
	}
	return 0;
}

enum MHT
{
	mhtNil,
	mhtEnter,
	mhtMove,
	mhtExit
};


/*
 *
 *	System commands
 * 
 *	system command values are all negative
 */


enum {
	cmdClosePanel = -32
};


/*
 *
 *	UI class
 * 
 *	The base user interface item class.
 * 
 */

class UITIP;

class UI
{
protected:
	static BRS* pbrBack;
	static BRS* pbrScrollBack;
	static BRS* pbrGridLine;
	static BRS* pbrText;
	static BRS* pbrHilite;
	static BRS* pbrWhite;
	static BRS* pbrBlack;
	static TX* ptxText;
	static TX* ptxTextBold;
	static TX* ptxList;
	static TX* ptxListBold;
	static TX* ptxTip;
	static wchar_t szFontFamily[];

public:
	static bool FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcStatic(void);
	GEOM* PgeomCreate(const PT apt[], int cpt) const;
	BMP* PbmpFromPngRes(int idb);
	TX* PtxCreate(float dyHeight, bool fBold, bool fItalic) const;

protected:
	UI* puiParent;
	vector<UI*> vpuiChild;
	RC rcBounds;	// rectangle is in global coordinates
	bool fVisible;
	ColorF coText, coBack;

public:
	
	/* construct and destruct */

	UI(UI* puiParent, bool fVisible=true);
	UI(UI* puiParent, const RC& rcBounds, bool fVisible=true);
	virtual ~UI(void);
	virtual APP& App(void) const;
	const GA& Ga(void) const;
	GA& Ga(void);

	/* the UI tree */

	void AddChild(UI* puiChild);
	void RemoveChild(UI* puiChild);
	UI* PuiParent(void) const;
	UI* PuiPrevSib(void) const;
	UI* PuiNextSib(void) const;

	/* showing, hiding, sizing, moving, and layout */

	RC RcInterior(void) const;	// in local coordinates (top left is always {0,0})
	RC RcBounds(void) const;	// in parent coordinates
	bool FVisible(void) const;
	void SetBounds(const RC& rcNew);
	void Resize(const PT& ptNew);
	void Move(const PT& ptNew);
	void OffsetBounds(float dx, float dy);
	void Show(bool fShow);
	void ShowAll(bool fShow);
	void Relayout(void);
	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	void BringToTop(void);

	/* mouse interface */

	UI* PuiFromPt(const PT& pt);
	virtual void SetDrag(UI* pui);
	virtual void ReleaseDrag(void);	
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);
	virtual void StartRightDrag(const PT& pt);
	virtual void EndRightDrag(const PT& pt, bool fClick);
	virtual void RightDrag(const PT& pt);
	virtual void NoButtonDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void ScrollWheel(const PT& pt, int dwheel);
	virtual void SetDefCursor(void);

	/* keyboard interface */

	virtual void SetFocus(UI* pui);
	virtual void KeyUp(int vk);
	virtual void KeyDown(int vk);

	/* timers */

	TID StartTimer(DWORD dmsec);
	void StopTimer(TID tid);
	virtual TID StartTimer(UI* pui, DWORD dmsec);
	virtual void StopTimer(UI* pui, TID tid);
	virtual void TickTimer(TID tid, DWORD dmsec);

	/* command dispatch */

	virtual void DispatchCmd(int cmd);
	virtual bool FEnabledCmd(int cmd) const;

	/* tool tips */

	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTip(void) const;
	virtual wstring SzTipFromCmd(int cmd) const;
	virtual SIZ SizOfTip(UITIP& uitip) const;
	virtual void DrawTip(UITIP& uitip);

	/* coordinate transforms */

	RC RcParentFromLocal(const RC& rc) const;
	RC RcGlobalFromLocal(const RC& rc) const;
	RC RcLocalFromGlobal(const RC& rc) const;
	RC RcLocalFromParent(const RC& rc) const;
	RC RcLocalFromUiLocal(const UI* pui, const RC& rc) const;
	PT PtParentFromLocal(const PT& pt) const;
	PT PtGlobalFromLocal(const PT& pt) const;
	PT PtLocalFromGlobal(const PT& pt) const;
	PT PtLocalFromUiLocal(const UI* pui, const PT& pt) const;
	PT PtGlobalFromUiLocal(const UI* pui, const PT& pt) const;

	/* window updating */

	void Redraw(void);
	void Redraw(const RC& rcUpdate, bool fParentDrawn);
	void RedrawWithChildren(const RC& rcUpdate, bool fParentDrawn);
	void RedrawNoChildren(const RC& rc, bool fParentDrawn);
	virtual void InvalRc(const RC& rc, bool fErase) const;
	virtual void Draw(const RC& rcDraw);
	virtual void Erase(const RC& rcDraw, bool fParentDrawn);
	void TransparentErase(const RC& rcUpdate, bool fParentDrawn);
	void RedrawOverlappedSiblings(const RC& rcUpdate);
	virtual void RedrawCursor(const RC& rcUpdate);
	virtual void DrawCursor(UI* puiDraw, const RC& rcUpdate);
	void InvalOutsideRc(const RC& rc) const;

	virtual void PresentSwch(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	void EnsureRsrc(bool fStatic);
	virtual void ComputeMetrics(bool fStatic);
	void DiscardAllRsrc(void);

	/* drawing primitives */

	void SetCoText(ColorF co);
	void SetCoBack(ColorF co);
	virtual ColorF CoText(void) const;
	virtual ColorF CoBack(void) const;
	void FillRc(const RC& rc, BR* pbr) const;
	void FillRc(const RC& rc, ColorF co) const;
	virtual void FillRcBack(const RC& rc) const;
	void FillRr(const RR& rr, BR* pbr) const;
	void DrawRr(const RR& rr, BR* pbr) const;
	void FillEll(const ELL& ell, BR* pbr) const;
	void DrawEll(const ELL& ell, BR* pbr, float dxyWidth=1.0f) const;
	void DrawEll(const ELL& ell, ColorF co, float dxyWidth = 1.0f) const;
	void DrawSz(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawSzCenter(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawSzBaseline(const wstring& sz, TX* ptx, const RC& rc, float dyBaseline, BR* pbr = nullptr) const;
	float DyBaselineSz(const wstring& sz, TX* ptx) const;
	void DrawAch(const wchar_t* ach, int cch, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	SIZ SizFromSz(const wstring& sz, TX* ptx, float dx=1.0e6f, float dy=1.0e6f) const;
	SIZ SizFromAch(const wchar_t* ach, int cch, TX* ptx, float dx = 1.0e6f, float dy = 1.0e6f) const;
	void DrawSzFit(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawSzFitBaseline(const wstring& sz, TX* ptxBase, const RC& rcFit, float dyBaseline, BR* pbrText = nullptr) const;

	void DrawBmp(const RC& rcTo, BMP* pbmp, const RC& rcFrom, float opacity = 1.0f) const;
	void FillGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, BR* pbr = nullptr) const;
	void FillGeom(GEOM* pgeom, PT ptOffset, float dxyScale, BR* pbr = nullptr) const;
	void FillGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, ColorF coFill) const;
	void FillGeom(GEOM* pgeom, PT ptOffset, float dxyScale, ColorF coFill) const;
	void FillRotateGeom(GEOM* pgeom, PT ptOffset, SIZ sizScale, float angle, BR* pbr = nullptr) const;
	void FillRotateGeom(GEOM* pgeom, PT ptOffset, float dxyScale, float angle, BR* pbr = nullptr) const;
	void FillRotateGeom(GEOM* pgeom, PT ptOffset, float dxyScale, float angle, ColorF coFill) const;
};


/*
 *
 *	UITIP class
 *
 *	Tooltip user interface item
 *
 */


class UITIP : public UI
{
protected:
	UI* puiOwner;

public:
	UITIP(UI* puiParent);
	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoBack(void) const { return coTipBack; }
	void AttachOwner(UI* pui);
};


/*
 *
 *	BTN class
 * 
 *	A simple button UI element
 * 
 */


class BTN : public UI
{
protected:
	bool fHilite;
	bool fTrack;
public:
	int cmd;

public:
	BTN(UI* puiParent, int cmd);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual ColorF CoText(void) const;
	void Track(bool fTrackNew);
	void Hilite(bool fHiliteNew);

	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void SetDefCursor(void);

	virtual wstring SzTip(void) const;
};


class BTNCH : public BTN
{
	wchar_t ch;
protected:
	float dyFont;
	TX* ptxButton;
public:

public:
	BTNCH(UI* puiParent, int cmd, wchar_t ch);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	void SetTextSize(float dyFontNew);
	virtual void Draw(const RC& rcUpdate);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	void DrawText(const wstring& sz);
	virtual float DxWidth(void);

};


class BTNTEXT : public BTNCH
{
protected:
	wstring szText;
public:
	BTNTEXT(UI* puiParent, int icmd, const wstring& sz);
	void SetText(const wstring& szNew);
	virtual void Draw(const RC& rcUpdate);
	virtual float DxWidth(void);
};


class BTNIMG : public BTN
{
	int idb;
	BMP* pbmp;
public:
	BTNIMG(UI* puiParent, int cmd, int idb);
	~BTNIMG(void);
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Draw(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	SIZ SizImg(void) const;
};


/*
 *
 *	BTNGEOM
 * 
 *	Geomtries are assumed to be scaled from a ...
 *
 */


class BTNGEOM : public BTN
{
	GEOM* pgeom;

public:
	BTNGEOM(UI* puiParent, int cmd, const PT apt[], int cpt);
	~BTNGEOM();
	virtual void Draw(const RC& rcUpdate);
};


/*
 *
 *	STATIC ui control
 * 
 *	Just a simple static non-responsive area on the screen.
 * 
 */


class STATIC : public UI
{
protected:
	TX* ptxStatic;
	wstring szText;
	float dyFont;

public:
	STATIC(UI* puiParent, const wstring& sz);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual ColorF CoText(void) const;
	void SetTextSize(float dyFontNew);

	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Draw(const RC& rcUpdate);

	virtual wstring SzText(void) const;
	void SetText(const wstring& sz);
};


/*
 *
 *	SPIN ui control
 * 
 *	Spinner control, with arrows to go up/down and an integer value displayed
 *	between them.
 * 
 */

class BTNUP : public BTNGEOM
{
	static const PT aptUp[];
public:
	BTNUP(UI* puiParent, int cmd);
};

class BTNDOWN : public BTNGEOM
{
	static const PT aptDown[];
public:
	BTNDOWN(UI* puiParent, int cmd);
};

class SPIN : public UI
{
protected:
	TX* ptxSpin;
	BTNDOWN btndown;
	BTNUP btnup;

public:
	SPIN(UI* puiParent, int cmdUp, int cmdDown);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Layout(void);

	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Draw(const RC& rcUpdate);
	virtual wstring SzValue(void) const = 0;
};

