/*
 *
 *	pl.cpp
 * 
 *	Code for the player class
 * 
 */

#include "pl.h"
#include "ga.h"
#include "debug.h"
#include "Resources/Resource.h"

/* these must be powers of two */
#ifndef NDEBUG
const uint16_t dcYield = 8;
#else
const uint16_t dcYield = 1024;
#endif



PL::PL(GA& ga, wstring szName) : ga(ga), szName(szName), level(3),
		mvNext(MV()), spmvNext(SPMV::Animate)
{
}


PL::~PL(void)
{
}


/*
 *	Evaluation manipulators and constants
 */

const int plyMax = 127;
const EV evInf = 640*100;	/* largest possible evaluation, in centi-pawns */
const EV evMate = evInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EV evMateMin = evMate - plyMax;
const EV evTempo = 33;	/* evaluation of a single move advantage */
const EV evDraw = 0;	/* evaluation of a draw */
const EV evQuiescent = evInf + 1;

inline EV EvMate(int ply)
{
	return evMate - ply;
}

inline bool FEvIsMate(EV ev)
{
	return ev >= evMateMin;
}

inline int PlyFromEvMate(EV ev)
{
	return evMate - ev;
}


wstring SzFromEv(EV ev)
{
	wchar_t sz[20], *pch = sz;
	if (ev >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		ev = -ev;
	}
	if (ev == evInf)
		*pch++ = L'\x221e';
	else if (ev == evQuiescent)
		*pch++ = L'Q';
	else if (ev > evInf)
		*pch++ = L'*';
	else if (FEvIsMate(ev)) {
		*pch++ = L'#';
		pch = PchDecodeInt((PlyFromEvMate(ev)+1) / 2, pch);
	}
	else {
		pch = PchDecodeInt(ev / 100, pch);
		ev %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + ev / 10;
		*pch++ = L'0' + ev % 10;
	}
	*pch = 0;
	return wstring(sz);
}


/*	PL::ReceiveMv
 *
 *	receives a move from an external source, like the human UI, or by playing from
 *	a file.
 */
void PL::ReceiveMv(MV mv, SPMV spmv)
{
	if (!ga.fInPlay) {
		ga.MakeMv(mv, spmv);
		ga.Play();
	}
	else {
		mvNext = mv;
		spmvNext = spmv;
	}
}


EV PL::EvFromPhaseApcSq(PHASE phase, APC apc, SQ sq) const noexcept
{
	return EvBaseApc(apc);
}


EV PL::EvBaseApc(APC apc) const noexcept
{
	EV mpapcev[APC::ActMax] = { 0, 100, 275, 300, 500, 900, 200 };
	return mpapcev[apc];
}


bool PL::FDepthLog(LGT lgt, int& depth)
{
	return ga.FDepthLog(lgt, depth);
}


void PL::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	ga.AddLog(lgt, lgf, depth, tag, szData);
}


int PL::DepthLog(void) const
{
	return ga.DepthLog();
}


void PL::SetDepthLog(int depthNew)
{
	ga.SetDepthLog(depthNew);
}


/*
 *
 *	PLAI
 * 
 *	AI computer players. Base PLAI implements alpha-beta pruned look ahead tree.
 * 
 */


PLAI::PLAI(GA& ga) : PL(ga, L"SQ Mobly"), rgen(372716661UL), evalRandomDist(-1, 1), 
		cYield(0), cmvevEval(0), cmvevNode(0)
{
	fecoMaterial = 100;
	fecoMobility = 10;
	fecoKingSafety = 20;
	fecoPawnStructure = 20;
	fecoTempo = 100;
	fecoRandom = 0;
	InitWeightTables();
}


/*	PLAI::FHasLevel
 *
 *	Returns true if the player has a quality of play level control.
 */
bool PLAI::FHasLevel(void) const noexcept
{
	return true;
}


/*	PLAI::SetLevel
 *
 *	Sets the quality of play level for the AI.
 */
void PLAI::SetLevel(int level) noexcept
{
	this->level = clamp(level, 1, 10);
}


/*	PLAI::CmvFromLevel
 *
 *	Converts quality of play level into number of moves to consider when
 *	making a move.
 */
