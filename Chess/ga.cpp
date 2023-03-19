/*
 *
 *	ga.cpp
 * 
 *	Game implementation. A game is the whole big collection of
 *	everything that goes into playing the game, including the
 *	board, players, and display.
 * 
 */

#include "ga.h"


/*
 *
 *	GA class
 * 
 *	Chess game implementation
 * 
 */


GA::GA() : prule(nullptr), pprocpgn(nullptr), puiga(nullptr),
		szEvent(L"Casual Game"), szSite(L"Local Machine")
{
	tpStart = system_clock::now();
	mpcpcppl[cpcWhite] = mpcpcppl[cpcBlack] = nullptr;
	mpcpcdmsec[cpcWhite] = mpcpcdmsec[cpcBlack] = 0;
}


GA::~GA(void)
{
	for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
		if (mpcpcppl[cpc])
			delete mpcpcppl[cpc];
	if (prule)
		delete prule;
	if (pprocpgn)
		delete pprocpgn;
}


/*	GA::SetUiga
 *
 *	Attaches the user-interface part of the game to the virtual game
 */
void GA::SetUiga(UIGA* puiga)
{
	this->puiga = puiga;
}


/*	GA::SetPl
 *
 *	Sets the player of the given color. Takes ownership of the PL, which
 *	will be freed when the game is done with it.
 */
void GA::SetPl(CPC cpc, PL* ppl)
{
	if (mpcpcppl[cpc])
		delete mpcpcppl[cpc];
	mpcpcppl[cpc] = ppl;
}


void GA::SetRule(RULE* prule)
{
	if (prule == nullptr)
		prule = new RULE;
	if (prule == this->prule)
		return;
	if (this->prule)
		delete this->prule;
	this->prule = prule;
}


const char* GA::InitGame(const char* szFEN, RULE* prule)
{
	SetRule(prule);
	bdg.InitGame();
	const char* pch = bdg.SetFen(szFEN);
	bdgInit = bdg;
	tpStart = system_clock::now();
	return pch;
}


void GA::InitGame(void)
{
	bdg.InitGame();
	bdgInit = bdg;
	tpStart = system_clock::now();
}


void GA::StartGame(void)
{
	InitGame();
	bdg.SetGs(gsPlaying);
}


void GA::EndGame(void)
{
}


void GA::SetTimeRemaining(CPC cpc, DWORD dmsec) noexcept
{
	mpcpcdmsec[cpc] = dmsec;
}

DWORD GA::DmsecRemaining(CPC cpc) const noexcept
{
	return mpcpcdmsec[cpc];
}
