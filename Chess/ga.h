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
	FlipBoard
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


class SPA
{
protected:
	GA& ga;
	RCF rcfBounds;
public:
	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr);
	static void DiscardRsrc(void);
	static ID2D1SolidColorBrush* pbrBack;
	static ID2D1SolidColorBrush* pbrText;
	static ID2D1SolidColorBrush* pbrTextSel;
	static ID2D1SolidColorBrush* pbrGridLine;
	static ID2D1SolidColorBrush* pbrAltBack;
	static IDWriteTextFormat* ptfText;
	static IDWriteTextFormat* ptfTextSm;
	
	SPA(GA& ga);
	~SPA(void);
	virtual void Draw(void);
	void Redraw(void);
	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	RCF RcfBounds(void) const;
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;
	void SetShadow(void);

	ID2D1RenderTarget* PrtGet(void) const;
	void FillRcf(RCF rcf, ID2D1Brush* pbr) const;
	void FillEllf(ELLF ellf, ID2D1Brush* pbr) const;
	void DrawSz(const wstring& sz, IDWriteTextFormat* ptf, RCF rcf, ID2D1Brush* pbr=NULL) const;
	void DrawBmp(RCF rcfTo, ID2D1Bitmap* pbmp, RCF rcfFrom, float opacity = 1.0f) const;

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
	SPATI(GA& ga);
	virtual void Draw(void);
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
 *	A scrolling screen panel
 * 
 */


class SPAS : public SPA
{
private:
	RCF rcfView;
	RCF rcfCont;
protected:
	const float dxyfScrollBarWidth = 12.0f;
public:
	SPAS(GA& ga) : SPA(ga) { }


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

	virtual void Draw(void)
	{
		SPA::Draw();
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


class SPARGMV : public SPAS
{
	static IDWriteTextFormat* ptfList;
	static float mpcoldxf[4];
	static float dyfList;
	static IDWriteTextFormat* ptfClock;
	float XfFromCol(int col) const;
	float DxfFromCol(int col) const; 
	RCF RcfFromCol(float yf, int col) const;
	RCF RcfFromImv(int imv) const;

	BDG bdgInit;	// initial board at the start of the game list
	int imvSel;

public:	
	SPARGMV(GA& ga);
	~SPARGMV(void);

	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr);
	static void DiscardRsrc(void);

	void NewGame(void);

	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual void Draw(void);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;

	void DrawMv(RCF rcf, const BDG& bdg, MV mv);
	void DrawMoveNumber(RCF rcf, int imv);
	WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch) const;
	void DrawSel(int imv);
	void SetSel(int imv);

	bool FMakeVis(int imv);
	void UpdateContSize(void);

	void DrawPl(CPC cpcPointOfView, RCF rcfArea, bool fTop) const;
	void DrawClock(CPC cpc, RCF rcfArea) const;
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
	DWORD tickGame;
	DWORD dtickMove;
public:
	GTM(void) : tickGame(10 * 60 * 100), dtickMove(0) { }
	DWORD TickGame(void) const {
		return tickGame;
	}
	DWORD DtickMove(void) const {
		return dtickMove;
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

class GA
{
	friend class SPA;
	friend class SPATI;
	friend class SPABD;
	friend class SPARGMV;

	APP& app;

	RCF rcfBounds;
	SPATI spati;
	SPABD spabd;
	SPARGMV spargmv;
	vector<SPA> rgspa;
	HT* phtCapt;

public:
	BDG bdg;	// board
	PL* mpcpcppl[2];	// players
	GTM gtm;
	DWORD mpcpctickClock[2];	// player clocks

public:
	GA(APP& app);
	~GA(void);
	void Init(void);
	void NewGame(void);
	
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

	void Resize(int dx, int dy);
	void Layout(void);

	void Draw(void);
	void Redraw(bool fBackground);
	HT* PhtHitTest(PTF ptf);
	void MouseMove(HT* pht);
	void LeftDown(HT* pht);
	void LeftUp(HT* pht);
	void SetCapt(HT* pht);
	void ReleaseCapt(void);
	inline PL*& PlFromCpc(CPC cpc) { return mpcpcppl[cpc]; }
	inline PL* PplFromCpc(CPC cpc) { return PlFromCpc(cpc); }
	void SetPl(CPC cpc, PL* ppl);

	void MakeMv(MV mv, bool fRedraw);
	void StartClock(CPC cpc, DWORD tickCur);
	void StopClock(CPC cpc, DWORD tickCur);

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
};