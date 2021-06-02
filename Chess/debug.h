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


class UIDB : public UIPS
{
	UIDBBTNS uidbbtns;
	vector<wstring> rgszLog;
	TX* ptxLog;
	float dyfLine;
public:
	UIDB(GA* pga);
	virtual void Layout(void);
	virtual void CreateRsrc(void);
	virtual void DiscardRsrc(void);
	virtual void Draw(const RCF* prcfUpdate = NULL);
	virtual void DrawContent(const RCF& rcfCont);
	virtual float DyfLine(void) const;
	void ShowLog(LGT lgt, const wstring& sz);
	void ClearLog(void);
};