float PLAI::CmvFromLevel(int level) const noexcept
{
	switch (level) {
	case 1: return 100.0f;
	case 2: return 8.0e+3f;
	case 3: return 3.2e+4f;
	case 4: return 1.2e+5f;
	default:
	case 5: return 5.0e+5f;
	case 6: return 2.0e+6f;
	case 7: return 8.0e+6f;
	case 8: return 3.2e+7f;
	case 9: return 1.2e+8f;
	case 10: return 5.0e+8f;
	}
}


int PLAI::PlyLim(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int plyLim = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return plyLim;
}


/*	PLAI::StartMoveLog
 *
 *	Opens a log entry for AI move generation. Must be paired with a corresponding
 *	EndMoveLog
 */
void PLAI::StartMoveLog(void)
{
	tpStart = high_resolution_clock::now();
	cmvevEval = 0L;
	cmvevNode = 0L;
	LogOpen(TAG(ga.bdg.cpcToMove == CPC::White ? L"White" : L"Black", ATTR(L"FEN", ga.bdg)),
		L"(" + szName + L")");
}


/*	PLAI::EndMoveLog
 *
 *	Closes the move generation logging entry for AI move generation.
 */
void PLAI::EndMoveLog(void)
{
	time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
	LogData(L"Evaluated " + to_wstring(cmvevEval) + L" boards");
	LogData(L"Nodes: " + to_wstring(cmvevNode));
	duration dtp = tpEnd - tpStart;
	milliseconds ms = duration_cast<milliseconds>(dtp);
	LogData(L"Time: " + to_wstring(ms.count()) + L"ms");
	float sp = (float)cmvevNode / (float)ms.count();
 	LogData(L"Speed: " + to_wstring((int)round(sp)) + L" n/ms");
	LogClose(L"", L"", LGF::Normal);
}


/*
 *
 *	LOGOPENMVEV
 * 
 *	Little open/close log wrapper
 * 
 */
class LOGOPENMVEV
{
	PL& pl;
	const BDG& bdg;
	MVEV& mvev;
	wstring szData;
	LGF lgf;
	int depthLogSav;
	int imvExpandSav;

	static int imvExpand;
	static MV rgmv[8];

public:
	inline LOGOPENMVEV(PL& pl, const BDG& bdg, MVEV& mvev, 
			EV evAlpha, EV evBeta, int ply, wchar_t chType=L' ') : 
		pl(pl), bdg(bdg), mvev(mvev), szData(L""), lgf(LGF::Normal)
	{
		depthLogSav = DepthLog();
		imvExpandSav = imvExpand;
		if (FExpandLog(mvev))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)),
				wstring(1, chType) + to_wstring(ply) + L" " + SzFromEv(mvev.ev) + 
					L" [" + SzFromEv(evAlpha) + L", " + SzFromEv(evBeta) + L"] ");
	}

	inline void SetData(const wstring& szNew, LGF lgfNew)
	{
		szData = szNew;
		lgf = lgfNew;
	}

	inline ~LOGOPENMVEV()
	{
		LogClose(bdg.SzDecodeMvPost(mvev.mv), 
			szData.size() > 0 ? szData : 
				SzFromEv(mvev.ev) /*+ L" (" + bdg.SzDecodeMvPost(mvev.mvReplyBest) + L")"*/,
			lgf);
		SetDepthLog(depthLogSav);
		imvExpand = imvExpandSav;
	}

	inline bool FExpandLog(MVEV& mvev) const
	{
		if (mvev.mv.sqFrom() != rgmv[imvExpand].sqFrom() ||	
				mvev.mv.sqTo() != rgmv[imvExpand].sqTo() ||
				mvev.mv.apcPromote() != rgmv[imvExpand].apcPromote())
			return false;
		imvExpand++;
		return true;
	}

	inline void SetDepthLog(int depth)
	{
		pl.SetDepthLog(depth);
	}

	inline int DepthLog(void) const
	{
		return pl.DepthLog();
	}

	inline bool FDepthLog(LGT lgt, int& depth)
	{
		return pl.FDepthLog(lgt, depth);
	}

	inline void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
	{
		pl.AddLog(lgt, lgf, depth, tag, szData);
	}
};

/* a little debugging aid to trigger a change in log depth after a 
   specific sequence of moves */
