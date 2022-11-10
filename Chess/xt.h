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

class XEV
{
private:
	uint64_t uhabd;
	uint64_t uev : 16,
		umv : 32,
		uevt : 2,
		uply : 8,
		unused:6;
public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline XEV(void) { }
	inline EV ev(void) const noexcept { return static_cast<EV>(uev); }
	inline void SetEv(EV ev) noexcept { assert(ev != evInf && ev != -evInf);  uev = static_cast<uint16_t>(ev); }
	inline EVT evt(void) const noexcept { return static_cast<EVT>(uevt); }
	inline void SetEvt(EVT evt) noexcept { uevt = static_cast<unsigned>(evt); }
	inline int ply(void) const noexcept { return static_cast<int>(uply); }
	inline void SetPly(int ply) noexcept { uply = (unsigned)ply; }
	inline bool FMatchHabd(HABD habd) const noexcept { return uhabd == habd; }
	inline void SetHabd(HABD habd) noexcept { uhabd = habd; }
	inline void SetMv(MV mv) noexcept { this->umv = mv; }
	inline MV mv(void) const noexcept { return umv; }
	inline void Save(HABD habd, EV ev, EVT evt, int ply, MV mv) noexcept {
		SetHabd(habd);
		SetEv(ev);
		SetEvt(evt);
		SetPly(ply);
		SetMv(mv);
	}

};

static_assert(sizeof(XEV) == 16);

struct XEV2 {
	XEV xevDeep;
	XEV xevNew;
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
	XEV2* rgxev2;
public:
	const uint32_t cxev2Max = 1UL << 23;
	const uint32_t cxevMax = cxev2Max * 2;
	/* cache stats */
	uint64_t cxevProbe, cxevProbeCollision, cxevProbeHit;
	uint64_t cxevSave, cxevSaveCollision, cxevSaveReplace, cxevInUse;

public:
	XT(void) : rgxev2(nullptr), cxevProbe(0), cxevProbeCollision(0), cxevProbeHit(0),  
		cxevSave(0), cxevSaveCollision(0), cxevSaveReplace(0), cxevInUse(0)
	{
	}

	~XT(void)
	{
		if (rgxev2 != nullptr) {
			delete[] rgxev2;
			rgxev2 = nullptr;
		}
	}


	/*	XT::Init
	 *
	 *	Initializes a new transposition table. This must be called before first use.
	 */
	void Init(void)
	{
		if (rgxev2 == nullptr) {
			rgxev2 = new XEV2[cxev2Max];
			if (rgxev2 == nullptr)
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
		for (unsigned ixev2 = 0; ixev2 < cxev2Max; ixev2++) {
			rgxev2[ixev2].xevDeep.SetEvt(EVT::Nil);
			rgxev2[ixev2].xevNew.SetEvt(EVT::Nil);
		}
		cxevProbe = cxevProbeCollision = cxevProbeHit = 0;
		cxevSave = cxevSaveCollision = cxevSaveReplace = 0;
		cxevInUse = 0;
	}


	/*	XT::array index
	 *
	 *	Returns a reference to the hash table entry that may or may not be used by the
	 *	board. Caller is responsible for making sure the entry is valid.
	 */
	inline XEV2& operator[](const BDG& bdg) noexcept
	{
		return rgxev2[bdg.habd & (cxev2Max - 1)];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	inline XEV* Save(const BDG& bdg, const EMV& emv, EVT evt, int ply) noexcept
	{
		assert(emv.ev != evInf && emv.ev != -evInf);
		cxevSave++;

		XEV2& xev2 = (*this)[bdg];
		if (xev2.xevDeep.evt() == EVT::Nil) {
			cxevInUse++;
			xev2.xevDeep.Save(bdg.habd, emv.ev, evt, ply, emv.mv);
			return &xev2.xevDeep;
		}
		if (ply >= xev2.xevDeep.ply()) {
			if (!xev2.xevDeep.FMatchHabd(bdg.habd))
				cxevSaveCollision++;
			cxevSaveReplace++;
			xev2.xevDeep.Save(bdg.habd, emv.ev, evt, ply, emv.mv);
			return &xev2.xevDeep;
		}
		if (xev2.xevNew.evt() == EVT::Nil)
			cxevInUse++;
		else if (!xev2.xevNew.FMatchHabd(bdg.habd)) {
			cxevSaveCollision++;
			cxevSaveReplace++;
		}
		xev2.xevNew.Save(bdg.habd, emv.ev, evt, ply, emv.mv);
		return &xev2.xevNew;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as ply. Returns nullptr if no such entry exists.
	 */
	inline XEV* Find(const BDG& bdg, int ply) noexcept 
	{
		cxevProbe++;
		XEV2& xev2 = (*this)[bdg];
		if (xev2.xevDeep.evt() == EVT::Nil)
			return nullptr;
		if (xev2.xevDeep.FMatchHabd(bdg.habd)) {
			if (ply > xev2.xevDeep.ply())
				goto CheckNew;
			cxevProbeHit++;
			return &xev2.xevDeep;
		}
CheckNew:
		if (xev2.xevNew.evt() == EVT::Nil)
			return nullptr;
		if (xev2.xevNew.FMatchHabd(bdg.habd)) {
			if (ply > xev2.xevNew.ply())
				return nullptr;
			cxevProbeHit++;
			return &xev2.xevNew;
		}
		cxevProbeCollision++;
		return nullptr;
	}
 };

