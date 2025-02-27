/*
 *
 *	uipa.h
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
protected:
	UIGA& uiga;

public:
	UIP(UIGA& uiga);
	virtual ~UIP(void);
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
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);

	RC RcThumb(void) const;
	float DyThumb(void) const;
	HTSBAR HitTest(const PT& pt);
	void StartScrollRepeat(void);
	void ContinueScrollRepeat(void);
	void EndScrollRepeat(void);
	virtual void TickTimer(TID tid, UINT dtm);
	virtual void SetDefCursor();
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
	const float dxyScrollBarWidth = 14.0f;

public:
	UIPS(UIGA& uiga);

	virtual void Layout(void);
	void SetView(const RC& rcView);
	void SetContent(const RC& rcCont);
	RC RcView(void) const;
	RC RcContent(void) const;
	void UpdateContSize(const SIZ& siz);
	void UpdateContHeight(float dyNew);

	void AdjustRcView(const RC& rc);
	virtual float DyLine(void) const;
		
	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rcUpdate);
	void RedrawContent(void);
	void RedrawContent(const RC& rcUpdate);

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
	virtual ColorF CoBack(void) const;
	void AdjustBtnRcBounds(UI* pui, RC& rc, float dxWidth);
};


class BTNCLOSE : public BTNTEXT
{
public:
	BTNCLOSE(UI* puiParent, int icmd, const wstring& sz) : BTNTEXT(puiParent, icmd, sz) { }
	
	virtual ColorF CoText(void) const
	{
		return FEnabledCmd(cmd) ?
			CoBlend(coWhite, coBtnHilite, (float)(fHilite + fTrack) / 2.0f) :
			coBtnDisabled;
	}
};


/*
 *
 *	UITITLE
 * 
 *	Titlebar element used on screen panels, which displays a close box and title text
 * 
 */


class UITITLE : public UI
{
	wstring szTitle;
	BTNCLOSE btnclose;
public:
	UITITLE(UIP* puipParent, const wstring& szTitle);
	virtual void Layout(void);
	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoBack(void) const { return coBlack; }
	virtual ColorF CoText(void) const { return coWhite; }
};