int LOGOPENMVEV::imvExpand = 0;
MV LOGOPENMVEV::rgmv[] = { /*
	   MV(sqD2, sqD4, pcWhitePawn),
	   MV(sqD7, sqD6, pcBlackPawn),
	   MV(sqC1, sqG5, pcWhiteBishop),
	   MV(sqE7, sqE5, pcBlackPawn),
	   MV(sqG5, sqD8, pcWhiteBishop), */
	   MV()
};



/*	PLAI::MvGetNext
 *
 *	Returns the next move for the player. Returns information for how the board 
 *	should display the move in spmv. 
 */
MV PLAI::MvGetNext(SPMV& spmv)
{
	spmv = SPMV::Animate;
	cYield = 0;

	StartMoveLog();

	InitWeightTables();
	int plyLim;

	GMV gmv;
	BDG bdg = ga.bdg;
	vector<MVEV> vmvev;

	/* figure out how deep we're going to search */

	bdg.GenGmv(gmv, GG::Pseudo);
	plyLim = clamp(PlyLim(bdg, gmv), 2, 14);
	LogData(L"Search depth: " + to_wstring(plyLim));

	/* pre-sort to improve quality of alpha-beta pruning */

	PreSortVmvev(bdg, gmv, vmvev);

	EV evAlpha = -evInf, evBeta = evInf;
	EV evBest = -evInf;
	const MVEV* pmvevBest = nullptr;

	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makemv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			EV ev = -EvBdgDepth(bdg, mvev, 1, plyLim, -evBeta, -evAlpha);
			if (ev > evBest) {
				evBest = ev;
				pmvevBest = &mvev;
			}
			// no pruning at top level 
			assert(ev < evBeta);
			if (ev > evAlpha) {
				evAlpha = ev;
				if (FEvIsMate(ev))
					plyLim = PlyFromEvMate(ev);
			}
		}
	}
	catch (...) {
		/* force aborted games to return nil */
		pmvevBest = nullptr;
	}

	EndMoveLog();
	return pmvevBest == nullptr ? MV() : pmvevBest->mv;
}


/*	PLAI::EvalBdgDepth
 *
 *	Evaluates the board/move from the point of view of the person who has the move,
 *	Evaluates to the target depth depthMax; depth is current. Uses alpha-beta pruning.
 * 
 *	On entry mvevLast has the last move made. It will also contain pseudo-legal 
 *	moves for the next player to move (i.e., some moves may leave king in check).
 * 
 *	Returns board evaluation in centi-pawns from the point of view of the side with
 *	the move.
 */
EV PLAI::EvBdgDepth(BDG& bdg, MVEV& mvevLast, int ply, int plyLim, EV evAlpha, EV evBeta)
{
	if (ply >= plyLim)
		return EvBdgQuiescent(bdg, mvevLast, ply, evAlpha, evBeta);

	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evAlpha, evBeta, ply, L' ');
	PumpMsg();

	/* pre-sort the pseudo-legal moves in mvevEval */
	vector<MVEV> vmvev;
	PreSortVmvev(bdg, mvevLast.gmvPseudoReply, vmvev);
	
	int cmvLegal = 0;
	EV evBest = -evInf;
	const MVEV* pmvevBest = nullptr;

	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makevmv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			cmvLegal++;
			EV ev = -EvBdgDepth(bdg, mvev, ply + 1, plyLim, -evBeta, -evAlpha);
			if (ev > evBest) {
				evBest = ev;
				pmvevBest = &mvev;
			}
			if (ev >= evBeta)
				return mvevLast.ev = evBest;
			if (ev > evAlpha) {
				evAlpha = ev;
				if (FEvIsMate(ev))
					plyLim = PlyFromEvMate(ev);
			}
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* test for checkmates, stalemates, and other draws */

	if (cmvLegal == 0) 
		evBest = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(ply) : evDraw;
	else if (bdg.GsTestGameOver(mvevLast.gmvPseudoReply, *ga.prule) != GS::Playing)
		evBest = evDraw;
	return mvevLast.ev = evBest;
}


/*	PLAI::EvBdgQuiescent
 *
 *	Returns the evaluation after the given board/last move from the point of view of the
 *	player next to move. Alpha-beta prunes. 
 */
