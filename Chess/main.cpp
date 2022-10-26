/*
 *
 *  main.cpp
 * 
 *  The main entry point for the app and basic interface to the OS
 * 
 */

#include "app.h"
#include "ga.h"
#include "Resources/Resource.h"



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


/*  wWinMain
 *
 *  The main entry point for Windows applications. 
 */
int APIENTRY wWinMain(_In_ HINSTANCE hinst, _In_opt_ HINSTANCE hinstPrev, _In_ LPWSTR szCmdLine, _In_ int sw)
{
    try {
        APP app(hinst, sw);
        return app.MessagePump();
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
        cmdlist()
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

    pga = new GA(*this);
    pga->SetPl(CPC::Black, rginfopl.PplFactory(*pga, 0));
    pga->SetPl(CPC::White, rginfopl.PplFactory(*pga, 1));
    pga->NewGame(new RULE, SPMV::Animate);

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

    ::ShowWindow(hwnd, sw);

    pga->uipvt.Show(false);
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


DWORD APP::TmMessage(void)
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
    GA::CreateRsrcClass(pdc, pfactd2, pfactdwr, pfactwic);
    pga->CreateRsrc();
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
    pga->DiscardRsrc();
    GA::DiscardRsrcClass();
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
 *  Creates a timer that goes every dtm milliseconds.
 */
TID APP::StartTimer(UINT dtm)
{
    return ::SetTimer(nullptr, 0, dtm, nullptr);
}


/*  APP::StopTimer
 *
 *  Kills the timer
 */
void APP::StopTimer(TID tid)
{
    ::KillTimer(nullptr, tid);
}


void APP::DispatchTimer(TID tid, UINT dtm)
{
    pga->DispatchTimer(tid, dtm);
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
    pga->ShowAll(false);
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
    pga->Resize(PT((float)dx, (float)dy));
}


/*  APP::OnPaint
 *
 *  Handles painting updates on the window.
 */
void APP::OnPaint(void)
{
    PAINTSTRUCT ps;
    ::BeginPaint(hwnd, &ps);

    if (pga) {
        pga->BeginDraw();
        RC rc((float)ps.rcPaint.left, (float)ps.rcPaint.top,
            (float)ps.rcPaint.right, (float)ps.rcPaint.bottom);
        pga->Update(rc);
        pga->EndDraw();
    }

    ::EndPaint(hwnd, &ps);
}


bool APP::OnMouseMove(int x, int y)
{
    PT pt;
    UI* pui = pga->PuiHitTest(&pt, x, y);
    if (pui == nullptr)
        return true;

    if (pga->puiCapt) {
        pui->LeftDrag(pt);
        return true;
    }

    if (pga->puiHover == pui) {
        pui->MouseHover(pt, MHT::Move);
        return true;
    }

    if (pga->puiHover) {
        PT ptExit = pga->puiHover->PtLocalFromGlobal(PT((float)x, (float)y));
        pga->puiHover->MouseHover(ptExit, MHT::Exit);
    }
    pui->MouseHover(pt, MHT::Enter);
    pga->SetHover(pui);
    
    return true;
}


bool APP::OnLeftDown(int x, int y)
{
    PT pt;
    UI* pui = pga->PuiHitTest(&pt, x, y);
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
    UI* pui = pga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->EndLeftDrag(pt);
    return true;
}


bool APP::OnMouseWheel(int x, int y, int dwheel)
{
    PT pt;
    UI* pui = pga->PuiHitTest(&pt, x, y);
    if (pui)
        pui->ScrollWheel(pt, dwheel);
    return true;
}


bool APP::OnKeyDown(int vk)
{
    UI* pui = pga->PuiFocus();
    if (pui)
        pui->KeyDown(vk);
    return true;
}


bool APP::OnKeyUp(int vk)
{
    UI* pui = pga->PuiFocus();
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
    DispatchTimer(tid, TmMessage());
    return true;
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
        app.pga->NewGame(new RULE, SPMV::Animate);
        app.pga->Redraw();
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
        app.pga->Play();
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
        app.pga->UndoMv(SPMV::Animate);
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
        app.pga->RedoMv(SPMV::Animate);
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
        app.pga->uibd.FlipBoard(~app.pga->uibd.cpcPointOfView);
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
        app.pga->Test();
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


enum class TPERFT {
    Normal = 0,
    Bulk = 1,
    Divide = 2
};

wstring to_wstring(TPERFT tperft)
{
    switch (tperft) {
    case TPERFT::Bulk:
        return L"Bulk";
    case TPERFT::Divide:
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
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(TPERFT::Normal).c_str());
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(TPERFT::Bulk).c_str());
        ::SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)to_wstring(TPERFT::Divide).c_str());
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

        app.pga->XLogOpen(L"perft " + to_wstring(ddperft.tperft), to_wstring(ddperft.depth));
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();
        uint64_t cmv;
        switch (ddperft.tperft) {
        case TPERFT::Divide:
            cmv = app.pga->CmvPerftDivide(ddperft.depth);
            break;
        case TPERFT::Bulk:
            cmv = app.pga->CmvPerftBulk(ddperft.depth);
            break;
        default:
            cmv = app.pga->CmvPerft(ddperft.depth);
            break;
        }
        time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
        duration dtp = tpEnd - tpStart;
        microseconds us = duration_cast<microseconds>(dtp);
        float sp = (float)us.count() / 1000.0f;
        app.pga->XLogData(L"Time: " + to_wstring((int)round(sp)) + L" ms");
        sp = 1000.0f * (float)cmv / (float)us.count();
        app.pga->XLogData(L"Speed: " + to_wstring((int)round(sp)) + L" moves/ms");
        app.pga->XLogClose(L"perft", to_wstring(cmv), LGF::Normal);
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

DDPERFT CMDPERFTDIVIDE::ddperft = { TPERFT::Bulk, 6 };


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
        for (CPC cpc = CPC::White; cpc < CPC::ColorMax; ++cpc) {
            PL* ppl = app.pga->PplFromCpc(cpc);
            if (ppl->FHasLevel())
                ppl->SetLevel(7);
            ppl->SetFecoRandom(0);
        }
        app.pga->NewGame(new RULE, SPMV::Hidden);

        app.pga->ClearLog();
        app.pga->XLogOpen(L"AI Speed Test", L"");
        time_point<high_resolution_clock> tpStart = high_resolution_clock::now();

        int depthSav = app.pga->DepthLog();
        app.pga->SetDepthLog(2);

        for (int imv = 0; imv < 10; imv++) {
            SPMV spmv = SPMV::Hidden;
            MV mv = app.pga->PplToMove()->MvGetNext(spmv);
            if (mv.fIsNil())
                break;
            app.pga->MakeMv(mv, spmv);
        }

        app.pga->SetDepthLog(depthSav);

        time_point<high_resolution_clock> tpEnd = high_resolution_clock::now();
        duration dtp = tpEnd - tpStart;
        microseconds us = duration_cast<microseconds>(dtp);
        float sp = (float)us.count() / 1000.0f;
    
        app.pga->XLogData(L"Time: " + to_wstring((int)round(sp)) + L" ms");
        app.pga->XLogClose(L"AI Speed Test", L"", LGF::Normal);
    
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
        app.pga->uipvt.Show(!app.pga->uipvt.FVisible());
        return 1;
    }

    virtual bool FCustomSzMenu(void) const
    {
        return true;
    }

    virtual int IdsMenu(void) const
    {
        return app.pga->uipvt.FVisible() ? idsHidePieceValues: idsShowPieceValues;
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
            app.pga->OpenPGNFile(szFileName);
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
            app.pga->InitGame(WszWidenSz(pch).c_str(), SPMV::Animate);
            app.pga->Redraw();
        }
        return 1;
    }
};


