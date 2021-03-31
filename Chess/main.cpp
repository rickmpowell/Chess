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
APP::APP(HINSTANCE hinst, int sw) : hinst(hinst), hwnd(NULL), haccel(NULL), prth(NULL)
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


void APP::CreateRsrc(void)
{
    if (prth)
        return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    pfactd2d->CreateHwndRenderTarget(RenderTargetProperties(),
        HwndRenderTargetProperties(hwnd, SizeU(rc.right - rc.left, rc.bottom - rc.top)),
        (ID2D1HwndRenderTarget**)&prth);
    GA::CreateRsrc(prth, pfactd2d, pfactdwr, pfactwic);
}


void APP::DiscardRsrc(void)
{
    GA::DiscardRsrc();
    SafeRelease(&prth);
}


bool APP::FSizeEnv(int dx, int dy)
{
    bool fChange = true;
    CreateRsrc();
    assert(prth != NULL);
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


void APP::OnSize(UINT dx, UINT dy)
{
    if (prth)
        prth->Resize(SizeU(dx, dy));

    if (!FSizeEnv(dx, dy))
        return;
}


void APP::OnPaint(void)
{
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    CreateRsrc();
    prth->BeginDraw();
 
    if (pga) {
        RCF rcf((float)ps.rcPaint.left, (float)ps.rcPaint.top, 
            (float)ps.rcPaint.right, (float)ps.rcPaint.bottom);
        pga->Update(&rcf);
    }

    if (prth->EndDraw() == D2DERR_RECREATE_TARGET)
        DiscardRsrc();
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


bool APP::OnCommand(int cmd)
{
    switch (cmd) {
    case cmdAbout:
        DialogBox(hinst, MAKEINTRESOURCE(iddAbout), hwnd, AboutDlgProc);
        return true;

    case cmdExit:
        DestroyWindow(hwnd);
        return true;

    case cmdNewGame:
        CmdNewGame();
        break;

    case cmdTest:
        CmdTest();
        break;

    default:
        break;
    }
    return false;
}


int APP::CmdNewGame(void)
{
    pga->NewGame();
    pga->Redraw();
    return 1;
}


int APP::CmdTest(void)
{
    pga->Test();
    return 1;
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

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hwnd, wm, wparam, lparam);
}
