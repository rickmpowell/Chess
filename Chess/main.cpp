/*
 *
 *  main.cpp
 * 
 *  The main entry point for the app and basic interface to the OS
 * 
 */

#include "framework.h"
#include "chess.h"


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
        EndDialog(hdlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}



/*  wWinMain
 *
 *  The main entry point for Windows applications. 
 */
int APIENTRY wWinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPWSTR szCmdLine, int sw)
{
    try {
        APP app(hinst, sw);
        return app.MessagePump();
    }
    catch (int err) {
        MessageBox(NULL,
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
APP::APP(HINSTANCE hinst, int sw) : hinst(hinst), hwnd(NULL), haccel(NULL), pdcd2(NULL), cmdlist(*this)
{
    hcurArrow = LoadCursor(NULL, IDC_ARROW);
    hcurMove = LoadCursor(NULL, IDC_SIZEALL);
    hcurNo = LoadCursor(NULL, IDC_NO);
    assert(hcurArrow != NULL);  /* these cursors are system cursors and loading them can't fail */
    assert(hcurMove != NULL);
    assert(hcurNo != NULL);

    const TCHAR szWndClassMain[] = L"ChessMain";

     /* register the window class */

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = APP::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(this);
    wcex.hInstance = hinst;
    wcex.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(idiApp));
    wcex.hCursor = NULL;
    wcex.hbrBackground = NULL; // (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(idmApp);
    wcex.lpszClassName = szWndClassMain;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(idiSmall));
    if (!RegisterClassEx(&wcex))
        throw 1;

    /* load keyboard interface */

    if (!(haccel = LoadAccelerators(hinst, MAKEINTRESOURCE(idaApp))))
        throw 1;

    InitCmdList();

    pga = new GA(*this);
    pga->SetPl(cpcWhite, new PL(L"Squub"));
    pga->SetPl(cpcBlack, new PL(L"Frapija"));
    pga->NewGame();

    /* create the main window */

    sw = SW_MAXIMIZE;
    TCHAR szTitle[100];
    if (!::LoadString(hinst, idsApp, szTitle, CArray(szTitle)))
        throw 1;
    hwnd = ::CreateWindowW(szWndClassMain, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
        NULL, NULL, hinst, this);
    if (hwnd == NULL)
        throw 1;    // BUG: cleanup haccel
    ShowWindow(hwnd, sw);
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
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (TranslateAccelerator(msg.hwnd, haccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}


DWORD APP::TmMessage(void)
{
    return ::GetMessageTime();
}


void APP::CreateRsrc(void)
{
    if (pdcd2)
        return;
 
    D3D_FEATURE_LEVEL rgfld3[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    ID3D11Device* pdevd3T;
    ID3D11DeviceContext* pdcd3T;
    D3D_FEATURE_LEVEL flRet;
    D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, 
            rgfld3, CArray(rgfld3), D3D11_SDK_VERSION,
            &pdevd3T, &flRet, &pdcd3T);
    pdevd3T->QueryInterface(__uuidof(ID3D11Device1), (void**)&pdevd3);
    pdcd3T->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&pdcd3);
    IDXGIDevice* pdevDxgi;
    pdevd3->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdevDxgi);
    pfactd2->CreateDevice(pdevDxgi, &pdevd2);
    pdevd2->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pdcd2);
    IDXGIAdapter* padaptDxgi;
    pdevDxgi->GetAdapter(&padaptDxgi);
    IDXGIFactory2* pfactDxgi;
    padaptDxgi->GetParent(IID_PPV_ARGS(&pfactDxgi));
    DXGI_SWAP_CHAIN_DESC1 swchd = { 0 };
    swchd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swchd.SampleDesc.Count = 1;
    swchd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swchd.BufferCount = 2;
    swchd.Scaling = DXGI_SCALING_STRETCH;
    swchd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    pfactDxgi->CreateSwapChainForHwnd(pdevd3, hwnd, &swchd, NULL, NULL, &pswch);
    IDXGISurface* psurfDxgi;
    pswch->GetBuffer(0, IID_PPV_ARGS(&psurfDxgi));
    float dxyf = (float)GetDpiForWindow(hwnd);
    D2D1_BITMAP_PROPERTIES1 propBmp = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dxyf, dxyf);
    pdcd2->CreateBitmapFromDxgiSurface(psurfDxgi, &propBmp, &pbmpBackBuf);
    pdcd2->SetTarget(pbmpBackBuf);
    SafeRelease(&psurfDxgi);
    SafeRelease(&pfactDxgi);
    SafeRelease(&padaptDxgi);
    SafeRelease(&pdevDxgi);
    SafeRelease(&pdevd3T);
    SafeRelease(&pdcd3T);

    GA::CreateRsrc(pdcd2, pfactd2, pfactdwr, pfactwic);
}


