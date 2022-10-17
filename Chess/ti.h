/*
 *
 *	ti.h
 * 
 *	Title block panel
 *
 */

#pragma once

#include "panel.h"
#include "ml.h"



class UIICON : public UI
{
	int idb;
	BMP* pbmp;
public:
	UIICON(UI* puiParent, int idb);
	~UIICON(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	virtual void Draw(const RC& rcUpdate);
	SIZ Siz(void) const;
};


class UILOCALE : public UI
{
public:
	UILOCALE(UI* puiParent);
	virtual void Draw(const RC& rcUpdate);
};


class UIGT : public UI
{
public:
	UIGT(UI* puiParent);
	virtual void Draw(const RC& rcUpdate);
};


class UIGTM : public UI
{
public:
	UIGTM(UI* puiParent);
	virtual void Draw(const RC& rcUpdate);
};


/*
 *
 *	UITI class
 * 
 *	The title box screen panel on the screen
 * 
 */

class UIPL;


class UITI : public UIP
{
	UIICON uiicon;
	UILOCALE uilocale;
	UIGT uigt;
	UIGTM uigtm;
	UIPL uiplWhite;
	UIPL uiplBlack;

public:
	UITI(GA* pga);
	void SetPl(CPC cpc, PL* ppl);
	virtual void Layout(void);
	virtual void Draw(const RC& rcUpdate);
};


/*
 *
 *	UIPVTPL
 * 
 *	Displays piece value table for the player
 * 
 */


class UIPVTPL : public UI
{
private:
	PL* ppl;
	GPH gph;
	ColorF CoFromApcSq(APC apc, SQ sq) const;
public:
	UIPVTPL(UI* puiParent, GPH gph);
	void Draw(const RC& rcUpdate);
	void SetPl(PL* ppl);
	void SetGph(GPH gph);
};


/*
 *
 *	UIPVT
 *
 *	The piece value table panel. A simple little panel that just shows
 *	a graphical representation of the PLAI and PLAI2 piece value tables.
 *	Not usually useful, so we don't show it by default.
 *
 */


class UIPVT : public UIP
{
private:
	UIPVTPL uipvtplOpening, uipvtplMidGame, uipvtplEndGame;

public:
	UIPVT(GA* pga);
	virtual void Layout(void);
	virtual void Draw(const RC& rcUpdate);
	void SetPl(CPC cpc, PL* ppl);
};