EV PLAI::EvBdgQuiescent(BDG& bdg, MVEV& mvevLast, int ply, EV evAlpha, EV evBeta)
{
	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evAlpha, evBeta, ply, L'Q');
	PumpMsg();

	int cmvLegal = 0;

	/* first off, get full static eval and check current board already in a pruning 
	   situations */

	EV evBest = EvBdgStatic(bdg, mvevLast, true);
	MVEV* pmvevBest = nullptr;
	if (evBest >= evBeta)
		return mvevLast.ev = evBest;
	if (evBest > evAlpha)
		evAlpha = evBest;

	/* don't bother pre-sorting moves during quiescent search because noisy moves are 
	   rare */

	vector<MVEV> vmvev;
	FillVmvev(bdg, mvevLast.gmvPseudoReply, vmvev);

	try {
		/* recursively evaluate more moves */

		for (MVEV& mvev : vmvev) {
			MAKEMV makemv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			cmvLegal++;
			if (bdg.FMvIsQuiescent(mvev.mv))
				continue;
			bdg.GenGmv(mvev.gmvPseudoReply, GG::Pseudo);	// these aren't generated by FillVmvev
			EV ev = -EvBdgQuiescent(bdg, mvev, ply + 1, -evBeta, -evAlpha);
			if (ev > evBest) {
				evBest = ev;
				pmvevBest = &mvev;
			}
			if (ev >= evBeta)
				return mvevLast.ev = evBest;
			if (ev > evAlpha)
				evAlpha = ev;
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* checkmate and stalemate - checking for repeat draws during quiescent search 
	   doesn't work because we don't evaluate every move */

	if (cmvLegal == 0)
		evBest = bdg.FInCheck(bdg.cpcToMove) ? -EvMate(ply) : evDraw;

	return mvevLast.ev = evBest;
}


void PLAI::PumpMsg(void)
{
	if (++cYield % dcYield == 0)
		ga.PumpMsg();
}


/*	PLAI::FillVmvev
 *
 *	Fills the BDGMV array with the moves in gmv. Does not pre-sort the array.
 *	The mvev's evaluation and reply all list are not filled in.
 */
void PLAI::FillVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept
{
	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(gmv[imv]);
		vmvev.push_back(move(mvev));
	}
}


/*	PLAI::PreSortVmvev
 *
 *	Using generated moves in gmv (generated from bdg),  pre-sorts them by fast 
 *	evaluation to optimize the benefits of alpha-beta pruning. Sorted BDGMV
 *	list is returned in vbdgmv.
 */
void PLAI::PreSortVmvev(BDG& bdg, const GMV& gmv, vector<MVEV>& vmvev) noexcept
{
	/* insertion sort */

	vmvev.reserve(gmv.cmv());
	for (int imv = 0; imv < gmv.cmv(); imv++) {
		MVEV mvev(gmv[imv]);
		bdg.MakeMv(mvev.mv);
		bdg.GenGmv(mvev.gmvPseudoReply, GG::Pseudo);
		mvev.ev = -EvBdgStatic(bdg, mvev, false);
		bdg.UndoMv();
		size_t imvFirst, imvLim;
		for (imvFirst = 0, imvLim = vmvev.size(); imvFirst != imvLim; ) {
			size_t imvMid = (imvFirst + imvLim) / 2;
			assert(imvMid < imvLim);
			if (vmvev[imvMid].ev < mvev.ev)
				imvLim = imvMid;
			else
				imvFirst = imvMid+1;
		}
		vmvev.insert(vmvev.begin() + imvFirst, move(mvev));
	}
}


EV PLAI::EvTempo(const BDG& bdg, CPC cpc) const noexcept
{
	return cpc == bdg.cpcToMove ? evTempo : -evTempo;
}


/*	PLAI::EvBdgAttackDefend
 *
 *	Little heuristic for board evaluation that tries to detect bad moves, which are
 *	if we have moved to an attacked square that isn't defended. This is only useful
 *	for pre-sorting alpha-beta pruning, because it's somewhat more accurate than not
 *	doing it at all, but it's not good enough for full static board evaluation. 
 *	Someday, a better heuristic would be to exchange until quiescence.
 * 
 *	Destination square of the previous move is in sqTo. Returns the amount to adjust 
 *	the evaluation by.
 */
