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
	UIPL uiplWhite, uiplBlack;

public:
	UITI(UIGA& uiga);
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
	UIGA& uiga;
	GPH gph;
	CPC cpc;
	ColorF CoFromApcSq(APC apc, SQ sq) const;
public:
	UIPVTPL(UI* puiParent, UIGA& uiga, CPC cpc, GPH gph);
	void Draw(const RC& rcUpdate);
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
	CPC cpc;
	UIPVTPL uipvtplOpening, uipvtplMidGame, uipvtplEndGame;

public:
	UIPVT(UIGA& uiga, CPC cpc);
	virtual void Layout(void);
	virtual void Draw(const RC& rcUpdate);
};