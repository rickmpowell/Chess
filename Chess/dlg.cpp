/*
 *
 *  dlg.cpp
 * 
 *  A little dialog manager on top of Windows dialog boxes. Currently this is not
 *  very functional. Eventually, it'd be nice if we supported standard controls
 *  with default controls, initial values, parsing, decoding, and validation. 
 * 
 */

#include "dlg.h"


INT_PTR CALLBACK DLG::DlgProc(HWND hdlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    DLG* pdlg = (DLG*)::GetWindowLongPtr(hdlg, DWLP_USER);
    if (wm == WM_INITDIALOG) {
        pdlg = (DLG*)lparam;
        pdlg->hdlg = hdlg;
        ::SetWindowLongPtr(hdlg, DWLP_USER, lparam);
        pdlg->Init();
        pdlg->CenterDialog();
        pdlg->PositionButtons();
        return (INT_PTR)true;
    }
    if (pdlg == nullptr)
        return (INT_PTR)false;

    return (INT_PTR)pdlg->FDialogProc(wm, wparam, lparam);
}


DLG::DLG(APP& app, int idd) : app(app), idd(idd), hdlg(NULL)
{
}

void DLG::AddDctl(DCTL* pdctl)
{
    vpdctl.push_back(pdctl);
}

int DLG::Run(void)
{
    return (int)::DialogBoxParamW(app.hinst, MAKEINTRESOURCE(idd), app.hwnd, DlgProc, (LPARAM)this);
}


void DLG::Init(void)
{
    for (DCTL*& pdctl : vpdctl)
        pdctl->Init();
}

bool DLG::FValidate(void)
{
    for (DCTL*& pdctl : vpdctl) {
        if (!pdctl->FValidate())
            return false;
    }
    return true;
}

/*  DLG::CenterDialog
 *
 *  We automatically center dialogs on the main window
 */
void DLG::CenterDialog(void)
{
    HWND hwndParent = GetParent(hdlg);
    RECT rcParent, rcDlg;
    GetWindowRect(hwndParent, &rcParent);
    GetWindowRect(hdlg, &rcDlg);
    SetWindowPos(hdlg, NULL,
                 (rcParent.left + rcParent.right - (rcDlg.right - rcDlg.left)) / 2,
                 (rcParent.top + rcParent.bottom - (rcDlg.bottom - rcDlg.top)) / 2,
                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
}


/*  DLG::PositionButtons
 *
 *  Automatic layout of OK and cancel buttons in a dialog. This will override any
 *  any layout in the dialog resource.
 */
void DLG::PositionButtons(void)
{
    HWND hwndOK = GetDlgItem(hdlg, IDOK);
    HWND hwndCancel = GetDlgItem(hdlg, IDCANCEL);
    RECT rcDlg;
    GetClientRect(hdlg, &rcDlg);
    int dxMargin = 15, dyMargin = 10;
    int dxButton = 100, dyButton = 30;
    if (hwndOK) {
        SetWindowPos(hwndOK, NULL,
                     rcDlg.right - dxMargin - dxButton,
                     rcDlg.bottom - dyMargin - dyButton,
                     dxButton, dyButton,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }
    if (hwndCancel) {
        SetWindowPos(hwndCancel, NULL,
                     rcDlg.right - dxMargin - dxButton - dxMargin - dxButton,
                     rcDlg.bottom - dyMargin - dyButton,
                     dxButton, dyButton,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }
}


/*  DLG::Error
 *
 *  Call this to report parse errors when getting ready to handle the OK button
 */
void DLG::Error(wstring sz)
{
    ::MessageBoxW(hdlg, sz.c_str(), L"Error", MB_OK);
}


/*  DLG::FDialogProc
 *
 *  If we need to override the dialog procedure, this virtual function can be
 *  imnplemented in DLG implementations.
 */
bool DLG::FDialogProc(UINT wm, WPARAM wparam, LPARAM lparam)
{
    switch (wm) {

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDOK:
            if (!FValidate())
                return false;
            [[fallthrough]];
        case IDCANCEL:
            ::EndDialog(hdlg, LOWORD(wparam));
            return true;
        default:
            break;
        }
        break;

    default:
        break;
    }
    
    return false;
}


DCTL::DCTL(DLG& dlg, int id, const wstring& szError) : dlg(dlg), id(id), szError(szError)
{
    dlg.AddDctl(this);
}


DCTLINT::DCTLINT(DLG& dlg, int id, int* pw, const wstring& szError) : DCTL(dlg, id, szError),
            pw(pw), wFirst(-1), wLast(-1)
{
}

DCTLINT::DCTLINT(DLG& dlg, int id, int* pw, int wFirst, int wLast, const wstring& szError) : DCTL(dlg, id, szError),
            pw(pw), wFirst(wFirst), wLast(wLast)
{
}

void DCTL::Init(void)
{
}

bool DCTL::FValidate(void)
{
    return true;
}

DCTL::operator HWND()
{
    return ::GetDlgItem(dlg, id);
}

bool DCTLINT::FValidate(void)
{
    wchar_t sz[256];
    ::GetDlgItemTextW(dlg, id, sz, CArray(sz));
    int w;
    try {
        w = stoi(sz);
        if (w < wFirst || w > wLast)
            throw 0;
    }
    catch (...) {
        dlg.Error(szError);
        return false;
    }
    *pw = w;
    return true;
}

void DCTLINT::Init(void)
{
    ::SetDlgItemTextW(dlg, id, to_wstring(*pw).c_str());
}


DCTLCOMBO::DCTLCOMBO(DLG& dlg, int id, int* pw) : DCTLINT(dlg, id, pw, L"")
{
}


void DCTLCOMBO::Add(const wstring& sz)
{
    ::SendMessageW(*this, CB_ADDSTRING, 0, (LPARAM)sz.c_str());
}

void DCTLCOMBO::Init(void)
{
    ::SendMessageW(*this, CB_SETCURSEL, (WPARAM)*pw, 0);
}

bool DCTLCOMBO::FValidate(void)
{
    *pw = (int)::SendMessage(*this, CB_GETCURSEL, 0, 0);
    return true;
}
