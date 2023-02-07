/*
 *
 *	app.cpp
 * 
 *	Application-independent windows application stuff. Try to keep this file reuseable
 *	so we can steal it for new apps.
 * 
 */

#include "app.h"


/*
 *
 *	class CO
 * 
 *	Just the simplest app base class that does nothing except initializes COM. Throws
 *	an exception on errors.
 *
 */


CO::CO(void)
{
	int err;
	if ((err = CoInitialize(nullptr)) != S_OK)
		throw err;
}


CO::~CO(void)
{
	CoUninitialize();
}


/*
 *
 *	D2 class
 * 
 *	Simple base application class that simply initializes Direct2D graphics. Throws an exception
 *	on errors.
 * 
 */


D2::D2(void) : pfactd2(nullptr), pfactwic(nullptr), pfactdwr(nullptr)
{
	try {
		int err;
		D2D1_FACTORY_OPTIONS opt;
		memset(&opt, 0, sizeof(opt));
		if ((err = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opt,
				reinterpret_cast<void**>(&pfactd2))) != S_OK)
			throw err;
		if ((err = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
				reinterpret_cast<void**>(&pfactwic))) != S_OK)
			throw err;
		if ((err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pfactdwr),
				reinterpret_cast<IUnknown**>(&pfactdwr))) != S_OK)
			throw err;
	}
	catch (...) {
		Cleanup();
		throw;
	}
}


D2::~D2(void)
{
	Cleanup();
}


void D2::Cleanup(void)
{
	SafeRelease(&pfactdwr);
	SafeRelease(&pfactwic);
	SafeRelease(&pfactd2);
}


/*
 *
 *  VCMD
 *
 *  This is the collection of commands the application handles. We keep them
 *	collected 
 *
 */


VCMD::VCMD(void)
{
}


VCMD::~VCMD(void)
{
	while (size() > 0) {
		CMD* pcmd = back();
		pop_back();
		delete pcmd;
	}
}


/*  VCMD::Add
 *
 *  Adds the given command to the command list. The command list takes
 *  ownership of the command.
 */
void VCMD::Add(CMD* pcmd)
{
	size_t ccmd = size();
	if (pcmd->icmd >= ccmd) {
		resize(pcmd->icmd + 1);
		for (size_t icmd = ccmd; icmd < pcmd->icmd + 1; icmd++)
			(*this)[icmd] = nullptr;
	}
	assert((*this)[pcmd->icmd] == nullptr);
	(*this)[pcmd->icmd] = pcmd;
}


/*  VCMD::Execute
 *
 *  Executes the given command.
 */
int VCMD::Execute(int icmd)
{
	assert(icmd < size());
	assert((*this)[icmd] != nullptr);
	if (!(*this)[icmd]->FEnabled())
		return 1;
	return (*this)[icmd]->Execute();
}


/*  VCMD::InitMenu
 *
 *  Called before the given menu drops something down, so that variable menu text
 *  can be set up.
 */
void VCMD::InitMenu(HMENU hmenu)
{
	for (int icmd = 0; icmd < size(); icmd++) {
		if ((*this)[icmd]) {
			(*this)[icmd]->InitMenu(hmenu);
		}
	}
}


/*  VCMD::SzTip
 *
 *  Returns the tip text for the given command.
 */
wstring VCMD::SzTip(int icmd)
{
	return (*this)[icmd]->SzTip();
}


/*
 *
 *  CMD base class
 *
 *  Base class for all commands.
 *
 */


CMD::CMD(APP& app, size_t icmd) : app(app), icmd(icmd)
{
}


int CMD::Execute(void)
{
	return 0;
}


bool CMD::FEnabled(void) const
{
	return app.FEnableCmds();
}

bool CMD::FChecked(void) const
{
	return false;
}


bool CMD::FCustomSzMenu(void) const
{
	return false;
}


int CMD::IdsMenu(void) const
{
	return 0;
}


wstring CMD::SzMenu(void) const
{
	int ids = IdsMenu();
	return ids ? app.SzLoad(ids) : L"(error)";
}


void CMD::InitMenu(HMENU hmenu)
{
	MENUITEMINFOW mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STATE;
	mi.fState = MFS_UNCHECKED | MFS_ENABLED;
	assert(mi.fState == 0);
	if (!FEnabled())
		mi.fState |= MFS_DISABLED | MF_GRAYED;
	if (FChecked())
		mi.fState |= MFS_CHECKED;
	UINT cmd = (UINT)icmd;
	if (FCustomSzMenu()) {
		mi.fMask |= MIIM_TYPE;
		wchar_t szText[256];
		lstrcpy(szText, SzMenu().c_str());
		mi.dwTypeData = szText;
		::SetMenuItemInfoW(hmenu, cmd, false, &mi);
	}
	else
		::SetMenuItemInfoW(hmenu, cmd, false, &mi);

}


wstring CMD::SzTip(void) const
{
	return L"";
}


/*
 *
 *	ClIPB class
 * 
 *	Implementation of a Windows clipboard class wrapper.
 * 
 */


CLIPB::CLIPB(APP& app) : app(app), fClipOpen(false), hdata(nullptr), hdataGet(nullptr), pdata(nullptr)
{
	Open();
}


CLIPB::~CLIPB(void)
{
	Unlock();
	Free();
	Close();
}


void CLIPB::Open(void)
{
	if (!::OpenClipboard(app.hwnd))
		throw 1;
	fClipOpen = true;
}


void CLIPB::Close(void)
{
	if (!fClipOpen)
		return;
	::CloseClipboard();
	fClipOpen = false;
}


void* CLIPB::PLock(void)
{
	assert(!pdata);
	assert(hdata);
	pdata = ::GlobalLock(hdata);
	if (!pdata)
		throw 1;
	return pdata;
}


void CLIPB::Unlock(void)
{
	if (!pdata)
		return;
	::GlobalUnlock(hdata);
	pdata = nullptr;
}


void CLIPB::Empty(void)
{
	::EmptyClipboard();
}


void* CLIPB::PAlloc(int cb)
{
	hdata = ::GlobalAlloc(GMEM_MOVEABLE, cb);
	if (!hdata)
		throw 1;
	return PLock();
}


void CLIPB::SetData(int cf, void* pv, int cb)
{
	PAlloc(cb);
	assert(pdata);
	memcpy(pdata, pv, cb);
	Unlock();
	assert(hdata);
	::SetClipboardData(cf, hdata);
	Free();
}


void CLIPB::Free(void)
{
	if (!hdata)
		return;
	if (hdataGet) {
		assert(hdata == hdataGet);
	}
	else {
		::GlobalFree(hdata);
		hdata = nullptr;
	}
}


void* CLIPB::PGetData(int cf)
{
	hdataGet = ::GetClipboardData(cf);
	if (!hdataGet)
		throw 1;
	hdata = hdataGet;
	return PLock();
}

