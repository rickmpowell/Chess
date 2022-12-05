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
 *	TEV - evaluation types stored in the transposition table, tells if the board 
 *	evaluation is equal to the saved value, above it, or below it. 
 * 
 *	Comparison operators return the relative importance of the transposition entry,
 *	all else being equal. PV nodes are most important, cut nodes second, etc.
 */

enum TEV : int {
	tevNull = 0,
	tevLower = 1,
	tevHigher = 2,
	tevEqual = 3
};

inline int crunch(TEV tev) noexcept { return tev - (tev >> 1); }
inline bool operator>(TEV tev1, TEV tev2) noexcept { return crunch(tev1) > crunch(tev2); }
inline bool operator<(TEV tev1, TEV tev2) noexcept { return crunch(tev1) < crunch(tev2); }


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
	uint32_t umv;
	uint16_t utev : 2,
		udepth : 8,
		uage : 6;
	uint16_t uev;
public:

#pragma warning(suppress:26495)	// don't warn about uninitialized member variables 
	inline XEV(void) { SetNull(); }
	inline XEV(HABD habd, MV mv, TEV tev, EV ev, int depth) 
	{ uhabd = habd;  umv = mv; utev = tev; uev = ev; udepth = depth; }
	inline void SetNull(void) noexcept { *(uint64_t*)this = 0; *((uint64_t*)this + 1) = 0; }
	inline EV ev(void) const noexcept { return static_cast<EV>(uev); }
	inline void SetEv(EV ev) noexcept { assert(ev != evInf && ev != -evInf);  uev = static_cast<uint16_t>(ev); }
	inline TEV tev(void) const noexcept { return static_cast<TEV>(utev); }
	inline void SetTev(TEV tev) noexcept { utev = static_cast<unsigned>(tev); }
	inline int depth(void) const noexcept { return static_cast<int>(udepth); }
	inline void SetDepth(int depth) noexcept { udepth = (unsigned)depth; }
	inline bool FMatchHabd(HABD habd) const noexcept { return uhabd == habd; }
	inline void SetHabd(HABD habd) noexcept { uhabd = habd; }
	inline void SetMv(MV mv) noexcept { this->umv = mv; }
	inline MV mv(void) const noexcept { return umv; }
	inline void SetAge(unsigned age) noexcept { this->uage = age; }
	inline unsigned age(void) const noexcept { return uage; }
	inline void Save(HABD habd, EV ev, TEV tev, int depth, MV mv, unsigned age) noexcept {
		SetHabd(habd);
		SetEv(ev);
		SetTev(tev);
		SetDepth(depth);
		SetMv(mv);
		SetAge(age);
	}

};


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
	const uint32_t cxev2Max = 1UL << /*23*/ 18;
	static_assert(sizeof(XEV2) == (1 << 5));
	const uint32_t cxev2MaxMask = cxev2Max - 1;
	const uint32_t cxevMax = cxev2Max * 2;
	const unsigned ageMax = 2;
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
			rgxev2[ixev2].xevDeep.SetNull();
			rgxev2[ixev2].xevNew.SetNull();
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
			if (Dage(rgxev2[ixev2].xevDeep) >= ageMax - 1) {
#ifndef NOSTATS
				if (rgxev2[ixev2].xevDeep.tev() != tevNull)
					cxevInUse--;
#endif
				rgxev2[ixev2].xevDeep.SetNull();
			}
			if (Dage(rgxev2[ixev2].xevNew) >= ageMax - 1) {
#ifndef NOSTATS
				if (rgxev2[ixev2].xevNew.tev() != tevNull)
					cxevInUse--;
#endif
				rgxev2[ixev2].xevNew.SetNull();
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
		return rgxev2[bdg.habd & cxev2MaxMask];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	inline XEV* Save(const BDG& bdg, const EMV& emv, TEV tev, int depth) noexcept
	{
		assert(emv.ev != evInf && emv.ev != -evInf);
#ifndef NOSTATS
		cxevSave++;
#endif
		/* keep track of the deepest search */

		XEV2& xev2 = (*this)[bdg];
		if (!(tev < xev2.xevDeep.tev()) && depth >= xev2.xevDeep.depth()) {
#ifndef NOSTATS
			if (xev2.xevDeep.tev() == tevNull)
				cxevInUse++;
			else {
				cxevSaveReplace++;
				if (!xev2.xevDeep.FMatchHabd(bdg.habd))
					cxevSaveCollision++;
			}
#endif
			xev2.xevDeep.Save(bdg.habd, emv.ev, tev, depth, emv.mv, age);
			return &xev2.xevDeep;
		}

		if (!(tev < xev2.xevNew.tev())) {
#ifndef NOSTATS
			if (xev2.xevNew.tev() == tevNull)
				cxevInUse++;
			else {
				cxevSaveReplace++;
				if (!xev2.xevNew.FMatchHabd(bdg.habd))
					cxevSaveCollision++;
			}
#endif
			xev2.xevNew.Save(bdg.habd, emv.ev, tev, depth, emv.mv, age);
			return &xev2.xevNew;
		}

		return nullptr;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as depth. Returns nullptr if no such entry exists.
	 */
	/*inline*/ __declspec(noinline) XEV* Find(const BDG& bdg, int depth) noexcept
	{
#ifndef NOSTATS
		cxevProbe++;
#endif
		XEV2& xev2 = (*this)[bdg];
		if (xev2.xevDeep.tev() == tevNull) [[likely]]
			return nullptr;
		if (xev2.xevDeep.FMatchHabd(bdg.habd) && depth <= xev2.xevDeep.depth()) {
#ifndef NOSTATS
			cxevProbeHit++;
#endif
			return &xev2.xevDeep;
		}

		if (xev2.xevNew.tev() == tevNull)
			return nullptr;
		if (xev2.xevNew.FMatchHabd(bdg.habd)) {
			if (depth > xev2.xevNew.depth())
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