void APP::DiscardRsrc(void)
{
    GA::DiscardRsrc();
    SafeRelease(&pdevd3);
    SafeRelease(&pdcd3);
    SafeRelease(&pdevd2);
    SafeRelease(&pswch);
    SafeRelease(&pbmpBackBuf);
    SafeRelease(&pdcd2);
}



bool APP::FSizeEnv(int dx, int dy)
{
    bool fChange = true;
    CreateRsrc();
    assert(pdcd2 != NULL);
    pga->Resize(dx, dy);
    return fChange;
}


/*  APP::Redraw
 *
 *  Force a redraw of the application. Redraws the entire app if prcf is NULL.
 */
void APP::Redraw(const RCF* prcf)
{
    if (prcf == NULL)
        ::InvalidateRect(hwnd, NULL, true);
    else {
        RECT rc;
        ::SetRect(&rc, (int)prcf->left, (int)prcf->top, (int)prcf->right, (int)prcf->bottom);
        ::InvalidateRect(hwnd, &rc, true);
    }
    ::UpdateWindow(hwnd);
}


/*  APP::CreateTimer
 *
 *  Creates a timer that goes every dtm milliseconds.
 */
void APP::CreateTimer(UINT tid, DWORD dtm)
{
    ::SetTimer(hwnd, tid, dtm, NULL);
}


/*  APP::DestroyTimer
 *
 *  Kills the timer
 */
void APP::DestroyTimer(UINT tid)
{
    ::KillTimer(hwnd, tid);
}


/*  APP::OnSize
 *
 *  Handles the window size notification for the top-level window. New size
 *  is dx and dy, in pixels.
 */
void APP::OnSize(UINT dx, UINT dy)
{
    DiscardRsrc();
    if (!FSizeEnv(dx, dy))
        return;
}


/*  APP::OnPaint
 *
 *  Handles painting updates on the window.
 */
void APP::OnPaint(void)
{
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    CreateRsrc();
    pdcd2->BeginDraw();

    if (pga) {
        RCF rcf((float)ps.rcPaint.left, (float)ps.rcPaint.top, 
            (float)ps.rcPaint.right, (float)ps.rcPaint.bottom);
        pga->Update(&rcf);
    }

    if (pdcd2->EndDraw() == D2DERR_RECREATE_TARGET)
        DiscardRsrc();
    DXGI_PRESENT_PARAMETERS pp = { 0 };
    pswch->Present1(1, 0, &pp);

    EndPaint(hwnd, &ps);
}


bool APP::OnMouseMove(UINT x, UINT y)
{
    HT* pht = pga->PhtHitTest(PTF((float)x, (float)y));
    pga->MouseMove(pht);
    delete pht;
    return true;
}


bool APP::OnLeftDown(UINT x, UINT y)
{
    HT* pht = pga->PhtHitTest(PTF((float)x, (float)y));
    if (pht == NULL)
        return true;
    pga->LeftDown(pht);
    delete pht;
    return true;
}


bool APP::OnLeftUp(UINT x, UINT y)
{
    HT* pht = pga->PhtHitTest(PTF((float)x, (float)y));
    pga->LeftUp(pht);
    delete pht;
    return true;
}


