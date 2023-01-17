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
#include "Resources/Resource.h"


APP* papp;


/*  wWinMain
 *
 *  The main entry point for Windows applications. 
 */
int APIENTRY wWinMain(_In_ HINSTANCE hinst, _In_opt_ HINSTANCE hinstPrev, _In_ LPWSTR szCmdLine, _In_ int sw)
{
    try {
        papp = new APP(hinst, sw);
        return papp->MessagePump();
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

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = APP::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(this);
    wcex.hInstance = hinst;
    wcex.hIcon = ::LoadIcon(hinst, MAKEINTRESOURCE(idiApp));
    wcex.hCursor = nullptr;
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
    pga->SetPl(cpcBlack, rginfopl.PplFactory(*pga, 0));
    pga->SetPl(cpcWhite, rginfopl.PplFactory(*pga, 2));
    pga->InitGame(nullptr, nullptr);
    
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
 
    CreateRsrc();
    puiga = new UIGA(*this, *pga);
    puiga->CreateRsrc();
    pga->SetUiga(puiga);
    puiga->InitGame(nullptr, nullptr);
    puiga->uipvt.Show(false);

    ::ShowWindow(hwnd, sw);

}


APP::~APP(void)
{
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
    POINT pt;
    pt.x = GET_X_LPARAM(dw);
    pt.y = GET_Y_LPARAM(dw);
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


void APP::CreateRsrc(void)
{
    if (pdc == nullptr) {

        D3D_FEATURE_LEVEL rgfld3[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
        };
        ID3D11Device* pdevd3T;
        ID3D11DeviceContext* pdcd3T;
        D3D_FEATURE_LEVEL flRet;
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            rgfld3, CArray(rgfld3), D3D11_SDK_VERSION,
            &pdevd3T, &flRet, &pdcd3T);
        if (pdevd3T->QueryInterface(__uuidof(ID3D11Device1), (void**)&pdevd3) != S_OK)
            throw 1;
        pdcd3T->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&pdcd3);
        IDXGIDevice* pdevDxgi;
        if (pdevd3->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdevDxgi) != S_OK)
            throw 1;
        pfactd2->CreateDevice(pdevDxgi, &pdevd2);
        pdevd2->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pdc);
        SafeRelease(&pdevDxgi);
        SafeRelease(&pdevd3T);
        SafeRelease(&pdcd3T);
    }

    CreateRsrcSize();
    UIGA::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
    if (puiga)
        puiga->CreateRsrc();
}


void APP::CreateRsrcSize(void)
{
    if (pswch)
        return;

    IDXGIDevice* pdevDxgi;
    if (pdevd3->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdevDxgi) != S_OK)
        throw 1;

    IDXGIAdapter* padaptDxgi;
    pdevDxgi->GetAdapter(&padaptDxgi);
    IDXGIFactory2* pfactDxgi;

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
}


void APP::DiscardRsrcSize(void)
{
    SafeRelease(&pswch);
    SafeRelease(&pbmpBackBuf);
}


void APP::DiscardRsrc(void)
{
    puiga->DiscardRsrc();
    UIGA::DiscardRsrcClass();
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
 *  The path will be something like C:\Users\[Name]\AppData\Local\Chess
 */
wstring APP::SzAppDataPath(void) const
{
    wchar_t szPath[1024];
    ::SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, szPath);
    return wstring(szPath) + L"\\" + L"SQ" + L"\\" + L"Chess";
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
    DiscardRsrcSize();
    if (puiga)
    puiga->ShowAll(false);
}


/*  APP::OnSize
 *
 *  Handles the window size notification for the top-level window. New size
 *  is dx and dy, in pixels.
 */
void APP::OnSize(UINT dx, UINT dy)
{
    DiscardRsrcSize();
    CreateRsrc();
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
        puiga->Update(rc);
        puiga->EndDraw();
    }

    ::EndPaint(hwnd, &ps);
}


