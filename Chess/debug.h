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
	LGF lgf;
	TAG tag;
	wstring szData;
	int depth;
	float dyTop;	// position of the line relative to rcCont
	float dyHeight;
	LG* plgParent;
	vector <LG*> vplgChild;

	LG(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) : 
		plgParent(nullptr), lgt(lgt), lgf(lgf), tag(tag), szData(szData), depth(depth), dyTop(0.0f), dyHeight(10.0f)
	{
	}

	LG(void) : plgParent(nullptr), lgt(lgtTemp), lgf(lgfNormal), tag(L""), szData(L""), depth(0), dyTop(0), dyHeight(0)
	{
	}

	void AddChild(LG* plg) {
		plg->plgParent = this;
		vplgChild.push_back(plg);
	}

	~LG()
	{
		while (vplgChild.size() > 0) {
			LG* plg = vplgChild.back();
			vplgChild.pop_back();
			delete plg;
		}
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
	LG lgRoot;;
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
	size_t IlgentryFromY(int y) const;

	bool FDepthLog(LGT lgt, int& depth) noexcept; 
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;	
	bool FCombineLogEntries(const LG& lg1, const LG& lg2) const noexcept;
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