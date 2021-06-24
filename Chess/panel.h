/*
 *
 *	panel.h
 * 
 *	Definitions for screen panel UI elements
 * 
 */

#pragma once
#include "ui.h"

class GA;


/*
 *
 *	UIP class
 *
 *	Screen panel class. Base class for reserving space on the
 *	graphics screen where we can display random stuff.
 *
 */


class UIP : public UI
{
public:
	static BRS* pbrTextSel;
	static TX* ptxTextSm;
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

protected:
	GA& ga;

public:
	UIP(GA* pga);
	virtual ~UIP(void);
	virtual void Draw(RC rcUpdate);
	void SetShadow(void);
	void AdjustUIRcBounds(UI* pui, RC& rc, bool fTop);

	virtual bool FDepthLog(LGT lgt, int& depth);
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
};


/*
 *
 *	UIPS class
 *
 *	A scrolling screen panel base class.
 *
 */


class UIPS : public UIP
{
private:
	RC rcView;	/* relative to the UI */
	RC rcCont;
protected:
	const float dxyScrollBarWidth = 10.0f;
	void DrawScrollBar(void);
public:
	UIPS(GA* pga);
	
	void SetView(const RC& rcView);
	void SetContent(const RC& rcCont);
	RC RcView(void) const;
	RC RcContent(void) const;
	void UpdateContSize(const SIZ& siz);
	void AdjustRcView(RC rc);
	virtual float DyLine(void) const;
		
	virtual void Draw(RC rcUpdate);
	virtual void DrawContent(RC rc);

	virtual void MouseHover(PT pt, MHT mht);
	virtual void ScrollWheel(PT pt, int dwheel);

	void ScrollTo(float yTop);
	bool FMakeVis(float y, float dy);
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
	virtual void Draw(RC rcUpdate);
	void AttachOwner(UI* pui);
};


/*
 *
 *	UIBB
 * 
 *	Button bar
 * 
 */

class UIBB : public UI
{
public:
	UIBB(UIPS* puiParent);
	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	virtual void Draw(RC rcUpdate);
	void AdjustBtnRcBounds(UI* pui, RC& rc, float dxWidth);
};
