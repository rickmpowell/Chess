/*
 *
 *  main.cpp
 * 
 *  The main entry point for the app and basic interface to the OS
 * 
 */

#include "app.h"
#include "uiga.h"
#include "ga.h"
#include "dlg.h"


APP* papp;


/*  wWinMain
 *
 *  The main entry point for Windows applications. 
 */
int APIENTRY wWinMain(_In_ HINSTANCE hinst, 
                      _In_opt_ HINSTANCE hinstPrev, 
                      _In_ LPWSTR szCmdLine, 
                      _In_ int sw)
{
    try {
        papp = new APP(hinst, sw);
        int err;
        if (!::GetStdHandle(STD_INPUT_HANDLE)) 
            err = papp->MessagePump();
        else {
            UCI uci(papp->puiga);
            err = uci.ConsolePump();
        }
        APP::DiscardRsrcStatic();
        delete papp;
        return err;
    }
    catch (int err) {
        ::MessageBoxW(nullptr,
                      L"Could not initialize application", L"Fatal Error",
                      MB_OK);
        return err;
    }

}


/*
 *
 *  APP implementation
 *
 *  Our top-level application container, including the top-level window.
 *
 */


/*  APP::APP
 *
 *  Initializes the Windows application. Creates the main application container 
 *  window. This class 
 * 
 *  Throws an exception if something fails.
 */
APP::APP(HINSTANCE hinst, int sw) : hinst(hinst), hwnd(nullptr), haccel(nullptr), 
        pdevd3(nullptr), pdcd3(nullptr), pdevd2(nullptr), pswch(nullptr), pbmpBackBuf(nullptr), pdc(nullptr),
        puiga(nullptr)
{
    hcurArrow = ::LoadCursor(nullptr, IDC_ARROW);
    hcurMove = ::LoadCursor(nullptr, IDC_SIZEALL);
    hcurNo = ::LoadCursor(nullptr, IDC_NO);
    hcurUpDown = ::LoadCursor(nullptr, IDC_SIZENS);
    hcurHand = ::LoadCursor(nullptr, IDC_HAND);
    hcurCrossHair = ::LoadCursor(nullptr, IDC_CROSS);
    hcurUp = ::LoadCursor(nullptr, IDC_UPARROW);
    assert(hcurArrow);  /* these cursors are system cursors and loading them can't fail */
    assert(hcurMove);
    assert(hcurNo);
    assert(hcurUpDown);
    assert(hcurHand);
    assert(hcurCrossHair);

    const TCHAR szWndClassMain[] = L"ChessMain";

    /* register the window class */

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = APP::WndProc;
    wcex.cbWndExtra = sizeof(this);
    wcex.hInstance = hinst;
    wcex.hIcon = ::LoadIcon(hinst, MAKEINTRESOURCE(idiApp));
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(idmApp);
    wcex.lpszClassName = szWndClassMain;
    wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(idiSmall));
    if (!::RegisterClassEx(&wcex))
        throw 1;

    /* load keyboard interface */

    if (!(haccel = ::LoadAccelerators(hinst, MAKEINTRESOURCE(idaApp))))
        throw 1;

    InitCmdList();

    pga = new GA();
    pga->SetPl(cpcBlack, ainfopl.PplFactory(*pga, 0));
    pga->SetPl(cpcWhite, ainfopl.PplFactory(*pga, 1));
    pga->InitGameStart(nullptr);
    
    /* create the main window */

    sw = SW_MAXIMIZE;
    TCHAR szTitle[100];
    if (!::LoadString(hinst, idsApp, szTitle, CArray(szTitle)))
        throw 1;
    hwnd = ::CreateWindowW(szWndClassMain, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hinst, this);
    if (!hwnd)
        throw 1;    // BUG: cleanup haccel
 
    FCreateRsrc();
    APP::FCreateRsrcStatic(pdc, pfactd2, pfactdwr, pfactwic);
 
    puiga = new UIGA(*this, *pga);
    pga->SetUiga(puiga);
    puiga->InitGameStart(nullptr);
    puiga->uipvt.Show(false);
    puiga->uipcp.Show(false);
    puiga->uidt.Show(false);

    ::ShowWindow(hwnd, sw);

}


APP::~APP(void)
{
    if (puiga)
        delete puiga;
    puiga = nullptr;
    if (pga)
        delete pga;
    pga = nullptr;
    DiscardRsrc();
}


/*  APP::MessagePump
 *
 *  The main message loop for a Windows application. Handles a single
 *  accelerator table, and exits when the app posts a WM_QUIT message.
 */
int APP::MessagePump(void)
{
    MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_TIMER) {
            DispatchTimer(msg.wParam, msg.time);
            continue;
        }
        if (::TranslateAccelerator(msg.hwnd, haccel, &msg))
            continue;
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}


DWORD APP::MsecMessage(void)
{
    return ::GetMessageTime();
}


POINT APP::PtMessage(void)
{
    DWORD dw = ::GetMessagePos();
    POINT pt = { GET_X_LPARAM(dw), GET_Y_LPARAM(dw) };
    ::ScreenToClient(hwnd, &pt);
    return pt;
}


int APP::Error(const wstring& szMsg, int mb)
{
    return ::MessageBoxW(hwnd, szMsg.c_str(), L"Error", mb);
}

int APP::Error(const string& szMsg, int mb)
{
    return Error(WszWidenSz(szMsg), mb);
}


bool APP::FCreateRsrcStatic(DC* pdc, FACTD2* pfactd2, FACTDWR* pfactdwr, FACTWIC* pfactwic)
{
    bool fChange = false;
    fChange |= UI::FCreateRsrcStatic(pdc, pfactd2, pfactdwr, pfactwic);
    fChange |= UICLOCK::FCreateRsrcStatic(pdc, pfactd2, pfactdwr, pfactwic);
    if (pbmppc == nullptr) {
        pbmppc = new BMPPC;
        fChange = true;
    }
    return fChange;
}


void APP::DiscardRsrcStatic(void)
{
    UI::DiscardRsrcStatic();
    UICLOCK::DiscardRsrcStatic();
    if (pbmppc) {
        delete pbmppc;
        pbmppc = nullptr;
    }
}
 


/*  APP::FCreateRsrc
 *
 *  Initializes Direct2D for all our drawing
 */