ERR PROCPGNPASTE::ProcessTag(int tkpgn, const string& szValue)
{
    return PROCPGNOPEN::ProcessTag(tkpgn, szValue);
}

ERR PROCPGNPASTE::ProcessMv(MV mv)
{
    return PROCPGNOPEN::ProcessMv(mv);
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


class CMDCLOCKTOGGLE : public CMD
{
public:
    CMDCLOCKTOGGLE(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        DWORD tmOld = app.pga->prule->TmGame();
        app.pga->prule->SetTmGame(tmOld ? 0 : 60 * 60 * 1000);
        app.pga->uiml.ShowClocks(tmOld == 0);
        app.pga->Layout();
        app.pga->Redraw();
        return 1;
    }

    virtual bool FCustomSzMenu(void) const
    {
        return true;
    }

    virtual int IdsMenu(void) const
    {
        return app.pga->prule->TmGame() ? idsClocksOff : idsClocksOn;
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
        app.pga->uiml.Redraw();
        app.pga->uiti.Redraw();
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
        app.pga->uidb.Show(!app.pga->uidb.FVisible());
        return 1;
    }

    virtual bool FCustomSzMenu(void) const
    {
        return true;
    }

    virtual int IdsMenu(void) const
    {
        return app.pga->uidb.FVisible() ? idsHideDebugPanel : idsShowDebugPanel;
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
        int depth = app.pga->uidb.DepthLog();
        app.pga->uidb.SetDepthLog(depth + 1);
        app.pga->uidb.Redraw();
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
        int depth = app.pga->uidb.DepthLog();
        if (depth > 1)
            depth--;
        app.pga->uidb.SetDepthLog(depth);
        app.pga->uidb.Redraw();
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
        app.pga->uidb.EnableLogFile(!app.pga->uidb.FLogFileEnabled());
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipLogFileToggle);
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
    return cmdlist.Execute(cmd);
}


void APP::InitCmdList(void)
{
    cmdlist.Add(new CMDABOUT(*this, cmdAbout));
    cmdlist.Add(new CMDNEWGAME(*this, cmdNewGame));
    cmdlist.Add(new CMDTEST(*this, cmdTest));
    cmdlist.Add(new CMDEXIT(*this, cmdExit));
    cmdlist.Add(new CMDUNDOMOVE(*this, cmdUndoMove));
    cmdlist.Add(new CMDREDOMOVE(*this, cmdRedoMove));
    cmdlist.Add(new CMDROTATEBOARD(*this, cmdRotateBoard));
    cmdlist.Add(new CMDOFFERDRAW(*this, cmdOfferDraw));
    cmdlist.Add(new CMDRESIGN(*this, cmdResign));
    cmdlist.Add(new CMDPLAY(*this, cmdPlay));
    cmdlist.Add(new CMDSAVEPGN(*this, cmdSavePGN));
    cmdlist.Add(new CMDOPENPGN(*this, cmdOpenPGN));
    cmdlist.Add(new CMDCOPY(*this, cmdCopy));
    cmdlist.Add(new CMDPASTE(*this, cmdPaste));
    cmdlist.Add(new CMDSETUPBOARD(*this, cmdSetupBoard));
    cmdlist.Add(new CMDCLOCKTOGGLE(*this, cmdClockOnOff));
    cmdlist.Add(new CMDDEBUGPANEL(*this, cmdDebugPanel));
    cmdlist.Add(new CMDLOGDEPTHUP(*this, cmdLogDepthUp));
    cmdlist.Add(new CMDLOGDEPTHDOWN(*this, cmdLogDepthDown));
    cmdlist.Add(new CMDLOGFILETOGGLE(*this, cmdLogFileToggle));
    cmdlist.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlUp));
    cmdlist.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlUpBlack));
    cmdlist.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlDown));
    cmdlist.Add(new CMDPLAYERLVL(*this, cmdPlayerLvlDownBlack));
    cmdlist.Add(new CMDPERFTDIVIDE(*this, cmdPerftDivide, true));
    cmdlist.Add(new CMDPERFTDIVIDE(*this, cmdPerftDivideGo, false));
    cmdlist.Add(new CMDSHOWPIECEVALUES(*this, cmdShowPieceValues));
    cmdlist.Add(new CMDAISPEEDTEST(*this, cmdAISpeedTest));
}


/*  APP::OnInitMenu
 *
 *  Initializes the dropdown menus, enabling/disabling and filling in toggling
 *  command texts.
 */
bool APP::OnInitMenu(void)
{
    HMENU hmenu = ::GetMenu(hwnd);
    cmdlist.InitMenu(hmenu);
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
    size_t cch = wsz.length();
    if (cch >= sizeof(sz) - 1)
        cch = sizeof(sz) - 1;
    sz[::WideCharToMultiByte(CP_ACP, 0, wsz.c_str(), (int)cch, sz, sizeof(sz), nullptr, nullptr)] = 0;
    return sz;
}


wstring WszWidenSz(const string& sz)
{
    wstring wsz(sz.begin(), sz.end());
    return wsz;
}