/*
 *
 *	uiml.h
 * 
 *	Definitions for the move list screen panel. This thing is mostly UI
 *	stuff.
 * 
 */

#pragma once
#include "uipa.h"
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
	UIGA& uiga;
	CPC cpc;
	bool fChooser;
	int iinfoplHit;
	float dyLine;

public:
	UIPL(UI& uiParent, UIGA& uiga, CPC cpc);
	virtual void ComputeMetrics(bool fStatic);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	virtual void Draw(const RC& rcUpdate);
	void DrawChooser(const RC& rcUpdate);
	void DrawChooserItem(const INFOPL& infopl, RC& rc);

	virtual void MouseHover(const PT& pt, MHT mht);
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
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
	UIGA& uiga;
	CPC cpc;
	bool fFlag;
	static TX* ptxClock;
	static TX* ptxClockNote;
	static TX* ptxClockNoteBold;
public:
	static bool FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcStatic(void);

public:
	UICLOCK(UIML& uiml, CPC cpc);


	virtual SIZ SizLayoutPreferred(void);
	
	virtual void Draw(const RC& rcUpdate);
	virtual ColorF CoText(void) const;
	virtual ColorF CoBack(void) const;

	void Flag(bool fFlag);

	RC DrawTimeControls(int nmvSel) const;
	void DrawTmi(const TMI& tmi, RC rc, int nmvSel) const;
	void DrawColon(RC& rc, unsigned frac) const;
	void DrawFlag(void) const;

	wchar_t* PchDecodeDmsec(DWORD dmsec, wchar_t* pch) const;
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
	UIGA& uiga;
	BTNIMG btnResign;
	BTNIMG btnOfferDraw;
	TX* ptxScore;

public:
	UIGC(UIML& uiml);
	virtual ~UIGC(void);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);

	virtual void Draw(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual ColorF CoBack(void) const;
};


/*
 *
 *	UIML class
 * 
 *	The move list screen panel
 * 
 */

enum HTML {
	htmlMiss,
	htmlPageUp,
	htmlPageDown,
	htmlThumb,
	htmlMoveNumber,
	htmlList,
	htmlEmptyBefore,
	htmlEmptyAfter
};

class PL;

class UIML : public UIPS
{
	friend class UIPL;
	friend class UICLOCK;
	friend class UIGC;
	friend class UIGA;
	friend class GA;

	TX* ptxList;
	TX* ptxPiece;
	float mpcoldx[4];
	float dxCellMarg, dyCellMarg;
	float dyList, dyListBaseline;

	float XFromCol(int col) const;
	float DxFromCol(int col) const;
	RC RcFromCol(float y, int col) const;
	RC RcFromImv(int imv) const;

	int imveSel;
	UIPL uiplWhite, uiplBlack;
	UICLOCK uiclockWhite, uiclockBlack;
	UIGC uigc;

public:
	UIML(UIGA& uiga);
	virtual ~UIML(void);

	void InitGame(void);
	void EndGame(void);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	void ShowClocks(bool fTimed);
	
	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void ComputeMetrics(bool fStatic);

	void DrawAndMakeMv(const RC& rc, BDG& bdg, MVE mve);
	void DrawMoveNumber(const RC& rc, int imv);
	void DrawSel(int imv);
	void SetSel(int imv, SPMV spmv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);
	virtual float DyLine(void) const;

	HTML HtmlHitTest(const PT& pt, int* pimv);
	virtual void StartLeftDrag(const PT& pt);
	virtual void EndLeftDrag(const PT& pt, bool fClick);
	virtual void LeftDrag(const PT& pt);

	virtual void KeyDown(int vk);

	UIPL& UiplFromCpc(CPC cpc);
	UICLOCK& UiclockFromCpc(CPC cpc);
};
