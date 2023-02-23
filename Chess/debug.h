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
		lgt(lgtNil), lgfOpen(lgfNormal), lgfClose(lgfNormal), 
		tagOpen(L""), tagClose(L""), szDataOpen(L""), szDataClose(L""),
		depth(-1), 
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
	LG lgRoot, *plgCur;		// the logging data and location we're logging into
	TX* ptxLog;
	TX* ptxLogBold;
	TX* ptxLogItalic;
	TX* ptxLogBoldItalic;
	float dyLine;
	int depthCur, depthShow, depthFile;
	ofstream* posLog;

public:
	UIDB(UIGA& uiga);
	virtual ~UIDB(void);

	/* UI layout and drawing */

	virtual void Layout(void);
	virtual SIZ SizLayoutPreferred(void);
	virtual float DyLine(void) const;

	virtual void Draw(const RC& rcUpdate);
	virtual void DrawContent(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void ComputeMetrics(bool fStatic);

private:
	void DrawLg(LG& lg, float yTop, float yBot);
	void DrawItem(const wstring& sz, int depth, LGF lgf, RC rc);

	/*	adding log entries and recomputing layout information */

public:
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;	
private:
	float DyComputeLgPos(LG& lg);
	LG* PlgPrev(const LG* plg) const;
	float DyBlock(LG& lg) const;
	bool FRcOfLgOpen(const LG& lg, RC& rc, float yTop, float yBot) const;
	bool FRcOfLgClose(const LG& lg, RC& rc, float yTop, float yBot) const;
	void FullRelayoutLg(LG& lg, float& yTop);
	bool FLgAnyChildVisible(const LG& lg) const;
	bool FCollapsedLg(const LG& lg) const;

	/* the rest of the public logging interface */

public:
	void InitLog(void) noexcept;
	void ClearLog(void) noexcept;
	
	/*	UIDB::DepthLog
	 *
	 *	Returns the depth we're currently logging to.
	 */
	__forceinline int DepthLog(void) const noexcept
	{
		return max(depthShow, depthFile);
	}

	/*	UIDB::FDepthLog
	 *
	 *	Returns true if the item should be logged. Keeps track of the depth of our
	 *	structured log in the Open/Close. This function is used as an optimization
	 *	so we don't construct logging data if the data isn't going to be logged.
	 *
	 *	Speed of FDepthLog is critical to overall AI speed.
	 */
	__forceinline bool FDepthLog(LGT lgt, int& depth) noexcept
	{
		if (lgt == lgtClose)
			depthCur--;
		assert(depthCur >= 0);
		depth = depthCur;
		if (lgt == lgtOpen)
			depthCur++;
		return depth <= DepthLog();
	}

	int DepthShow(void) const noexcept;
	int DepthFile(void) const noexcept;
	void SetDepthShow(int depth) noexcept;
	void SetDepthFile(int depth) noexcept;
	void EnableLogFile(bool fSave);
	bool FLogFileEnabled(void) const noexcept;
	void RelayoutLog(void);
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
	virtual ~UIDT(void);
	
	virtual void Draw(const RC& rcUpdate);
	virtual bool FCreateRsrc(void);
	virtual void ReleaseRsrc(void);
	void AdvanceDrawSz(const wstring& sz, TX* ptx, RC& rc);
	void AdvanceDrawSzFit(const wstring& sz, TX* ptx, RC& rc);
	void AdvanceDrawSzBaseline(const wstring& sz, TX* ptx, RC& rc, float dyBaseline);
};