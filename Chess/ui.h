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
 *	UI class
 * 
 *	The base user interface item class.
 * 
 */

class UI
{
protected:
	static BRS* pbrBack;
	static BRS* pbrAltBack;
	static BRS* pbrGridLine;
	static BRS* pbrText;
	static BRS* pbrTip;
	static BRS* pbrHilite;
	static TX* ptxText;
	static TX* ptxList;
	static TX* ptxTip;
	static wchar_t szFontFamily[];

public:
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	GEOM* PgeomCreate(const PT rgpt[], int cpt);
	BMP* PbmpFromPngRes(int idb);
	TX* PtxCreate(float dyHeight, bool fBold, bool fItalic);

protected:
	UI* puiParent;
	vector<UI*> vpuiChild;
	RC rcBounds;	// rectangle is in global coordinates
	bool fVisible;

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
	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);

	/* mouse interface */

	UI* PuiFromPt(const PT& pt);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);	
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void ScrollWheel(const PT& pt, int dwheel);

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

	/* tool tips */

	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTip(void) const;
	virtual wstring SzTipFromCmd(int cmd) const;

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
	void Redraw(const RC& rcUpdate);
	void RedrawWithChildren(const RC& rcUpdate);
	virtual void InvalRc(const RC& rc, bool fErase) const;
	virtual void Draw(const RC& rcDraw);
	void RedrawOverlappedSiblings(const RC& rcUpdate);
	virtual void RedrawCursor(const RC& rcUpdate);
	virtual void DrawCursor(UI* puiDraw, const RC& rcUpdate);
	void InvalOutsideRc(const RC& rc) const;

	virtual void PresentSwch(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	/* drawing primitives */

	void FillRc(const RC& rc, BR* pbr) const;
	virtual void FillRcBack(const RC& rc) const;
	void FillRr(const RR& rr, BR* pbr) const;
	void DrawRr(const RR& rr, BR* pbr) const;
	void FillEll(const ELL& ell, BR* pbr) const;
	void DrawEll(const ELL& ell, BR* pbr, float dxyWidth=1.0f) const;
	void DrawSz(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawSzCenter(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawRgch(const wchar_t* rgch, int cch, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	SIZ SizSz(const wstring& sz, TX* ptx, float dx=1.0e6f, float dy=1.0e6f) const;
	void DrawSzFit(const wstring& sz, TX* ptx, const RC& rc, BR* pbr = nullptr) const;
	void DrawBmp(const RC& rcTo, BMP* pbmp, const RC& rcFrom, float opacity = 1.0f) const;
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

	virtual void Draw(const RC& rcUpdate);
	void Track(bool fTrackNew);
	void Hilite(bool fHiliteNew);

	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	
	virtual wstring SzTip(void) const;
};


class BTNCH : public BTN
{
	wchar_t ch;
protected:
	static TX* ptxButton;
	static BRS* pbrsButton;
public:
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	BTNCH(UI* puiParent, int cmd, wchar_t ch);
	virtual void Draw(const RC& rcUpdate);
};


class BTNIMG : public BTN
{
	int idb;
	BMP* pbmp;
public:
	BTNIMG(UI* puiParent, int cmd, int idb);
	~BTNIMG(void);
	virtual void Draw(const RC& rcUpdate);
	virtual void CreateRsrc(void);
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
	BTNGEOM(UI* puiParent, int cmd, PT rgpt[], int cpt);
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
	BRS* pbrsStatic;
	wstring szText;

public:
	STATIC(UI* puiParent, const wstring& sz);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

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


class SPIN : public UI
{
protected:
	TX* ptxSpin;
	BTNGEOM btngeomDown;
	BTNGEOM btngeomUp;

public:
	SPIN(UI* puiParent, int cmdUp, int cmdDown);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	virtual wstring SzValue(void) const = 0;
};


/*
 *
 *	CYCLE control
 * 
 *	Displays a value that cycles through on mouse clicks
 * 
 */

class CYCLE : public BTN
{
public:
	CYCLE(UI* puiParent, int cmdCycle);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Layout(void);

	virtual void Draw(const RC& rcUpdate);
	virtual wstring SzValue(void) const = 0;
};
