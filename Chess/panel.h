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
	~UIP(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void SetShadow(void);
	void AdjustUIRcfBounds(UI* pui, RCF& rcf, bool fTop);
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
	RCF rcfView;	/* relative to the UI */
	RCF rcfCont;
protected:
	const float dxyfScrollBarWidth = 10.0f;
	void DrawScrollBar(void);
public:
	UIPS(GA* pga);
	
	void SetView(const RCF& rcfView);
	void SetContent(const RCF& rcfCont);
	RCF RcfView(void) const;
	RCF RcfContent(void) const;
	void UpdateContSize(const SIZF& sizf);
	void AdjustRcfView(RCF rcf);
	virtual float DyfLine(void) const;
		
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcf);

	virtual void MouseHover(PTF ptf, MHT mht);
	virtual void ScrollWheel(PTF ptf, int dwheel);

	void ScrollTo(float yfTop);
	bool FMakeVis(float yf, float dyf);
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
	UI* puiOwner;
public:

public:
	UITIP(UI* puiParent);
	virtual void Draw(const RCF* prcfUpdate);
	void AttachOwner(UI* pui);
};

