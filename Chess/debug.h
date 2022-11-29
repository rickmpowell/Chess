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
	UIGA& uiga;
public:
	SPINDEPTH(UIBBDBLOG* puiParent);
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
	BTNIMG btnLogOnOff;
	SPINDEPTH spindepth;
public:
	UIBBDBLOG(UIDB* puiParent);
	virtual void Layout(void);
};


/*
 *
 *	LGENTRY structure
 * 
 *	A single log entry. Logs are heirarchical, with open/close elements and data elements.
 *	Individual entries are tag/value pairs.
 * 
 */


struct LGENTRY
{
	LGT lgt;
	LGF lgf;
	TAG tag;
	wstring szData;
	int depth;
	float dyTop;	// position of the line relative to rcCont
	float dyHeight;

	LGENTRY(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) : 
		lgt(lgt), lgf(lgf), tag(tag), szData(szData), depth(depth), dyTop(0.0f), dyHeight(10.0f)
	{
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

	UIBBDB uibbdb;
	UIBBDBLOG uibbdblog;
	vector<LGENTRY> vlgentry;
	TX* ptxLog;
	TX* ptxLogBold;
	TX* ptxLogItalic;
	TX* ptxLogBoldItalic;
	float dyLine;
	int depthCur;
	int depthShowSet;
	int depthShowDefault;
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
	bool FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const noexcept;
	void InitLog(int depth) noexcept;
	void ClearLog(void) noexcept;
	void SetDepthLog(int depth) noexcept;
	void SetDepthLogDefault(int depth) noexcept;
	int DepthLog(void) const noexcept;
	void EnableLogFile(bool fSave);
	bool FLogFileEnabled(void) const noexcept;
};
