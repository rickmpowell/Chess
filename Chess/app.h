/*
 *
 *	app.h
 * 
 *	Application classes, for Windows apps.
 * 
 */

#pragma once
#include "framework.h"


/*
 *
 *	class CO and D2
 *
 *	Application base classes that initialize and shutdown COM and Direct2D
 *	objects in the correct order.
 *
 *	Derive your APP class from these to initiaize COM and/or Direct2D.
 *
 */


class CO
{
public:
	CO(void);
	~CO(void);
};


class D2 : public CO
{
protected:
	void Cleanup(void);

public:
	FACTD2* pfactd2;
	FACTWIC* pfactwic;
	FACTDWR* pfactdwr;

public:
	D2(void);
	~D2(void);
};


/*
 *
 *  CMD class
 *
 *  Little class for wrapping together and organiziong top-level commands. Useful for
 *	common interface for dispatching commands, getting tip text, initializing menus, 
 *	enable states, etc.
 *
 */


class APP;

class CMD
{
	friend class VCMD;

protected:
	APP& app;
	size_t icmd;

public:
	CMD(APP& app, size_t icmd);
	virtual ~CMD(void) { }
	virtual int Execute(void);
	virtual bool FEnabled(void) const;
	virtual bool FChecked(void) const;
	virtual wstring SzMenu(void) const;
	virtual void InitMenu(HMENU hmenu);
	virtual int IdsMenu(void) const;
	virtual wstring SzTip(void) const;
	virtual int IdsTip(void) const;
};


/*
 *
 *	VCMD
 *
 *	The command list, used for dispatching to command handlers.
 *
 */


class VCMD : public vector<CMD*> {
public:
	VCMD(void);
	~VCMD(void);
	void Add(CMD* pcmd);
	int Execute(int icmd);
	bool FEnabled(int icmd);
	void InitMenu(HMENU hmenu);
	wstring SzTip(int cmd);
};


/*
 *
 *	Logging types
 *
 */


enum LGT : int {
	lgtOpen,
	lgtClose,
	lgtData,
	lgtTemp,
	lgtNil
};

enum LGF : int {
	lgfNormal,
	lgfBold,
	lgfItalic,
	lgfBoldItalic
};

struct ATTR
{
	wstring name, val;

	ATTR(const wstring& name, const wstring& val) : name(name), val(val) { }
};


struct TAG : public map<wstring, wstring>
{
	wstring sz;

	TAG(const wchar_t* sz) : sz(wstring(sz)) { }
	TAG(const wstring& sz) : sz(sz) { }
	TAG(const wstring& sz, const ATTR& attr) : sz(sz) { (*this)[attr.name] = attr.val; }
};


/*
 *
 *	Mouse click information
 * 
 */


struct MCI
{
	PT pt;
	DWORD tm;
	MCI(void) : pt(0,0), tm(0) { }
	MCI(PT pt, DWORD tm) : pt(pt), tm(tm) { }

	/*	FIsSingleClick
	 *
	 *	Returns true if this mouse up information is the trigger for single click. Must
	 *	be quick and more-or-less in the same location
	 */
	bool FIsSingleClick(const MCI& mciDown) const 
	{
		return abs(pt.x - mciDown.pt.x) < 2 && abs(pt.y - mciDown.pt.y) < 2;
	}
};


/*
 *
 *	APP class
 *
 *	Base application class, which simply initiaizes the app and creates the top-level
 *	window. This includes the WndProc for the top application window, which dispatches
 *	to virtual "On" function to handle the messages.
 *
 */

class UIGA;
class GA;

class APP : public D2
{
	friend class UIGA;
	friend class CMD;

public:

	/* crap we need for Windows */

	HINSTANCE hinst;
	HWND hwnd;
	HACCEL haccel;

	/* crap we need to draw on Direct2D contexts */

	ID3D11Device1* pdevd3;
	ID3D11DeviceContext1* pdcd3;
	ID2D1Device* pdevd2;
	SWCH* pswch;
	ID2D1Bitmap1* pbmpBackBuf;
	DC* pdc;

	MCI mciLeftDown, mciRightDown;
	
	GA* pga;
	UIGA* puiga;
	VCMD vcmd;

public:
	APP(HINSTANCE hinst, int sw);
	~APP(void);

