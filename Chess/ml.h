#pragma once
/*
 *
 *	ml.h
 * 
 *	Definitions for the move list screen panel. This thing is mostly UI
 *	stuff.
 * 
 */

#include "framework.h"
#include "ui.h"
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

public:
	UICLOCK(UIML* puiml, CPC cpc);
	virtual SIZF SizfLayoutPreferred(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void DrawColon(RCF& rcf, unsigned frac) const;
	bool FTimeOutWarning(DWORD tm) const;
};



/*
 *
 *	UIGC
 * 
 *	Game control UI element. Button control list for doing a few random
 *	game operations, like resign, offer draw, new game, etc.
 * 
 */


class UIGC : public UI
{
private:
	BTNIMG* pbtnResign;
	BTNIMG* pbtnOfferDraw;
public:
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	UIGC(UIML* puiml);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
};


/*
 *
 *	UIGO
 *
 *	Game over sub-panel in the move list.
 *
 */


class UIGO : public UI
{
protected:
	GA& ga;
	static TX* ptxScore;

public:
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	UIGO(UIML* puiml, bool fVisible);
	virtual SIZF SizfLayoutPreferred(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
};


/*
 *
 *	UIML class
 * 
 *	The move list screen panel
 * 
 */


class PL;

class UIML : public UIPS
{
	friend class UIPL;
	friend class UICLOCK;
	friend class UIGO;
	friend class GA;

	static TX* ptxList;
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
	UIGO uigo;
	UIGC uigc;

public:
	UIML(GA* pga);
	~UIML(void);

	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

	void SetPl(CPC cpc, PL* ppl);

	void NewGame(void);
	void EndGame(void);

	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
	void ShowClocks(bool fTimed);
	
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcfCont);

	void DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv);
	void DrawMoveNumber(RCF rcf, int imv);
	void DrawSel(int imv);
	void SetSel(int imv, SPMV spmv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);

	virtual void KeyDown(int vk);

	void DrawPl(CPC cpcPointOfView, RCF rcfArea, bool fTop) const;
};
