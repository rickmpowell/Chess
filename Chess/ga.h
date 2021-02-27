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
	EmptyPc
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
	static ID2D1SolidColorBrush* pbrGridLine;
	static ID2D1SolidColorBrush* pbrAltBack;
	static IDWriteTextFormat* ptfText;
	
	SPA(GA& ga);
	~SPA(void);
	virtual void Draw(ID2D1RenderTarget* prt);
	void Redraw(void);
	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;
	void SetShadow(ID2D1RenderTarget* prt);

	virtual HT* PhtHitTest(const PTF& ptf);
	virtual void StartLeftDrag(HT* pht);
	virtual void EndLeftDrag(HT* pht);
	virtual void LeftDrag(HT* pht);
	virtual void MouseHover(HT* pht);
};


class SPATI : public SPA
{
public:
	SPATI(GA& ga);
	virtual void Draw(ID2D1RenderTarget* prt);
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
protected:
	RCF rcfView;
	RCF rcfCont;
	const float dxyfScrollBarWidth = 20.0f;
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


	void ScrollTo(int yfTop)
	{
		rcfCont.Offset(0, rcfView.top - rcfCont.top + yfTop);
		Redraw();
	}


	virtual void Draw(ID2D1RenderTarget* prt)
	{
		SPA::Draw(prt);
		/* just redraw the entire content area clipped to the view */
		prt->PushAxisAlignedClip(rcfView, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		DrawContent(prt, rcfCont);
		prt->PopAxisAlignedClip();
		DrawScrollBar(prt);
	}


	virtual void DrawContent(ID2D1RenderTarget* prt, const RCF& rcf)
	{
	}


	void DrawScrollBar(ID2D1RenderTarget* prt)
	{
		RCF rcf = rcfView;
		rcf.left = rcf.right;
		rcf.right = rcf.left + dxyfScrollBarWidth;
		prt->FillRectangle(rcf, pbrAltBack);
	}
};

class SPARGMV : public SPAS
{
	static IDWriteTextFormat* ptfList;
	static float mpcoldxf[4];
	static float dyfList;
	BDG bdgInit;	// initial board at the start of the game list

public:	
	SPARGMV(GA& ga);
	~SPARGMV(void);

	static void CreateRsrc(ID2D1RenderTarget* prt, IDWriteFactory* pfactdwr);
	static void DiscardRsrc(void);

	void NewGame(void);

	virtual void Layout(const PTF& ptf, SPA* pspa, LL ll);
	virtual void Draw(ID2D1RenderTarget* prt);
	virtual void DrawContent(ID2D1RenderTarget* prt, const RCF& rcfCont);
	void DrawMv(ID2D1RenderTarget* prt, RCF rcf, MV mv);
	void DrawMoveNumber(ID2D1RenderTarget* prt, RCF rcf, int imv);
	WCHAR* PchDecodeInt(unsigned imv, WCHAR* pch);
	virtual float DxWidth(void) const;
	virtual float DyHeight(void) const;
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
	SPATI spati;
	SPABD spabd;
	SPARGMV spargmv;
	vector<SPA> rgspa;
	PL* rgppl[2];
	HT* phtCapt;

public:
	BDG bdg;

public:
	GA(APP& app);
	~GA(void);
	void NewGame(void);
	
	static void CreateRsrc(ID2D1RenderTarget* prt, ID2D1Factory* pfactd2d, IDWriteFactory* pfactdwr, IWICImagingFactory* pfactwic);
	static void DiscardRsrc(void);

	void Draw(ID2D1RenderTarget* prt);
	HT* PhtHitTest(const PTF& ptf);
	void MouseMove(HT* pht);
	void LeftDown(HT* pht);
	void LeftUp(HT* pht);
	void SetCapt(HT* pht);
	void ReleaseCapt(void);
	inline PL*& PlFromTpc(BYTE tpc) { return rgppl[(tpc & tpcColor) == tpcWhite]; }
	inline PL* PplFromTpc(BYTE tpc) { return PlFromTpc(tpc); }
	void SetPl(BYTE tpc, PL* ppl);
	void MakeMv(MV mv);
	
	void Test(void);
	void ValidateFEN(const WCHAR* szFEN) const;
	void ValidatePieces(const WCHAR*& sz) const;
	void ValidateMoveColor(const WCHAR*& sz) const;
	void ValidateCastle(const WCHAR*& sz) const;
	void ValidateEnPassant(const WCHAR*& sz) const;
	void SkipWhiteSpace(const WCHAR*& sz) const;
	void SkipToWhiteSpace(const WCHAR*& sz) const;
};