	int MessagePump(void);
	DWORD MsecMessage(void);
	POINT PtMessage(void);

	wstring SzLoad(int ids) const;
	wstring SzAppDataPath(void);
	void EnsureDirectory(const wstring& szDir);

	int Error(const wstring& szMsg, int mb);
	int Error(const string& szMsg, int mb);

	static bool FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic);
	static void DiscardRsrcStatic(void);
	bool FCreateRsrc(void);
	void DiscardRsrc(void);
	bool FCreateRsrcSize(void);
	void DiscardRsrcSize(void);

	void InitCmdList(void);
	bool FEnableCmds(void) const;

	void Redraw(void);
	void Redraw(RC rc);

	/*	
	 *	Window message handlers 
	 */

	void OnDestroy(void);
	void OnPaint(void);
	void OnSize(UINT dx, UINT dy);
	bool OnCommand(int idm);
	bool OnMouseMove(UINT mk, int x, int y);
	bool OnLeftDown(int x, int y);
	bool OnLeftUp(int x, int y);
	bool OnRightDown(int x, int y);
	bool OnRightUp(int x, int y);
	bool OnMouseWheel(int x, int y, int dwheel);
	bool OnTimer(UINT tid);
	bool OnInitMenu(void);
	bool OnKeyDown(int vk);
	bool OnKeyUp(int vk);

public:
	
	/*
	 *	Timers
	 */

	TID StartTimer(DWORD dmsec);
	void StopTimer(TID tid);
	void DispatchTimer(TID tid, DWORD msec);
	
	/*
	 *	Cursors
	 */

	void SetCursor(HCURSOR hcrs);
	HCURSOR hcurArrow;
	HCURSOR hcurMove;
	HCURSOR hcurNo;
	HCURSOR hcurUpDown;
	HCURSOR hcurHand;
	HCURSOR hcurCrossHair;
	HCURSOR hcurUp;

	/*
	 *	Logging. For now, this just hard-coded to delegate to the debug panel,
	 *	but this'll need to change
	 */

	void ClearLog(void) noexcept;
	void InitLog(void) noexcept;
	inline bool FDepthLog(LGT lgt, int& depth) noexcept;
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;
	inline int DepthLog(void) const noexcept;
	inline int DepthShow(void) const noexcept;
	inline void SetDepthShow(int depth) noexcept;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam);
};

extern APP* papp;


/*
 *
 *	CLIPB
 * 
 *	A little clipboard class to make sure we open/close the clipboard right with
 *	error handling
 *
 */


class CLIPB
{
	APP& app;
	bool fClipOpen;
	HGLOBAL hdata;
	HGLOBAL hdataGet;
	void* pdata;

public:
	CLIPB(APP& app);
	~CLIPB(void);

	void Open(void);
	void Close(void);
	void Empty(void);
	void* PLock(void);
	void Unlock(void);
	void* PAlloc(int cb);
	void Free(void);
	void SetData(int cf, void* pv, int cb);
	void* PGetData(int cf);
};


inline void InitLog(void)
{
	papp->InitLog();
}

inline void ClearLog(void)
{
	papp->ClearLog();
}

inline void SetDepthShow(int depth) noexcept
{
	papp->SetDepthShow(depth);
}

inline int DepthLog(void) noexcept
{
	return papp->DepthLog();
}

inline int DepthShow(void) noexcept
{
	return papp->DepthShow();
}


/* these are done with macros to avoid computation of the string arguments if 
   we're not actually logging */

#define LogHelper(lgt, lgf, tag, szData) \
	{ \
		int depthLog; \
		if (papp->FDepthLog(lgt, depthLog)) \
			papp->AddLog(lgt, lgf, depthLog, tag, szData); \
	}
	
#define LogOpen(tag, szData, lgf) LogHelper(lgtOpen, lgf, tag, szData)
#define LogClose(tag, szData, lgf) LogHelper(lgtClose, lgf, tag, szData)
#define LogData(szData) LogHelper(lgtData, lgfNormal, L"", szData)
#define LogTemp(szData) LogHelper(lgtTemp, lgfNormal, L"", szData)

inline void Log(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData)
{
	papp->AddLog(lgt, lgf, depth, tag, szData);
}


/*
 *	Some utility functions
 */


ColorF CoBlend(ColorF co1, ColorF co2, float pct);