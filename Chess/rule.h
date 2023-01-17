/*
 *
 *	rule.h
 * 
 *	class that defines the rules that the game is being played under.
 * 
 */
 #pragma once


/*
 *
 *	TMI
 *
 *	Time Interval information. The game time rules are basically a series
 *	of time intervals that get added at certain points of the game.
 * 
 */


struct TMI
{
	int mvnFirst;	/* 1-based move number when interval starts */
	int mvnLast;	/* 1-based move number when interval ends. -1 for no end. */
	DWORD dmsec;	/* Amount of time to add to the timer at the start of the interval */
	DWORD dmsecMove;	/* Amount of time to add after each move */

	TMI(int mvnFirst, int mvnLast, DWORD dmsec, DWORD dmsecMove) :
		mvnFirst(mvnFirst), mvnLast(mvnLast),
		dmsec(dmsec), dmsecMove(dmsecMove)
	{
	}
};


/*
 *
 *	RULE 
 * 
 *	The rules 
 * 
 */


class RULE
{
	vector<TMI> vtmi;
	unsigned cmvRepeatDraw; // if zero, no automatic draw detection
public:
	RULE(void);
	RULE(int dsecGame, int dsecMove, unsigned cmvRepeatDraw);
	void ClearTmi(void);
	void AddTmi(int nmvFirst, int nmvLast, int dsecGame, int dsecMove);
	bool FUntimed(void) const;
	DWORD DmsecAddBlock(CPC cpc, int mvn) const;
	DWORD DmsecAddMove(CPC cpc, int mvn) const;
	int CmvRepeatDraw(void) const;
	void SetGameTime(CPC cpc, DWORD dsec);
};


