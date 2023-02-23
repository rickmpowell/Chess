/*
 *
 *	uiti.h
 * 
 *	Title block panel
 *
 */

#pragma once

#include "uipa.h"
#include "uiml.h"



class UIICON : public UI
{
	int idb;
	BMP* pbmp;
public:
	UIICON(UI* puiParent, int idb);
	virtual ~UIICON(void);

	virtual void Draw(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);

	SIZ Siz(void) const;
};


class UILOCALE : public UI
{
	UIGA& uiga;
public:
	UILOCALE(UI* puiParent, UIGA& uiga);
	virtual void Draw(const RC& rcUpdate);
};


class UIGT : public UI
{
	UIGA& uiga;
public:
	UIGT(UI* puiParent, UIGA& uiga);
	virtual void Draw(const RC& rcUpdate);
};


class UIGTM : public UI
{
	UIGA& uiga;
public:
	UIGTM(UI* puiParent, UIGA& uiga);
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
	virtual void Erase(const RC& rcUpdate, bool fParentDrawn);
	virtual void Draw(const RC& rcUpdate);
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
	virtual ColorF CoBack(void) const;
};