bool APP::OnMouseMove(int x, int y)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui == nullptr)
        return true;

    if (puiga->puiCapt) {
        pui->LeftDrag(pt);
        return true;
    }

    if (puiga->puiHover == pui) {
        pui->MouseHover(pt, mhtMove);
        return true;
    }

    if (puiga->puiHover) {
        PT ptExit = puiga->puiHover->PtLocalFromGlobal(PT((float)x, (float)y));
        puiga->puiHover->MouseHover(ptExit, mhtExit);
    }
    pui->MouseHover(pt, mhtEnter);
    puiga->SetHover(pui);
    
    return true;
}


bool APP::OnLeftDown(int x, int y)
{
    PT pt;
    UI* pui = puiga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->StartLeftDrag(pt);
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
        pui->EndLeftDrag(pt);
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

void APP::InitLog(int depth) noexcept
{
    puiga->uidb.InitLog(depth);
}

bool APP::FDepthLog(LGT lgt, int& depth) noexcept
{
    return puiga->uidb.FDepthLog(lgt, depth);
}

void APP::AddLog(LGT lgt, LGF lgf, int depth, const TAG& tag, const wstring& szData) noexcept
{
    return puiga->uidb.AddLog(lgt, lgf, depth, tag, szData);
}

int APP::DepthLog(void) const noexcept
{
    return puiga->uidb.DepthLog();
}

void APP::SetDepthLog(int depth) noexcept
{
    puiga->uidb.SetDepthLog(depth);
}

/*  
 *
 *  CMDABOUT command
 *
 *  Just an about dialog box
 *
 */


 /*  AboutDlgProc
  *
  *  Message handler for about box.
  */
INT_PTR CALLBACK AboutDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL)
            break;
        ::EndDialog(hdlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}


class CMDABOUT : public CMD
{
public:
    CMDABOUT(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void) 
    {
        ::DialogBox(app.hinst, MAKEINTRESOURCE(iddAbout), app.hwnd, AboutDlgProc);
        return 1;
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
        app.puiga->InitGame(nullptr, nullptr);
        app.puiga->Layout();
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
        app.puiga->UndoMvu(spmvAnimate);
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipUndoMove);
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
        app.puiga->RedoMvu(spmvAnimate);
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipRedoMove);
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

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipRotateBoard);
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

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipResign);
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

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipOfferDraw);
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

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipTest);
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


enum TPERFT {
    tperftNormal = 0,
    tperftBulk = 1,
    tperftDivide = 2
};

wstring to_wstring(TPERFT tperft)
{
    switch (tperft) {
    case tperftBulk:
        return L"Bulk";
    case tperftDivide:
        return L"Divide";
    default:
        return L"Normal";
    }
}

struct DDPERFT {
    TPERFT tperft;
    int depth;
};

INT_PTR CALLBACK TestPerftDlgProc(HWND hdlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    DDPERFT* pddperft;
    switch (wm) {
    case WM_INITDIALOG:
    {
        pddperft = (DDPERFT*)lparam;
        ::SetWindowLongPtr(hdlg, DWLP_USER, lparam);
        ::SetDlgItemTextW(hdlg, idePerftDepth, to_wstring(pddperft->depth).c_str());
        HWND hwndCombo = ::GetDlgItem(hdlg, idcPerftType);
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(tperftNormal).c_str());
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(tperftBulk).c_str());
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(tperftDivide).c_str());
        ::SendMessageW(hwndCombo, CB_SETCURSEL, (WPARAM)pddperft->tperft, 0);
        HWND hwndParent = GetParent(hdlg);
        RECT rcParent, rcDlg;
        GetWindowRect(hwndParent, &rcParent);
        GetWindowRect(hdlg, &rcDlg);
        SetWindowPos(hdlg, NULL,
            (rcParent.left + rcParent.right - (rcDlg.right - rcDlg.left)) / 2,
            (rcParent.top + rcParent.bottom - (rcDlg.bottom - rcDlg.top)) / 2,
            0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        return (INT_PTR)true;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDOK:
            wchar_t sz[256];
            pddperft = (DDPERFT*)::GetWindowLongPtr(hdlg, DWLP_USER);
            ::GetDlgItemTextW(hdlg, idePerftDepth, sz, CArray(sz));
            int depth;
            try {
                depth = stoi(sz);
                if (depth < 2 || depth > 20)
                    throw 0;
            } 
            catch (...) {
                ::MessageBoxW(hdlg, L"Depth must be an integer between 2 and 20", L"Error", MB_OK);
                break;
            }
            pddperft->tperft = (TPERFT)::SendDlgItemMessage(hdlg, idcPerftType, CB_GETCURSEL, 0, 0);
            pddperft->depth = depth;
            [[fallthrough]];
        case IDCANCEL:
            ::EndDialog(hdlg, LOWORD(wparam));
            return (INT_PTR)true;
        default:
            break;
        }
        break;
    }
    return (INT_PTR)false;
}