bool APP::FCreateRsrc(void)
{
    if (pdc)
        return false;

    D3D_FEATURE_LEVEL afld3[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, 
        D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, 
        D3D_FEATURE_LEVEL_9_1
    };
    ID3D11Device* pdevd3T = nullptr;
    ID3D11DeviceContext* pdcd3T = nullptr;
    D3D_FEATURE_LEVEL flRet = D3D_FEATURE_LEVEL_1_0_CORE;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, 
                        afld3, CArray(afld3), D3D11_SDK_VERSION,
                        &pdevd3T, &flRet, &pdcd3T);
    if (pdevd3T->QueryInterface(__uuidof(ID3D11Device1), (void**)&pdevd3) != S_OK)
        throw 1;
    pdcd3T->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&pdcd3);
    IDXGIDevice* pdevDxgi = nullptr;
    if (pdevd3->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdevDxgi) != S_OK)
        throw 1;
    pfactd2->CreateDevice(pdevDxgi, &pdevd2);
    pdevd2->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pdc);
    SafeRelease(&pdevDxgi);
    SafeRelease(&pdevd3T);
    SafeRelease(&pdcd3T);

    FCreateRsrcSize();
    return true;
}


bool APP::FCreateRsrcSize(void)
{
    if (pswch)
        return false;

    IDXGIDevice* pdevDxgi = nullptr;
    if (pdevd3->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdevDxgi) != S_OK)
        throw 1;

    IDXGIAdapter* padaptDxgi = nullptr;
    pdevDxgi->GetAdapter(&padaptDxgi);
    IDXGIFactory2* pfactDxgi = nullptr;
    if (padaptDxgi->GetParent(IID_PPV_ARGS(&pfactDxgi)) != S_OK)
        throw 1;

    DXGI_SWAP_CHAIN_DESC1 swchd = { 0 };
    swchd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swchd.SampleDesc.Count = 1;
    swchd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swchd.BufferCount = 2;
    swchd.Scaling = DXGI_SCALING_STRETCH;
    swchd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (pfactDxgi->CreateSwapChainForHwnd(pdevd3, hwnd, &swchd, nullptr, nullptr, &pswch) != S_OK)
        throw 1;

    IDXGISurface* psurfDxgi;
    if (pswch->GetBuffer(0, IID_PPV_ARGS(&psurfDxgi)) != S_OK)
        throw 1;

    float dxy = (float)GetDpiForWindow(hwnd);
    D2D1_BITMAP_PROPERTIES1 propBmp;
    propBmp = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dxy, dxy);
    pdc->CreateBitmapFromDxgiSurface(psurfDxgi, &propBmp, &pbmpBackBuf);
    pdc->SetTarget(pbmpBackBuf);
    
    SafeRelease(&psurfDxgi);
    SafeRelease(&pfactDxgi);
    SafeRelease(&padaptDxgi);
    SafeRelease(&pdevDxgi);

    return true;
}


void APP::DiscardRsrcSize(void)
{
    SafeRelease(&pswch);
    SafeRelease(&pbmpBackBuf);
}


void APP::DiscardRsrc(void)
{
    DiscardRsrcSize();
    SafeRelease(&pdc);
    SafeRelease(&pdevd3);
    SafeRelease(&pdcd3);
    SafeRelease(&pdevd2);
}


wstring APP::SzLoad(int ids) const
{
    wchar_t sz[1024];
    ::LoadString(hinst, ids, sz, CArray(sz));
    return wstring(sz);
}


/*  APP::SzAppDataPath
 *
 *  Returns a path to the app data directory, which is where we save log files
 *  and the like. Ensures that the directory exists.
 * 
 *  The path will be something like C:\Users\[Name]\AppData\Local\SQ\Chess
 */
wstring APP::SzAppDataPath(void)
{
    wchar_t szPath[1024];
    ::SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, szPath);
    wstring szAppData(szPath);
    szAppData += L"\\" L"SQ";
    EnsureDirectory(szAppData);
    szAppData += L"\\" L"Chess";
    EnsureDirectory(szAppData);
    return szAppData;
}


void APP::EnsureDirectory(const wstring& szDir)
{
    ::CreateDirectoryW(szDir.c_str(), NULL);
}


/*  APP::Redraw
 *
 *  Force a redraw of the application. 
 */
void APP::Redraw(void)
{
    ::InvalidateRect(hwnd, nullptr, true);
    ::UpdateWindow(hwnd);
}


void APP::Redraw(RC rc)
{
    RECT rect;
    ::SetRect(&rect, (int)floor(rc.left), (int)floor(rc.top), (int)ceil(rc.right), (int)ceil(rc.bottom));
    ::InvalidateRect(hwnd, &rect, true);
    ::UpdateWindow(hwnd);
}


/*  APP::SetCursor
 *
 *  Just a logical place to put the cursor API. Should probably rethink this and put it somewhere
 *  else.
 */

void APP::SetCursor(HCURSOR hcrs)
{
    ::SetCursor(hcrs);
}


/*  APP::StartTimer
 *
 *  Creates a timer that goes every dmsec milliseconds.
 */
TID APP::StartTimer(DWORD dmsec)
{
    return ::SetTimer(nullptr, 0, dmsec, nullptr);
}


/*  APP::StopTimer
 *
 *  Kills the timer
 */
void APP::StopTimer(TID tid)
{
    ::KillTimer(nullptr, tid);
}


void APP::DispatchTimer(TID tid, DWORD msec)
{
    puiga->DispatchTimer(tid, msec);
}


/*  APP::OnDestroy
 *
 *  Handles the window destroy notification. Since we have modal loops and the
 *  main window can be destroyed in them, we need to invalidate all our drawing
 *  operations here so we can gracefully shut down
 */
void APP::OnDestroy(void)
{
    if (puiga)
        puiga->ShowAll(false);
    DiscardRsrc();
    hwnd = NULL;
}


/*  APP::OnSize
 *
 *  Handles the window size notification for the top-level window. New size
 *  is dx and dy, in pixels.
 */
void APP::OnSize(UINT dx, UINT dy)
{
    DiscardRsrcSize();
    FCreateRsrcSize();
    if (puiga)
        puiga->Resize(PT((float)dx, (float)dy));
}


/*  APP::OnPaint
 *
 *  Handles painting updates on the window.
 */
void APP::OnPaint(void)
{
    PAINTSTRUCT ps;
    ::BeginPaint(hwnd, &ps);

    if (puiga) {
        puiga->BeginDraw();
        RC rc((float)ps.rcPaint.left, (float)ps.rcPaint.top,
            (float)ps.rcPaint.right, (float)ps.rcPaint.bottom);
        puiga->RedrawWithChildren(rc, true);
        puiga->RedrawCursor(rc);
        puiga->EndDraw();
    }

    ::EndPaint(hwnd, &ps);
}


