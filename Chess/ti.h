/*
 *
 *	ti.h
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

	virtual void Draw(RC rcUpdate);
	SIZ Siz(void) const;
};


class UILOCALE : public UI
{
public:
	UILOCALE(UI* puiParent);
	virtual void Draw(RC rcUpdate);
};


class UIGT : public UI
{
public:
	UIGT(UI* puiParent);
	virtual void Draw(RC rcUpdate);
};


class UIGTM : public UI
{
public:
	UIGTM(UI* puiParent);
	virtual void Draw(RC rcUpdate);
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
	virtual void Draw(RC rcUpdate);
};