class CMDPERFTDIVIDE : public CMD
{
public:

    static DDPERFT ddperft;
    bool fPrompt;

public:
    CMDPERFTDIVIDE(APP& app, int icmd, bool fPrompt) : CMD(app, icmd), fPrompt(fPrompt) {}

    virtual int Execute(void)
    {
        if (fPrompt) {
            if (::DialogBoxParamW(app.hinst, MAKEINTRESOURCE(iddTestPerft), app.hwnd, TestPerftDlgProc, (LPARAM)&ddperft) != IDOK)
                return 1;
        }

        LogOpen(L"perft " + to_wstring(ddperft.tperft), to_wstring(ddperft.depth), lgfBold);
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();
        uint64_t cmv;
        switch (ddperft.tperft) {
        case tperftDivide:
            cmv = app.puiga->CmvPerftDivide(ddperft.depth);
            break;
        case tperftBulk:
            cmv = app.puiga->ga.CmvPerftBulk(ddperft.depth);
            break;
        default:
            cmv = app.puiga->ga.CmvPerft(ddperft.depth);
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

    virtual bool FCustomSzMenu(void) const
    {
        return true;
    }

    virtual wstring SzMenu(void) const
    {
        return wstring(L"perft ") + to_wstring(ddperft.depth) + L"\tCtrl+P";
    }
};

DDPERFT CMDPERFTDIVIDE::ddperft = { tperftBulk, 6 };


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
                ppl->SetLevel(6);
            ppl->SetFecoRandom(0);
        }

        /* start a new untimed game */

        app.puiga->InitGame(nullptr, nullptr);
        app.puiga->ga.prule->SetGameTime(cpcWhite, 0);
        app.puiga->ga.prule->SetGameTime(cpcBlack, 0);
        app.puiga->uiml.ShowClocks(false);
        app.puiga->Layout();

        ClearLog();
        LogOpen(L"AI Speed Test", L"", lgfBold);
        int depthSav = DepthLog();
        SetDepthLog(2);

        app.puiga->StartGame(spmvFast);
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();

        for (int imv = 0; imv < 10; imv++) {
            SPMV spmv = spmvFast;
            MVU mvu = app.puiga->ga.PplToMove()->MvuGetNext(spmv);
            if (mvu.fIsNil())
                break;
            app.puiga->MakeMvu(mvu, spmvFast);
        }

        time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();

        duration dtp = tpEnd - tpStart;
        microseconds us = duration_cast<microseconds>(dtp);
        float sp = (float)us.count() / 1000.0f;

        SetDepthLog(depthSav);
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


class DDAIBREAK
{
public:
    UIGA* puiga;
    CPC cpc;
    vector<MV> vmv;
    int cRepeat;

    DDAIBREAK() : puiga(nullptr), cpc(cpcWhite), cRepeat(1) {
    }

    wstring SzDecodeMoveList(void) {
        wstring sz;
        for (MV mv : vmv)
            sz += SzFromMv(mv) + L" ";
        return sz;
    }

    void ParseSq(wstring sz, int& ich, SQ& sq)
    {
        wchar_t ch;
        if (!in_range(ch = tolower(sz[ich++]), L'a', L'h'))
            throw 1;
        int file = ch - L'a';
        if (!in_range(ch = sz[ich++], L'1', L'8'))
            throw 1;
        int rank = ch - '1';
        sq = SQ(rank, file);
    }

