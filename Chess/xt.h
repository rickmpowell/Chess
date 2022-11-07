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
	Equal,
	Lower,
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

/* cache entries don't save the entire HABD, because the lower bits of the HABD are 
 * used to index into the cache, so we know they already match */ 
#define bitsHabdUpper 64
#define bitsHabdLower 25
static_assert(bitsHabdUpper + bitsHabdLower >= 64);

class XEV
{
private:
	uint64_t habdUpperGrf : bitsHabdUpper;
	uint64_t evGrf : 16,
		mvGrf : 32,
		evtGrf : 2,
		plyGrf : 8,
		unused:6;
public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline XEV(void) { }
	inline EV ev(void) const noexcept { return static_cast<EV>(evGrf); }
	inline void SetEv(EV ev) noexcept { assert(ev != evInf && ev != -evInf);  evGrf = static_cast<uint16_t>(ev); }
	inline EVT evt(void) const noexcept { return static_cast<EVT>(evtGrf); }
	inline void SetEvt(EVT evt) noexcept { evtGrf = static_cast<unsigned>(evt); }
	inline unsigned ply(void) const noexcept { return static_cast<unsigned>(plyGrf); }
	inline void SetPly(unsigned ply) noexcept { plyGrf = ply; }
	inline bool FMatchHabd(HABD habd) const noexcept { return habdUpperGrf == (habd >> (64 - bitsHabdUpper)); }
	inline void SetHabd(HABD habd) noexcept { habdUpperGrf = habd >> (64 - bitsHabdUpper); }
	inline void SetMv(MV mv) noexcept { this->mvGrf = mv; }
	inline MV mv(void) const noexcept { return mvGrf; }
};

static_assert(sizeof(XEV) == 16);


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
	const uint32_t cxevMax = 1UL << bitsHabdLower;
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
	void Clear(void) noexcept
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
	inline XEV& operator[](const BDG& bdg) noexcept
	{
		return rgxev[bdg.habd & (cxevMax - 1)];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	inline XEV* Save(const BDG& bdg, const EMV& emv, EVT evt, unsigned ply) noexcept
	{
		assert(emv.ev != evInf && emv.ev != -evInf);
		cxevSave++;

		XEV& xev = (*this)[bdg];
		if (xev.evt() == EVT::Nil)
			cxevInUse++;
		else {
			/* we have a collision; don't replace deeper evaluated boards, and don't 
			   overwrite a un-pruned valuation unless we're saving another */
			if (!xev.FMatchHabd(bdg.habd))
				cxevSaveCollision++;
			if (evt == EVT::Equal) {
				if (xev.evt() == EVT::Equal && ply < xev.ply())
					return nullptr;
			}
			else {
				if (xev.evt() == EVT::Equal || ply < xev.ply())
					return nullptr;
			}
			cxevSaveReplace++;
		}

		xev.SetHabd(bdg.habd);
		xev.SetEv(emv.ev);
		xev.SetEvt(evt);
		xev.SetPly(ply);
		xev.SetMv(emv.mv);
		return &xev;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as ply. Returns nullptr if no such entry exists.
	 */
	inline XEV* Find(const BDG& bdg, unsigned ply) noexcept 
	{
		cxevProbe++;
		XEV& xev = (*this)[bdg];
		if (xev.evt() == EVT::Nil)
			return nullptr;
		if (!xev.FMatchHabd(bdg.habd) || ply > xev.ply()) {
			cxevProbeCollision++;
			return nullptr;
		}
		cxevProbeHit++;
		return &xev;
	}
 };

