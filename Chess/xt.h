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
 *	Evaluations in this table is always based on eval, not fast scoring.
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

__forceinline int crunch(TEV tev) noexcept { return tev - (tev >> 1); }
__forceinline bool operator>(TEV tev1, TEV tev2) noexcept { return crunch(tev1) > crunch(tev2); }
__forceinline bool operator<(TEV tev1, TEV tev2) noexcept { return crunch(tev1) < crunch(tev2); }


/*
 *
 *	XEV class
 * 
 *	The transposition table evaluation structure. Holds the results of a single
 *	alpha-beta search board eval. Uses a partial board hash to index into the
 *	table, so we need to keep the hash itself in the structure to verify the
 *	match.
 * 
 */

#pragma pack(push, 1)
class XEV
{
private:
	uint32_t 
		uhabdLow:16,
		umv:16;			
	uint32_t
		udd : 7,		/* word-aligned */
		ufVisited : 1,
		utev : 2,		/* word-aligned */
		uage : 2,
		unused1 : 4,
		uevBiased : 15, /* word-aligned */
		unused2 : 1;
public:

	/* most of this crap is just here to cast bit fields to the correct type  */

#pragma warning(suppress:26495)	// don't warn about uninitialized member variables 
	__forceinline XEV(void) { SetNull(); }
	__forceinline XEV(HABD habd, MVU mvu, TEV tev, EV ev, int d, int dLim) { Save(habd, ev, tev, d, dLim, mvu, 0); }
	__forceinline void SetNull(void) noexcept { memset(this, 0, sizeof(XEV)); }

	/* store ev biased so we can get the sign extended on extraction; mate evals
	   also must be made relative to depth we're storing from (since they can be
	   extracted at a different depth than they were stored from), while during
	   search they are always relative to the root */
	
	__forceinline EV ev(int d) const noexcept
	{
		EV evT = static_cast<EV>(uevBiased) - evBias;
		if (FEvIsMate(evT))
			evT -= d;
		else if (FEvIsMate(-evT))
			evT += d;
		return evT;
	}
	
	__forceinline void SetEv(EV ev, int d) noexcept 
	{ 
		assert(ev < evInf && ev > -evInf); 
		if (FEvIsMate(ev))
			ev += d;
		else if (FEvIsMate(-ev))
			ev -= d;
		uevBiased = static_cast<uint16_t>(ev + evBias); 
	}
	
	/* eval type, equal, less, or greater; null for unused entries */
	__forceinline TEV tev(void) const noexcept { return static_cast<TEV>(utev); }
	__forceinline void SetTev(TEV tev) noexcept { utev = static_cast<unsigned>(tev); }
	
	/* depth of the search that resulted in this eval */
	__forceinline int dd(void) const noexcept { return static_cast<int>(udd); }
	__forceinline void SetDd(int dd) noexcept { udd = (unsigned)dd; }
	
	/* hash match; this is not exact, but the high bits are used to index into the table, 
	   and this test makes sure the low bits match */
	__forceinline bool FMatchHabd(HABD habd) const noexcept { return uhabdLow == (uint16_t)habd; }
	__forceinline void SetHabd(HABD habd) noexcept { uhabdLow = (uint16_t)habd; }
	
	/* for tevEqual entries, the best move from this position. For higher than
	   entries, it'll be the move that caused the cut; for lower, I think it might
	   be the best move in the position */
	__forceinline MV mv(void) const noexcept { return umv; }
	__forceinline void SetMv(MV mv) noexcept { this->umv = mv; }

	/* age of the entry */
	__forceinline unsigned age(void) const noexcept { return uage; }
	__forceinline void SetAge(unsigned age) noexcept { this->uage = age; }
	
	/* visited bit used for PV extraction to detect move loops */
	__forceinline bool fVisited(void) const noexcept { return ufVisited; }
	__forceinline void SetFVisited(bool fVisitedNew) noexcept { this->ufVisited = static_cast<unsigned>(fVisitedNew); }
	
	void Save(HABD habd, EV ev, TEV tev, int d, int dLim, MV mv, unsigned age) noexcept 
	{
		SetHabd(habd);
		SetEv(ev, d);
		SetTev(tev);
		SetDd(dLim - d);
		SetMv(mv);
		SetAge(age);
	}
};
#pragma pack(pop)


/*
 *
 *	XEV2
 * 
 *	Our cache map uses a 2-strategy entry for each hash index. We keep both
 *	the deepest node found during the search, and the newest. 
 * 
 */


#pragma pack(push, 1)
__declspec(align(2)) struct XEV2 {
	XEV xevDeep;
	XEV xevNew;
};
#pragma pack(pop)


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
	XEV2* axev2;

public:
	int shfXev2Max;
	int shfXev2MaxIndex;
	uint32_t cxev2Max;
	uint32_t cxevMax;
	const unsigned ageMax = 4;

public:
	unsigned age;
#ifndef NOSTATS
	/* cache stats */
	uint64_t cxevProbe, cxevProbeHit;
	uint64_t cxevSave, cxevSaveCollision, cxevSaveReplace, cxevInUse;
#endif

public:
	XT(void) : axev2(nullptr), 
#ifndef NOSTATS
		cxevProbe(0), cxevProbeHit(0),  
		cxevSave(0), cxevSaveCollision(0), cxevSaveReplace(0), cxevInUse(0),