bool APP::OnMouseMove(UINT mk, int x, int y)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui == nullptr)
        return true;

    if (puiga->puiDrag) {
        if (!(mk & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))) 
            pui->NoButtonDrag(pt);
        else {
            if (mk & MK_LBUTTON)
                pui->LeftDrag(pt);
            if (mk & MK_RBUTTON)
                pui->RightDrag(pt);
        }
        return true;
    }

    /* hovering enter move and exit, hovering only happens if no buttons are down */

    if (mk & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))
        return true;

    if (puiga->puiHover == pui)
        pui->MouseHover(pt, mhtMove);
    else {
        if (puiga->puiHover) {
            PT ptExit = puiga->puiHover->PtLocalFromGlobal(PT((float)x, (float)y));
            puiga->puiHover->MouseHover(ptExit, mhtExit);
        }
        pui->MouseHover(pt, mhtEnter);
        puiga->SetHover(pui);
    }

    return true;
}


bool APP::OnLeftDown(int x, int y)
{
    UI* pui = puiga->PuiHitTest(&mciLeftDown.pt, x, y);
    if (pui) {
        mciLeftDown.tm = ::GetMessageTime();
        pui->StartLeftDrag(mciLeftDown.pt);
    }
    return true;
}


/*  APP::OnLeftUp
 *
 *  Left mouse button up handler
 */
bool APP::OnLeftUp(int x, int y)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->EndLeftDrag(pt, MCI(pt, ::GetMessageTime()).FIsSingleClick(mciLeftDown));
    return true;
}

bool APP::OnRightDown(int x, int y)
{
    UI* pui = puiga->PuiHitTest(&mciRightDown.pt, x, y);
    if (pui) {
        mciRightDown.tm = ::GetMessageTime();
        pui->StartRightDrag(mciRightDown.pt);
    }
    return true;
}


/*  APP::OnRightUp
 *
 *  Right mouse button up handler
 */
bool APP::OnRightUp(int x, int y)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->EndRightDrag(pt, MCI(pt, ::GetMessageTime()).FIsSingleClick(mciRightDown));
    return true;
}


bool APP::OnMouseWheel(int x, int y, int dwheel)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->ScrollWheel(pt, dwheel);
    return true;
}


bool APP::OnKeyDown(int vk)
{
    UI* pui = puiga->PuiFocus();
    if (pui)
        pui->KeyDown(vk);
    return true;
}


bool APP::OnKeyUp(int vk)
{
    UI* pui = puiga->PuiFocus();
    if (pui)
        pui->KeyUp(vk);
    return true;
}


/*  APP::OnTimer
 *
 *  Handles the WM_TIMER message from the wndproc. Returns false if
 *  the default processing should still happen.
 * 
 *  (I don't think this gets called currently, because we only use nullptr
 *  window timers which are not dispatched to WndProcs).
 */
bool APP::OnTimer(UINT tid)
{
    DispatchTimer(tid, MsecMessage());
    return true;
}


/*
 *  Logging stubs
 */


void APP::ClearLog(void) noexcept
{
    puiga->uidb.ClearLog();
}

void APP::InitLog(void) noexcept
{
    puiga->uidb.InitLog();
}

bool APP::FDepthLog(LGT lgt, int& depth) noexcept
{
    return puiga->uidb.FDepthLog(lgt, depth);
}

void APP::AddLog(LGT lgt, LGF lgf, int lgd, const TAG& tag, const wstring& szData) noexcept
{
    return puiga->uidb.AddLog(lgt, lgf, lgd, tag, szData);
}

int APP::LgdCur(void) const noexcept
{
    return puiga->uidb.LgdCur();
}

void APP::SetLgdShow(int lgd) noexcept
{
    puiga->uidb.SetLgdShow(lgd);
}

int APP::LgdShow(void) const noexcept
{
    return puiga->uidb.LgdShow();
}


/*  
 *
 *  CMDABOUT command
 *
 *  Just an about dialog box
 *
 */


class CMDABOUT : public CMD
{
public:
    CMDABOUT(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void) 
    {
        DLG dlg(app, iddAbout);
        return dlg.Run();
    }
};


/*
 *
 *  CMDNEWGAME command
 * 
 *  Starts a new game
 * 
 */


class CMDNEWGAME : public CMD
{
public:
    CMDNEWGAME(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->InitGameStart(nullptr);
        app.puiga->Relayout();
        app.puiga->Redraw();
        return 1;
    }
};


/*
 *
 *  CMDPLAY command
 * 
 *  Starts playing a game with the current game state set up.
 * 
 */


class CMDPLAY : public CMD
{
public:
    CMDPLAY(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->Play(mvuNil, spmvAnimate);
        return 1;
    }
};


/*
 *
 *  CMDUNDOMOVE command
 *
 *  Undoes the last move on the board.
 *
 */


class CMDUNDOMOVE : public CMD
{
public:
    CMDUNDOMOVE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->UndoMv(spmvAnimate);
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipUndoMove;
    }
};


/*
 *
 *  CMDREDOMOVE command
 *
 *  Redoes the last move on the board.
 *
 */


class CMDREDOMOVE : public CMD
{
public:
    CMDREDOMOVE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->RedoMv(spmvAnimate);
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipRedoMove;
    }
};


/*
 *
 *  CMDROTATEBOARD command
 * 
 *  Command to rotate the orientation of the screen board.
 */


class CMDROTATEBOARD : public CMD
{
public:
    CMDROTATEBOARD(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->uibd.FlipBoard(~app.puiga->uibd.cpcPointOfView);
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipRotateBoard;
    }
};


/*
 *
 *  CMDRESIGN command
 * 
 *  Command for the player with the current move to resign
 * 
 */


class CMDRESIGN : public CMD
{
public:
    CMDRESIGN(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipResign;
    }
};


/*
 *
 *  CMDOFFERDRAW
 * 
 *  Command for the player with the move to offer a draw to the other player.
 * 
 */


class CMDOFFERDRAW : public CMD
{
public:
    CMDOFFERDRAW(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipOfferDraw;
    }
};


/*
 *
 *  CMDTEST command
 *
 *  Runs the test.
 *
 */


class CMDTEST : public CMD
{
public:
    CMDTEST(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->Test();
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return app.puiga->fInTest ? idsTipStopTest : idsTipTest;
    }

    virtual int IdsMenu(void) const
    {
        return app.puiga->fInTest ? idsMenuStopTest : idsMenuTest;
    }

