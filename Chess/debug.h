/*
 *
 *	debug.h
 * 
 *	Definitions for the debug panel
 * 
 */

#pragma once
#include "uipa.h"
#include "bd.h"


/*
 *
 *	UIBBDB
 * 
 *	Button bar for debug panel
 * 
 */

class UIDB;
class UIBBDB;
class UIBBDBLOG;

class SPINDEPTH : public SPIN
{
	int& depth;
public:
	SPINDEPTH(UIBBDBLOG* puiParent, int& depth, int cmdUp, int cmdDown);
	virtual wstring SzValue(void) const;
};

class UIBBDB : public UIBB
{
	BTNCH btnTest;
public:
	UIBBDB(UIDB* puiParent);
	virtual void Layout(void);
};


/*
 *
 *	UIBBDBLOG
 * 
 *	Log controls in the debug panel
 *
 */

class UIBBDBLOG : public UIBB
{
	UIDB& uidb;
	friend class SPINDEPTH;
	SPINDEPTH spindepth;
	STATIC staticFile;
	BTNIMG btnLogOnOff;
	SPINDEPTH spindepthFile;
public:
	UIBBDBLOG(UIDB* puiParent);
	virtual void Layout(void);
};


/*
 *
 *	LG structure
 * 
 *	A single log entry. Logs are heirarchical, with open/close elements and data elements.
 *	Individual entries are tag/value pairs.
 * 
 */


struct LG
{
	LGT lgt;
	LGF lgfOpen, lgfClose;
	TAG tagOpen, tagClose;
	wstring szDataOpen, szDataClose;
	int depth;
	float yTop;	// position of the line relative to rcCont
	float dyLineOpen, dyLineClose;	// height of open line
	float dyBlock;	// height of entire subblock, including open children
	LG* plgParent;
	vector <LG*> vplgChild;

	LG(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) : plgParent(nullptr), 
		lgt(lgt), lgfOpen(lgf), lgfClose(lgfNormal), 
		tagOpen(tag), tagClose(L""), szDataOpen(szData), szDataClose(L""),
		depth(depth), 
		yTop(0), dyLineOpen(10), dyLineClose(0), dyBlock(0)
	{
	}

	LG(void) : plgParent(nullptr), 
		lgt(lgtTemp), lgfOpen(lgfNormal), lgfClose(lgfNormal), 
		tagOpen(L""), tagClose(L""), szDataOpen(L""), szDataClose(L""),
		depth(0), 
		yTop(0), dyLineOpen(0), dyLineClose(0), dyBlock(0)
	{
	}

	void AddChild(LG* plg) {
		plg->plgParent = this;
		vplgChild.push_back(plg);
	}

	bool FIsLastSibling(void) {
		if (plgParent == nullptr)
			return true;
		return this == plgParent->vplgChild.back();
	}

	~LG()
	{
		for (; !vplgChild.empty(); vplgChild.pop_back())
			delete vplgChild.back();
	}
};


/*
 *
 *	UIDB user interface panel
 * 
 *	The Debug panel
 *
 */


class UIDB : public UIPS
{
	friend class SPINDEPTH;
	friend class UIBBDBLOG;

	UIBBDB uibbdb;
	UIBBDBLOG uibbdblog;
	LG lgRoot;
	LG* plgCur;
	TX* ptxLog;
	TX* ptxLogBold;
	TX* ptxLogItalic;
	TX* ptxLogBoldItalic;
	float dyLine;
	int depthCur;
	int depthShow;
	int depthFile;
	ofstream* posLog;
public:
	UIDB(UIGA& uiga);
	~UIDB(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);

	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rcCont);
	virtual float DyLine(void) const;

	bool FDepthLog(LGT lgt, int& depth) noexcept; 
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;	
	void ComputeLgPos(LG* plg);
	LG* PlgPrev(const LG* plg) const;
	float DyBlock(LG* plg) const;
	void DrawLg(LG& lg);
	void DrawItem(const wstring& sz, int depth, LGF lgf, RC rc);
	RC RcOfLgOpen(const LG& lg) const;
	RC RcOfLgClose(const LG& lg) const;

	void InitLog(void) noexcept;
	void ClearLog(void) noexcept;
	int DepthLog(void) const noexcept;
	int DepthShow(void) const noexcept;
	int DepthFile(void) const noexcept;
	void SetDepthShow(int depth) noexcept;
	void SetDepthFile(int depth) noexcept;
	void EnableLogFile(bool fSave);
	bool FLogFileEnabled(void) const noexcept;
};



/*
 *
 *	UIDT class
 * 
 *	Draw test panel
 * 
 */


class UIDT : public UIP
{
	TX* ptxTest;
	TX* ptxTest2;
public:
	UIDT(UIGA& uiga);
	virtual void CreateRsrc(void);
	virtual void ReleaseRsrc(void);
	virtual void Draw(const RC& rcUpdate);
	void AdvanceDrawSz(const wstring& sz, TX* ptx, RC& rc);
	void AdvanceDrawSzFit(const wstring& sz, TX* ptx, RC& rc);
	void AdvanceDrawSzBaseline(const wstring& sz, TX* ptx, RC& rc, float dyBaseline);
};