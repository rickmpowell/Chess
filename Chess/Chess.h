#pragma once

#include "framework.h"
#include "resource.h"




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
	CO(void) { int err;  if (err = CoInitialize(NULL)) throw err; }
	~CO(void) { CoUninitialize(); }
};


class D2 : public CO
{
protected:
	ID2D1Factory* pfactd2d;
	IWICImagingFactory* pfactwic;
	IDWriteFactory* pfactdwr;
public:
	D2(void) : pfactd2d(NULL), pfactwic(NULL), pfactdwr(NULL)
	{
		try {
			int err;
			if (err = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pfactd2d))
				throw err;
			if (err = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&pfactwic)))
				throw err;
			if (err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pfactdwr), reinterpret_cast<IUnknown**>(&pfactdwr)))
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
		SafeRelease(&pfactd2d);
	}
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
	ID2D1HwndRenderTarget* prth;

	HCURSOR hcurArrow;
	HCURSOR hcurMove;
	HCURSOR hcurNo;
	class GA* pga;

public:
	APP(HINSTANCE hinst, int sw);
	~APP(void);
	int MessagePump(void);
	DWORD TmMessage(void);

private:
	void CreateRsrc(void);
	void DiscardRsrc(void);
	bool FSizeEnv(int dx, int dy);
	void Redraw(const RCF* prcf);

	void OnPaint(void);
	void OnSize(UINT dx, UINT dy);
	bool OnCommand(int idm);
	bool OnMouseMove(UINT x, UINT y);
	bool OnLeftDown(UINT x, UINT y);
	bool OnLeftUp(UINT x, UINT y);
	bool OnTimer(UINT tid);

	int CmdNewGame(void);
	int CmdTest(void);

	void CreateTimer(UINT tid, DWORD dtm);
	void DestroyTimer(UINT tid);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam);
};


#include "ga.h"