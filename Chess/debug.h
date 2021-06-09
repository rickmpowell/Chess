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




class UIDBBTNS : public UI
{
	BTNCH btnTest;
	BTNCH btnUpDepth;
	BTNCH btnDnDepth;
	BTNCH btnLogOnOff;
public:
	UIDBBTNS(UI* puiParent);
	void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
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
	wstring szTag;
	wstring szData;
	int depth;
	float dyfTop;	// position of the line relative to rcfCont
	float dyfHeight;

	LGENTRY(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData) : 
		lgt(lgt), szTag(szTag), szData(szData), lgf(lgf), depth(0), dyfTop(0.0f), dyfHeight(10.0f)
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
	UIDBBTNS uidbbtns;
	vector<LGENTRY> rglgentry;
	TX* ptxLog;
	TX* ptxLogBold;
	float dyfLine;
	int depthCur;
	int depthShow;
	ofstream* posLog;
public:
	UIDB(GA* pga);
	~UIDB(void);
	virtual void Layout(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DyfLine(void) const;
	void AddLog(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData);
	bool FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const;
	void InitLog(int depth);
	void ClearLog(void);
	void SetDepthLog(int depth);
	int DepthLog(void) const;
	void EnableLogFile(bool fSave);
	bool FLogFileEnabled(void) const;
};