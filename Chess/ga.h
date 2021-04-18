#pragma once

/*
 *
 *	ga.h
 *
 *	Chess game
 *
 */

#include "framework.h"
#include "pl.h"
#include "ui.h"


/*
 *	Timer IDs. Must be unique per application.
 */

const UINT tidClock = 1;


/*
 *
 *	SPA class
 *
 *	Screen panel class. Base class for reserving space on the
 *	graphics screen where we can display random stuff.
 *
 */
class GA;

enum class LL
{
	None,
	Absolute,
	Left,
	Right,
	Below,
	Above
};

enum class HTT
{
	Miss,
	Static,
	MoveablePc,
	UnmoveablePc,
	OpponentPc,
	EmptyPc,
	FlipBoard, 
	Resign
};

class SPA;


/*
 *
 *	HT class
 * 
 *	Hit test class, which includes information for the hit test over various
 *	parts of the game screen. This structure is virtual, which allows for
 *	different data to be hit on in different screen panels.
 * 
 *	Because of the virtualness, it must be allocated with new and they must
 *	implement a Clone method. No copy constructors!
 * 
 */


class HT
{
public:
	HTT htt;
	PTF ptf;
	SPA* pspa;
	HT(const PTF& ptf, HTT htt, SPA* pspa) : ptf(ptf), htt(htt), pspa(pspa) { }
	virtual HT* PhtClone(void) { return new HT(ptf, htt, pspa); }
};



/*
 *
 *	SPA class
 * 
 *	A screen panel.
 * 
 */


class SPA : public UI
{
public:
	static ID2D1SolidColorBrush* pbrTextSel;
	static IDWriteTextFormat* ptfTextSm;
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

protected:
	GA& ga;
public:
	SPA(GA* pga);
	~SPA(void);
	virtual void Draw(const RCF* prcfUpdate=NULL);
	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;
	void SetShadow(void);

	virtual HT* PhtHitTest(PTF ptf);
	virtual void StartLeftDrag(HT* pht);
	virtual void EndLeftDrag(HT* pht);
	virtual void LeftDrag(HT* pht);
	virtual void MouseHover(HT* pht);
};


class SPATI : public SPA
{
	wstring szText;

public:
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);
	static IDWriteTextFormat* ptfPlayers;
	static ID2D1Bitmap* pbmpLogo;

public:
	SPATI(GA* pga);
	virtual void Draw(const RCF* prcfUpdate=NULL);
	void SetText(const wstring& sz);
};


#include "bd.h"


class HTBD : public HT
{
public:
	SQ sq;
	HTBD(const PTF& ptf, HTT htt, SPA* pspa, SQ sq) : HT(ptf, htt, pspa), sq(sq) { }
	virtual HT* PhtClone(void) { return new HTBD(ptf, htt, pspa, sq); }
};


/*
 *
 *	SPAS class
 * 
 *	A scrolling screen panel base class. 
 * 
 */


class SPAS : public SPA
{
private:
	RCF rcfView;
	RCF rcfCont;
protected:
	const float dxyfScrollBarWidth = 10.0f;
public:
	SPAS(GA* pga) : SPA(pga) { }


	void SetView(const RCF& rcfView)
	{
		this->rcfView = rcfView;
		this->rcfView.right -= dxyfScrollBarWidth;
		this->rcfView.Offset(rcfBounds.left, rcfBounds.top);
	}


	void SetContent(const RCF& rcfCont)
	{
		this->rcfCont = rcfCont;
		this->rcfCont.Offset(rcfBounds.left, rcfBounds.top);
	}


	virtual RCF RcfView(void) const
	{
		RCF rcf = rcfView;
		rcf.Offset(-rcfBounds.left, -rcfBounds.top);
		return rcf;
	}


	void UpdateContSize(const PTF& ptf)
	{
		rcfCont.bottom = rcfCont.top + ptf.y;
		rcfCont.right = rcfCont.left + ptf.x;
	}


	virtual RCF RcfContent(void) const
	{
		RCF rcf = rcfCont;
		rcf.Offset(-rcfBounds.left, -rcfBounds.top);
		return rcf;
	}


	void ScrollTo(int yfTop)
	{
		rcfCont.Offset(0, rcfView.top - rcfCont.top + yfTop);
		Redraw();
	}


	bool FMakeVis(float yf, float dyf)
	{
		yf += rcfBounds.top;
		if (yf < rcfView.top)
			rcfCont.Offset(0, rcfView.top - yf);
		else if (yf+dyf > rcfView.bottom)
			rcfCont.Offset(0, rcfView.bottom - (yf+dyf));
		else
			return false;
		Redraw();
		return true;
	}

