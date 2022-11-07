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
	Bold,
	Italic,
	BoldItalic
};

struct ATTR
{
	wstring name;
	wstring val;

	ATTR(const wstring& name, const wstring& val) : name(name), val(val) {
	}
};


struct TAG
{
	wstring sz;
	map<wstring, wstring> mpnameval;

	TAG(const wchar_t* sz) : sz(wstring(sz)) {
	}

	TAG(const wstring& sz) : sz(sz) {
	}

	TAG(const wstring sz, const ATTR& attr) : sz(sz)
	{
		mpnameval[attr.name] = attr.val;
	}

	TAG(const wstring& sz, const ATTR rgattr[], int cattr) : sz(sz)
	{
		for (int iattr = 0; iattr < cattr; iattr++)
			mpnameval[rgattr[iattr].name] = rgattr[iattr].val;
	}
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
	GEOM* PgeomCreate(PT rgpt[], int cpt);
	BMP* PbmpFromPngRes(int idb);
	TX* PtxCreate(float dyHeight, bool fBold, bool fItalic);


protected:
	UI* puiParent;
	vector<UI*> vpuiChild;
	RC rcBounds;	// rectangle is in global coordinates
	bool fVisible;

public:
	UI(UI* puiParent, bool fVisible=true);
	UI(UI* puiParent, const RC& rcBounds, bool fVisible=true);
	virtual ~UI(void);

	void AddChild(UI* puiChild);
	void RemoveChild(UI* puiChild);
	UI* PuiParent(void) const;
	UI* PuiPrevSib(void) const;
	UI* PuiNextSib(void) const;

	virtual APP& App(void) const;
	const GA& Ga(void) const;
	GA& Ga(void);

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

	UI* PuiFromPt(const PT& pt);
	virtual void SetCapt(UI* pui);
	virtual void ReleaseCapt(void);	
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void ScrollWheel(const PT& pt, int dwheel);

	virtual void DispatchCmd(int cmd);
	
	virtual void SetFocus(UI* pui);
	virtual void KeyUp(int vk);
	virtual void KeyDown(int vk);

	TID StartTimer(UINT dtm);
	void StopTimer(TID tid);
	virtual TID StartTimer(UI* pui, UINT dtm);
	virtual void StopTimer(UI* pui, TID tid);
	virtual void TickTimer(TID tid, UINT tmCur);


	virtual void ShowTip(UI* puiAttach, bool fShow);
	virtual wstring SzTip(void) const;
	virtual wstring SzTipFromCmd(int cmd) const;

	RC RcParentFromLocal(const RC& rc) const; // local to parent coordinates
	RC RcGlobalFromLocal(const RC& rc) const; // local to app global coordinates
	RC RcLocalFromGlobal(const RC& rc) const;
	RC RcLocalFromParent(const RC& rc) const;
	PT PtParentFromLocal(const PT& pt) const;
	PT PtGlobalFromLocal(const PT& pt) const;
	PT PtLocalFromGlobal(const PT& pt) const;

	void Update(const RC& rcUpdate);
	void Redraw(const RC& rcUpdate);
	void Redraw(void);
	virtual void InvalRc(const RC& rc, bool fErase) const;
	virtual void Draw(const RC& rcDraw);
	void RedrawOverlappedSiblings(const RC& rcUpdate);

	virtual bool FDepthLog(LGT lgt, int& depth) noexcept;
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;

	inline void XLogOpen(const TAG& tag, const wstring& szData, LGF lgf = LGF::Normal) noexcept
	{
		int depthLog;
		if (FDepthLog(LGT::Open, depthLog))
			AddLog(LGT::Open, lgf, depthLog, tag, szData);
	}

#define LogOpen(tag, szData) \
	{ int depthLog; \
		if (FDepthLog(LGT::Open, depthLog)) \
			AddLog(LGT::Open, LGF::Normal, depthLog, tag, szData); }

	inline void XLogClose(const TAG& tag, const wstring& szData, LGF lgf = LGF::Normal) noexcept
	{
		int depthLog;
		if (FDepthLog(LGT::Close, depthLog))
			AddLog(LGT::Close, lgf, depthLog, tag, szData);
	}

#define LogClose(szTag, szData, lgf) \
	{ int depthLog; \
		if (FDepthLog(LGT::Close, depthLog)) \
			AddLog(LGT::Close, lgf, depthLog, szTag, szData); }

	inline void XLogData(const wstring& szData) noexcept
	{
		int depthLog;
		if (FDepthLog(LGT::Data, depthLog))
			AddLog(LGT::Data, LGF::Normal, depthLog, L"", szData);
	}

#define LogData(szData) \
	{ int depthLog; \
		if (FDepthLog(LGT::Data, depthLog)) \
			AddLog(LGT::Data, LGF::Normal, depthLog, L"", szData); }

	inline void LogTemp(const wstring& szData) noexcept
	{
		int depthLog;
		if (FDepthLog(LGT::Temp, depthLog))
			AddLog(LGT::Temp, LGF::Normal, depthLog, L"", szData);
	}


	virtual void PresentSwch(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void FillRc(const RC& rc, BR* pbr) const;
	virtual void FillRcBack(const RC& rc) const;
	void FillRr(const RR& rr, BR* pbr) const;
	void DrawRr(const RR& rr, BR* pbr) const;
	void FillEll(const ELL& ell, BR* pbr) const;
	void DrawEll(const ELL& ell, BR* pbr) const;
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
