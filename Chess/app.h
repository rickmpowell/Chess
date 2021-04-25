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
	CO(void) { int err;  if (err = CoInitialize(NULL)) throw err; }
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
			if (err = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opt,
				reinterpret_cast<void**>(&pfactd2)))
				throw err;
			if (err = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
				reinterpret_cast<void**>(&pfactwic)))
				throw err;
			if (err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pfactdwr),
				reinterpret_cast<IUnknown**>(&pfactdwr)))
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
protected:
	APP& app;
public:
	CMD(APP& app);
	virtual int Execute(void);
	virtual bool FEnabled(void) const;
	virtual bool FCustomSzMenu(void) const;
	virtual wstring SzMenu(void) const;
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
	APP& app;
	vector<CMD*> mpicmdpcmd;
public:
	CMDLIST(APP& app);
	~CMDLIST(void);
	void Add(int icmd, CMD* pcmd);
	int Execute(int icmd);
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
	class GA* pga;
	CMDLIST cmdlist;

public:
	APP(HINSTANCE hinst, int sw);
	~APP(void);
	int MessagePump(void);
	DWORD TmMessage(void);

private:
	void CreateRsrc(void);
	void DiscardRsrc(void);

	void InitCmdList(void);

	bool FSizeEnv(int dx, int dy);
	void ResizeDc(UINT dx, UINT dy);
	void Redraw(const RCF* prcf);

	void OnPaint(void);
	void OnSize(UINT dx, UINT dy);
	bool OnCommand(int idm);
	bool OnMouseMove(UINT x, UINT y);
	bool OnLeftDown(UINT x, UINT y);
	bool OnLeftUp(UINT x, UINT y);
	bool OnTimer(UINT tid);

	int CmdUndoMove(void);
	int CmdRedoMove(void);

	void CreateTimer(UINT tid, DWORD dtm);
	void DestroyTimer(UINT tid);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam);
};
