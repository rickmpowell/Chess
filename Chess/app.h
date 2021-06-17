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

#pragma once
#include "framework.h"


class CO
{
public:
	CO(void) { int err;  if ((err = CoInitialize(NULL)) != S_OK) throw err; }
	~CO(void) { CoUninitialize(); }
};


class D2 : public CO
{
public:
	FACTD2* pfactd2;
	FACTWIC* pfactwic;
	FACTDWR* pfactdwr;
public:
	D2(void) : pfactd2(NULL), pfactwic(NULL), pfactdwr(NULL)
	{
		try {
			int err;
			D2D1_FACTORY_OPTIONS opt;
			memset(&opt, 0, sizeof(opt));
			if ((err = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opt,
				reinterpret_cast<void**>(&pfactd2))) != S_OK)
				throw err;
			if ((err = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
				reinterpret_cast<void**>(&pfactwic))) != S_OK)
				throw err;
			if ((err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pfactdwr),
				reinterpret_cast<IUnknown**>(&pfactdwr))) != S_OK)
				throw err;
		}
		catch (int err) {
			Cleanup();
			throw err;
		}
	}

	~D2(void)
	{
		Cleanup();
	}

	void Cleanup(void)
	{
		SafeRelease(&pfactdwr);
		SafeRelease(&pfactwic);
		SafeRelease(&pfactd2);
	}
};


/*
 *
 *  CMD class
 *
 *  Little class for wrapping together and organiziong top-level
 *  commands
 *
 */


class APP;

class CMD
{
	friend class CMDLIST;
protected:
	APP& app;
	int icmd;
public:
	CMD(APP& app, int icmd);
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
 *	The command list, used for dispatching to handlers
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
 *	APP class
 *
 *	Base application class, which simply initiaizes the app and creates the top-level
 *	window
 *
 */


class APP : public D2
{
	friend class GA;
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
	class GA* pga;
	CMDLIST cmdlist;

public:
	APP(HINSTANCE hinst, int sw);
	~APP(void);
	int MessagePump(void);
	DWORD TmMessage(void);
	wstring SzLoad(int ids) const;
	wstring SzAppDataPath(void) const;

private:
	void CreateRsrc(void);
	void DiscardRsrc(void);
	void CreateRsrcSize(void);
	void DiscardRsrcSize(void);

	void InitCmdList(void);

	void Redraw(void);
	void Redraw(RCF rcf);

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

	void CreateTimer(UINT tid, DWORD dtm);
	void DestroyTimer(UINT tid);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam);
};


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
	CLIPB(APP& app) : app(app), fClipOpen(false), hdata(NULL), hdataGet(NULL), pdata(NULL)
	{
		Open();
	}

	~CLIPB(void)
	{
		Unlock();
		Free();
		Close();
	}

	void Open(void)
	{
		if (!::OpenClipboard(app.hwnd))
			throw 1;
		fClipOpen = true;
	}

	void Close(void)
	{
		if (!fClipOpen)
			return;
		::CloseClipboard();
		fClipOpen = false;
	}

	void* PLock(void)
	{
		assert(pdata == NULL);
		assert(hdata != NULL);
		pdata = ::GlobalLock(hdata);
		if (pdata == NULL)
			throw 1;
		return pdata;
	}

	void Unlock(void)
	{
		if (!pdata)
			return;
		::GlobalUnlock(hdata);
		pdata = NULL;
	}

	void Empty(void)
	{
		::EmptyClipboard();
	}

	void* PAlloc(int cb)
	{
		hdata = ::GlobalAlloc(GMEM_MOVEABLE, cb);
		if (hdata == NULL)
			throw 1;
		return PLock();
	}

	void SetData(int cf, void* pv, int cb)
	{
		PAlloc(cb);
		assert(pdata != NULL);
		memcpy(pdata, pv, cb);
		Unlock();
		assert(hdata);
		::SetClipboardData(cf, hdata);
		Free();
	}

	void Free(void)
	{
		if (hdata == NULL)
			return;
		if (hdataGet) {
			assert(hdata == hdataGet);
		}
		else {
			::GlobalFree(hdata);
			hdata = NULL;
		}
	}

	void* PGetData(int cf)
	{
		hdataGet = ::GetClipboardData(cf);
		if (hdataGet == NULL)
			throw 1;
		hdata = hdataGet;
		return PLock();
	}
		
};