	virtual void Draw(const RCF* prcfUpdate=NULL)
	{
		SPA::Draw(prcfUpdate);
		/* just redraw the entire content area clipped to the view */
		PrtGet()->PushAxisAlignedClip(rcfView, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		DrawContent(rcfCont);
		PrtGet()->PopAxisAlignedClip();
		DrawScrollBar();
	}


	virtual void DrawContent(const RCF& rcf)
	{
	}


	void DrawScrollBar(void)
	{
		RCF rcf = rcfView;
		rcf.left = rcf.right;
		rcf.right = rcf.left + dxyfScrollBarWidth;
		PrtGet()->FillRectangle(rcf, pbrAltBack);
	}
};


/*
 *
 *	UICLOCK class
 *
 *	A UI element to display a chess clock. Usually two of these
 *	on the screen at any given time.
 * 
 */

class SPARGMV;

class UICLOCK : public UI
{
protected:
	GA& ga;
	CPC cpc;
	static IDWriteTextFormat* ptfClock;
public:
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

public:
	UICLOCK(SPARGMV* pspargmv, CPC cpc);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	void DrawColon(RCF& rcf, unsigned frac) const;
	bool FTimeOutWarning(DWORD tm) const;
};


/*
 *
 *	UIGO
 * 
 *	Game over sub-panel in the move list.
 * 
 */


class UIGO : public UI
{
protected:
	GA& ga;
	static IDWriteTextFormat* ptfScore;

public:
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

public:
	UIGO(SPARGMV* pspargmv, bool fVisible);
	virtual void Draw(const RCF* prcfUpdate = NULL);
};



class SPARGMV : public SPAS
{
	friend class UICLOCK;
	friend class UIGO;
	friend class GA;

	static IDWriteTextFormat* ptfList;
	static float mpcoldxf[4];
	static float dyfList;

	float XfFromCol(int col) const;
	float DxfFromCol(int col) const; 
	RCF RcfFromCol(float yf, int col) const;
	RCF RcfFromImv(int imv) const;

	BDG bdgInit;	// initial board at the start of the game list
	int imvSel;
	UICLOCK* mpcpcpuiclock[2];
	UIGO* puigo;

public:	
	SPARGMV(GA* pga);
	~SPARGMV(void);

	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwicfHTT);
	static void DiscardRsrc(void);

	void NewGame(void);
	void EndGame(void);

	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual void Draw(const RCF* prcfUpdate=NULL);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;

	void DrawAndMakeMv(RCF rcf, BDG& bdg, MV mv);
	void DrawMoveNumber(RCF rcf, int imv);
	void DrawSel(int imv);
	void SetSel(int imv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);

	void DrawPl(CPC cpcPointOfView, RCF rcfArea, bool fTop) const;
};


/*
 *
 *	GTM
 * 
 *	Game timing options
 * 
 */
class GTM
{
	DWORD tmGame;
	DWORD dtmMove;
public:
	GTM(void) : tmGame(3 * 60 * 1000), dtmMove(5 * 1000) { }
	DWORD TmGame(void) const {
		return tmGame;
	}
	DWORD DtmMove(void) const {
		return dtmMove;
	}
};



/*
 *
 *	GA class
 * 
 *	The chess game. 
 * 
 */


class APP;

class GA : public UI
{
	friend class SPA;
	friend class SPATI;
	friend class SPABD;
	friend class SPARGMV;

protected:
	static ID2D1SolidColorBrush* pbrDesktop;
public:
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

	APP& app;
	SPATI spati;
	SPABD spabd;
	SPARGMV spargmv;
	HT* phtCapt;

public:
	BDG bdg;	// board
	PL* mpcpcppl[2];	// players
	GTM gtm;
	DWORD mpcpctmClock[2];	// player clocks
	DWORD tmLast;	// time of last move

public:
	GA(APP& app);
	~GA(void);

	void Init(void);

	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual ID2D1RenderTarget* PrtGet(void) const;
	virtual void BeginDraw(void);
	virtual void EndDraw(void);

	void Resize(int dx, int dy);
	void Layout(void);

	HT* PhtHitTest(PTF ptf);
	void MouseMove(HT* pht);
	void LeftDown(HT* pht);
	void LeftUp(HT* pht);
	void SetCapt(HT* pht);
	void ReleaseCapt(void);
	inline PL*& PlFromCpc(CPC cpc) { return mpcpcppl[cpc]; }
	inline PL* PplFromCpc(CPC cpc) { return PlFromCpc(cpc); }
	void SetPl(CPC cpc, PL* ppl);

	void Timer(UINT tid, DWORD tm);

	void NewGame(void);
	void StartGame(void);
	void EndGame(void);
	void MakeMv(MV mv, bool fRedraw);
	void SwitchClock(DWORD tmCur);
	void StartClock(CPC cpc, DWORD tmCur);
	void PauseClock(CPC cpc, DWORD tmCur);
	void UndoMv(void);
	void RedoMv(void);

	void Test(void);
	void ValidateFEN(const WCHAR* szFEN) const;
	void ValidatePieces(const WCHAR*& sz) const;
	void ValidateMoveColor(const WCHAR*& sz) const;
	void ValidateCastle(const WCHAR*& sz) const;
	void ValidateEnPassant(const WCHAR*& sz) const;
	void SkipWhiteSpace(const WCHAR*& sz) const;
	void SkipToWhiteSpace(const WCHAR*& sz) const;

	void PlayPGNFiles(const WCHAR szPath[]);
	int PlayPGNFile(const WCHAR szFile[]);
	int PlayPGNGame(ISTKPGN& istkpgn);
	int ReadPGNHeaders(ISTKPGN& istkpgn);
	int ReadPGNMoveList(ISTKPGN& istkpgn);
	int ReadPGNTag(ISTKPGN& istkpgn);
	int ReadPGNMove(ISTKPGN& istkpgn);
	bool FIsMoveNumber(TK* ptk, int& w) const;
	void ProcessTag(const string& szTag, const string& szVal);
	void HandleTag(int tkpgn, const string& szVal);
	void ProcessMove(const string& szMove);

	void UndoTest(void);
	int PlayUndoPGNFile(const WCHAR* szFile);
	void UndoFullGame(void);
};