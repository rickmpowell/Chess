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
	virtual void Draw(const RC& rcUpdate);
	void SetShadow(void);
	void AdjustUIRcBounds(UI* pui, RC& rc, bool fTop);

	virtual bool FDepthLog(LGT lgt, int& depth);
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
};


/*
 *
 *	SBAR class
 * 
 *	Scrollbar class. UI element for implementing a scrollbar.
 *
 */


enum class HTSBAR {
	None,
	LineUp,
	LineDown,
	PageUp,
	PageDown,
	Thumb
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
	const float dxyScrollBarWidth = 10.0f;

public:
	UIPS(GA* pga);

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