    virtual bool FEnabled(void) const
    {
        return CMD::FEnabled() || app.puiga->fInTest;
    }

};


/*
 *
 *  CMDPERFTDIVIDE
 * 
 *  Runs the perft test on the current board position. Output is sent to 
 *  debug panel.
 * 
 */


enum TPERFT : int {
    tperftNormal = 0,
    tperftBulk = 1,
    tperftDivide = 2
};

wstring to_wstring(TPERFT tperft)
{
    switch (tperft) {
    case tperftBulk: return L"Bulk";
    case tperftDivide: return L"Divide";
    default: return L"Normal";
    }
}


struct DDPERFT {
    int tperft;
    int d;
};
static DDPERFT ddperft = { tperftBulk, 6 };


class DLGPERFT : public DLG
{
public:
    DDPERFT ddperft;
    DCTLINT dctlDepth;
    DCTLCOMBO dctlTperft;

    DLGPERFT(APP& app, const DDPERFT& ddperft) : DLG(app, iddTestPerft),
            ddperft(ddperft),
            dctlDepth(*this, idePerftDepth, this->ddperft.d, 2, 30, L"Depth must be an integer between 2 and 30"),
            dctlTperft(*this, idcPerftType, this->ddperft.tperft)
    {
    }

    virtual void Init(void)
    {
        dctlTperft.Add(to_wstring(tperftNormal));
        dctlTperft.Add(to_wstring(tperftBulk));
        dctlTperft.Add(to_wstring(tperftDivide));
        DLG::Init();
    }
};


class CMDPERFTDIVIDE : public CMD
{
public:
    bool fPrompt;

public:
    CMDPERFTDIVIDE(APP& app, int icmd, bool fPrompt) : CMD(app, icmd), fPrompt(fPrompt) {}

    virtual int Execute(void)
    {
        if (fPrompt) {
            DLGPERFT dlgperft(app, ddperft);
            if (dlgperft.Run()!= IDOK)
                return 0;
            ddperft = dlgperft.ddperft;
        }

        LogOpen(L"perft " + to_wstring((TPERFT)ddperft.tperft), to_wstring(ddperft.d), lgfBold);
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();
        uint64_t cmv;
        switch (ddperft.tperft) {
        case tperftDivide:
            cmv = app.puiga->CmvPerftDivide(ddperft.d);
            break;
        case tperftBulk:
            cmv = app.puiga->ga.CmvPerftBulk(ddperft.d);
            break;
        default:
            cmv = app.puiga->ga.CmvPerft(ddperft.d);
            break;
        }
        time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
        duration dtp = tpEnd - tpStart;
        microseconds us = duration_cast<microseconds>(dtp);
        float sp = (float)us.count() / 1000.0f;
        LogData(L"Time: " + to_wstring((int)round(sp)) + L" ms");
        sp = 1000.0f * (float)cmv / (float)us.count();
        LogData(L"Speed: " + to_wstring((int)round(sp)) + L" moves/ms");
        LogClose(L"perft", to_wstring(cmv), lgfBold);
        return 1;
    }

    virtual wstring SzMenu(void) const
    {
        return wstring(L"perft ") + to_wstring(ddperft.d) + L"\tCtrl+P";
    }
};


/*  
 *  
 *  CMDAISPEEDTEST
 * 
 */
class CMDAISPEEDTEST : public CMD
{
public:
    CMDAISPEEDTEST(APP& app, int icmd) : CMD(app, icmd) {}

    virtual int Execute(void)
    {
        /* create standard players with depth and initialize the game */

        for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc) {
            PL* ppl = app.puiga->ga.PplFromCpc(cpc);
            if (ppl->FHasLevel())
                ppl->SetLevel(10);
            ppl->SetTtm(ttmConstDepth);
            ppl->SetFecoRandom(0);
        }

        /* start a new untimed game */

        app.puiga->InitGameEpd("5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - bm Rg3; id \"WAC.003\";", nullptr);
        app.puiga->ga.prule->SetGameTime(cpcWhite, 0);
        app.puiga->ga.prule->SetGameTime(cpcBlack, 0);
        app.puiga->uiml.ShowClocks(false);
        app.puiga->Relayout();
        app.puiga->Redraw();

        ClearLog();
        LogOpen(L"AI Speed Test", L"", lgfBold);
        int lgdSav = LgdShow();
        SetLgdShow(2);

        app.puiga->StartGame(spmvFast);
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();

        for (int imv = 0; imv < 10 && !app.puiga->ga.bdg.FGsGameOver(); imv++) {
            SPMV spmv = spmvFast;
            MVE mve = app.puiga->ga.PplToMove()->MveGetNext(spmv);
            if (mve.fIsNil())
                break;
            app.puiga->MakeMv(mve, spmvFast);
        }

        time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();

        duration dtp = tpEnd - tpStart;
        microseconds us = duration_cast<microseconds>(dtp);
        float sp = (float)us.count() / 1000.0f;

        SetLgdShow(lgdSav);
        LogData(L"Time: " + to_wstring((int)round(sp)) + L" ms");
        LogClose(L"AI Speed Test", L"", lgfBold);

        return 1;
    }
};


/*
 *
 *  CMDAIBREAK
 * 
 *  Debugging aid for tracking down bugs that only happen after a sequence
 *  of moves during AI move search.
 * 
 */


/* 
 *  Dialog box data
 */

class DDAIBREAK
{
public:
    vector<MV> vmv;
    int cpc;
    int cRepeat;
    DDAIBREAK() : cpc(cpcWhite), cRepeat(1) { }
};
static DDAIBREAK ddaibreak;

/*
 *  Move list edit control, space separated list of moves
 */

class DCTLMOVELIST : public DCTLTEXT
{
    vector<MV>& vmvVal;
    wstring szDummy;

public:
    DCTLMOVELIST(DLG& dlg, int id, vector<MV>& vmvInit) : DCTLTEXT(dlg, id, szDummy), vmvVal(vmvInit)
    {
    }

    virtual wstring SzDecode(void)
    {
        wstring sz;
        for (MV mv : vmvVal)
            sz += to_wstring(mv) + L" ";
        return sz;
    }

    virtual bool FValidate(void)
    {
        wstring sz = SzRaw();
        vmvVal.clear();
        MV mv;
        for (int ich = 0; FParseMv(sz, ich, mv); ) {
            if (mv.fIsNil())
                return true;
            vmvVal.push_back(mv);
        }
        Error(SzError());
        return false;
    }

