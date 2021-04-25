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


class SPARGMV;
class GA;

class UICLOCK : public UI
{
protected:
	GA& ga;
	CPC cpc;
	static TF* ptfClock;
public:
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	UICLOCK(SPARGMV* pspargmv, CPC cpc);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void DrawColon(RCF& rcf, unsigned frac) const;
	bool FTimeOutWarning(DWORD tm) const;
};


/*
 *
 *	UIPL
 * 
 *	Player name UI element in the move list. Pretty simple control.
 * 
 */

class PL;

class UIPL : public UI
{
private:
	PL* ppl;
	CPC cpc;
public:
	UIPL(SPARGMV* pspargmv, PL* ppl, CPC cpc);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void SetPl(PL* pplNew);
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
	UIGC(SPARGMV* pspargmv);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(void);
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
	static IDWriteTextFormat* ptfScore;

public:
	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

public:
	UIGO(SPARGMV* pspargmv, bool fVisible);
	virtual void Draw(const RCF* prcfUpdate = NULL);
};


/*
 *
 *	SPARGMV class
 * 
 *	The move list screen panel
 * 
 */


class PL;

class SPARGMV : public SPAS
{
	friend class UIPL;
	friend class UICLOCK;
	friend class UIGO;
	friend class GA;

	static TF* ptfList;
	static float mpcoldxf[4];
	static float dyfList;

	float XfFromCol(int col) const;
	float DxfFromCol(int col) const;
	RCF RcfFromCol(float yf, int col) const;
	RCF RcfFromImv(int imv) const;

	BDG bdgInit;	// initial board at the start of the game list
	int imvSel;
	UIPL* mpcpcpuipl[2];
	UICLOCK* mpcpcpuiclock[2];
	UIGO* puigo;
	UIGC* puigc;

public:
	SPARGMV(GA* pga);
	~SPARGMV(void);

	static void CreateRsrcClass(DC* pdc, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcClass(void);

	void SetPl(CPC cpc, PL* ppl);

	void NewGame(void);
	void EndGame(void);

	virtual void Layout(void);
	void AdjustUIRcfBounds(UI* pui, RCF& rcf, bool fTop, float dyfHeight);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;

	void DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv);
	void DrawMoveNumber(RCF rcf, int imv);
	void DrawSel(int imv);
	void SetSel(int imv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);

	void DrawPl(CPC cpcPointOfView, RCF rcfArea, bool fTop) const;
};
