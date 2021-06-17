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


WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch);


/*
 *	Move speeds
 */
enum class SPMV
{
	Hidden,
	Fast,
	AnimateVeryFast,
	AnimateFast,
	Animate
};

inline bool FSpmvAnimate(SPMV spmv)
{
	return spmv == SPMV::Animate || spmv == SPMV::AnimateFast || spmv == SPMV::AnimateVeryFast;
}

inline unsigned DframeFromSpmv(SPMV spmv)
{
	switch (spmv) {
	case SPMV::AnimateVeryFast:
		return 5;
	case SPMV::AnimateFast:
		return 10;
	case SPMV::Animate:
		return 30;
	default:
		break;
	}
	return 0;
}

enum class MHT
{
	Nil,
	Enter,
	Move,
	Exit
};


/*
 *
 *	Logging types 
 * 
 */


enum class LGT
{
	Open,
	Close,
	Data,
	Temp
};

enum class LGF {
	Normal,
	Bold
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
	GEOM* PgeomCreate(PTF rgptf[], int cptf);
	BMP* PbmpFromPngRes(int idb);

protected:
	UI* puiParent;
	vector<UI*> vpuiChild;
	RCF rcfBounds;	// rectangle is in global coordinates
	bool fVisible;

public:
	UI(UI* puiParent, bool fVisible=true);
	UI(UI* puiParent, RCF rcfBounds, bool fVisible=true);
	virtual ~UI(void);

	void AddChild(UI* puiChild);
	void RemoveChild(UI* puiChild);
	UI* PuiParent(void) const;
	UI* PuiPrevSib(void) const;
	UI* PuiNextSib(void) const;

	virtual APP& App(void) const;
	const GA& Ga(void) const;
	GA& Ga(void);

	RCF RcfInterior(void) const;	// in local coordinates (top left is always {0,0})
	RCF RcfBounds(void) const;	// in parent coordinates
	bool FVisible(void) const;
	void SetBounds(RCF rcfNew);
	void Resize(PTF ptfNew);
	void Move(PTF ptfNew);
	void OffsetBounds(float dxf, float dyf);
	void Show(bool fShow);
	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);

	UI* PuiFromPtf(PTF ptf);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);	
	virtual void StartLeftDrag(PTF ptf);
	virtual void EndLeftDrag(PTF ptf);
	virtual void LeftDrag(PTF ptf);
	virtual void MouseHover(PTF ptf, MHT mht);
	virtual void ScrollWheel(PTF ptf, int dwheel);

	virtual void DispatchCmd(int cmd);
	
	virtual void SetFocus(UI* pui);
	virtual void KeyUp(int vk);
	virtual void KeyDown(int vk);

	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTip(void) const;
	virtual wstring SzTipFromCmd(int cmd) const;

	RCF RcfParentFromLocal(RCF rcf) const; // local to parent coordinates
	RCF RcfGlobalFromLocal(RCF rcf) const; // local to app global coordinates
	RCF RcfLocalFromGlobal(RCF rcf) const;
	RCF RcfLocalFromParent(RCF rcf) const;
	PTF PtfParentFromLocal(PTF ptf) const;
	PTF PtfGlobalFromLocal(PTF ptf) const;
	PTF PtfLocalFromGlobal(PTF ptf) const;

	void Update(RCF rcfUpdate);
	void Redraw(RCF rcfUpdate);
	void Redraw(void);
	virtual void InvalRcf(RCF rcf, bool fErase) const;
	virtual void Draw(RCF rcfDraw);

	virtual void PresentSwch(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void FillRcf(RCF rcf, BR* pbr) const;
	virtual void FillRcfBack(RCF rcf) const;
	void FillEllf(ELLF ellf, BR* pbr) const;
	void DrawEllf(ELLF ellf, BR* pbr) const;
	void DrawSz(const wstring& sz, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	void DrawSzCenter(const wstring& sz, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	void DrawRgch(const WCHAR* rgch, int cch, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	SIZF SizfSz(const wstring& sz, TX* ptx, float dxf=1.0e6f, float dyf=1.0e6f) const;
	void DrawSzFit(const wstring& sz, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	void DrawBmp(RCF rcfTo, BMP* pbmp, RCF rcfFrom, float opacity = 1.0f) const;
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
	int cmd;
public:
	BTN(UI* puiParent, int cmd);

	void Track(bool fTrackNew);
	void Hilite(bool fHiliteNew);
	virtual void Draw(RCF rcfUpdate);

	virtual void StartLeftDrag(PTF ptf);
	virtual void EndLeftDrag(PTF ptf);
	virtual void LeftDrag(PTF ptf);
	virtual void MouseHover(PTF ptf, MHT mht);
	
	virtual wstring SzTip(void) const;
};


class BTNCH : public BTN
{
	WCHAR ch;
protected:
	static TX* ptxButton;
	static BRS* pbrsButton;
public:
	static void CreateRsrcClass(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	BTNCH(UI* puiParent, int cmd, WCHAR ch);
	virtual void Draw(RCF rcfUpdate);
};


class BTNIMG : public BTN
{
	int idb;
	BMP* pbmp;
public:
	BTNIMG(UI* puiParent, int cmd, int idb);
	~BTNIMG(void);
	virtual void Draw(RCF rcfUpdate);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	SIZF SizfImg(void) const;
};


/*
 *
 *	BTNGEOM
 * 
 *	Geomtries are assumed to be scaled from a ...
 */


class BTNGEOM : public BTN
{
	GEOM* pgeom;
public:
	BTNGEOM(UI* puiParent, int cmd, PTF rgptf[], int cptf);
	~BTNGEOM();
	virtual void Draw(RCF rcfUpdate);
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

	virtual void Draw(RCF rcfUpdate);

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

enum SPDIR
{
	spdirNil = -1,
	spdirUp = 0,
	spdirDown = 1,
	spdirMax = 2
};

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

	virtual void Draw(RCF rcfUpdate);
	virtual wstring SzValue(void) const = 0;
};