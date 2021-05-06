/*
 *
 *	debug.cpp
 * 
 *	Debugging panels
 * 
 */

#include "debug.h"



UIDB::UIDB(GA* pga) : SPA(pga)
{
}


void UIDB::Layout(void)
{
}


void UIDB::Draw(const RCF* prcfUpdate)
{
	FillRcf(RcfInterior(), pbrBack);
}


void UIDB::AddEval(const BDG& bdg, MV mv, float eval)
{
}