#endif
		cxevMax(0x20000L), cxev2Max(0x10000L), shfXev2Max(17), shfXev2MaxIndex(64-17), age(0)
	{
	}

	~XT(void)
	{
		if (axev2 != nullptr) {
			delete[] axev2;
			axev2 = nullptr;
		}
	}

	
	/*	XT::Init
	 *
	 *	Initializes a new transposition table. This must be called before first use.
	 *	Size of the cache we're allowed to use is in cbCache.
	 */
	void Init(uint32_t cbCache)
	{
		/* cache needs to be power of 2 number of elements */

		for (cxev2Max = cbCache / sizeof(XEV2); cxev2Max & (cxev2Max - 1); cxev2Max &= cxev2Max - 1)
			;
		cxevMax = 2 * cxev2Max;
		shfXev2Max = bitscan(cxev2Max);
		shfXev2MaxIndex = 64 - shfXev2Max;
		
		if (axev2 == nullptr) {
			axev2 = new XEV2[cxev2Max];
			if (axev2 == nullptr)
				throw 1;
		}
		else {
			for (unsigned ixev2 = 0; ixev2 < cxev2Max; ixev2++) {
				axev2[ixev2].xevDeep.SetNull();
				axev2[ixev2].xevNew.SetNull();
			}
		}

#ifndef NOSTATS
		cxevProbe = cxevProbeHit = 0;
		cxevSave = cxevSaveCollision = cxevSaveReplace = 0;
		cxevInUse = 0;
#endif
	}


	/*	XT::Dage
	 *
	 *	Difference in ages between the entry and the table's current age. This
	 *	is always a positive number. Handles the wrap around at the overflow
	 *	of the bits we save for the age.
	 */
	__forceinline unsigned Dage(const XEV& xev) const noexcept
	{
		assert((ageMax & (ageMax-1)) == 0);
		return (age - xev.age()) & (ageMax - 1);
	}

	
	__forceinline bool FXevTooOld(const XEV& xev) const noexcept
	{
		return Dage(xev) >= ageMax - 1;
	}

	/*	XT::Clear
	 *
	 *	Clears out old entries in the hash table and bumps the age of
	 *	the table.
	 */
	void Clear(void) noexcept
	{
		/* age out really old entries */

		for (unsigned ixev2 = 0; ixev2 < cxev2Max; ixev2++) {
			if (FXevTooOld(axev2[ixev2].xevDeep)) {
#ifndef NOSTATS
				if (axev2[ixev2].xevDeep.tev() != tevNull)
					cxevInUse--;
#endif
				axev2[ixev2].xevDeep.SetNull();
			}
			if (FXevTooOld(axev2[ixev2].xevNew)) {
#ifndef NOSTATS
				if (axev2[ixev2].xevNew.tev() != tevNull)
					cxevInUse--;
#endif
				axev2[ixev2].xevNew.SetNull();
			}
		}
	}

	void BumpAge(void)
	{
		assert((ageMax & (ageMax-1)) == 0);
		age = (age + 1) & (ageMax - 1);
	}

	/*	XT::array index
	 *
	 *	Returns a reference to the hash table entry that may or may not be used by the
	 *	board. Caller is responsible for making sure the entry is valid.
	 */
	__forceinline XEV2& operator[](const BDG& bdg) noexcept
	{
		uint32_t ixev2 = (uint32_t)(bdg.habd >> shfXev2MaxIndex);
		assert(ixev2 < cxev2Max);
		return axev2[ixev2];
	}


	/*	XT::Save
	 *
	 *	Saves the evaluation information in the transposition table. Not guaranteed to 
	 *	actually save the eval, using our aging heuristics.
	 */
	__declspec(noinline) XEV* Save(const BDG& bdg, const MVE& mve, TEV tev, int d, int dLim) noexcept
	{
		assert(mve.ev != evInf && mve.ev != -evInf);
		assert(tev != tevNull);
#ifndef NOSTATS
		cxevSave++;
#endif
		/* keep track of the deepest search */

		XEV2& xev2 = (*this)[bdg];
		if (!(tev < xev2.xevDeep.tev()) && dLim-d >= xev2.xevDeep.dd()) {
#ifndef NOSTATS
			if (xev2.xevDeep.tev() == tevNull)
				cxevInUse++;
			else {
				cxevSaveReplace++;
				if (!xev2.xevDeep.FMatchHabd(bdg.habd))
					cxevSaveCollision++;
			}
#endif
			xev2.xevDeep.Save(bdg.habd, mve.ev, tev, d, dLim, mve, age);
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
			xev2.xevNew.Save(bdg.habd, mve.ev, tev, d, dLim, mve, age);
			return &xev2.xevNew;
		}

		return nullptr;
	}


	/*	XT::Find
	 *
	 *	Searches for the board in the transposition table, looking for an evaluation that is
	 *	at least as deep as depth. Returns nullptr if no such entry exists.
	 */
	__declspec(noinline) XEV* Find(const BDG& bdg, int d, int dLim) noexcept
	{
#ifndef NOSTATS
		cxevProbe++;
#endif
		XEV2& xev2 = (*this)[bdg];
		if (xev2.xevDeep.FMatchHabd(bdg.habd) && dLim-d <= xev2.xevDeep.dd()) {
#ifndef NOSTATS
			cxevProbeHit++;
#endif
			xev2.xevDeep.SetAge(age);
			return &xev2.xevDeep;
		}

		if (xev2.xevNew.FMatchHabd(bdg.habd) && dLim-d <= xev2.xevNew.dd()) {
#ifndef NOSTATS
			cxevProbeHit++;
#endif
			xev2.xevNew.SetAge(age);
			return &xev2.xevNew;
		}
		return nullptr;
	}
 };

