#pragma once

/*
 *
 *	bd.h
 *
 *	Board definitions
 *
 */
#include "framework.h"


enum {
	tpcQueenRook = 0,
	tpcQueenKnight = 1,
	tpcQueenBishop = 2,
	tpcQueen = 3,
	tpcKing = 4,
	tpcKingBishop = 5,
	tpcKingKnight = 6,
	tpcKingRook = 7,
	tpcPawnFirst = 8,
	tpcPawnQR = 8,
	tpcPawnQN = 9,
	tpcPawnQB = 10,
	tpcPawnQ = 11,
	tpcPawnK = 12,
	tpcPawnKB = 13,
	tpcPawnKN = 14,
	tpcPawnKR = 15,
	tpcPawnLim = 16,
	tpcPieceMax = 16,

	tpcWhite = 0x00,
	tpcBlack = 0x80,
	tpcColor = 0x80,

	tpcPiece = 0x0f,

	tpcApc = 0x70,

	tpcEmpty = 0x80
};

typedef BYTE TPC;


enum {
	cpcWhite = 0,
	cpcBlack = 1,
	cpcMax = 2,
	cpcNil = 2
};
typedef int CPC;

enum {
	apcNull = 0,
	apcPawn = 1,
	apcKnight = 2,
	apcBishop = 3,
	apcRook = 4,
	apcQueen = 5,
	apcKing = 6,
	apcBishop2 = 7	// only used for draw detection computation to keep track of bishop square color
};

typedef BYTE APC;

inline APC ApcFromTpc(BYTE tpc) { return (tpc & tpcApc) >> 4; }
inline TPC Tpc(TPC tpc, CPC cpc, APC apc) { return tpc | (cpc << 7) | (apc << 4); }
inline int CpcFromTpc(TPC tpc) { return (tpc & tpcColor) == tpcBlack; }
inline TPC TpcSetApc(TPC tpc, APC apc) { return (tpc & ~tpcApc) | (apc << 4); }

enum {
	fileQueenRook = 0,
	fileQueenKnight = 1,
	fileQueenBishop = 2,
	fileQueen = 3,
	fileKing = 4,
	fileKingBishop = 5,
	fileKingKnight = 6,
	fileKingRook = 7,
	fileMax = 8
};

const int rankMax = 8;


/*	
 *
 *	SQ type
 *
 *	A square is file and rank encoded into a single byte
 *
 */


class SQ {
private:
	friend class MV;
	BYTE grf;
public:
	inline SQ(void) : grf(0xff) { }
	inline SQ(BYTE grf) : grf(grf) { }
	inline SQ(int rank, int file) { grf = (rank << 3) | file; }
	inline SQ(const SQ& sq) { grf = sq.grf; }
	inline int file(void) const { return grf & 0x07; }
	inline int rank(void) const { return (grf >> 3) & 0x07; }
	inline bool FIsNil(void) const { return grf == 0xff; }
	inline bool FIsMax(void) const { return grf == rankMax * fileMax; }
	inline SQ operator++(int) { BYTE grfT = grf++; return SQ(grfT); }
	inline SQ& operator+=(int dsq) { grf += dsq; return *this; }
	inline SQ operator+(int dsq) const { return SQ(grf + dsq); }
	inline operator int() const { return grf; }
	inline bool FIsValid(void) const { return (grf & 0xc0) == 0; }
};

const int sqMax = rankMax*8;
const SQ sqNil = SQ();


/*	
 * 
 *	MV type
 *
 *	A move is a from and to square, with a little extra info for
 *	weird moves, along with enough information to undo the move.
 * 
 *	Low 6 bits is the source square, the next 6 bits are the 
 *	destination square. The rest of the bottom word is currently
 *	unused, reserved for a change in board representation.
 *	
 *	The high word is mostly used for undo information. On captures
 *	the tpc of the captured piece is in the bottom 4 bits, and
 *	the next 3 bits are the 1pc of the captured piece. If the capture
 *	apc is apcNull, then the move was not a capture. The next bit
 *	is 1 if en passant capture ws possible in previous position. 
 *	The next three bits are the file of the en passant piece if en
 *	passant is true. The next two bits are the previous castle bits.
 *	The top 3 bits are the apc of the new piece on pawn promotions. 
 * 
 *	There are opportunities to compress the upper word of the move.
 * 
 *   15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *  +-----------+--------+--------+--------+--------+
 *  |           |        to       |       from      |
 *  +-----------+--------+--------+--------+--------+
 *  +--------+-----+--------+--+--------+-----------+
 *  | promote|  cs | ep file|ep| cap apc|  cap tpc  |
 *  +--------+-----+--------+--+--------+-----------+
 *   31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 */

