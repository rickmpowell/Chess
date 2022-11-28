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
	UIGA& uiga;

public:
	UIP(UIGA& uiga);
	virtual ~UIP(void);
	virtual void Draw(const RC& rcUpdate);
	void SetShadow(void);
	void AdjustUIRcBounds(UI& ui, RC& rc, bool fTop);
};


/*
 *
 *	SBAR class
 * 
 *	Scrollbar class. UI element for implementing a scrollbar.
 *
 */


enum HTSBAR {
	htsbarNone,
	htsbarLineUp,
	htsbarLineDown,
	htsbarPageUp,
	htsbarPageDown,
	htsbarThumb
};


class UIPS;

class SBAR : public UI
{
	static constexpr float dyThumbMin = 16.0f;

private:
	UIPS* puips;
	float yTopCont, yBotCont;
	float yTopView, yBotView;
	TID tidScroll;
	bool fScrollDelay;
	HTSBAR htsbarTrack;
	PT ptMouseInit, ptThumbTopInit;

public:
	SBAR(UIPS* puiParent);

	void SetRange(float yTop, float yBot);
	void SetRangeView(float yTop, float yBot);

	virtual void Draw(const RC& rcUpdate);

	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);

	RC RcThumb(void) const;
	float DyThumb(void) const;
	HTSBAR HitTest(const PT& pt);
	void StartScrollRepeat(void);
	void ContinueScrollRepeat(void);
	void EndScrollRepeat(void);
	virtual void TickTimer(TID tid, UINT dtm);

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
	SBAR sbarVert;

protected:
	const float dxyScrollBarWidth = 12.0f;

public:
	UIPS(UIGA& uiga);

	virtual void Layout(void);
	void SetView(const RC& rcView);
	void SetContent(const RC& rcCont);
	RC RcView(void) const;
	RC RcContent(void) const;
	void UpdateContSize(const SIZ& siz);
	void AdjustRcView(const RC& rc);
	virtual float DyLine(void) const;
		
	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rc);

	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void ScrollWheel(const PT& pt, int dwheel);
	virtual void ScrollLine(int dline);
	virtual void ScrollPage(int dpage);

	void ScrollBy(float dy);
	void ScrollTo(float yTopNew);
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
	virtual void Draw(const RC& rcUpdate);
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
	virtual void Draw(const RC& rcUpdate);
	void AdjustBtnRcBounds(UI* pui, RC& rc, float dxWidth);
};
