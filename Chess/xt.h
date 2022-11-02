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
private:
	uint32_t evGrf:16,
		evtGrf : 2,
		unused1b : 6,
		plyGrf : 8;
public:

	inline EV ev(void) const {
		return static_cast<EV>(evGrf);
	}
	inline void SetEv(EV ev) {
		assert(ev != evInf && ev != -evInf);
		evGrf = static_cast<uint16_t>(ev);
	}
	inline EVT evt(void) const {
		return static_cast<EVT>(evtGrf);
	}
	inline void SetEvt(EVT evt)
	{
		evtGrf = static_cast<unsigned>(evt);
	}
	inline unsigned ply(void) const {
		return static_cast<unsigned>(plyGrf);
	}
	inline void SetPly(unsigned ply)
	{
		plyGrf = ply;
	}
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
	XEV* rgxev;
public:
	//const uint32_t cxevMax = 1UL << 27;
	const uint32_t cxevMax = 1UL << 25;
	/* cache stats */
	uint64_t cxevProbe, cxevProbeCollision, cxevProbeHit;
	uint64_t cxevSave, cxevSaveCollision, cxevSaveReplace, cxevInUse;

public:
	XT(void) : rgxev(nullptr), cxevProbe(0), cxevProbeCollision(0), cxevProbeHit(0),  
		cxevSave(0), cxevSaveCollision(0), cxevSaveReplace(0), cxevInUse(0)
	{
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
		if (rgxev == nullptr) {
			rgxev = new XEV[cxevMax];
			if (rgxev == nullptr)
				throw 1;
		}
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
			rgxev[ixev].SetEvt(EVT::Nil);
		cxevProbe = cxevProbeCollision = cxevProbeHit = 0;
		cxevSave = cxevSaveCollision = cxevSaveReplace = 0;
		cxevInUse = 0;
	}


	/*	XT::array index
	 *
	 *	Returns a reference to the hash table entry that may or may not be used by the
	 *	board. Caller is responsible for making sure the entry is valid.
	 */
	inline XEV& operator[](const BDG& bdg)
	{
		return rgxev[bdg.habd & (cxevMax - 1)];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	inline XEV* Save(const BDG& bdg, EV ev, EVT evt, unsigned ply)
	{
		assert(ev != evInf && ev != -evInf);
		cxevSave++;
		XEV& xev = (*this)[bdg];
		if (xev.evt() == EVT::Nil)
			cxevInUse++;
		else {
			/* we have a collision; don't replace deeper evaluated boards, and don't overwrite
			   a principle variation unless we're saving another PV */
			if (xev.habd != bdg.habd)
				cxevSaveCollision++;
			if (ply < xev.ply())
				return nullptr;
			if (evt != EVT::Equal && xev.evt() == EVT::Equal)
				return nullptr;
			cxevSaveReplace++;
		}

		xev.habd = bdg.habd;
		xev.SetEv(ev);
		xev.SetEvt(evt);
		xev.SetPly(ply);
		return &xev;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as ply. Returns nullptr if no such entry exists.
	 */
	inline XEV* Find(const BDG& bdg, unsigned ply)
	{
		cxevProbe++;
		XEV& xev = (*this)[bdg];
		if (xev.evt() == EVT::Nil)
			return nullptr;
		if (xev.habd != bdg.habd || ply > xev.ply()) {
			cxevProbeCollision++;
			return nullptr;
		}
		cxevProbeHit++;
		return &xev;
	}
 };