class MV {
private:
	static const unsigned long grfNil = 0xffffffffL;
	unsigned long grf;
public:
	MV(void) { grf = grfNil; }
	MV(SQ sqFrom, SQ sqTo, APC apcPromote = apcNull) {
		grf = (unsigned long)sqFrom.grf | ((unsigned long)sqTo.grf << 6) | 
			((unsigned long)apcPromote << 29);
	}
	
	SQ SqFrom(void) const { return grf & 0x3f; }
	SQ SqTo(void) const { return (grf >> 6) & 0x3f; }
	APC ApcPromote(void) const { return (grf >> 29) & 0x07; }
	bool FIsNil(void) const { return grf == grfNil; }
	MV& SetApcPromote(APC apc) { grf = (grf & 0x1fffffffL) | ((unsigned long)apc << 29); return *this;  }
	MV& SetCapture(APC apc, TPC tpc) { grf = (grf & 0xff80ffffL) | ((unsigned long)apc << 20) | ((unsigned long)tpc << 16); return *this;  }
	MV& SetCsEp(int cs, const SQ& sqEnPassant) {
		grf = (grf & 0xe07fffffL) | 
			((unsigned long)cs << 27) |
			((unsigned long)sqEnPassant.file() << 24) | 
			((unsigned long)!sqEnPassant.FIsNil() << 23); 
		return *this;
	}
	int CsPrev(void) const { return (grf >> 27) & 0x03; }
	int FileEpPrev(void) const { return (grf >> 24) & 0x07; }
	bool FEpPrev(void) const { return (grf >> 23) & 0x01; }
	APC ApcCapture(void) const { return (grf >> 20) & 0x07; }
	TPC TpcCapture(void) const { return (grf >> 16) & 0x0f; }
	bool operator==(const MV& mv) const { return grf == mv.grf; }
	bool operator!=(const MV& mv) const { return grf != mv.grf; }
};


/*
 *
 *	GS emumeration
 * 
 *	Game state. Either we're playing, or the game is over by several possible
 *	situations.
 *
 */


enum class GS {
	Playing = 0,
	WhiteCheckMated,
	BlackCheckMated,
	WhiteTimedOut,
	BlackTimedOut,
	WhiteResigned,
	BlackResigned,
	StaleMate,
	DrawDead,
	DrawAgree,
	Draw3Repeat,
	Draw50Move
};

enum {
	csKing = 0x01, csQueen = 0x04,
	csWhiteKing = 0x01,
	csBlackKing = 0x02,
	csWhiteQueen = 0x04,
	csBlackQueen = 0x08
};

/* pack the castle state of a one side into 2 bits for storing in the MV */
inline int CsPackColor(int csUnpack, CPC cpc)
{
	int csPack = csUnpack >> cpc;
	return ((csPack>>1)&2) | (csPack&1);
}

inline int CsUnpackColor(int csPack, CPC cpc)
{
	int csUnpack = (csPack&1) | ((csPack&2)<<1);
	return csUnpack << cpc;
}


inline int RankPromoteFromCpc(CPC cpc)
{
	return ~-cpc & 7;
}

inline int RankInitPawnFromCpc(CPC cpc)
{
	return (-cpc ^ 1) & 7;
}

inline int DsqPawnFromCpc(CPC cpc)
{
	return 8 - (cpc << 4);
}

inline CPC CpcOpposite(CPC cpc)
{
	return cpc ^ 1;
}



/*
 *
 *	BD class
 * 
 *	The board.
 * 
 */

enum class RMCHK {	// GenRgmv Option to optionally remove checks
	Remove,
	NoRemove
};

class BD
{
public:
	BD(void);
	BD(const BD& bd);
	
	TPC mpsqtpc[sqMax];	// the board itself (maps square to piece)
	UINT64 rggrfAttacked[cpcMax];	// bit field of attacked squares (black=1, white=0)
	SQ mptpcsq[cpcMax][tpcPieceMax]; // reverse mapping of mpsqtpc (black=1, white=0)
	SQ sqEnPassant;	/* non-nil when previous move was a two-square pawn move, destination
					   of en passant capture */
	BYTE cs;	/* castle sides */

	void InitFENPieces(const WCHAR*& szFEN);
	void AddPieceFEN(SQ sq, TPC tpc, CPC cpc, APC apc);
	void SkipToNonSpace(const WCHAR*& sz);
	void SkipToSpace(const WCHAR*& sz);
	int TpcUnusedPawn(CPC cpc) const;