    bool FParseMv(const wstring& sz, int& ich, MV& mv)
    {
        mv = mvNil;
        /* skip whitespace */
        while (ich < sz.size() && isspace(sz[ich]))
            ich++;
        if (sz[ich] == L'\0')
            return true;
        SQ sqFrom, sqTo;
        if (!FParseSq(sz, ich, sqFrom) || !FParseSq(sz, ich, sqTo))
            return false;
        mv = MV(sqFrom, sqTo);
        if (sz[ich] == L'=') {
            switch (tolower(sz[++ich])) {
            case 'q': mv.SetApcPromote(apcQueen); break;
            case 'r': mv.SetApcPromote(apcRook); break;
            case 'b': mv.SetApcPromote(apcBishop); break;
            case 'n': mv.SetApcPromote(apcKnight); break;
            default:
                return false;
            }
        }
        return true;
    }

    bool FParseSq(const wstring& sz, int& ich, SQ& sq)
    {
        wchar_t ch;
        if (!in_range(ch = tolower(sz[ich++]), L'a', L'h'))
            return false;
        int file = ch - L'a';
        if (!in_range(ch = sz[ich++], L'1', L'8'))
            return false;
        int rank = ch - '1';
        sq = SQ(rank, file);
        return true;
    }

    virtual wstring SzError(void)
    {
        return L"Move list should be from/to squares (e.g., a2a3) separated by spaces";
    }
};

/*
 *  dialog box
 */

class DLGAIBREAK : public DLG
{
public:
    DDAIBREAK ddaibreak;
    DCTLINT dctlRepeat;
    DCTLCOMBO dctlPlayer;
    DCTLMOVELIST dctlMoveList;
    DCTL dctlInstructions;
    HFONT hfontInstructions;

    DLGAIBREAK(APP& app, const DDAIBREAK& ddaibreak) : DLG(app, iddAIBreak),
        ddaibreak(ddaibreak),
        dctlRepeat(*this, ideAIBreakRepeatCount, 
                   this->ddaibreak.cRepeat, 1, 20, L"Repeat count must be an integer between 1 and 20"),
        dctlPlayer(*this, idcAIBreakPlayer, this->ddaibreak.cpc),
        dctlMoveList(*this, ideAIBreakMoveSequence, this->ddaibreak.vmv),
        dctlInstructions(*this, idtAIBreakInst),
        hfontInstructions(NULL)
    { 
    }

    virtual void Init(void)
    {
        dctlInstructions.SetFont(L"Arial", -12);
        for (CPC cpc = cpcWhite; cpc <= cpcBlack; ++cpc)
            dctlPlayer.Add(to_wstring(cpc) + L" (" + app.puiga->ga.PplFromCpc(cpc)->SzName() + L")");
        DLG::Init();
    }
};

/*
 *  The command
 */

class CMDAIBREAK : public CMD
{
public:
    CMDAIBREAK(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        DLGAIBREAK dlgaibreak(app, ddaibreak);
        if (dlgaibreak.Run() == IDOK) {
            ddaibreak = dlgaibreak.ddaibreak;
            app.puiga->ga.PplFromCpc((CPC)ddaibreak.cpc)->SetAIBreak(ddaibreak.vmv);
        }
        return 1;
    }
};


/*
 *
 *  CMDSHOWPIECEVALUES
 * 
 *  Commannd to show a visual respresentation of the piece value tables
 *  in the PLAI classes.
 *
 */


class CMDSHOWPIECEVALUES : public CMD
{
public:
    CMDSHOWPIECEVALUES(APP& app, int icmd) : CMD(app, icmd) {}

    virtual int Execute(void)
    {
        app.puiga->uipvt.Show(!app.puiga->uipvt.FVisible());
        return 1;
    }

    virtual int IdsMenu(void) const
    {
        return app.puiga->uipvt.FVisible() ? idsHidePieceValues: idsShowPieceValues;
    }
};


/*
 *
 *  CMDSAVEPGN
 * 
 *  Saves the current board state as a PGN file
 * 
 */


class CMDSAVEPGN : public CMD
{
public:
    CMDSAVEPGN(APP& app, int icmd) : CMD(app, icmd) {}

    virtual int Execute(void)
    {
        wchar_t szFileName[1024] = L"game.pgn";
        OPENFILENAME ofn = { 0 };

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = app.hwnd;
        ofn.lpstrFilter = L"PGN Files\0*.pgn\0\0";
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = CArray(szFileName);
        ofn.lpstrTitle = L"Save Game";
        ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"pgn";
        if (::GetSaveFileName(&ofn))
            app.pga->SavePGNFile(szFileName);

        return 1;
    }
};


/*
 *
 *  CMDOPENPGN
 * 
 *  Command to open a PGN file to read/analyze an already played agame
 * 
 */


class CMDOPENPGN : public CMD
{
public:
    CMDOPENPGN(APP& app, int icmd) : CMD(app, icmd) {}
    
    virtual int Execute(void)
    {
        wchar_t szFileName[1024] = { 0 };
        OPENFILENAME ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = app.hwnd;
        ofn.lpstrFilter = L"PGN Files\0*.pgn\0\0";
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = CArray(szFileName);
        ofn.lpstrTitle = L"Open Game";
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"pgn";

        if (!::GetOpenFileName(&ofn))
            return 1;

        app.puiga->ga.OpenPGNFile(szFileName);
        app.puiga->uiml.UpdateContSize();
        app.puiga->uiml.SetSel(app.puiga->ga.bdg.vmveGame.size() - 1, spmvHidden);
        app.puiga->uiml.FMakeVis(app.puiga->ga.bdg.imveCurLast);
        app.puiga->Redraw();

        return 1;
    }
};


/*
 *
 *  CMDCOPY
 * 
 *  The copy to clipboard command. Puts a copy of the PGN representation of the
 *  board in the clipboard as a text item
 * 
 */


class CMDCOPY : public CMD
{
public:
    CMDCOPY(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        ostringstream os;
        app.pga->Serialize(os);
        return RenderSz(os.str());
    }

    int RenderSz(const string& sz)
    {
        try {
            CLIPB clipb(app);
            clipb.Empty();
            clipb.SetData(CF_TEXT, (void*)sz.c_str(), (int)sz.size() + 1);
        }
        catch (int err) {
            app.Error(L"Rendering clipboard failed.", MB_OK);
            return err;
        }
        return 1;
    }
};


/*
 *
 *  CMDCOPYFEN
 * 
 *  Copies a FEN string to the clipboard.
 * 
 */