EV PLAI::EvBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept
{
	APC apcMove = bdg.ApcFromSq(mvPrev.sqTo());
	APC apcAttacker = bdg.ApcSqAttacked(mvPrev.sqTo(), bdg.cpcToMove);
	if (apcAttacker != APC::Null) {
		if (apcAttacker < apcMove)
			return EvBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(mvPrev.sqTo(), ~bdg.cpcToMove);
		if (apcDefended == APC::Null)
			return EvBaseApc(apcMove);
	}
	return 0;
}


/*	PLAI::EvBdgStatic
 *
 *	Evaluates the board from the point of view of the color with the 
 *	move. Previous move is in mvevPrev, which should be pre-populated 
 *	with the legal move list for the player with the move.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
EV PLAI::EvBdgStatic(BDG& bdg, MVEV& mvevPrev, bool fFull) noexcept
{
	EV evMaterial = 0, evMobility = 0, evKingSafety = 0, evPawnStructure = 0;
	EV evTempo = 0, evRandom = 0;

	if (fecoMaterial) {
		evMaterial = EvPstFromCpc(bdg);
		if (!fFull)
			evMaterial += EvBdgAttackDefend(bdg, mvevPrev.mv);
	}

	static GMV gmvSelf;
	if (fecoMobility) {
		bdg.GenGmvColor(gmvSelf, ~bdg.cpcToMove);
		evMobility = mvevPrev.gmvPseudoReply.cmv() - gmvSelf.cmv();
	}

	/* slow factors aren't used for pre-sorting */

	EV evPawnToMove, evPawnDef;
	EV evKingToMove, evKingDef;
	if (fFull) {
		if (fecoKingSafety) {
			evKingToMove = EvBdgKingSafety(bdg, bdg.cpcToMove);
			evKingDef = EvBdgKingSafety(bdg, ~bdg.cpcToMove);
			evKingSafety = evKingToMove - evKingDef;
		}
		if (fecoPawnStructure) {
			evPawnToMove = EvBdgPawnStructure(bdg, bdg.cpcToMove);
			evPawnDef = EvBdgPawnStructure(bdg, ~bdg.cpcToMove);
			evPawnStructure = evPawnToMove - evPawnDef;
		}
		if (fecoRandom)
			evRandom = evalRandomDist(rgen); /* distribution already scaled by fecoRandom */
		if (fecoTempo)
			evTempo = EvTempo(bdg, bdg.cpcToMove);
	}
	
	cmvevEval++;
	EV ev = (fecoMaterial * evMaterial +
			fecoMobility * evMobility +
			fecoKingSafety * evKingSafety +
			fecoPawnStructure * evPawnStructure +
			fecoTempo * evTempo +
			evRandom +
		50) / 100;

	if (fFull) {
		cmvevEval++;
		LogData(bdg.cpcToMove == CPC::White ? L"White" : L"Black");
		if (fecoMaterial)
			LogData(L"Material " + SzFromEv(evMaterial));
		if (fecoMobility)
			LogData(L"Mobility " + to_wstring(mvevPrev.gmvPseudoReply.cmv()) + L" - " + to_wstring(gmvSelf.cmv()));
		if (fecoKingSafety)
			LogData(L"King Safety " + to_wstring(evKingToMove) + L" - " + to_wstring(evKingDef));
		if (fecoPawnStructure)
			LogData(L"Pawn Structure " + to_wstring(evPawnToMove) + L" - " + to_wstring(evPawnDef));
		if (fecoTempo)
			LogData(L"Tempo " + SzFromEv(evTempo));
		if (fecoRandom)
			LogData(L"Random " + SzFromEv(evRandom/100));
		LogData(L"Total " + SzFromEv(ev));
	}

	return ev;
}


/*	PLAI:EvPstFromCpc
 *
 *	Returns the piece value table board evaluation for the side with 
 *	the move. 
 */
