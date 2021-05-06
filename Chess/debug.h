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



class UIDB : public SPA
{
public:
	UIDB(GA* pga);
	virtual void Layout(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void AddEval(const BDG& bdg, MV mv, float eval);
};