class CMDCOPYFEN : public CMDCOPY
{
public:
    CMDCOPYFEN(APP& app, int icmd) : CMDCOPY(app, icmd) { }

    virtual int Execute(void)
    {
        return RenderSz(app.pga->bdg.SzFEN());
    }
};


/*
 *
 *  CMDPASTE
 *
 *  The paste from clipboard command. Pulls a text item from the clipboard. If it's
 *  a PGN file, it sets up the board with it, along with the move list. If it's a
 *  FEN string, just sets up the board.
 *
 */


class CMDPASTE : public CMD
{
public:
    CMDPASTE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        if (!::IsClipboardFormatAvailable(CF_TEXT))
            return 0;
        try {
            CLIPB clipb(app);
            istringstream is;
            ISTKPGN istkpgn(is);
            char* sz = (char*)clipb.PGetData(CF_TEXT);
            if (app.pga->FIsPgnData(sz)) {
                is.str(sz);
                PROCPGNGA procpgngaSav(*app.pga, new PROCPGNPASTE(*app.pga));
                app.pga->Deserialize(istkpgn);
            }
            else {
                app.puiga->InitGameEpd(sz, nullptr);
                app.puiga->Redraw();
            }
        }
        catch (exception& ex) {
            app.Error(ex.what(), MB_OK);
        }
        return 1;
    }
};


ERR PROCPGNPASTE::ProcessTag(int tkpgn, const string& szValue)
{
    return PROCPGNOPEN::ProcessTag(tkpgn, szValue);
}

ERR PROCPGNPASTE::ProcessMv(MVE mve)
{
    return PROCPGNOPEN::ProcessMv(mve);
}


/*
 *
 *  CMDMAKENULLMOVE
 * 
 *  Makes the null move on the current board..
 *
 */


class CMDMAKENULLMOVE : public CMD
{
public:
    CMDMAKENULLMOVE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        GA& ga = *app.pga;
        if (ga.bdg.FGsPlaying())
            ga.PplToMove()->ReceiveMv(mveNil, spmvAnimate);
        return 1;
    }
};


/*
 *
 *  CMDSETUPBOARD command
 * 
 *  Toggles the board setup panels. The desktop enters a special mode where we're not
 *  playing chess, but setting up a game.
 * 
 */


class CMDSETUPBOARD : public CMD
{
public:
    CMDSETUPBOARD(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->uipcp.Show(!app.puiga->uipcp.FVisible());
        if (!app.puiga->uipcp.FVisible())
            app.puiga->InitGame();
        app.puiga->Redraw();
        return 1;
    }

    virtual int IdsMenu(void) const
    {
        return app.puiga->FInBoardSetup() ? idsExitBoardSetup : idsEnterBoardSetup;
    }
};


/*
 *
 *  CMDPLAYERLVL
 * 
 *  Dispatches the command player level up/down commands for white and black
 *  players (4 commands total)
 * 
 */


class CMDPLAYERLVL : public CMD
{
public:
    CMDPLAYERLVL(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        PL* ppl = app.pga->mpcpcppl[(icmd - cmdPlayerLvlUp) & 1];
        int level = ppl->Level();
        if (icmd >= cmdPlayerLvlDown)
            level--;
        else
            level++;
        ppl->SetLevel(level);
        app.puiga->uiml.Redraw();
        app.puiga->uiti.Redraw();
        return 1;
    }
    
    virtual int IdsTip(void) const
    {
        return idsTipPlayerLvlUp + (int)icmd - cmdPlayerLvlUp;
    }
};


/*
 *
 *  CMDDEBUGPANEL
 * 
 *  Command to toggle the debug panel on and off
 * 
 */


class CMDDEBUGPANEL : public CMD
{
public:
    CMDDEBUGPANEL(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->uidb.Show(!app.puiga->uidb.FVisible());
        return 1;
    }

    virtual int IdsMenu(void) const
    {
        return app.puiga->uidb.FVisible() ? idsHideDebugPanel : idsShowDebugPanel;
    }
};


/*
 *
 *  CMDLOGDEPTHUP
 * 
 *  Notifcation command for setting the logging depth up one level
 * 
 */


class CMDLOGDEPTHUP : public CMD
{
public:
    CMDLOGDEPTHUP(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        int lgd = app.puiga->uidb.LgdShow();
        app.puiga->uidb.SetLgdShow(lgd + 1);
        app.puiga->uidb.RelayoutLog();
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipLogDepthUp;
    }
};


/*
 *
 *  CMDLOGDEPTHDOWN
 * 
 *  Notification command for lowering the logging depth down one level
 * 
 */


class CMDLOGDEPTHDOWN : public CMD
{
public:
    CMDLOGDEPTHDOWN(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        int lgd = max(0, app.puiga->uidb.LgdShow() - 1);
        app.puiga->uidb.SetLgdShow(lgd);
        app.puiga->uidb.RelayoutLog();
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipLogDepthDown;
    }
};


class CMDFILELOGDEPTHUP : public CMD
{
public:
    CMDFILELOGDEPTHUP(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        int lgd = app.puiga->uidb.LgdFile();
        app.puiga->uidb.SetLgdFile(lgd + 1);
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipLogFileDepthUp;
    }
};

class CMDFILELOGDEPTHDOWN : public CMD
{
public:
    CMDFILELOGDEPTHDOWN(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        int lgd = app.puiga->uidb.LgdFile();
        app.puiga->uidb.SetLgdFile(lgd - 1);
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipLogFileDepthDown;
    }
};

/*
 *
 *  CMDLOGFILETOGGLE
 * 
 *  Toggles saving the log file on/off.
 * 
 */


class CMDLOGFILETOGGLE : public CMD
{
public:
    CMDLOGFILETOGGLE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->uidb.EnableLogFile(!app.puiga->uidb.FLogFileEnabled());
        return 1;
    }

    virtual int IdsTip(void) const
    {
        return idsTipLogFileToggle;
    }
};


/*
 *
 *  CMDTOGGLEVALIDATION
 *
 *  Toggles saving the debug validation state, which speeds up the debug
 *  version by a tonil.
 *
 */


class CMDTOGGLEVALIDATION : public CMD
{
public:
    CMDTOGGLEVALIDATION(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        fValidate = !fValidate;
        return 1;
    }

    virtual int IdsMenu(void) const
    {
        return fValidate ? idsValidationOff : idsValidationOn;
    }
};


/*
 *
 *  CMDLINKUCI command
 *
 *  Attach as an engine to a UCI GUI. THis is probably a temporary command.
 *
 */


