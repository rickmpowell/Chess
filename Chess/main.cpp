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
        EndDialog(hdlg, LOWORD(wParam));
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
APP::APP(HINSTANCE hinst, int sw) : hinst(hinst), hwnd(NULL), haccel(NULL), pdc(NULL), cmdlist(*this)
{
    hcurArrow = LoadCursor(NULL, IDC_ARROW);
    hcurMove = LoadCursor(NULL, IDC_SIZEALL);
    hcurNo = LoadCursor(NULL, IDC_NO);
    hcurUpDown = LoadCursor(NULL, IDC_SIZENS);
    assert(hcurArrow != NULL);  /* these cursors are system cursors and loading them can't fail */
    assert(hcurMove != NULL);
    assert(hcurNo != NULL);
    assert(hcurUpDown != NULL);

    const TCHAR szWndClassMain[] = L"ChessMain";

    /* register the window class */

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = APP::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(this);
    wcex.hInstance = hinst;
    wcex.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(idiApp));
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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
    pga->SetPl(CPC::White, new PLAI(*pga));
    pga->SetPl(CPC::Black, new PLAI2(*pga));
 //   pga->SetPl(CPC::White, new PLHUMAN(*pga, L"Rick Powell"));
    pga->NewGame(new RULE);

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
    if (pdc == nullptr) {

        D3D_FEATURE_LEVEL rgfld3[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
        };
        ID3D11Device* pdevd3T;
        ID3D11DeviceContext* pdcd3T;
        D3D_FEATURE_LEVEL flRet;
        D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
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
    if (pfactDxgi->CreateSwapChainForHwnd(pdevd3, hwnd, &swchd, NULL, NULL, &pswch) != S_OK)
        throw 1;

    IDXGISurface* psurfDxgi;
    if (pswch->GetBuffer(0, IID_PPV_ARGS(&psurfDxgi)) != S_OK)
        throw 1;

    float dxyf = (float)GetDpiForWindow(hwnd);
    D2D1_BITMAP_PROPERTIES1 propBmp;
    propBmp = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dxyf, dxyf);
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
    WCHAR sz[1024];
    ::LoadString(hinst, ids, sz, CArray(sz));
    return wstring(sz);
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
    DiscardRsrcSize();
    CreateRsrc();
    pga->Resize(PTF((float)dx, (float)dy));
}


/*  APP::OnPaint
 *
 *  Handles painting updates on the window.
 */
void APP::OnPaint(void)
{
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    if (pga) {
        pga->BeginDraw();
        RCF rcf((float)ps.rcPaint.left, (float)ps.rcPaint.top,
            (float)ps.rcPaint.right, (float)ps.rcPaint.bottom);
        pga->Update(&rcf);
        pga->EndDraw();
    }

    EndPaint(hwnd, &ps);
}


bool APP::OnMouseMove(int x, int y)
{
    PTF ptf;
    UI* pui = pga->PuiHitTest(&ptf, x, y);
    if (pui) {
        if (pga->puiCapt)
            pui->LeftDrag(ptf);
        else {
            if (pga->puiHover == pui)
                pui->MouseHover(ptf, MHT::Move);
            else {
                if (pga->puiHover)
                    pga->puiHover->MouseHover(ptf, MHT::Exit);
                pui->MouseHover(ptf, MHT::Enter);
                pga->SetHover(pui);
            }
        }
    }
    return true;
}


bool APP::OnLeftDown(int x, int y)
{
    PTF ptf;
    UI* pui = pga->PuiHitTest(&ptf, x, y);
    if (pui)
        pui->StartLeftDrag(ptf);
    return true;
}


bool APP::OnLeftUp(int x, int y)
{
    PTF ptf;
    UI* pui = pga->PuiHitTest(&ptf, x, y);
    if (pui)
        pui->EndLeftDrag(ptf);
    return true;
}


bool APP::OnMouseWheel(int x, int y, int dwheel)
{
    PTF ptf;
    UI* pui = pga->PuiHitTest(&ptf, x, y);
    if (pui)
        pui->ScrollWheel(ptf, dwheel);
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
    while (rgpcmd.size() > 0) {
        CMD* pcmd = rgpcmd.back();
        rgpcmd.pop_back();
        delete pcmd;
    }
}


void CMDLIST::Add(CMD* pcmd)
{
    size_t ccmd = rgpcmd.size();
    if (pcmd->icmd >= ccmd) {
        rgpcmd.resize(pcmd->icmd + 1);
        for (size_t icmd = ccmd; icmd < pcmd->icmd+1; icmd++)
            rgpcmd[icmd] = NULL;
    }
    assert(rgpcmd[pcmd->icmd] == NULL);
    rgpcmd[pcmd->icmd] = pcmd;
}


int CMDLIST::Execute(int icmd)
{
    assert(icmd < rgpcmd.size());
    assert(rgpcmd[icmd] != NULL);
    return rgpcmd[icmd]->Execute();
}


void CMDLIST::InitMenu(HMENU hmenu)
{
    for (size_t icmd = 0; icmd < rgpcmd.size(); icmd++)
        if (rgpcmd[icmd])
            rgpcmd[icmd]->InitMenu(hmenu);
}


wstring CMDLIST::SzTip(int icmd)
{
    return rgpcmd[icmd]->SzTip();
}


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
    CMDNEWGAME(APP& app, int icmd) : CMD(app, icmd) { }

    virtual int Execute(void)
    {
        app.pga->NewGame(new RULE);
        app.pga->Redraw();
        return 1;
    }
};


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
        app.pga->UndoMv();
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
        app.pga->RedoMv();
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
        app.pga->Test(SPMV::Hidden);
        return 1;
    }

    virtual wstring SzTip(void) const
    {
        return app.SzLoad(idsTipTest);
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
        WCHAR szFileName[1024] = L"game.pgn";
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
        WCHAR szFileName[1024] = { 0 };
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
            ::MessageBox(app.hwnd, L"Rendering clipboard failed.", L"Clipboard Error", MB_OK);
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
        is.str((char*)clipb.PGetData(CF_TEXT));
        app.pga->Deserialize(is);
        return 1;
    }
};


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
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)papp);
        break;

    case WM_SIZE:
        papp->OnSize(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
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
        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hwnd, wm, wparam, lparam);
}
