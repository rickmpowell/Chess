/*
 *
 *	ml.h
 * 
 *	Definitions for the move list screen panel. This thing is mostly UI
 *	stuff.
 * 
 */

#pragma once
#include "panel.h"
#include "bd.h"


/*
 *
 *	UICLOCK class
 *
 *	A UI element to display a chess clock. Usually two of these
 *	on the screen at any given time.
 *
 */


class UIML;
class GA;

class UICLOCK : public UI
{
protected:
	GA& ga;
	CPC cpc;
	static TX* ptxClock;
public:
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

public:
	UICLOCK(UIML* puiml, CPC cpc);
	virtual SIZF SizfLayoutPreferred(void);
	virtual void Draw(RCF rcfUpdate);
	void DrawColon(RCF& rcf, unsigned frac) const;
	bool FTimeOutWarning(DWORD tm) const;
};



/*
 *
 *	UIGC
 * 
 *	Game control UI element. Button control list for doing a few random
 *	game operations, like resign, offer draw, new game, etc. At end of
 *	game, it expand and shows results of the game.
 * 
 */


class UIGC : public UI
{
protected:
	GA& ga;
	BTNIMG btnResign;
	BTNIMG btnOfferDraw;
	TX* ptxScore;

public:
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

public:
	UIGC(UIML* puiml);
	virtual void Draw(RCF rcfUpdate);
	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
};


/*
 *
 *	UIML class
 * 
 *	The move list screen panel
 * 
 */

enum class HTML {
	Miss,
	PageUp,
	PageDown,
	Thumb,
	MoveNumber,
	List,
	EmptyBefore,
	EmptyAfter
};

class PL;

class UIML : public UIPS
{
	friend class UIPL;
	friend class UICLOCK;
	friend class UIGC;
	friend class GA;

	TX* ptxList;
	float mpcoldxf[4];
	float dxfCellMarg, dyfCellMarg;
	float dyfList;

	float XfFromCol(int col) const;
	float DxfFromCol(int col) const;
	RCF RcfFromCol(float yf, int col) const;
	RCF RcfFromImv(int imv) const;

	BDG bdgInit;	// initial board at the start of the game list
	int imvSel;
	UIPL* mpcpcpuipl[CPC::ColorMax];
	UICLOCK* mpcpcpuiclock[CPC::ColorMax];
	UIGC uigc;

public:
	UIML(GA* pga);
	~UIML(void);

	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	void SetPl(CPC cpc, PL* ppl);

	void NewGame(void);
	void EndGame(void);

	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
	void ShowClocks(bool fTimed);
	
	virtual void Draw(RCF rcfUpdate);
	virtual void DrawContent(RCF rcfCont);

	void DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv);
	void DrawMoveNumber(RCF rcf, int imv);
	void DrawSel(int imv);
	void SetSel(int imv, SPMV spmv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);
	virtual float DyfLine(void) const;

	HTML HtmlHitTest(PTF ptf, int* pimv);
	virtual void StartLeftDrag(PTF ptf);
	virtual void EndLeftDrag(PTF ptf);
	virtual void LeftDrag(PTF ptf);

	virtual void KeyDown(int vk);
};
