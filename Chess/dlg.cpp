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

void DLG::RemoveDctl(DCTL* pdctl)
{
    vector<DCTL*>::iterator it = find(vpdctl.begin(), vpdctl.end(), pdctl);
    assert(it != vpdctl.end());
    vpdctl.erase(it);
}

DCTL* DLG::PdctlFromId(int id)
{
    for (DCTL*& pdctl : vpdctl)
        if (pdctl->id == id)
            return pdctl;
    return nullptr;
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
void DLG::Error(int id, const wstring& sz)
{
    ::MessageBoxW(*this, sz.c_str(), L"Error", MB_OK);
    PdctlFromId(id)->SelectError();
}

wstring DLG::SzError(int id)
{
    return L"";
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


/*
 *
 *  DCTL base class
 * 
 *  Base class for dialog controls
 *
 */


/*  DCTL::DCTL
 *
 *  Base class just hooks the control up to the dialog. Derived classes must
 *  keep a reference to the variable that is connected to this control
 */
DCTL::DCTL(DLG& dlg, int id) : dlg(dlg), id(id), hfont(NULL)
{
    dlg.AddDctl(this);
}


DCTL::~DCTL(void)
{
    if (hfont)
        ::DeleteObject(hfont);
    dlg.RemoveDctl(this);
}


/*  DCTL::Init
 *
 *  Function for initializing the control for the dialog. Your code here
 *  should translate any underlying data types into whatever Windows needs
 *  to display it as a control, like decoding text, or translating option
 *  list into an integer. This is where some Windows-specific code may
 *  need to exist.
 * 
 *  A reference to the value to initialize to should be passed in to the
 *  control in its constructor, and should be kept as a member variable
 *  in the DCTL.
 */
void DCTL::Init(void)
{
}


/*  DCTL::FValidate
 *
 *  Function for extracting the data out of the dialog to return to the
 *  app. The returned value is stored in the variable sent as a reference
 *  in the DCTL constructor.
 * 
 *  This function should parse and validate the data and display an error
 *  message if something is wrong. Returns true if data validated OK.
 */
bool DCTL::FValidate(void)
{
    return true;
}


void DCTL::SetFont(const wstring& szName, int dyHeight)
{
    assert(hfont == NULL);
    hfont = CreateFontW(dyHeight, 0, 0, 0, FW_NORMAL, false, false, false,
                        ANSI_CHARSET, 0, 0, 0, FF_DONTCARE, szName.c_str());
    SendMessage(WM_SETFONT, (WPARAM)hfont, 1);
}


/*  DCTL::Error
 *
 *  Displays an error in the control. This is just a simple wrapper on the
 *  actual error handler, which is in the DLG.
 */
void DCTL::Error(const wstring& sz)
{
    dlg.Error(id, sz);
}


/*  DCTL::SelectError
 *
 *  When an error occurs, we select the control that had the error. Some control
 *  types may have extra work to do to highlight the error, so this is overrideable.
 */
void DCTL::SelectError(void)
{
    ::SetFocus(*this);
}


/*  DCTL::SzError
 *
 *  Returns the error string to display when something goes wrong with validation.
 *  This is just a little convenience function that delegates the error string
 *  to the DLG, where error strings can be returned from a centralized place,
 *  which allows us to get away with using standard controls for more use cases;
 *  otherwise we have to create a stupid derived class that does nothing but
 *  change the error message.
 */
wstring DCTL::SzError(void)
{
    return dlg.SzError(id);
}


/*  DCTL::operator HWND
 *
 *  Yeah, this is dumb. It just lets us get the HWND out of the DLG without
 *  a lot of annoying extra syntax.
 */
DCTL::operator HWND()
{
    return ::GetDlgItem(dlg, id);
}


/*  DCTL::SendMessage
 *
 *  Convenience function to send Windows messages to the control. This should only
 *  be necessary for unusual dialog behavior, or when implementing new control
 *  classes from scratch.
 */
LRESULT DCTL::SendMessage(UINT wm, WPARAM wparam, LPARAM lparam)
{
    return ::SendDlgItemMessageW(dlg, id, wm, wparam, lparam);
}


/*
 *  
 *  DCTLINT
 * 
 *  An edit control that holds an integer. Does integer parsing and range checking.
 *  Error handling is done a little differently here, because this is a super
 *  common control and it lets us implement an integer edit control with no supporting
 *  code outside the constructor.
 * 
 */


/*  DCTLINT::DCTLINT
 *
 *  Two constructors, one that does range checking, one that doesn't.
 */
DCTLINT::DCTLINT(DLG& dlg, int id, int& wInit, const wstring& szError) : DCTL(dlg, id),
    wVal(wInit), wFirst(-1), wLast(-1), szError(szError)
{
}

DCTLINT::DCTLINT(DLG& dlg, int id, int& wInit, int wFirst, int wLast, const wstring& szError) : DCTL(dlg, id),
    wVal(wInit), wFirst(wFirst), wLast(wLast), szError(szError)
{
}

bool DCTLINT::FValidate(void)
{
    wchar_t sz[256];
    ::GetDlgItemTextW(dlg, id, sz, CArray(sz));
    int wNew;
    try {
        wNew = stoi(sz);
        if ((wFirst != -1 && wLast != -1) && !in_range(wNew, wFirst, wLast))
            throw 0;
    }
    catch (...) {
        Error(szError);
        return false;
    }
    wVal = wNew;
    return true;
}

void DCTLINT::Init(void)
{
    ::SetDlgItemTextW(dlg, id, to_wstring(wVal).c_str());
}

void DCTLINT::SelectError(void)
{
    DCTL::SelectError();
    ::SendDlgItemMessageW(dlg, id, EM_SETSEL, 0, -1);
}

wstring DCTLINT::SzError(void)
{
    return szError;
}


/*
 *
 *  DCTLCOMBO
 * 
 *  Wrapper for the combobox. This implementation is for a combobox that does
 *  not have an attached textbox, so there is no error checking.
 * 
 */


DCTLCOMBO::DCTLCOMBO(DLG& dlg, int id, int& wInit) : DCTLINT(dlg, id, wInit, L"")
{
}

void DCTLCOMBO::Add(const wstring& sz)
{
    ::SendMessageW(*this, CB_ADDSTRING, 0, (LPARAM)sz.c_str());
}

void DCTLCOMBO::Init(void)
{
    ::SendMessageW(*this, CB_SETCURSEL, (WPARAM)wVal, 0);
}

bool DCTLCOMBO::FValidate(void)
{
    wVal = (int)::SendMessage(*this, CB_GETCURSEL, 0, 0);
    return true;
}


/*
 *
 *  DCTLTEXT
 * 
 *  Dialog wrapper control for a Windows edit control.
 * 
 */

DCTLTEXT::DCTLTEXT(DLG& dlg, int id, wstring& szInit) : DCTL(dlg, id), szVal(szInit)
{
}


void DCTLTEXT::Init(void)
{
    ::SetDlgItemTextW(dlg, id, SzDecode().c_str());
}


/*  DCTLTEXT::FValidate
 * 
 *  Extract and validate the data from the control and return the value. This 
 *  implementation does no parsing and never reports an error.
 */
bool DCTLTEXT::FValidate(void)
{
    szVal = SzRaw();
    return true;
}


/*  DCTLTEXT::SzDecode
 *
 *  Returns the text of the data type for display in the edit box. Override
 *  this for text controls that represent non-text underlying types.
 */
wstring DCTLTEXT::SzDecode(void)
{
    return szVal;
}


/*  DCTLTEXT::SzRaw
 *
 *  Returns the raw string from the edit control without translation or error
 *  checking. Use this in your validation to get the text to validate.
 */
wstring DCTLTEXT::SzRaw(void)
{
    wchar_t sz[512];
    ::GetDlgItemTextW(dlg, id, sz, CArray(sz));
    return sz;
}


/*  DCTLTEXT::SelectError
 *
 *  Set focus to the control and select all the text so user can quickly fix
 *  any errors.
 */
void DCTLTEXT::SelectError(void)
{
    DCTL::SelectError();
    ::SendDlgItemMessageW(dlg, id, EM_SETSEL, 0, -1);
}

