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
	static ID2D1SolidColorBrush* pbrText;
	static IDWriteTextFormat* ptfText;
public:
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

protected:
	RCF rcfBounds;	// rectangle is in parent coordinates
	UI* puiParent;
	vector<UI*> rgpuiChild;

public:
	UI(UI* puiParent);
	UI(UI* puiParent, RCF rcfBounds);
	~UI(void);

	void AddChild(UI* puiChild);
	void RemoveChild(UI* puiChild);
	
	RCF RcfInterior(void) const;	// in local coordinates (top left is always {0,0})
	void SetBounds(RCF rcfNew);
	void Resize(PTF ptfNew);
	void Move(PTF ptfNew);
	RCF RcfToParent(RCF rcf) const; // local to parent coordinates
	RCF RcfToGlobal(RCF rcf) const; // local to app global coordinates
	PTF PtfToParent(PTF ptf) const;
	PTF PtfToGlobal(PTF ptf) const;

	virtual void Draw(void);
	virtual ID2D1RenderTarget* PrtGet(void) const;
	void FillRcf(RCF rcf, ID2D1Brush* pbr) const;
	void FillEllf(ELLF ellf, ID2D1Brush* pbr) const;
	void DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr = NULL) const;
	void DrawBmp(RCF rcfTo, ID2D1Bitmap* pbmp, RCF rcfFrom, float opacity = 1.0f) const;
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