EV PLAI::EvPstFromCpc(const BDG& bdg) const noexcept
{
	static const int mpapcphase[APC::ActMax] = { 0, 0, 1, 1, 2, 4, 0 };
	int phase = 0;
	EV mpcpcevOpening[2] = { 0, 0 };
	EV mpcpcevMid[2] = { 0, 0 };
	EV mpcpcevEnd[2] = { 0, 0 };

	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		phase += bdg.mppcbb[PC(CPC::White, apc)].csq() * mpapcphase[apc];
		phase += bdg.mppcbb[PC(CPC::Black, apc)].csq() * mpapcphase[apc];
		for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
			for (BB bb = bdg.mppcbb[PC(cpc, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				if (cpc == CPC::White)
					sq = sq.sqFlip();
				mpcpcevOpening[cpc] += mpapcsqevOpening[apc][sq];
				mpcpcevMid[cpc] += mpapcsqevMiddleGame[apc][sq];
				mpcpcevEnd[cpc] += mpapcsqevEndGame[apc][sq];
			}
		}
	}
	phase = min(phase, phaseMax);

	if (phase > phaseOpening)
		return mpcpcevOpening[bdg.cpcToMove] - mpcpcevOpening[~bdg.cpcToMove];
	
	if (phase > phaseMid)
		return EvInterpolate(phase, 
			mpcpcevOpening[bdg.cpcToMove] - mpcpcevOpening[~bdg.cpcToMove], phaseOpening,
			mpcpcevMid[bdg.cpcToMove] - mpcpcevMid[~bdg.cpcToMove], phaseMid);
		
	if (phase > phaseEnd)
		return EvInterpolate(phase, 
			mpcpcevMid[bdg.cpcToMove] - mpcpcevMid[~bdg.cpcToMove], phaseMid, 
			mpcpcevEnd[bdg.cpcToMove] - mpcpcevEnd[~bdg.cpcToMove], phaseEnd); 
	
	return mpcpcevEnd[bdg.cpcToMove] - mpcpcevEnd[~bdg.cpcToMove];
}


EV PLAI::EvInterpolate(int phase, EV ev1, int phase1, EV ev2, int phase2) const noexcept
{
	/* careful - phases are a decreasing function */
	assert(phase1 > phase2);
	assert(phase >= phase2);
	assert(phase <= phase1);
	int dphase = phase1 - phase2;
	return (ev1 * (phase - phase2) + ev2 * (phase1 - phase) + dphase/2) / dphase;
}


EV PLAI::EvFromPhaseApcSq(PHASE phase, APC apc, SQ sq) const noexcept
{
	if (phase <= phaseEnd)
		return mpapcsqevEndGame[apc][sq];
	else if (phase <= phaseMid)
		return mpapcsqevMiddleGame[apc][sq];
	else
		return mpapcsqevOpening[apc][sq];
}


void PLAI::InitWeightTables(void)
{
	InitWeightTable(mpapcevOpening, mpapcsqdevOpening, mpapcsqevOpening);
	InitWeightTable(mpapcevMiddleGame, mpapcsqdevMiddleGame, mpapcsqevMiddleGame);
	InitWeightTable(mpapcevEndGame, mpapcsqdevEndGame, mpapcsqevEndGame);
}


void PLAI::InitWeightTable(const EV mpapcev[APC::ActMax], const EV mpapcsqdev[APC::ActMax][sqMax], EV mpapcsqev[APC::ActMax][sqMax])
{
	memset(&mpapcsqev[APC::Null], 0, sqMax * sizeof(EV));
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
		for (uint64_t sq = 0; sq < sqMax; sq++)
			mpapcsqev[apc][sq] = mpapcev[apc] + mpapcsqdev[apc][sq];
}


EV PLAI::EvBdgKingSafety(BDG& bdg, CPC cpc) noexcept
{
	return 0;
}


/*	PLAI::CfileDoubledPawns
 *
 *	Returns the number of doubled pawns the side cpc has, i.e.,
 *	the number of files that contain multiple pawns.
 */
int PLAI::CfileDoubledPawns(BDG& bdg, CPC cpc) const noexcept
{
	int cfile = 0;
	BB bbPawn = bdg.mppcbb[PC(cpc, APC::Pawn)];
	BB bbFile = bbFileA;
	for (int file = 0; file < fileMax; file++, bbFile <<= 1) {
		int csq = (bbPawn & bbFile).csq();
		if (csq)
			cfile += csq - 1;
	}
	return cfile;
}


/*	PLAI::CfileIsoPawns
 *
 *	Returns the number of isolated pawns the side cpc has; i.e., 
 *	the number of files that have pawns when neither adjacent file
 *	have pawns.
 */