    void ParseMv(wstring sz, int& ich, MV& mv)
    {
        mv = mvNil;
        /* skip whitespace */
        while (ich < sz.size() && isspace(sz[ich]))
            ich++;
        if (sz[ich] == L'\0')
            return;
        SQ sqFrom, sqTo;
        ParseSq(sz, ich, sqFrom);
        ParseSq(sz, ich, sqTo);
        mv = MV(sqFrom, sqTo);
        if (sz[ich] == L'=') {
            switch (tolower(sz[++ich])) {
            case 'q': mv.SetApcPromote(apcQueen); break;
            case 'r': mv.SetApcPromote(apcRook); break;
            case 'b': mv.SetApcPromote(apcBishop); break;
            case 'n': mv.SetApcPromote(apcKnight); break;
            default:
                throw 1;
            }
            ich++;
        }
    }

    vector<MV> ParseMoveList(wstring sz) 
    {
        vector<MV> vmv;
        for (int ich = 0;;) {
            MV mv;
            ParseMv(sz, ich, mv);
            if (mv.fIsNil())
                break;
            vmv.push_back(mv);
        }    
        return vmv;
    }
};

INT_PTR CALLBACK AIBreakDlgProc(HWND hdlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    DDAIBREAK* pddaibreak;

    switch (wm) {
    case WM_INITDIALOG:
    {
        pddaibreak = (DDAIBREAK*)lparam;
        ::SetWindowLongPtr(hdlg, DWLP_USER, lparam);
        ::SendDlgItemMessageW(hdlg, idtAIBreakInst, WM_SETFONT,
                              (WPARAM)::CreateFontW(-12, 0, 0, 0, FW_NORMAL, false, false, false,
                                            ANSI_CHARSET, 0, 0, 0, FF_DONTCARE, L"Arial"), 1);
        ::SetDlgItemTextW(hdlg, ideAIBreakMoveSequence, pddaibreak->SzDecodeMoveList().c_str());
        ::SetDlgItemTextW(hdlg, ideAIBreakRepeatCount, to_wstring(pddaibreak->cRepeat).c_str());
        HWND hwndCombo = ::GetDlgItem(hdlg, idcAIBreakPlayer);
        for (CPC cpc = cpcWhite; cpc < cpcMax; ++cpc) {
            PL* ppl = pddaibreak->puiga->ga.PplFromCpc(cpc);
            ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)ppl->SzName().c_str());
        }
        ::SendMessageW(hwndCombo, CB_SETCURSEL, (WPARAM)pddaibreak->cpc, 0);
        return (INT_PTR)true;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDOK:
        {
            pddaibreak = (DDAIBREAK*)::GetWindowLongPtr(hdlg, DWLP_USER);
            
            HWND hwndCombo = ::GetDlgItem(hdlg, idcAIBreakPlayer);
            CPC cpc = (CPC)::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            int cRepeat;
            vector<MV> vmv;

            wchar_t sz[256];
            ::GetDlgItemTextW(hdlg, ideAIBreakRepeatCount, sz, CArray(sz));
            try {
                cRepeat = stoi(sz);
                if (cRepeat < 1 || cRepeat > 20)
                    throw 0;
            }
            catch (...) {
                ::MessageBoxW(hdlg, L"Repeat count musts be between 1 and 20", L"Error", MB_OK);
                break;
            }

            ::GetDlgItemTextW(hdlg, ideAIBreakMoveSequence, sz, CArray(sz));

            try {
                vmv = pddaibreak->ParseMoveList(sz);
            }
            catch (...) {
                ::MessageBoxW(hdlg, L"Invalid move list.", L"Error", MB_OK);
                break;
            }

            pddaibreak->cpc = cpc;
            pddaibreak->cRepeat = cRepeat;
            pddaibreak->vmv = vmv;
        }
            [[fallthrough]];
        case IDCANCEL:
            ::EndDialog(hdlg, LOWORD(wparam));
            return (INT_PTR)true;
        default:
            break;
        }
        break;
    }
    return (INT_PTR)false;
}


