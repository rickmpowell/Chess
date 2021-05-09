/*
 *
 *	debug.h
 * 
 *	Definitions for the debug panel
 * 
 */
#pragma once
#include "ui.h"
#include "bd.h"



class UIDBBTNS : public UI
{
	BTNCH btnTest;
public:
	UIDBBTNS(UI* puiParent);
	void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(void);
};


class UIDB : public UIPS
{
	UIDBBTNS uidbbtns;
public:
	UIDB(GA* pga);
	virtual void Layout(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void AddEval(const BDG& bdg, MV mv, float eval);
};