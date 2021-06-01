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


WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch);


/*
 *	Move speeds
 */
enum class SPMV
{
	Hidden,
	Fast,
	Animate
};


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
	SearchStartAI,
	SearchDepthAI,
	SearchMoveAI,
	SearchNodesAI
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
	static GEOM* PgeomCreate(FACTD2* pfactd2, PTF rgptf[], int cptf);
	static BMP* PbmpFromPngRes(int idb, DC* pdc, FACTWIC* pfactwic);

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
	UI* PuiParent(void) const;
	UI* PuiPrevSib(void) const;
	UI* PuiNextSib(void) const;
	
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

	void Update(const RCF* prcfUpdate = NULL);
	void Redraw(void);
	virtual void InvalRcf(RCF rcf, bool fErase) const;
	virtual void Draw(const RCF* prcfDraw=NULL);
	virtual void PresentSwch(void) const;
	virtual APP& AppGet(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void FillRcf(RCF rcf, BR* pbr) const;
	virtual void FillRcfBack(RCF rcf) const;
	void FillEllf(ELLF ellf, BR* pbr) const;
	void DrawSz(const wstring& sz, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	void DrawSzCenter(const wstring& sz, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	void DrawRgch(const WCHAR* rgch, int cch, TX* ptx, RCF rcf, BR* pbr = NULL) const;
	SIZF SizfSz(const wstring& sz, TX* ptx, float dxf=1.0e6f, float dyf=1.0e6f) const;
	void DrawBmp(RCF rcfTo, BMP* pbmp, RCF rcfFrom, float opacity = 1.0f) const;

	virtual void Log(LGT lgt, const wstring& sz) const;
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
	BTN(UI* puiParent, int cmd, RCF rcf);
	void Track(bool fTrackNew);
	void Hilite(bool fHiliteNew);
	virtual void Draw(DC* pdc);
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
	BTNCH(UI* puiParent, int cmd, RCF rcf, WCHAR ch);
	virtual void Draw(const RCF* prcfUpdate);
};


class BTNIMG : public BTN
{
	int idb;
	BMP* pbmp;
public:
	BTNIMG(UI* puiParent, int cmd, RCF rcf, int idb);
	~BTNIMG(void);
	virtual void Draw(const RCF* prcfUpdate);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	SIZF SizfImg(void) const;
};

