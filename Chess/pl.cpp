/*
 *
 *	pl.cpp
 * 
 *	Code for the player class
 * 
 */

#include "pl.h"
#include "ga.h"

mt19937 rgen(0);


PL::PL(wstring szName) : szName(szName)
{
}


PL::~PL(void)
{
}


MV PL::MvGetNext(GA& ga)
{
	vector <MV> rgmv;
	ga.GenRgmv(rgmv);
	if (rgmv.size() == 0)
		return MV();
	uniform_int_distribution<int> rdist(0, rgmv.size()-1);
	return rgmv[rdist(rgen)];
}