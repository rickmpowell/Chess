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
	int nmvFirst;	/* 1-based move number when interval starts */
	int nmvLast;	/* 1-based move number when interval ends. -1 for no end. */
	DWORD dmsec;	/* Amount of time to add to the timer at the start of the interval */
	DWORD dmsecMove;	/* Amount of time to add after each move */

	TMI(int mvnFirst, int nmvLast, DWORD dmsec, DWORD dmsecMove) :
		nmvFirst(mvnFirst), nmvLast(nmvLast),
		dmsec(dmsec), dmsecMove(dmsecMove)
	{
	}

	bool FContainsNmv(int nmv) const
	{
		return nmv >= nmvFirst && (nmvLast == -1 || nmv <= nmvLast);
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
	DWORD DmsecAddBlock(CPC cpc, int nmv) const;
	DWORD DmsecAddMove(CPC cpc, int nmv) const;
	int CmvRepeatDraw(void) const;
	void SetGameTime(CPC cpc, DWORD dsec);
	int CtmiTotal(void) const;
	const TMI& TmiFromItmi(int itmi) const;
	const TMI& TmiFromNmv(int nmv) const;

	wstring SzTimeControlTitle(void) const;
	wstring SzTimeControlSummary(void) const;
	wstring SzSummaryFromTmi(const TMI& tmi) const;
};


