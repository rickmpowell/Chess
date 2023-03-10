/*
 *
 *  dlg.h
 * 
 *  Wrapper class around Windows dialogs. THis is currently implemented worth a
 *  crap. Eventually, we should be providing a default interface to the OS, support
 *  standard controls, assign values, parse and evaluate, etc.
 * 
 */
#pragma once
#include "app.h"
#include "Resources/Resource.h"


/*
 *
 *	DCTL
 * 
 *	Dialog control
 * 
 */

class DLG;

class DCTL
{
protected:
	DLG& dlg;
	int id;
	wstring szError;
public:
	DCTL(DLG& dlg, int id, const wstring& szError);
	virtual bool FValidate(void);
	virtual void Init(void);
	operator HWND(void);
};


class DCTLINT : public DCTL
{
protected:
	int* pw;
	int wFirst, wLast;
public:
	DCTLINT(DLG& dlg, int id, int* pw, const wstring& szError);
	DCTLINT(DLG& dlg, int id, int* pw, int wFirst, int wLast, const wstring& szError);
	virtual bool FValidate(void);
	virtual void Init(void);
};


class DCTLCOMBO : public DCTLINT
{
public:
	DCTLCOMBO(DLG& dlg, int id, int* pw);
	virtual bool FValidate(void);
	virtual void Init(void);
	void Add(const wstring& sz);
};


/*
 *
 *	DLG class
 *
 */


class DD;

class DLG
{
protected:
	APP& app;
	int idd;
	HWND hdlg;
	vector<DCTL*> vpdctl;
	static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT wm, WPARAM wparam, LPARAM lparam);
public:
	DLG(APP& app, int idd);
	void AddDctl(DCTL* pdctl);

	int Run(void);
	virtual bool FDialogProc(UINT wm, WPARAM wparam, LPARAM lparam);
	void CenterDialog(void);
	void PositionButtons(void);

	virtual void Init(void);
	virtual bool FValidate(void);
	void Error(wstring sz);

	operator HWND(void) { return hdlg; }
};


/*
 *
 *	DD class
 *
 *	Base class for dialog data. For now this doesn't do anything. Eventually,
 *	we can walk standard data values and connect them with controls in the dialog
 *	and parse/decode/report errors.
 *
 */


class DD
{
public:
	DD(void) { }
};

