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
	friend class CMDLIST;

protected:
	APP& app;
	size_t icmd;

public:
	CMD(APP& app, size_t icmd);
	virtual ~CMD(void) { }
	virtual int Execute(void);
	virtual bool FEnabled(void) const;
	virtual bool FCustomSzMenu(void) const;
	virtual wstring SzMenu(void) const;
	virtual void InitMenu(HMENU hmenu);
	virtual int IdsMenu(void) const;
	virtual wstring SzTip(void) const;
};


/*
 *
 *	CMDLIST
 *
 *	The command list, used for dispatching to command handlers.
 *
 */


class CMDLIST {
private:
	vector<CMD*> vpcmd;
public:
	CMDLIST(void);
	~CMDLIST(void);
	void Add(CMD* pcmd);
	int Execute(int icmd);
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
	lgtTemp
};

enum LGF : int {
	lgfNormal,
	lgfBold,
	lgfItalic,
	lgfBoldItalic
};

struct ATTR
{
	wstring name;
	wstring val;

	ATTR(const wstring& name, const wstring& val) : name(name), val(val) {
	}
};


struct TAG
{
	wstring sz;
	map<wstring, wstring> mpnameval;

	TAG(const wchar_t* sz) : sz(wstring(sz)) {
	}

	TAG(const wstring& sz) : sz(sz) {
	}

	TAG(const wstring sz, const ATTR& attr) : sz(sz)
	{
		mpnameval[attr.name] = attr.val;
	}

	TAG(const wstring& sz, const ATTR rgattr[], int cattr) : sz(sz)
	{
		for (int iattr = 0; iattr < cattr; iattr++)
			mpnameval[rgattr[iattr].name] = rgattr[iattr].val;
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

public:
	HINSTANCE hinst;
	HWND hwnd;
	HACCEL haccel;

	ID3D11Device1* pdevd3;
	ID3D11DeviceContext1* pdcd3;
	ID2D1Device* pdevd2;
	SWCH* pswch;
	ID2D1Bitmap1* pbmpBackBuf;
	DC* pdc;

	HCURSOR hcurArrow;
	HCURSOR hcurMove;
	HCURSOR hcurNo;
	HCURSOR hcurUpDown;
	HCURSOR hcurHand;
	HCURSOR hcurCrossHair;
	HCURSOR hcurUp;
	
	GA* pga;
	UIGA* puiga;
	CMDLIST cmdlist;

public:
	APP(HINSTANCE hinst, int sw);
	~APP(void);

	int MessagePump(void);
	DWORD TmMessage(void);
	POINT PtMessage(void);

	wstring SzLoad(int ids) const;
	wstring SzAppDataPath(void) const;

	int Error(const wstring& szMsg, int mb);
	int Error(const string& szMsg, int mb);

private:
	void CreateRsrc(void);
	void DiscardRsrc(void);
	void CreateRsrcSize(void);
	void DiscardRsrcSize(void);

	void InitCmdList(void);

	void Redraw(void);
	void Redraw(RC rc);

	void OnDestroy(void);
	void OnPaint(void);
	void OnSize(UINT dx, UINT dy);
	bool OnCommand(int idm);
	bool OnMouseMove(int x, int y);
	bool OnLeftDown(int x, int y);
	bool OnLeftUp(int x, int y);
	bool OnMouseWheel(int x, int y, int dwheel);
	bool OnTimer(UINT tid);
	bool OnInitMenu(void);
	bool OnKeyDown(int vk);
	bool OnKeyUp(int vk);

public:
	TID StartTimer(UINT dtm);
	void StopTimer(TID tid);
	void DispatchTimer(TID tid, UINT dtm);

	/*
	 *	Logging. For now, this just hard-coded to delegate to the debug panel,
	 *	but this'll need to change
	 */

	void ClearLog(void) noexcept;
	void InitLog(int depth) noexcept;
	inline bool FDepthLog(LGT lgt, int& depth) noexcept;
	void AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept;
	inline int DepthLog(void) const noexcept;
	inline void SetDepthLog(int depth) noexcept;

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


inline void InitLog(int depth)
{
	papp->InitLog(depth);
}

inline void ClearLog(void)
{
	papp->ClearLog();
}

inline void SetDepthLog(int depth) noexcept
{
	papp->SetDepthLog(depth);
}

inline int DepthLog(void) noexcept
{
	return papp->DepthLog();
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