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
#include "pl.h"


class UIPL;

class SPINLVL : public SPIN
{
	UIPL& uipl;
public:
	SPINLVL(UIPL* puiParent, int cmdUp, int cmdDown);
	virtual wstring SzValue(void) const;
};


/*
 *
 *	UIPL
 *
 *	Player name UI element in the move list. Pretty simple control.
 *
 */


struct INFOPL;

class UIPL : public UI
{
	friend class SPINLVL;

private:
	SPINLVL spinlvl;
	PL* ppl;
	CPC cpc;
	bool fChooser;
	int iinfoplHit;
	float dyLine;

public:
	UIPL(UI* puiParent, CPC cpc);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	
	virtual void Draw(const RC& rcUpdate);
	void DrawChooser(const RC& rcUpdate);
	void DrawChooserItem(const INFOPL& infopl, RC& rc);

	void SetPl(PL* pplNew);

	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);
};


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
	virtual SIZ SizLayoutPreferred(void);
	virtual void Draw(const RC& rcUpdate);
	void DrawColon(RC& rc, unsigned frac) const;
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
	virtual void Draw(const RC& rcUpdate);
	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
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
	float mpcoldx[4];
	float dxCellMarg, dyCellMarg;
	float dyList;

	float XFromCol(int col) const;
	float DxFromCol(int col) const;
	RC RcFromCol(float y, int col) const;
	RC RcFromImv(int imv) const;

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

	void InitGame(void);
	void EndGame(void);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	void ShowClocks(bool fTimed);
	
	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rcCont);

	void DrawAndMakeMv(const RC& rc, BDG& bdg, MV mv);
	void DrawMoveNumber(const RC& rc, int imv);
	void DrawSel(int imv);
	void SetSel(int imv, SPMV spmv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);
	virtual float DyLine(void) const;

	HTML HtmlHitTest(const PT& pt, int* pimv);
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt);
	virtual void LeftDrag(const PT& pt);

	virtual void KeyDown(int vk);
};
