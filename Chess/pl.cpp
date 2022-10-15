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

const EVAL evalInf = 1000*100;	/* largest possible evaluation */
const EVAL evalMate = evalInf - 1;	/* checkmates are given evals of evalMate minus moves to mate */
const EVAL evalMateMin = evalMate - 63;
const EVAL evalTempo = 33;	/* evaluation of a single move advantage */
const EVAL evalDraw = 0;	/* evaluation of a draw */
const EVAL evalQuiescent = evalInf + 1;


bool FEvalIsMate(EVAL eval)
{
	return eval >= evalMateMin;
}


wstring SzFromEval(EVAL eval)
{
	wchar_t sz[20], *pch = sz;
	if (eval >= 0)
		*pch++ = L'+';
	else {
		*pch++ = L'-';
		eval = -eval;
	}
	if (eval == evalInf)
		*pch++ = L'\x221e';
	else if (eval == evalQuiescent)
		*pch++ = L'Q';
	else if (eval > evalInf)
		*pch++ = L'*';
	else if (FEvalIsMate(eval)) {
		*pch++ = L'#';
		pch = PchDecodeInt((evalMate - eval + 1) / 2, pch);
	}
	else {
		pch = PchDecodeInt(eval / 100, pch);
		eval %= 100;
		*pch++ = L'.';
		*pch++ = L'0' + eval / 10;
		*pch++ = L'0' + eval % 10;
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


EVAL PL::EvalFromPhaseApcSq(PHASE phase, APC apc, SQ sq) const noexcept
{
	return EvalBaseApc(apc);
}


EVAL PL::EvalBaseApc(APC apc) const noexcept
{
	EVAL mpapceval[APC::ActMax] = { 0, 100, 275, 300, 500, 900, 200 };
	return mpapceval[apc];
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


int PLAI::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
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
			EVAL evalAlpha, EVAL evalBeta, int depth, wchar_t chType=L' ') : 
		pl(pl), bdg(bdg), mvev(mvev), szData(L""), lgf(LGF::Normal)
	{
		depthLogSav = DepthLog();
		imvExpandSav = imvExpand;
		if (FExpandLog(mvev))
			SetDepthLog(depthLogSav + 1);
		LogOpen(TAG(bdg.SzDecodeMvPost(mvev.mv), ATTR(L"FEN", bdg)),
				wstring(1, chType) + to_wstring(depth) + L" " + SzFromEval(mvev.eval) + 
					L" [" + SzFromEval(evalAlpha) + L", " + SzFromEval(evalBeta) + L"] ");
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
				SzFromEval(mvev.eval) /*+ L" (" + bdg.SzDecodeMvPost(mvev.mvReplyBest) + L")"*/,
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
	int depthMax;

	GMV gmv;
	BDG bdg = ga.bdg;
	vector<MVEV> vmvev;

	/* figure out how deep we're going to search */

	bdg.GenGmv(gmv, GG::Pseudo);
	depthMax = clamp(DepthMax(bdg, gmv), 2, 14);
	LogData(L"Search depth: " + to_wstring(depthMax));

	/* pre-sort to improve quality of alpha-beta pruning */

	PreSortVmvev(bdg, gmv, vmvev);

	EVAL evalAlpha = -evalInf, evalBeta = evalInf;
	EVAL evalBest = -evalInf;
	const MVEV* pmvevBest = nullptr;

	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makemv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			EVAL eval = -EvalBdgDepth(bdg, mvev, 1, depthMax, -evalBeta, -evalAlpha);
			if (eval > evalBest) {
				evalBest = eval;
				pmvevBest = &mvev;
			}
			// no pruning at top level 
			assert(eval < evalBeta);
			if (eval > evalAlpha) {
				evalAlpha = eval;
				if (FEvalIsMate(eval))
					depthMax = evalMate - eval;
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
EVAL PLAI::EvalBdgDepth(BDG& bdg, MVEV& mvevLast, int depth, int depthMax, EVAL evalAlpha, EVAL evalBeta)
{
	if (depth >= depthMax)
		return EvalBdgQuiescent(bdg, mvevLast, depth, evalAlpha, evalBeta);

	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evalAlpha, evalBeta, depth, L' ');
	PumpMsg();

	/* pre-sort the pseudo-legal moves in mvevEval */
	vector<MVEV> vmvev;
	PreSortVmvev(bdg, mvevLast.gmvPseudoReply, vmvev);
	
	int cmvLegal = 0;
	EVAL evalBest = -evalInf;
	const MVEV* pmvevBest = nullptr;

	try {
		for (MVEV& mvev : vmvev) {
			MAKEMV makevmv(bdg, mvev.mv);
			if (bdg.FInCheck(~bdg.cpcToMove))
				continue;
			cmvLegal++;
			EVAL eval = -EvalBdgDepth(bdg, mvev, depth + 1, depthMax, -evalBeta, -evalAlpha);
			if (eval > evalBest) {
				evalBest = eval;
				pmvevBest = &mvev;
			}
			if (eval >= evalBeta)
				return mvevLast.eval = evalBest;
			if (eval > evalAlpha) {
				evalAlpha = eval;
				if (FEvalIsMate(eval))
					depthMax = evalMate - eval;
			}
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* test for checkmates, stalemates, and other draws */

	if (cmvLegal == 0) 
		evalBest = bdg.FInCheck(bdg.cpcToMove) ? -(evalMate - depth) : evalDraw;
	else if (bdg.GsTestGameOver(mvevLast.gmvPseudoReply, *ga.prule) != GS::Playing)
		evalBest = evalDraw;
	return mvevLast.eval = evalBest;
}


/*	PLAI::EvalBdgQuiescent
 *
 *	Returns the evaluation after the given board/last move from the point of view of the
 *	player next to move. Alpha-beta prunes. 
 */
EVAL PLAI::EvalBdgQuiescent(BDG& bdg, MVEV& mvevLast, int depth, EVAL evalAlpha, EVAL evalBeta)
{
	cmvevNode++;
	LOGOPENMVEV logopenmvev(*this, bdg, mvevLast, evalAlpha, evalBeta, depth, L'Q');
	PumpMsg();

	int cmvLegal = 0;

	/* first off, get full static eval and check current board already in a pruning 
	   situations */

	EVAL evalBest = EvalBdgStatic(bdg, mvevLast, true);
	MVEV* pmvevBest = nullptr;
	if (evalBest >= evalBeta)
		return mvevLast.eval = evalBest;
	if (evalBest > evalAlpha)
		evalAlpha = evalBest;

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
			EVAL eval = -EvalBdgQuiescent(bdg, mvev, depth + 1, -evalBeta, -evalAlpha);
			if (eval > evalBest) {
				evalBest = eval;
				pmvevBest = &mvev;
			}
			if (eval >= evalBeta)
				return mvevLast.eval = evalBest;
			if (eval > evalAlpha)
				evalAlpha = eval;
		}
	}
	catch (...) {
		logopenmvev.SetData(L"(interrupt)", LGF::Italic);
		throw;
	}

	/* checkmate and stalemate - checking for repeat draws during quiescent search 
	   doesn't work because we don't evaluate every move */

	if (cmvLegal == 0)
		evalBest = bdg.FInCheck(bdg.cpcToMove) ? -(evalMate - depth) : evalDraw;

	return mvevLast.eval = evalBest;
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
		mvev.eval = -EvalBdgStatic(bdg, mvev, false);
		bdg.UndoMv();
		size_t imvFirst, imvLim;
		for (imvFirst = 0, imvLim = vmvev.size(); imvFirst != imvLim; ) {
			size_t imvMid = (imvFirst + imvLim) / 2;
			assert(imvMid < imvLim);
			if (vmvev[imvMid].eval < mvev.eval)
				imvLim = imvMid;
			else
				imvFirst = imvMid+1;
		}
		vmvev.insert(vmvev.begin() + imvFirst, move(mvev));
	}
}


EVAL PLAI::EvalTempo(const BDG& bdg, CPC cpc) const noexcept
{
	return cpc == bdg.cpcToMove ? evalTempo : -evalTempo;
}


/*	PLAI::EvalBdgAttackDefend
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
EVAL PLAI::EvalBdgAttackDefend(BDG& bdg, MV mvPrev) const noexcept
{
	APC apcMove = bdg.ApcFromSq(mvPrev.sqTo());
	APC apcAttacker = bdg.ApcSqAttacked(mvPrev.sqTo(), bdg.cpcToMove);
	if (apcAttacker != APC::Null) {
		if (apcAttacker < apcMove)
			return EvalBaseApc(apcMove);
		APC apcDefended = bdg.ApcSqAttacked(mvPrev.sqTo(), ~bdg.cpcToMove);
		if (apcDefended == APC::Null)
			return EvalBaseApc(apcMove);
	}
	return 0;
}


/*	PLAI::EvalBdgStatic
 *
 *	Evaluates the board from the point of view of the color with the 
 *	move. Previous move is in mvevPrev, which should be pre-populated 
 *	with the legal move list for the player with the move.
 * 
 *	fFull is true for full, potentially slow, evaluation. 
 */
EVAL PLAI::EvalBdgStatic(BDG& bdg, MVEV& mvevPrev, bool fFull) noexcept
{
	EVAL evalMaterial = 0, evalMobility = 0, evalKingSafety = 0, evalPawnStructure = 0;
	EVAL evalTempo = 0, evalRandom = 0;

	if (fecoMaterial) {
		evalMaterial = EvalPstFromCpc(bdg);
		if (!fFull)
			evalMaterial += EvalBdgAttackDefend(bdg, mvevPrev.mv);
	}

	static GMV gmvSelf;
	if (fecoMobility) {
		bdg.GenGmvColor(gmvSelf, ~bdg.cpcToMove);
		evalMobility = mvevPrev.gmvPseudoReply.cmv() - gmvSelf.cmv();
	}

	/* slow factors aren't used for pre-sorting */

	EVAL evalPawnToMove, evalPawnDef;
	EVAL evalKingToMove, evalKingDef;
	if (fFull) {
		if (fecoKingSafety) {
			evalKingToMove = EvalBdgKingSafety(bdg, bdg.cpcToMove);
			evalKingDef = EvalBdgKingSafety(bdg, ~bdg.cpcToMove);
			evalKingSafety = evalKingToMove - evalKingDef;
		}
		if (fecoPawnStructure) {
			evalPawnToMove = EvalBdgPawnStructure(bdg, bdg.cpcToMove);
			evalPawnDef = EvalBdgPawnStructure(bdg, ~bdg.cpcToMove);
			evalPawnStructure = evalPawnToMove - evalPawnDef;
		}
		if (fecoRandom)
			evalRandom = evalRandomDist(rgen); /* distribution already scaled by fecoRandom */
		if (fecoTempo)
			evalTempo = EvalTempo(bdg, bdg.cpcToMove);
	}
	
	cmvevEval++;
	EVAL eval = (fecoMaterial * evalMaterial +
			fecoMobility * evalMobility +
			fecoKingSafety * evalKingSafety +
			fecoPawnStructure * evalPawnStructure +
			fecoTempo * evalTempo +
			evalRandom +
		50) / 100;

	if (fFull) {
		cmvevEval++;
		LogData(bdg.cpcToMove == CPC::White ? L"White" : L"Black");
		if (fecoMaterial)
			LogData(L"Material " + SzFromEval(evalMaterial));
		if (fecoMobility)
			LogData(L"Mobility " + to_wstring(mvevPrev.gmvPseudoReply.cmv()) + L" - " + to_wstring(gmvSelf.cmv()));
		if (fecoKingSafety)
			LogData(L"King Safety " + to_wstring(evalKingToMove) + L" - " + to_wstring(evalKingDef));
		if (fecoPawnStructure)
			LogData(L"Pawn Structure " + to_wstring(evalPawnToMove) + L" - " + to_wstring(evalPawnDef));
		if (fecoTempo)
			LogData(L"Tempo " + SzFromEval(evalTempo));
		if (fecoRandom)
			LogData(L"Random " + SzFromEval(evalRandom/100));
		LogData(L"Total " + SzFromEval(eval));
	}

	return eval;
}


/*	PLAI:EvalPstFromCpc
 *
 *	Returns the piece value table board evaluation for the side with 
 *	the move. 
 */
EVAL PLAI::EvalPstFromCpc(const BDG& bdg) const noexcept
{
	static const int mpapcphase[APC::ActMax] = { 0, 0, 1, 1, 2, 4, 0 };
	int phase = 0;
	EVAL mpcpcevalOpening[2] = { 0, 0 };
	EVAL mpcpcevalMid[2] = { 0, 0 };
	EVAL mpcpcevalEnd[2] = { 0, 0 };

	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc) {
		phase += bdg.mppcbb[PC(CPC::White, apc)].csq() * mpapcphase[apc];
		phase += bdg.mppcbb[PC(CPC::Black, apc)].csq() * mpapcphase[apc];
		for (CPC cpc = CPC::White; cpc <= CPC::Black; ++cpc) {
			for (BB bb = bdg.mppcbb[PC(cpc, apc)]; bb; bb.ClearLow()) {
				SQ sq = bb.sqLow();
				if (cpc == CPC::White)
					sq = sq.sqFlip();
				mpcpcevalOpening[cpc] += mpapcsqevalOpening[apc][sq];
				mpcpcevalMid[cpc] += mpapcsqevalMiddleGame[apc][sq];
				mpcpcevalEnd[cpc] += mpapcsqevalEndGame[apc][sq];
			}
		}
	}
	phase = min(phase, phaseMax);

	if (phase > phaseOpening)
		return mpcpcevalOpening[bdg.cpcToMove] - mpcpcevalOpening[~bdg.cpcToMove];
	
	if (phase > phaseMid)
		return EvalInterpolate(phase, 
			mpcpcevalOpening[bdg.cpcToMove] - mpcpcevalOpening[~bdg.cpcToMove], phaseOpening,
			mpcpcevalMid[bdg.cpcToMove] - mpcpcevalMid[~bdg.cpcToMove], phaseMid);
		
	if (phase > phaseEnd)
		return EvalInterpolate(phase, 
			mpcpcevalMid[bdg.cpcToMove] - mpcpcevalMid[~bdg.cpcToMove], phaseMid, 
			mpcpcevalEnd[bdg.cpcToMove] - mpcpcevalEnd[~bdg.cpcToMove], phaseEnd); 
	
	return mpcpcevalEnd[bdg.cpcToMove] - mpcpcevalEnd[~bdg.cpcToMove];
}


EVAL PLAI::EvalInterpolate(int phase, EVAL eval1, int phase1, EVAL eval2, int phase2) const noexcept
{
	/* careful - phases are a decreasing function */
	assert(phase1 > phase2);
	assert(phase >= phase2);
	assert(phase <= phase1);
	int dphase = phase1 - phase2;
	return (eval1 * (phase - phase2) + eval2 * (phase1 - phase) + dphase/2) / dphase;
}


EVAL PLAI::EvalFromPhaseApcSq(PHASE phase, APC apc, SQ sq) const noexcept
{
	if (phase <= phaseEnd)
		return mpapcsqevalEndGame[apc][sq];
	else if (phase <= phaseMid)
		return mpapcsqevalMiddleGame[apc][sq];
	else
		return mpapcsqevalOpening[apc][sq];
}


void PLAI::InitWeightTables(void)
{
	InitWeightTable(mpapcevalOpening, mpapcsqdevalOpening, mpapcsqevalOpening);
	InitWeightTable(mpapcevalMiddleGame, mpapcsqdevalMiddleGame, mpapcsqevalMiddleGame);
	InitWeightTable(mpapcevalEndGame, mpapcsqdevalEndGame, mpapcsqevalEndGame);
}


void PLAI::InitWeightTable(const EVAL mpapceval[APC::ActMax], const EVAL mpapcsqdeval[APC::ActMax][sqMax], EVAL mpapcsqeval[APC::ActMax][sqMax])
{
	memset(&mpapcsqeval[APC::Null], 0, sqMax * sizeof(EVAL));
	for (APC apc = APC::Pawn; apc < APC::ActMax; ++apc)
		for (uint64_t sq = 0; sq < sqMax; sq++)
			mpapcsqeval[apc][sq] = mpapceval[apc] + mpapcsqdeval[apc][sq];
}


EVAL PLAI::EvalBdgKingSafety(BDG& bdg, CPC cpc) noexcept
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


/*	PLAI::EvalBdgPawnStructure
 *
 *	Returns the pawn structure evaluation of the board position from the
 *	point of view of cpc.
 */
EVAL PLAI::EvalBdgPawnStructure(BDG& bdg, CPC cpc) noexcept
{
	EVAL eval = 0;

	eval -= CfileDoubledPawns(bdg, cpc);
	eval -= CfileIsoPawns(bdg, cpc);

	/* backwards pawns */
	/* overextended pawns */

	/* passed pawns */
	/* connected pawns */
	
	/* open and half-open files */

	return eval;
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


int PLAI2::DepthMax(const BDG& bdg, const GMV& gmv) const
{
	static GMV gmvOpp;
	bdg.GenGmvColor(gmvOpp, ~bdg.cpcToMove);
	const float cmvSearch = CmvFromLevel(level);	// approximate number of moves to analyze
	const float fracAlphaBeta = 0.33f; // alpha-beta pruning cuts moves we analyze by this factor.
	float size2 = (float)(gmv.cmv() * gmvOpp.cmv());
	int depthMax = (int)round(2.0f * log(cmvSearch) / log(size2 * fracAlphaBeta * fracAlphaBeta));
	return depthMax;
}


void PLAI2::InitWeightTables(void)
{
	InitWeightTable(mpapcevalOpening2, mpapcsqdevalOpening2, mpapcsqevalOpening);
	InitWeightTable(mpapcevalMiddleGame2, mpapcsqdevalMiddleGame2, mpapcsqevalMiddleGame);
	InitWeightTable(mpapcevalEndGame2, mpapcsqdevalEndGame2, mpapcsqevalEndGame);
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