class CMDLINKUCI : public CMD
{
public:
    CMDLINKUCI(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
#ifdef LATER
        if (app.puiga->puci)
            app.puiga->puci->ConsolePump();
#endif
        return 1;
    }
};


/*
 *
 *  CMDTIMECONTROL command
 * 
 *  Setting the game's time control.
 * 
 */


class CMDTIMECONTROL : public CMD
{
private:
    int secGame, secInc;
public:
    CMDTIMECONTROL(APP& app, int icmd, int secGame, int secInc) : CMD(app, icmd), secGame(secGame), secInc(secInc)
    {
    }

    virtual int Execute(void)
    {
        RULE* prule = app.pga->prule;
        prule->ClearTmi();
        if (secGame == 0) /* untimed game */
            ;
        else if (secInc == -1)  /* tournament mode */ {
            if (secGame == 100*60) {
                prule->AddTmi(1, 40, 100 * secMin, 0);
                prule->AddTmi(41, 60, 50 * secMin, 0);
                prule->AddTmi(61, -1, 15 * secMin, 30);
            }
            else if (secGame == 60) {
                prule->AddTmi(1, 40, 60, 0);
                prule->AddTmi(41, 60, 30, 0);
                prule->AddTmi(61, -1, 15, 1);
            }
        }
        else
            prule->AddTmi(1, -1, secGame, secInc);

        /* force clocks to update */
        app.puiga->InitClocks();
        app.puiga->uibd.InitGame();
        app.puiga->uiml.Relayout();
        app.puiga->uiml.InitGame();
        app.puiga->Redraw();

        return 1;
    }
};


/*
 *
 *  CMDSHOWDRAWTEST
 * 
 */

class CMDSHOWDRAWTEST : public CMD
{
public:
    CMDSHOWDRAWTEST(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.puiga->uidt.Show(!app.puiga->uidt.FVisible());
        return 1;
    }

};


/*
 *
 *  CMDCREATENEWPLAYER 
 * 
 */

struct DDNEWPLAYER
{
    wstring szName;
    int d;
    int ttm;
    int clpl;
    DDNEWPLAYER() : d(0), ttm(ttmNil), clpl(clplHuman) { }
};

class DCTLNAME : public DCTLTEXT
{
public:
    DCTLNAME(DLG& dlg, int id, wstring& szInit) : DCTLTEXT(dlg, id, szInit)
    {
    }

    virtual bool FValidate(void) 
    {
        if (SzRaw().size() <= 0) {
            Error(L"Must provide a name for the player.");
            return false;
        }
        return DCTLTEXT::FValidate();
    }
};

class DLGCREATENEWPLAYER : public DLG
{
    DCTLNAME dctlName;
    DCTLCOMBO dctlClass;
    DCTLINT dctlDepth;
    DCTLCOMBO dctlTime;
    DDNEWPLAYER ddnewplayer;

public:
    DLGCREATENEWPLAYER(APP& app, const DDNEWPLAYER& ddnewplayer) : DLG(app, iddCreateNewPlayer),
        ddnewplayer(ddnewplayer),
        dctlName(*this, ideNewPlayerName, this->ddnewplayer.szName),
        dctlClass(*this, idcNewPlayerClass, this->ddnewplayer.clpl),
        dctlTime(*this, idcNewPlayerTime, this->ddnewplayer.ttm),
        dctlDepth(*this, ideNewPlayerDepth, this->ddnewplayer.d, 1, 10, L"Depth must be between 1 and 10")
    {
    }

    virtual void Init(void)
    {
        for (CLPL clpl = clplFirst; clpl < clplMax; ++clpl)
            dctlClass.Add(to_wstring(clpl));        
        for (TTM ttm = ttmFirst; ttm < ttmMax; ++ttm)
            dctlTime.Add(to_wstring(ttm));
        DLG::Init();
    }
};


class CMDCREATENEWPLAYER : public CMD
{
public:
    CMDCREATENEWPLAYER(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        DDNEWPLAYER ddnewplayer;
        ddnewplayer.d = 3;
        ddnewplayer.ttm = ttmSmart;
        ddnewplayer.clpl = clplAI;
        DLGCREATENEWPLAYER dlg(app, ddnewplayer);

        if (dlg.Run() != IDOK)
            return 0;

        TPL tpl = ddnewplayer.clpl == clplHuman ? tplHuman : tplAI;
        ainfopl.vinfopl.push_back(INFOPL((CLPL)ddnewplayer.clpl, tpl, ddnewplayer.szName, (TTM)ddnewplayer.ttm, ddnewplayer.d));
        return 1;
    }
};


/*
 *
 *  CMDEXIT command
 * 
 *  Exits the app.
 *
 */


class CMDEXIT : public CMD
{
public:
    CMDEXIT(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        ::DestroyWindow(app.hwnd);
        return 1;
    }

    virtual bool FEnabled(void) const
    {
        return true;
    }
};


/*  APP::OnCommand
 *
 *  Dispatch various menu and keyboard commands.
 */
bool APP::OnCommand(int cmd)
{
    return vcmd.Execute(cmd);
}


