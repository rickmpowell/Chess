/*
 *
 *	xt.h
 *
 *	Transposition table
 *
 */
#pragma once
#include "bd.h"


/*
 *
 *	Transposition table eval may not be exact, since it's the result of an alpha-beta
 *	search, which is not guaranteed to exahustively search the move tree because 
 *	alpha-beta pruning can abort the search once it realizes the move sucks. But
 *	inexact eval can still be helpful.
 * 
 *	Eval in this table is always a full eval, not the fast eval used for pre-ordering.
 *
 */

enum class EVT {
	Lower,
	Equal,
	Higher,
	Nil
};


/*
 *
 *	XEV class
 * 
 *	The transposition table evaluation structure. Holds the results of a single
 *	alpha-beta search board eval.
 * 
 */


class XEV
{
public:
	HABD habd;	/* board hash */
	EV ev;		/* board evaluation */
	uint16_t evt : 2,	/* evaluation type */
		unused : 6,
		ply : 8;	/* ply depth the eval occurred at */
};


/*
 *
 *	XT
 * 
 *	The container of the transposition table. It's basically a hash table using
 *	the HABD board hash as an index to store various evaluation information of
 *	the board. This is a very large data structure and needs lightning fast 
 *	lookup.
 *
 */


class XT
{
	const unsigned cxevMax = 1UL << 24;
	XEV* rgxev;

public:
	XT(void)
	{
		rgxev = nullptr;
	}

	~XT(void)
	{
		if (rgxev != nullptr) {
			delete[] rgxev;
			rgxev = nullptr;
		}
	}


	/*	XT::Init
	 *
	 *	Initializes a new transposition table. This must be called before first use.
	 */
	void Init(void)
	{
		if (rgxev == nullptr)
			rgxev = new XEV[cxevMax];
		Clear();
	}


	/*	XT::Clear
	 *
	 *	Clears out the hash table. Current implementation requires we do this before
	 *	starting any move search.
	 */
	void Clear(void)
	{
		for (unsigned ixev = 0; ixev < cxevMax; ixev++)
			rgxev[ixev].evt = static_cast<uint16_t>(EVT::Nil);
	}


	/*	XT::array index
	 *
	 *	Returns a reference to the hash table entry that may or may not be used by the
	 *	board. Caller is responsible for making sure the entry is valid.
	 */
	XEV& operator[](const BDG& bdg)
	{
		return rgxev[bdg.habd & (cxevMax-1)];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	XEV* Save(const BDG& bdg, EV ev, EVT evt, unsigned ply)
	{
		XEV& xev = (*this)[bdg];
		/* don't overwrite an entry that is deeper than the new entry */
		if (xev.evt != static_cast<uint16_t>(EVT::Nil) && ply < xev.ply)
			return nullptr;
		xev.habd = bdg.habd;
		xev.ev = ev;
		xev.evt = static_cast<uint16_t>(evt);
		xev.ply = ply;
		return &xev;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as ply. Returns nullptr if no such entry exists.
	 */
	XEV* Find(const BDG& bdg, unsigned ply)
	{
		XEV& xev = (*this)[bdg];
		if (xev.evt == static_cast<uint16_t>(EVT::Nil) || xev.habd != bdg.habd || ply > xev.ply)
			return nullptr;
		return &xev;
	}
 };

