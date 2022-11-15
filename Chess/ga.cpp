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


const wchar_t GA::szInitFEN[] = L"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


/*
 *
 *	GA class
 * 
 *	Chess game implementation
 * 
 */


GA::GA() : prule(nullptr), pprocpgn(nullptr), puiga(nullptr)
{
	mpcpcppl[cpcWhite] = mpcpcppl[cpcBlack] = nullptr;
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
	puiga->InitUI(spmvAnimate);
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


/*	GA::NewGame 
 *
 *	Starts a new game with the given rule set, initializing everything and redrawing 
 *	the screen
 */
void GA::NewGame(RULE* prule)
{
	SetRule(prule);
	InitGame(szInitFEN);
}

void GA::SetRule(RULE* prule)
{
	if (this->prule)
		delete this->prule;
	this->prule = prule;
}

void GA::InitGame(const wchar_t* szFEN)
{
	bdg.InitGame(szFEN);
	bdgInit = bdg;
}


void GA::EndGame(void)
{
}


void GA::MakeMv(MV mv)
{
	bdg.MakeMv(mv);
	if (bdg.gs != gsPlaying)
		EndGame();
}


/*	GA::UndoMv
 *
 *	Moves the current move pointer back one through the move list and undoes
 *	the last move on the game board.
 */
void GA::UndoMv(void)
{
	MoveToImv(bdg.imvCur - 1);
}


/*	GA::RedoMv
 *
 *	Moves the current move pointer forward through the move list and remakes
 *	the next move on the game board.
 */
void GA::RedoMv(void)
{
	MoveToImv(bdg.imvCur + 1);
}


/*	GA::MoveToImv
 *
 *	Move to the given move number in the move list, animating the board as in the
 *	current game speed mode. imv can be -1 to go to the start of the game, up to
 *	the last move the game.
 */
void GA::MoveToImv(int64_t imv)
{
	imv = clamp(imv, (int64_t)-1, (int64_t)bdg.vmvGame.size() - 1);
	while (bdg.imvCur > imv)
		bdg.UndoMv();
	while (bdg.imvCur < imv)
		bdg.RedoMv();
}


void GA::GenVemv(VEMV& vemv)
{
	bdg.GenVemv(vemv, ggLegal);
}


