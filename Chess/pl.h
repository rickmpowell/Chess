/*
 *
 *	pl.h
 * 
 *	Definitions for players of the game
 * 
 */
#pragma once

#include "framework.h"


/*
 *
 *	PL class
 * 
 *	The player class
 * 
 */


class PL
{
	wstring szName;
public:
	PL(wstring szName);
	~PL(void);
	wstring& SzName(void) {
		return szName;
	}

	void SetName(const wstring& szNew) {
		szName = szNew;
	}
};


#include "ui.h"
#include "bd.h"


/*
 *
 *	UIPL
 *
 *	Player name UI element in the move list. Pretty simple control.
 *
 */

class UIPL : public UI
{
private:
	PL* ppl;
	CPC cpc;
public:
	UIPL(UI* puiParent, CPC cpc);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void SetPl(PL* pplNew);
};

