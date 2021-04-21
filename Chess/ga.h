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
#include "ml.h"
#include "uibd.h"


/*
 *	Timer IDs. Must be unique per application.
 */

const UINT tidClock = 1;



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
	void UndoMv(bool fRedraw);
	void RedoMv(bool fRedraw);

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