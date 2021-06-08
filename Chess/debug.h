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
public:
	UIDBBTNS(UI* puiParent);
	void Draw(const RCF* prcfUpdate = NULL);
	virtual void Layout(void);
	virtual SIZF SizfLayoutPreferred(void);
};


struct LGENTRY
{
	LGT lgt;
	LGF lgf;
	wstring szTag;
	wstring szData;
	int depth;
	float dyfTop;	// position of the line relative to rcfCont
	float dyfHeight;

	LGENTRY(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData) : lgt(lgt), szTag(szTag), szData(szData), lgf(lgf), depth(0), dyfTop(0.0f), dyfHeight(10.0f)
	{
	}
};

class UIDB : public UIPS
{
	UIDBBTNS uidbbtns;
	vector<LGENTRY> rglgentry;
	TX* ptxLog;
	TX* ptxLogBold;
	float dyfLine;
	int depthLog;
	int depthShow;
public:
	UIDB(GA* pga);
	virtual void Layout(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DyfLine(void) const;
	void ShowLog(LGT lgt, LGF lgf, const wstring& szTag, const wstring& szData);
	bool FCombineLogEntries(const LGENTRY& lgentry1, const LGENTRY& lgentry2) const;
	void ClearLog(void);
	void SetLogDepth(int depth);
};