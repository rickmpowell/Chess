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


WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch);


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
	static ID2D1SolidColorBrush* pbrBack;
	static ID2D1SolidColorBrush* pbrAltBack;
	static ID2D1SolidColorBrush* pbrGridLine;
	static ID2D1SolidColorBrush* pbrText;
	static IDWriteTextFormat* ptfText;
public:
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);
	static ID2D1PathGeometry* PgeomCreate(ID2D1Factory* pfactd2d, PTF rgptf[], int cptf);
	static ID2D1Bitmap* PbmpFromPngRes(int idb, ID2D1RenderTarget* prt, IWICImagingFactory* pfactwic);

protected:
	RCF rcfBounds;	// rectangle is in global coordinates
	UI* puiParent;
	vector<UI*> rgpuiChild;
	bool fVisible;

public:
	UI(UI* puiParent, bool fVisible=true);
	UI(UI* puiParent, RCF rcfBounds, bool fVisible=true);
	~UI(void);

	void AddChild(UI* puiChild);
	void RemoveChild(UI* puiChild);
	
	RCF RcfInterior(void) const;	// in local coordinates (top left is always {0,0})
	bool FVisible(void) const;
	void SetBounds(RCF rcfNew);
	void Resize(PTF ptfNew);
	void Move(PTF ptfNew);
	void Show(bool fShow);
	RCF RcfParentFromLocal(RCF rcf) const; // local to parent coordinates
	RCF RcfGlobalFromLocal(RCF rcf) const; // local to app global coordinates
	RCF RcfLocalFromGlobal(RCF rcf) const;
	RCF RcfLocalFromParent(RCF rcf) const;
	PTF PtfParentFromLocal(PTF ptf) const;
	PTF PtfGlobalFromLocal(PTF ptf) const;

	void Update(const RCF* prcfUpdate = NULL);
	void Redraw(void);
	virtual void InvalRcf(RCF rcf, bool fErase) const;
	virtual void Draw(const RCF* prcfDraw=NULL);
	virtual ID2D1RenderTarget* PrtGet(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	void FillRcf(RCF rcf, ID2D1Brush* pbr) const;
	void FillEllf(ELLF ellf, ID2D1Brush* pbr) const;
	void DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr = NULL) const;
	void DrawSzCenter(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr = NULL) const;
	void DrawRgch(const WCHAR* rgch, int cch, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr = NULL) const;
	void DrawBmp(RCF rcfTo, ID2D1Bitmap* pbmp, RCF rcfFrom, float opacity = 1.0f) const;
};


/*
 *
 *	SPA class
 *
 *	Screen panel class. Base class for reserving space on the
 *	graphics screen where we can display random stuff.
 *
 */
class GA;

enum class LL
{
	None,
	Absolute,
	Left,
	Right,
	Below,
	Above
};


/*
 *
 *	HT class
 *
 *	Hit test class, which includes information for the hit test over various
 *	parts of the game screen. This structure is virtual, which allows for
 *	different data to be hit on in different screen panels.
 *
 *	Because of the virtualness, it must be allocated with new and they must
 *	implement a Clone method. No copy constructors!
 *
 */

enum class HTT
{
	Miss,
	Static,
	MoveablePc,
	UnmoveablePc,
	OpponentPc,
	EmptyPc,
	FlipBoard,
	Resign
};

class SPA;

class HT
{
public:
	HTT htt;
	PTF ptf;
	SPA* pspa;
	HT(const PTF& ptf, HTT htt, SPA* pspa) : ptf(ptf), htt(htt), pspa(pspa) { }
	virtual HT* PhtClone(void) { return new HT(ptf, htt, pspa); }
};


/*
 *
 *	SPA class
 *
 *	A screen panel.
 *
 */


class SPA : public UI
{
public:
	static ID2D1SolidColorBrush* pbrTextSel;
	static IDWriteTextFormat* ptfTextSm;
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

protected:
	GA& ga;
public:
	SPA(GA* pga);
	~SPA(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;
	void SetShadow(void);

	virtual HT* PhtHitTest(PTF ptf);
	virtual void StartLeftDrag(HT* pht);
	virtual void EndLeftDrag(HT* pht);
	virtual void LeftDrag(HT* pht);
	virtual void MouseHover(HT* pht);
};


/*
 *
 *	SPAS class
 *
 *	A scrolling screen panel base class.
 *
 */


class SPAS : public SPA
{
private:
	RCF rcfView;
	RCF rcfCont;
protected:
	const float dxyfScrollBarWidth = 10.0f;
public:
	SPAS(GA* pga) : SPA(pga) { }


	void SetView(const RCF& rcfView)
	{
		this->rcfView = rcfView;
		this->rcfView.right -= dxyfScrollBarWidth;
		this->rcfView.Offset(rcfBounds.left, rcfBounds.top);
	}


	void SetContent(const RCF& rcfCont)
	{
		this->rcfCont = rcfCont;
		this->rcfCont.Offset(rcfBounds.left, rcfBounds.top);
	}


	virtual RCF RcfView(void) const
	{
		RCF rcf = rcfView;
		rcf.Offset(-rcfBounds.left, -rcfBounds.top);
		return rcf;
	}


	void UpdateContSize(const PTF& ptf)
	{
		rcfCont.bottom = rcfCont.top + ptf.y;
		rcfCont.right = rcfCont.left + ptf.x;
	}


	virtual RCF RcfContent(void) const
	{
		RCF rcf = rcfCont;
		rcf.Offset(-rcfBounds.left, -rcfBounds.top);
		return rcf;
	}


	void ScrollTo(int yfTop)
	{
		rcfCont.Offset(0, rcfView.top - rcfCont.top + yfTop);
		Redraw();
	}


	bool FMakeVis(float yf, float dyf)
	{
		yf += rcfBounds.top;
		if (yf < rcfView.top)
			rcfCont.Offset(0, rcfView.top - yf);
		else if (yf + dyf > rcfView.bottom)
			rcfCont.Offset(0, rcfView.bottom - (yf + dyf));
		else
			return false;
		Redraw();
		return true;
	}

	virtual void Draw(const RCF* prcfUpdate = NULL)
	{
		SPA::Draw(prcfUpdate);
		/* just redraw the entire content area clipped to the view */
		PrtGet()->PushAxisAlignedClip(rcfView, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		DrawContent(rcfCont);
		PrtGet()->PopAxisAlignedClip();
		DrawScrollBar();
	}


	virtual void DrawContent(const RCF& rcf)
	{
	}


	void DrawScrollBar(void)
	{
		RCF rcf = rcfView;
		rcf.left = rcf.right;
		rcf.right = rcf.left + dxyfScrollBarWidth;
		PrtGet()->FillRectangle(rcf, pbrAltBack);
	}
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
	WCHAR ch;
	bool fHilite;
	bool fTrack;
public:
	BTN(UI* puiParent, WCHAR ch, RCF rcf) : UI(puiParent, rcf) {
		this->ch = ch;
		this->fHilite = false;
		this->fTrack = false;
	}

	void Track(bool fTrackNew) {
		fTrack = fTrackNew;
	}

	void Hilite(bool fHiliteNew) {
		fHilite = fHiliteNew;
	}

	void Draw(ID2D1RenderTarget* prt) {
	}
};

