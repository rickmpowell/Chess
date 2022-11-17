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
 *	Evaluations in this table is always a full eval, not the fast scoring used for 
 *	pre-ordering.
 *
 */

/*
 *	EVT - evaluation types stored in the transposition table, tells if the board 
 *	evaluation is equal to the saved value, above it, or below it. 
 * 
 *	Comparison operators return the relative importance of the transposition entry,
 *	all else being equal. PV nodes are most important, cut nodes second, etc.
 */

enum EVT : int {
	evtNull = 0,
	evtLower = 1,
	evtHigher = 2,
	evtEqual = 3
};

inline int crunch(EVT evt) noexcept { return evt - (evt >> 1); }
inline bool operator>(EVT evt1, EVT evt2) noexcept { return crunch(evt1) > crunch(evt2); }
inline bool operator<(EVT evt1, EVT evt2) noexcept { return crunch(evt1) < crunch(evt2); }


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
		uage:6;
public:

#pragma warning(suppress:26495)	// don't warn about optimized bulk initialized member variables 
	inline XEV(void) { }
	inline XEV(HABD habd, MV mv, EVT evt, EV ev, int ply) 
	{ uhabd = habd;  umv = mv; uevt = evt; uev = ev; uply = ply; }
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
	inline void SetAge(unsigned age) noexcept { this->uage = age; }
	inline unsigned age(void) const noexcept { return uage; }
	inline void Save(HABD habd, EV ev, EVT evt, int ply, MV mv, unsigned age) noexcept {
		SetHabd(habd);
		SetEv(ev);
		SetEvt(evt);
		SetPly(ply);
		SetMv(mv);
		SetAge(age);
	}

};

const unsigned ageMax = 16;

static_assert(sizeof(XEV) == 16);


/*
 *
 *	XEV2
 * 
 *	Our cache map uses a 2-strategy entry for each hash index. We keep both
 *	the deepest node found during the search, and the newest. 
 * 
 */


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
	unsigned age;
#ifndef NOSTATS
	/* cache stats */
	uint64_t cxevProbe, cxevProbeCollision, cxevProbeHit;
	uint64_t cxevSave, cxevSaveCollision, cxevSaveReplace, cxevInUse;
#endif

public:
	XT(void) : rgxev2(nullptr), 
#ifndef NOSTATS
		cxevProbe(0), cxevProbeCollision(0), cxevProbeHit(0),  
		cxevSave(0), cxevSaveCollision(0), cxevSaveReplace(0), cxevInUse(0),
#endif
		age(0)
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
		for (unsigned ixev2 = 0; ixev2 < cxev2Max; ixev2++) {
			rgxev2[ixev2].xevDeep.SetEvt(evtNull);
			rgxev2[ixev2].xevDeep.SetAge(0);
			rgxev2[ixev2].xevNew.SetEvt(evtNull);
			rgxev2[ixev2].xevNew.SetAge(0);
		}
#ifndef NOSTATS
		cxevProbe = cxevProbeCollision = cxevProbeHit = 0;
		cxevSave = cxevSaveCollision = cxevSaveReplace = 0;
		cxevInUse = 0;
#endif
	}

	unsigned Dage(const XEV& xev) const noexcept
	{
		if (age < xev.age())
			return age + ageMax - xev.age();
		else
			return age - xev.age();
	}


	/*	XT::Clear
	 *
	 *	Clears out the hash table. Current implementation requires we do this before
	 *	starting any move search.
	 */
	void Clear(void) noexcept
	{
		/* age out really old entries, and bump the table age */
		for (unsigned ixev2 = 0; ixev2 < cxev2Max; ixev2++) {
			if (Dage(rgxev2[ixev2].xevDeep) > ageMax / 2) {
				rgxev2[ixev2].xevDeep.SetEvt(evtNull);
#ifndef NOSTATS
				cxevInUse--;
#endif
			}
			if (Dage(rgxev2[ixev2].xevNew) > ageMax / 2) {
				rgxev2[ixev2].xevNew.SetEvt(evtNull);
#ifndef NOSTATS
				cxevInUse--;
#endif
			}
		}
		age = (age + 1) & (ageMax - 1);
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
#ifndef NOSTATS
		cxevSave++;
#endif
		/* keep track of the deepest search */

		XEV2& xev2 = (*this)[bdg];
		if (!(evt < xev2.xevDeep.evt()) && ply >= xev2.xevDeep.ply()) {
#ifndef NOSTATS
			if (xev2.xevDeep.evt() == evtNull)
				cxevInUse++;
			else
				cxevSaveReplace++;
			if (!xev2.xevDeep.FMatchHabd(bdg.habd))
				cxevSaveCollision++;
#endif
			xev2.xevDeep.Save(bdg.habd, emv.ev, evt, ply, emv.mv, age);
			return &xev2.xevDeep;
		}

		if (!(evt < xev2.xevNew.evt())) {
#ifndef NOSTATS
			if (xev2.xevNew.evt() == evtNull)
				cxevInUse++;
			else
				cxevSaveReplace++;
			if (!xev2.xevNew.FMatchHabd(bdg.habd))
				cxevSaveCollision++;
#endif
			xev2.xevNew.Save(bdg.habd, emv.ev, evt, ply, emv.mv, age);
			return &xev2.xevNew;
		}

		return nullptr;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as ply. Returns nullptr if no such entry exists.
	 */
	inline XEV* Find(const BDG& bdg, int ply) noexcept 
	{
#ifndef NOSTATS
		cxevProbe++;
#endif
		XEV2& xev2 = (*this)[bdg];
		if (xev2.xevDeep.evt() == evtNull)
			return nullptr;
		if (xev2.xevDeep.FMatchHabd(bdg.habd)) {
			if (ply > xev2.xevDeep.ply())
				goto CheckNew;
#ifndef NOSTATS
			cxevProbeHit++;
#endif
			return &xev2.xevDeep;
		}
CheckNew:
		if (xev2.xevNew.evt() == evtNull)
			return nullptr;
		if (xev2.xevNew.FMatchHabd(bdg.habd)) {
			if (ply > xev2.xevNew.ply())
				return nullptr;
#ifndef NOSTATS
			cxevProbeHit++;
#endif
			return &xev2.xevNew;
		}
#ifndef NOSTATS
		cxevProbeCollision++;
#endif
		return nullptr;
	}
 };