class CMDAIBREAK : public CMD
{
    static DDAIBREAK ddaibreak;

public:
    CMDAIBREAK(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        ddaibreak.puiga = app.puiga;
        if (::DialogBoxParamW(app.hinst, MAKEINTRESOURCE(iddAIBreak), app.hwnd, AIBreakDlgProc, (LPARAM)&ddaibreak) != IDOK)
            return 1;
        app.puiga->ga.PplFromCpc(ddaibreak.cpc)->SetAIBreak(ddaibreak.vmv);
        return 1;
    }
};

DDAIBREAK CMDAIBREAK::ddaibreak;


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

    virtual bool FCustomSzMenu(void) const
    {
        return true;
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

        if (::GetOpenFileName(&ofn))
            app.puiga->ga.OpenPGNFile(szFileName);
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
        string sz = os.str();
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
        CLIPB clipb(app);
        istringstream is;
        ISTKPGN istkpgn(is);
        char* pch = (char*)clipb.PGetData(CF_TEXT);
        if (app.pga->FIsPgnData(pch)) {
            is.str(pch);
            PROCPGNGA procpgngaSav(*app.pga, new PROCPGNPASTE(*app.pga));
            try {
                app.pga->Deserialize(istkpgn);
            }
            catch (exception& ex) {
                app.Error(ex.what(), MB_OK);
            }
        }
        else {
            app.puiga->InitGame(WszWidenSz(pch).c_str(), nullptr);
            app.puiga->Redraw();
        }
        return 1;
    }
};


ERR PROCPGNPASTE::ProcessTag(int tkpgn, const string& szValue)
{
    return PROCPGNOPEN::ProcessTag(tkpgn, szValue);
}

ERR PROCPGNPASTE::ProcessMvu(MVU mvu)
{
    return PROCPGNOPEN::ProcessMvu(mvu);
}


class CMDSETUPBOARD : public CMD
{
public:
    CMDSETUPBOARD(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        return 1;
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
    
    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipPlayerLvlUp+(int)icmd-cmdPlayerLvlUp);
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

    virtual bool FCustomSzMenu(void) const
    {
        return true;
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
        int depth = app.puiga->uidb.DepthLog();
        app.puiga->uidb.SetDepthLog(depth + 1);
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipLogDepthUp);
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
        int depth = app.puiga->uidb.DepthLog();
        if (depth > 1)
            depth--;
        app.puiga->uidb.SetDepthLog(depth);
        app.puiga->uidb.Redraw();
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipLogDepthDown);
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

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipLogFileToggle);
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
    int minGame, secInc;
public:
    CMDTIMECONTROL(APP& app, int icmd, int minGame, int secInc) : CMD(app, icmd), minGame(minGame), secInc(secInc)
    {
    }

    virtual int Execute(void)
    {
        RULE* prule = app.pga->prule;
        prule->ClearTmi();
        if (minGame == 0) /* untimed game */
            ;
        else if (secInc == -1)  /* tournament mode */ {
            prule->AddTmi(1, 40, 100*secMin, 0);
            prule->AddTmi(41, 60, 50*secMin, 0);
            prule->AddTmi(61, -1, 15*secMin, 30);
        }
        else
            prule->AddTmi(1, -1, minGame * secMin, secInc);

        /* force clocks to update */
        app.puiga->InitClocks();
        app.puiga->uibd.InitGame();
        app.puiga->uiml.InitGame();
        app.puiga->Redraw();

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
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBullet, 2, 1));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockBlitz, 5, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockRapid, 10, 5));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockClassical, 30, 0));
    vcmd.Add(new CMDTIMECONTROL(*this, cmdClockTournament, 100, -1));
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
        if (!papp->OnMouseMove(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
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
    char sz[1024];
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

    int rgw[20];
    int iw;
    for (iw = 0; w; w /= 1000)
        rgw[iw++] = w % 1000;
    iw--;

    /* and print out each group */

    assert(iw >= 0);
    sz += to_wstring(rgw[iw]);
    while (iw > 0) {
        sz += L",";
        int wT = rgw[--iw];
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

