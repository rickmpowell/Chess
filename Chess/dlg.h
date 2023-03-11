/*
 *
 *  dlg.h
 * 
 *  Simple wrapper class around Windows dialogs. This is not currently implemented 
 *	worth a crap and we're only going to build it up as we need it.
 *
 *	The basic idea is to derive a DLG class that takes a Windows dialog resource
 *	id, then wrap the various controls in the dialog via their control ids. There
 *	are standard controls classes that automatically do a lot of the standard 
 *	boilerplate stuff, parsing, decoding, error handling, standard operating
 *	behavior. 
 *
 *	The control's wrapper classes (derived from DCTL) are responsible for parsing, 
 *	decoding, and error handling of the the data stored by the control.
 * 
 *	1.	Create a regular Windows dialog resource in the .rc file. Use an integer
 *		resource id.
 *	2.	Declare a DD (dialog data) structure that includes are the variables the
 *		dialog will operate on.
 *	3.	Declare a dialog wrapper class derived from DLG. 
 *  4.	Declare member variables in the DLG with all the wrapper control classes
 *		you need to interact with. 
 *	5.	Initialize the controls in the DLG constructor, including references to
 *		the data in the DD for the dialog to modify.
 *	6.	Any non-standard initialization, including filling comboboxes, goes in
 *		DLG::Init. 
 *	7.	For specialized data types, declare and implement custom DCTL wrappers 
 *		around the controls and do any validation and initialization (parsing
 *		and decoding).
 *	8.	Run the dialog, and if it returns IDOK, the DD structure will be filled
 *		with the new values. Only make the changes in the DD permanent if Run
 *		returns IDOK.
 * 
 *	Be careful of the the data type used by the standard controls! They only support
 *	integers and strings. If you pass something that isn't an int or a wstring, it
 *	may trash memory! Because of this, you may need to set up intermediate variables
 *	of type 'int' or 'wstring" to pass into the dialog for the dialog to modify, then
 *	re-cast them when the dialog returns IDOK.
 *
 */

#pragma once
#include "app.h"
#include "Resources/Resource.h"


/*
 *
 *	DCTL
 * 
 *	Dialog control base class. All controls in the Windows dialog that the
 *	program needs to interact with should be wrapped by one of these boys.
 * 
 */

class DLG;

class DCTL
{
	friend class DLG;
protected:
	DLG& dlg;
	int id;
public:
	DCTL(DLG& dlg, int id);

	virtual bool FValidate(void);
	virtual void Init(void);

	virtual void SelectError(void);
	virtual wstring SzError(void);
	void Error(const wstring& sz);
	
	operator HWND(void);
	LRESULT SendMessage(UINT wm, WPARAM wparam, LPARAM lparam);
};


/*
 *
 *	DCTLINT
 * 
 *	Dialog control that holds and returns an integer. This implementation parses 
 *	numbers out of an edit control and does range checking. Because this is simple 
 *	and common,	we handle error messages a little differently to minimize the
 *	need to create a whole derived class out of it.
 * 
 */


class DCTLINT : public DCTL
{
protected:
	int& wVal;
	int wFirst, wLast;
	wstring szError;
public:
	DCTLINT(DLG& dlg, int id, int& wInit, const wstring& szError);
	DCTLINT(DLG& dlg, int id, int& wInit, int wFirst, int wLast, const wstring& szError);

	virtual bool FValidate(void);
	virtual void Init(void);
	
	virtual void SelectError(void);
	virtual wstring SzError(void);
};


/*
 *
 *	DCTLCOMBO
 * 
 *	An integer control represented by a combo drop down box. Fill the combo
 *	box by calling Add() in DLG::Init.
 * 
 */


class DCTLCOMBO : public DCTLINT
{
public:
	DCTLCOMBO(DLG& dlg, int id, int& wInit);
	
	virtual bool FValidate(void);
	virtual void Init(void);

	void Add(const wstring& sz);
};


/*
 *
 *	DCTLTEXT
 * 
 *	Simple dialog control wrapper for edit controls. Returns a wstring and
 *	does no error checking. A few convenience functions for interacting with
 *	the control.
 * 
 */


class DCTLTEXT : public DCTL
{
	wstring& szVal;
public:
	DCTLTEXT(DLG& dlg, int id, wstring& szInit);

	virtual bool FValidate(void);
	virtual void Init(void);
	
	virtual wstring SzRaw(void);
	virtual wstring SzDecode(void);

	virtual void SelectError(void);
};


/*
 *
 *	DLG class
 *
 *	Wrapper class around a Windows dialog read from a resource file. This wrapper does
 *	some rudimentary validation on inputs and simplifies some of the boilerplate you
 *	need for a Windows dialog.
 * 
 */


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
	DCTL* PdctlFromId(int id);

	int Run(void);
	void CenterDialog(void);
	void PositionButtons(void);

	virtual void Init(void);
	virtual bool FValidate(void);

	void Error(int id, const wstring& sz);
	virtual wstring SzError(int id);

	virtual bool FDialogProc(UINT wm, WPARAM wparam, LPARAM lparam);
	operator HWND(void) { return hdlg; }
};