int PLAI::CfileIsoPawns(BDG& bdg, CPC cpc) const noexcept
{
	int cfile = 0;
	BB bbPawn = bdg.mppcbb[PC(cpc, APC::Pawn)];

	cfile += (bbPawn & bbFileA) && !(bbPawn & bbFileB);
	cfile += (bbPawn & bbFileH) && !(bbPawn & bbFileG);
	BB bbFile = bbFileB;
	BB bbAdj = bbFileA | bbFileC;
	for (int file = fileB; file <= fileG; file++, bbFile <<= 1, bbAdj <<= 1)
		cfile += (bbPawn & bbFile) && !(bbPawn & bbAdj);
	return cfile;
}


/*	PLAI::EvBdgPawnStructure
 *
 *	Returns the pawn structure evaluation of the board position from the
 *	point of view of cpc.
 */
EV PLAI::EvBdgPawnStructure(BDG& bdg, CPC cpc) noexcept
{
	EV ev = 0;

	ev -= CfileDoubledPawns(bdg, cpc);
	ev -= CfileIsoPawns(bdg, cpc);

	/* overextended pawns */

	/* backwards pawns */
	/* passed pawns */
	/* connected pawns */
	
	/* open and half-open files */

	return ev;
}


/*
 *
 *	PLAI2
 * 
 */


PLAI2::PLAI2(GA& ga) : PLAI(ga)
{
	fecoMaterial = 100;	
	fecoMobility = 0;	
	fecoKingSafety = 0;	
	fecoPawnStructure = 0;	
	fecoTempo = 100;
	fecoRandom = 5;
	evalRandomDist = uniform_int_distribution<int32_t>(-fecoRandom*100, fecoRandom*100);
	SetName(L"SQ Mathilda");
	InitWeightTables();
}


int PLAI2::PlyLim(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int plyLim = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return plyLim;
}


void PLAI2::InitWeightTables(void)
{
	InitWeightTable(mpapcevOpening2, mpapcsqdevOpening2, mpapcsqevOpening);
	InitWeightTable(mpapcevMiddleGame2, mpapcsqdevMiddleGame2, mpapcsqevMiddleGame);
	InitWeightTable(mpapcevEndGame2, mpapcsqdevEndGame2, mpapcsqevEndGame);
}


/*
 *
 *	PLHUMAN class
 * 
 *	Human player, which drives the UI for getting moves.
 * 
 */


PLHUMAN::PLHUMAN(GA& ga, wstring szName) : PL(ga, szName)
{
}


MV PLHUMAN::MvGetNext(SPMV& spmv)
{
	mvNext = MV();
	do
		ga.PumpMsg();
	while (mvNext.fIsNil());
	MV mv = mvNext;
	spmv = spmvNext;
	mvNext = MV();
	return mv;
}


/*
 *
 *	RGINFOPL
 * 
 *	The list of players wew have available to play.
 * 
 */


RGINFOPL::RGINFOPL(void) 
{
	vinfopl.push_back(INFOPL(IDCLASSPL::AI, TPL::AI, L"SQ Mobly", 5));
	vinfopl.push_back(INFOPL(IDCLASSPL::AI2, TPL::AI, L"SQ Mathilda", 3));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Rick Powell"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Hazel the Dog"));
	vinfopl.push_back(INFOPL(IDCLASSPL::Human, TPL::Human, L"Allain de Leon"));
}


RGINFOPL::~RGINFOPL(void) 
{
}


PL* RGINFOPL::PplFactory(GA& ga, int iinfopl) const
{
	PL* ppl = nullptr;
	switch (vinfopl[iinfopl].idclasspl) {
	case IDCLASSPL::AI:
		ppl = new PLAI(ga);
		break;
	case IDCLASSPL::AI2:
		ppl = new PLAI2(ga);
		break;
	case IDCLASSPL::Human:
		ppl = new PLHUMAN(ga, vinfopl[iinfopl].szName);
		break;
	default:
		assert(false);
		return nullptr;
	}
	ppl->SetLevel(vinfopl[iinfopl].level);
	return ppl;
}


int RGINFOPL::IdbFromInfopl(const INFOPL& infopl) const
{
	switch (infopl.tpl) {
	case TPL::AI:
		return idbAiLogo;
	default:
		break;
	}
	return idbHumanLogo;
}


