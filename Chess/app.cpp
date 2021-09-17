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
	catch (int err) {
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
 *  CMDLIST
 *
 *  This is the collection of commands the application handles.
 *
 */


CMDLIST::CMDLIST(void)
{
}


CMDLIST::~CMDLIST(void)
{
	while (vpcmd.size() > 0) {
		CMD* pcmd = vpcmd.back();
		vpcmd.pop_back();
		delete pcmd;
	}
}


/*  CMDLIST::Add
 *
 *  Adds the given command to the command list. The command list takes
 *  ownership of the command.
 */
void CMDLIST::Add(CMD* pcmd)
{
	int ccmd = (int)vpcmd.size();
	if (pcmd->icmd >= ccmd) {
		vpcmd.resize(pcmd->icmd + 1);
		for (int icmd = ccmd; icmd < pcmd->icmd + 1; icmd++)
			vpcmd[icmd] = nullptr;
	}
	assert(vpcmd[pcmd->icmd] == nullptr);
	vpcmd[pcmd->icmd] = pcmd;
}


/*  CMDLIST::Execute
 *
 *  Executes the given command.
 */
int CMDLIST::Execute(int icmd)
{
	assert(icmd < vpcmd.size());
	assert(vpcmd[icmd] != nullptr);
	return vpcmd[icmd]->Execute();
}


/*  CMDLIST::InitMenu
 *
 *  Called before the given menu drops something down, so that variable menu text
 *  can be set up.
 */
void CMDLIST::InitMenu(HMENU hmenu)
{
	for (size_t icmd = 0; icmd < vpcmd.size(); icmd++)
		if (vpcmd[icmd])
			vpcmd[icmd]->InitMenu(hmenu);
}


/*  CMDLIST::SzTip
 *
 *  Returns the tip text for the given command.
 */
wstring CMDLIST::SzTip(int icmd)
{
	return vpcmd[icmd]->SzTip();
}


/*
 *
 *  CMD base class
 *
 *  Base class for all commands.
 *
 */


CMD::CMD(APP& app, int icmd) : app(app), icmd(icmd)
{
}


int CMD::Execute(void)
{
	return 0;
}


bool CMD::FEnabled(void) const
{
	return true;
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
	if (!FCustomSzMenu())
		return;
	int mf = MF_UNCHECKED | MF_ENABLED;
	::ModifyMenuW(hmenu, icmd, MF_BYCOMMAND | MF_STRING | mf, icmd, SzMenu().c_str());
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