	void MakeMv(MV mv);
	void UndoMv(MV mv);
	void MakeMvSq(MV mv);
	void ComputeAttacked(CPC cpcMove);

	void GenRgmv(vector<MV>& rgmv, CPC cpcMove, RMCHK rmchk) const;
	void GenRgmvColor(vector<MV>& rgmv, CPC cpcMove) const;
	void GenRgmvPawn(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvKnight(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvBishop(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvRook(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvQueen(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvKing(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvCastle(vector<MV>& rgmv, SQ sqFrom) const; 
	void GenRgmvCastleSide(vector<MV>& rgmv, SQ sqKing, int fileRook, int dsq) const;
	void GenRgmvPawnCapture(vector<MV>& rgmv, SQ sqFrom, int dsq) const;
	void AddRgmvMvPromotions(vector<MV>& rgmv, MV mv) const;
	void GenRgmvEnPassant(vector<MV>& rgmv, SQ sqFrom) const;
	void GenRgmvSlide(vector<MV>& rgmv, SQ sqFrom, int dsq) const;
	bool FGenRgmvDsq(vector<MV>& rgmv, SQ sqFrom, SQ sq, TPC tpcFrom, int dsq) const;
	void AddRgmvMv(vector<MV>& rgmv, MV mv) const;

	void RemoveInCheckMoves(vector<MV>& rgmv, CPC co) const;
	bool FInCheck(SQ sqKing) const;
	bool FSqAttacked(SQ sq, CPC cpcBy) const;

	inline TPC& operator()(int rank, int file) { return mpsqtpc[rank*8+file]; }
	inline TPC& operator()(SQ sq) { return mpsqtpc[sq]; }

	inline SQ& SqFromTpc(TPC tpc) { return mptpcsq[CpcFromTpc(tpc)][tpc & tpcPiece]; }
	inline SQ SqFromTpc(TPC tpc) const { return mptpcsq[CpcFromTpc(tpc)][tpc & tpcPiece]; }
	inline APC ApcFromSq(SQ sq) const { return ApcFromTpc(mpsqtpc[sq]); }
	inline CPC CpcFromSq(SQ sq) const { return CpcFromTpc(mpsqtpc[sq]); }

	inline bool FCanCastle(CPC cpc, int csSide) const { return (this->cs & (csSide << cpc)) != 0;  }
	inline void SetCastle(CPC cpc, int csSide) { this->cs |= csSide << cpc; }
	inline void ClearCastle(CPC cpc, int csSide) { this->cs &= ~(csSide << cpc); }

	bool operator==(const BD& bd) const;
	bool operator!=(const BD& bd) const;

#ifndef NDEBUG
	void Validate(void) const;
#else
	inline void Validate(void) const { }
#endif
};



enum class TKMV {
	Error,
	End,
	
	King,
	Queen,
	Rook,
	Bishop,
	Knight,
	Pawn,

	Square,
	File,
	Rank,

	Take,
	To,
	
	Promote,
	
	Check,
	Mate,
	EnPassant,
	
	CastleKing,
	CastleQueen,

	WhiteWins,
	BlackWins,
	Draw,
	InProgress
};




/*
 *
 *	BDG class
 * 
 *	The game board, which is the regular board along with the move history and other
 *	game state.
 * 
 */


class BDG : public BD
{
public:
	GS gs;	// game state
	CPC cpcToMove;
	vector<MV> rgmvGame;	// the game moves that resulted in bd board state
	int imvCur;
	int imvPawnOrTakeLast;	/* index of last pawn or capture move (used directly for 50-move 
							   draw detection), but it is also a bound on how far back we need 
							   to search for 3-move repetition draws */

public:
	BDG(void);
	BDG(const BDG& BDG);
	BDG(const WCHAR* szFEN);

	void NewGame(void);

	void InitFEN(const WCHAR* szFen);
	void InitFENSideToMove(const WCHAR*& sz);
	void InitFENCastle(const WCHAR*& sz);
	void InitFENEnPassant(const WCHAR*& sz);
	void InitFENHalfmoveClock(const WCHAR*& sz);
	void InitFENFullmoveCounter(const WCHAR*& sz);

	void GenRgmv(vector<MV>& rgmv, RMCHK rmchk) const;
	void MakeMv(MV mv);
	void UndoMv(void);
	void RedoMv(void);
	void TestGameOver(const vector<MV>& rgmv);
	bool FDrawDead(void) const;
	bool FDraw3Repeat(int cbdDraw) const;
	bool FDraw50Move(int cmvDraw) const;
	void SetGs(GS gs);

	wstring SzMoveAndDecode(MV mv);
	wstring SzDecodeMv(MV mv) const;
	int ParseMv(const char*& pch, MV& mv) const;
	int ParsePieceMv(const vector<MV>& rgmv, TKMV tkmv, const char*& pch, MV& mv) const;
	int ParseSquareMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseMvSuffixes(MV& mv, const char*& pch) const;
	int ParseFileMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	int ParseRankMv(const vector<MV>& rgmv, SQ sq, const char*& pch, MV& mv) const;
	bool FMvMatchPieceTo(const vector<MV>& rgmv, APC apc, int rankFrom, int fileFrom, SQ sqTo, MV& mv) const;
	bool FMvMatchFromTo(const vector<MV>& rgmv, SQ sqFrom, SQ sqTo, MV& mv) const;
	TKMV TkmvScan(const char*& pch, SQ& sq) const;
};


/*
 *
 *	ANO class
 * 
 *	Class that represents an annotation on the board. Our annotations
 *	are either single square markers, which display as a circle on the
 *	board, or arrows between squares.
 *
 */


class ANO
{
	friend class SPABD;
	SQ sqFrom, sqTo;
public:
	ANO(void) { }
	~ANO(void) { }
	ANO(SQ sq) : sqFrom(sq), sqTo(sqNil) { }
	ANO(SQ sqFrom, SQ sqTo) : sqFrom(sqFrom), sqTo(sqTo) { }
};


/*
 *
 *	SPABD class
 * 
 *	Class that keeps and displays the game board on the screen inside
 *	the board panel
 * 
 */


class HTBD;

class SPABD : public SPA
{
public:
	static ID2D1SolidColorBrush* pbrLight;
	static ID2D1SolidColorBrush* pbrDark;	
	static ID2D1SolidColorBrush* pbrBlack;
	static ID2D1SolidColorBrush* pbrAnnotation;
	static ID2D1SolidColorBrush* pbrHilite;
	static IDWriteTextFormat* ptfLabel;
	static IDWriteTextFormat* ptfControls;
	static IDWriteTextFormat* ptfGameState;
	static ID2D1Bitmap* pbmpPieces;
	static ID2D1PathGeometry* pgeomCross;
	static ID2D1PathGeometry* pgeomArrowHead;

	CPC cpcPointOfView;
	RCF rcfSquares;
	float dxyfSquare, dxyfBorder, dxyfMargin;
	float dyfLabel;
	float angle;	// angle for rotation animation
	HTBD* phtDragInit;
	HTBD* phtCur;
	SQ sqHover;
	int ictlHover;
	RCF rcfDragPc;	// rectangle the dragged piece was last drawn in
	vector<MV> rgmvDrag;	// legal moves in the UI
	vector<ANO> rgano;	// annotations

public:
	SPABD(GA* pga);
	~SPABD(void);
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

	void NewGame(void);
	void MakeMv(MV mv, bool fRedraw);
	void UndoMv(bool fRedraw);
	void RedoMv(bool fRedraw);
	
	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);

	virtual void Draw(const RCF* prcfUpdate=NULL);
	void DrawMargins(void);
	void DrawSquares(void);
	void DrawLabels(void);
	void DrawFileLabels(void);
	void DrawRankLabels(void);
	void DrawHover(void);
	void DrawPieces(void);
	void DrawAnnotations(void);
	void DrawSquareAnnotation(SQ sq);
	void DrawArrowAnnotation(SQ sqFrom, SQ sqTo);
	void DrawControls(void);

	void DrawHilites(void);
	void DrawGameState(void);
	void DrawPc(RCF rcf, float opacity, TPC tpc);
	void DrawDragPc(const RCF& rcf);
	RCF RcfGetDrag(void);
	void InvalOutsideRcf(RCF rcf) const;
	void InvalRectF(float left, float top, float right, float bottom) const;
	void HiliteLegalMoves(SQ sq);
	void HiliteControl(int ictl);
	RCF RcfFromSq(SQ sq) const;
	RCF RcfControl(int ictl) const;
	void DrawControl(WCHAR ch, int ictl, HTT htt) const;

	void Resign(void);
	void FlipBoard(CPC cpcNew);

	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;

	virtual HT* PhtHitTest(PTF ptf);
	virtual void StartLeftDrag(HT* pht);
	virtual void EndLeftDrag(HT* pht);
	virtual void LeftDrag(HT* pht);
	virtual void MouseHover(HT* pht);

	bool FMoveablePc(SQ sq);
};


/*
 *
 *	TK class
 * 
 *	A simple file scanner token class. These are virtual classes, so
 *	they need to be allocated.
 * 
 */

static const string szNull("");
class TK
{
	int tk;
public:
	operator int() const { return tk; }
	TK(int tk) : tk(tk) { }
	virtual ~TK(void) { }

	virtual bool FIsString(void) const {
		return false;
	}

	virtual bool FIsInteger(void) const {
		return false;
	}

	virtual const string& sz(void) const {
		return szNull;
	}
};

class TKSZ : public TK
{
	string szToken;
public:
	TKSZ(int tk, const string& sz) : TK(tk), szToken(sz) { }
	TKSZ(int tk, const char* sz) : TK(tk) {
		szToken = string(sz);
	}

	virtual ~TKSZ(void) { }

	virtual bool FIsString(void) const {
		return true;
	}

	virtual const string& sz(void) const {
		return szToken;
	}
};

class TKW : public TK
{
	int wToken;
public:
	TKW(int tk, int w) : TK(tk), wToken(w) { }

	virtual bool FIsInteger(void) const {
		return true;
	}

	virtual int w(void) const {
		return wToken;
	}
};


/*
 *
 *	ISTK class
 * 
 *	A generic token input stream class. We'll build this up to be a
 *	general purpose scanner eventually.
 * 
 */


class ISTK
{
protected:
	int li;
	istream& is;
	TK* ptkPrev;

	/*	ISTK::ChNext
	 *
	 *	Reads the next character from the input stream. Returns null character at
	 *	EOF, and coallesces end of line characters into \n for all platforms.
	 */
	char ChNext(void)
	{
		if (is.eof())
			return '\0';
		char ch;
		if (!is.get(ch))
			return '\0';
		if (ch == '\r') {
			if (is.peek() == '\n')
				is.get(ch);
			li++;
			return '\n';
		}
		else if (ch == '\n')
			li++;
		return ch;
	}

	void UngetCh(char ch)
	{
		assert(ch != '\0');
		if (ch == '\n')
			li--;
		is.unget();
	}


	inline bool FIsWhite(char ch) const
	{
		return ch == ' ' || ch == '\t';
	}

	inline bool FIsEnd(char ch) const
	{
		return ch == '\0';
	}

	bool FIsDigit(char ch) const
	{
		return ch >= '0' && ch <= '9';
	}

	virtual bool FIsSymbol(char ch, bool fFirst) const
	{
		if (ch >= 'a' && ch <= 'z')
			return true;
		if (ch >= 'A' && ch <= 'Z')
			return true;
		if (ch == '_')
			return true;
		if (fFirst)
			return false;
		if (ch >= '0' && ch <= '9')
			return true;
		return false;
	}


public:
	ISTK(istream& is) : is(is), li(1), ptkPrev(NULL) { }
	~ISTK(void) { }
	operator bool() { return (bool)is; }
	virtual TK* PtkNext(void) = 0;
	void UngetTk(TK* ptk) { ptkPrev = ptk;  }
	int line(void) const { return li; }
};


/*
 *
 *	ISTKPGN class
 * 
 *	Class for scanning/tokenizing chess PGN files
 * 
 */


enum TKPGN {
	tkpgnWhiteSpace,
	tkpgnBlankLine,
	tkpgnEndLine,
	tkpgnString,
	tkpgnComment,
	tkpgnLineComment,
	tkpgnLParen,
	tkpgnRParen,
	tkpgnLBracket,
	tkpgnRBracket,
	tkpgnLAngleBracket,
	tkpgnRAngleBracket,
	tkpgnPeriod,
	tkpgnStar,
	tkpgnPound,
	tkpgnInteger,
	tkpgnNumAnno,
	tkpgnSymbol,
	tkpgnEnd,

	/* tag keywords */

	tkpgnEvent,
	tkpgnSite,
	tkpgnDate,
	tkpgnRound,
	tkpgnWhite,
	tkpgnBlack,
	tkpgnResult
};


class ISTKPGN : public ISTK
{
protected:
	bool fWhiteSpace;

	virtual bool FIsSymbol(char ch, bool fFirst) const
	{
		if (ch >= 'a' && ch <= 'z')
			return true;
		if (ch >= 'A' && ch <= 'Z')
			return true;
		if (ch >= '0' && ch <= '9')
			return true;
		if (fFirst)
			return false;
		return ch == '_' || ch == '+' || ch == '#' || ch == '=' || ch == ':' || ch == '-' || ch == '/';
	}

	char ChSkipWhite(void);

public:
	ISTKPGN(istream& is) : ISTK(is), fWhiteSpace(false) { }
	void WhiteSpace(bool fReturn) { fWhiteSpace = fReturn; }
	virtual TK* PtkNext(void);
};