void APP::InitCmdList(void)
{
    vcmd.Add(new CMDABOUT(*this, cmdAbout));
    vcmd.Add(new CMDNEWGAME(*this, cmdNewGame));
    vcmd.Add(new CMDTEST(*this, cmdTest));
    vcmd.Add(new CMDEXIT(*this, cmdExit));
    vcmd.Add(new CMDUNDOMOVE(*this, cmdUndoMove));
    vcmd.Add(new CMDREDOMOVE(*this, cmdRedoMove));
    vcmd.Add(new CMDROTATEBOARD(*this, cmdRotateBoard));
    vcmd.Add(new CMDOFFERDRAW(*this, cmdOfferDraw));
    vcmd.Add(new CMDRESIGN(*this, cmdResign));
    vcmd.Add(new CMDPLAY(*this, cmdPlay));
    vcmd.Add(new CMDSAVEPGN(*this, cmdSavePGN));
    vcmd.Add(new CMDOPENPGN(*this, cmdOpenPGN));
    vcmd.Add(new CMDCOPY(*this, cmdCopy));
    vcmd.Add(new CMDPASTE(*this, cmdPaste));
    vcmd.Add(new CMDSETUPBOARD(*this, cmdSetupBoard));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockUntimed, -1, -1));
    vcmd.Add(new CMDDEBUGPANEL(*this, cmdDebugPanel));
    vcmd.Add(new CMDLOGDEPTHUP(*this, cmdLogDepthUp));
    vcmd.Add(new CMDLOGDEPTHDOWN(*this, cmdLogDepthDown));
    vcmd.Add(new CMDFILELOGDEPTHUP(*this, cmdLogFileDepthUp));
    vcmd.Add(new CMDFILELOGDEPTHDOWN(*this, cmdLogFileDepthDown));
    vcmd.Add(new CMDLOGFILETOGGLE(*this, cmdLogFileToggle));
    vcmd.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlUp));
    vcmd.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlUpBlack));
    vcmd.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlDown));
    vcmd.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlDownBlack));
    vcmd.Add(new CMDPERFTDIVIDE(*this, cmdPerftDivide, true));
    vcmd.Add(new CMDPERFTDIVIDE(*this, cmdPerftDivideGo, false));
    vcmd.Add(new CMDSHOWPIECEVALUES(*this, cmdShowPieceValues));
    vcmd.Add(new CMDAISPEEDTEST(*this, cmdAISpeedTest));
    vcmd.Add(new CMDAIBREAK(*this, cmdAIBreak));
    vcmd.Add(new CMDLINKUCI(*this, cmdLinkUCI));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBullet_1_0, 1*60, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBullet_2_1, 2*60, 1));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBullet_3_0, 3*60, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBlitz_3_2, 3*60, 2));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBlitz_5_0, 5*60, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBlitz_5_3, 5*60, 3));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockRapid_10_0, 10*60, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockRapid_10_5, 10*60, 5));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockRapid_15_10, 15*60, 10));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockClassical_30_0, 30*60, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockClassical_30_20, 30*60,20));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockTourna_100_50_15, 100*60, -1));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockTest_60_30_15, 60, -1));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockTest_20, 20, 0));
    vcmd.Add(new CMDSHOWDRAWTEST(*this, cmdShowDrawTest));
    vcmd.Add(new CMDMAKENULLMOVE(*this, cmdMakeNullMove));
    vcmd.Add(new CMDTOGGLEVALIDATION(*this, cmdToggleValidation));
    vcmd.Add(new CMDCOPYFEN(*this, cmdCopyFen));
    vcmd.Add(new CMDCREATENEWPLAYER(*this, cmdCreateNewPlayer));
}


bool APP::FEnableCmds(void) const
{
    /* bulk disabling of commands can be done here */
    return puiga->FEnableCmds();
}


/*  APP::OnInitMenu
 *
 *  Initializes the dropdown menus, enabling/disabling and filling in toggling
 *  command texts.
 */
bool APP::OnInitMenu(void)
{
    HMENU hmenu = ::GetMenu(hwnd);
    vcmd.InitMenu(hmenu);
    return true;
}


/*  APP::WndProc
 *
 *  The static window procedure for the top-level app window. All we should do
 *  here is patch together the app pointer with the hwnd, and then delegate
 *  any window procedures we need to handle to the APP.
 */
LRESULT CALLBACK APP::WndProc(HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam)
{
    APP* papp = (APP*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (wm) {
    case WM_NCCREATE:
        papp = (APP*)((CREATESTRUCT*)lparam)->lpCreateParams;
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)papp);
        break;

    case WM_SIZE:
        papp->OnSize(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        return 0;

    case WM_DISPLAYCHANGE:
        ::InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT:
        papp->OnPaint();
        return 0;

    case WM_COMMAND:
        if (!papp->OnCommand(LOWORD(wparam)))
            break;
        return 0;
    
    case WM_INITMENU:
        if (!papp->OnInitMenu())
            break;
        return 0;

    case WM_MOUSEMOVE:
        if (!papp->OnMouseMove((UINT)wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            break;
        return 0;

    case WM_LBUTTONDOWN:
        if (!papp->OnLeftDown(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            break;
        return 0;

    case WM_LBUTTONUP:
        if (!papp->OnLeftUp(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            break;
        return 0;

    case WM_RBUTTONDOWN:
        if (!papp->OnRightDown(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            break;
        return 0;

    case WM_RBUTTONUP:
        if (!papp->OnRightUp(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            break;
        return 0;

    case WM_KEYDOWN:
        if (!papp->OnKeyDown((int)wparam))
            break;
        return 0;
        
    case WM_KEYUP:
        if (!papp->OnKeyUp((int)wparam))
            break;
        return 0;

    case WM_MOUSEWHEEL:
        if (!papp->OnMouseWheel(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), GET_WHEEL_DELTA_WPARAM(wparam)))
            break;
        return 0;

    case WM_TIMER:
        if (!papp->OnTimer((UINT)wparam))
            break;
        return 0;

    case WM_DESTROY:
        papp->OnDestroy();
        ::PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hwnd, wm, wparam, lparam);
}


/*  SzFlattenWsz
 *
 *  Flattens the unicode string to an ansi string
 */
string SzFlattenWsz(const wstring& wsz)
{
    char sz[1024] = "";
    int cch = min((int)wsz.length(), sizeof(sz) - 1);
    sz[::WideCharToMultiByte(CP_ACP, 0, wsz.c_str(), cch, sz, sizeof(sz), nullptr, nullptr)] = 0;
    return sz;
}


wstring WszWidenSz(const string& sz)
{
    wstring wsz(sz.begin(), sz.end());
    return wsz;
}


wchar_t* PchDecodeInt(unsigned imv, wchar_t* pch)
{
    if (imv / 10 != 0)
        pch = PchDecodeInt(imv / 10, pch);
    *pch++ = imv % 10 + L'0';
    *pch = '\0';
    return pch;
}


wstring SzCommaFromLong(int long long w)
{
    if (w == 0)
        return L"0";

    wstring sz = L"";
    if (w < 0) {
        sz = L"-";
        w = -w;
    }

    /* break the number into groups of 3 */

    int aw[20] = { 0 };
    int iw;
    for (iw = 0; w; w /= 1000)
        aw[iw++] = w % 1000;
    iw--;

    /* and print out each group */

    assert(iw >= 0);
    sz += to_wstring(aw[iw]);
    while (iw > 0) {
        sz += L",";
        int wT = aw[--iw];
        if (wT < 100)
            sz += wT < 10 ? L"00" : L"0";
        sz += to_wstring(wT);
    }

    return sz;
}


int WInterpolate(int wFrom, int wFrom1, int wFrom2, int wTo1, int wTo2)
{
    return wTo1 + (wFrom - wFrom1) * (wTo2 - wTo1) / (wFrom2 - wFrom1);
}

