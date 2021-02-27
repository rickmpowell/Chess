#pragma once

/*
 *
 *	pl.h
 * 
 *	Definitions for players of the game
 * 
 */

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
};