/*  APP::OnTimer
 *
 *  Handles the WM_TIMER message from the wndproc. Returns false if
 *  the default processing should still happen.
 */
bool APP::OnTimer(UINT tid)
{
    pga->Timer(tid, TmMessage());
    return true;
}


/*
 *
 *  CMDLIST
 * 
 */


CMDLIST::CMDLIST(APP& app) : app(app)
{
}


CMDLIST::~CMDLIST(void)
{
    while (mpicmdpcmd.size() > 0) {
        CMD* pcmd = mpicmdpcmd.back();
        mpicmdpcmd.pop_back();
        delete pcmd;
    }
}


void CMDLIST::Add(int icmd, CMD* pcmd)
{
    size_t ccmd = mpicmdpcmd.size();
    if (icmd >= ccmd) {
        mpicmdpcmd.resize(icmd + 1);
        for (size_t icmdT = ccmd; icmdT < icmd+1; icmdT++)
            mpicmdpcmd[icmdT] = NULL;
    }
    assert(mpicmdpcmd[icmd] == NULL);
    mpicmdpcmd[icmd] = pcmd;
}


int CMDLIST::Execute(int icmd)
{
    assert(icmd < mpicmdpcmd.size());
    assert(mpicmdpcmd[icmd] != NULL);
    return mpicmdpcmd[icmd]->Execute();
}


CMD::CMD(APP& app) : app(app)
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

wstring CMD::SzMenu(void) const
{
    return L"";
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
    CMDABOUT(APP& app) : CMD(app) { }

    virtual int Execute(void) 
    {
        DialogBox(app.hinst, MAKEINTRESOURCE(iddAbout), app.hwnd, AboutDlgProc);
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
    CMDNEWGAME(APP& app) : CMD(app) { }

    virtual int Execute(void)
    {
        app.pga->NewGame();
        app.pga->Redraw();
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
    CMDUNDOMOVE(APP& app) : CMD(app) { }

    virtual int Execute(void)
    {
        app.pga->UndoMv(true);
        return 1;
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
    CMDREDOMOVE(APP& app) : CMD(app) { }

    virtual int Execute(void)
    {
        app.pga->RedoMv(true);
        return 1;
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
    CMDTEST(APP& app) : CMD(app) { }

    virtual int Execute(void)
    {
        app.pga->Test();
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
    CMDEXIT(APP& app) : CMD(app) { }

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
    cmdlist.Add(cmdAbout, new CMDABOUT(*this));
    cmdlist.Add(cmdNewGame, new CMDNEWGAME(*this));
    cmdlist.Add(cmdTest, new CMDTEST(*this));
    cmdlist.Add(cmdExit, new CMDEXIT(*this));
    cmdlist.Add(cmdUndoMove, new CMDUNDOMOVE(*this));
    cmdlist.Add(cmdRedoMove, new CMDREDOMOVE(*this));
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
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)papp);
        break;

    case WM_SIZE:
        papp->OnSize(LOWORD(lparam), HIWORD(lparam));
        return 0;

    case WM_DISPLAYCHANGE:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT:
        papp->OnPaint();
        return 0;

    case WM_COMMAND:
        if (!papp->OnCommand(LOWORD(wparam)))
            break;
        return 0;

    case WM_MOUSEMOVE:
        if (!papp->OnMouseMove(LOWORD(lparam), HIWORD(lparam)))
            break;
        return 0;

    case WM_LBUTTONDOWN:
        if (!papp->OnLeftDown(LOWORD(lparam), HIWORD(lparam)))
            break;
        return 0;

    case WM_LBUTTONUP:
        if (!papp->OnLeftUp(LOWORD(lparam), HIWORD(lparam)))
            break;
        return 0;

    case WM_TIMER:
        if (!papp->OnTimer((UINT)wparam))
            break;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hwnd, wm, wparam, lparam);
}
