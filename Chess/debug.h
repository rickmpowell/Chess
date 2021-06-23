/*
 *
 *	debug.h
 * 
 *	Definitions for the debug panel
 * 
 */

#pragma once
#include "panel.h"
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
	GA& ga;
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
	float dyfTop;	// position of the line relative to rcfCont
	float dyfHeight;

	LGENTRY(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) : 
		lgt(lgt), lgf(lgf), tag(tag), szData(szData), depth(depth), dyfTop(0.0f), dyfHeight(10.0f)
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
	float dyfLine;
	int depthCur;
	int depthShowSet;
	int depthShowDefault;
	ofstream* posLog;
public:
	UIDB(GA* pga);
	~UIDB(void);
	virtual void Layout(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Draw(RCF rcfUpdate);
	virtual void DrawContent(RCF rcfCont);
	virtual float DyfLine(void) const;
	size_t IlgentryFromYf(int yf) const;

	virtual bool FDepthLog(LGT lgt, int& depth); 
	virtual void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData);
	
	bool FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const;
	void InitLog(int depth);
	void ClearLog(void);
	void SetDepthLog(int depth);
	void SetDepthLogDefault(int depth);
	int DepthLog(void) const;
	void EnableLogFile(bool fSave);
	bool FLogFileEnabled(void) const;
};
