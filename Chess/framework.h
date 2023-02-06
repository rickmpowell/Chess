/*
 *
 *	framework.h
 * 
 *	Standard windows includes, some simple utility classes
 * 
 */

#pragma once

#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#include <libloaderapi.h>

#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <d2d1effects.h>
#include <d2d1effecthelpers.h>
#include <d2d1_3.h>

#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <intrin.h>
#include <immintrin.h>

#include <format>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <streambuf>


using namespace D2D1;
using namespace std;
using namespace chrono;



/*
 *	Turn on/off validation by setting fValidate, which speeds up debug versions 
 *	a bit, which almost makes the debug version fast enough to play, but not quite.
 */


#ifndef NDEBUG
extern bool fValidate;
#endif
//#define NOSTATS


/*
 *
 *	Useful C/C++ extensions
 *
 */


#define CArray(a) (sizeof(a) / sizeof((a)[0]))

template<class Interface>
inline void SafeRelease(Interface** ppi)
{
	if (*ppi != nullptr) {
		(*ppi)->Release();
		*ppi = nullptr;
	}
}

string SzFlattenWsz(const wstring& wsz);
wstring WszWidenSz(const string& sz);
wchar_t* PchDecodeInt(unsigned imv, wchar_t* pch);
wstring SzCommaFromLong(int long long w);
int WInterpolate(int wFrom, int wFromFirst, int wFromLim, int wToFirst, int wToLim);


/*
 *
 *	EX class
 * 
 *	Exception wrapper class
 *
 */


class EX : public exception
{
protected:
	string szWhat;

public:
	EX(void) : szWhat("") { }
	EX(const string& sz) : szWhat(sz) {	}

	virtual const char* what(void) const
	{
		return szWhat.c_str();
	}

	virtual void set_what(const string& szNew)
	{
		szWhat = szNew;
	}
};


class ISTKPGN;

class EXPARSE : public EX
{
public:
	EXPARSE(const string& szMsg);
	EXPARSE(const string& szMsg, ISTKPGN& istkpgn);
	EXPARSE(const string& szMsg, const wchar_t* szFile, int line);
};


class EXINT : public EX
{
public:
	EXINT(void) : EX("interrupted") { }
};


class EXFAILTEST : public EX
{
public:
	EXFAILTEST(void) : EX("Test failed") { }
	EXFAILTEST(const string& szMsg) : EX("Test failed: " + szMsg) { }
};


enum ERR {
	errEndOfFile = 1,
	errNone = 0,
	errFatal = -1,
	errParse = -2,
	errInterrupted = -3,
	errFailed = -4
};


/*
 *
 *	PT class
 * 
 *	D2D1 floating point point class, with convenience features added on
 * 
 */


struct PT : public D2D1_POINT_2F 
{
	inline PT() 
	{ 
	}

	inline PT(float x, float y) 
	{ 
		this->x = x; 
		this->y = y; 
	}


	inline PT(POINT pt)
	{
		this->x = (float)pt.x;
		this->y = (float)pt.y;
	}


	inline PT(DWORD pt)
	{
		this->x = (float)GET_X_LPARAM(pt);
		this->y = (float)GET_Y_LPARAM(pt);
	}


	inline PT& Offset(float dx, float dy)
	{
		x += dx;
		y += dy;
		return *this;
	}

	inline PT& Offset(const PT& pt) 
	{
		return Offset(pt.x, pt.y);
	}

	inline PT Offset(const PT& pt) const
	{
		return PT(x + pt.x, y + pt.y);
	}

	inline PT operator+(const PT& pt) const 
	{
		return PT(x + pt.x, y + pt.y);
	}

	inline PT operator-(const PT& pt) const
	{
		return PT(x - pt.x, y - pt.y);
	}

	inline PT operator-(void) const 
	{
		return PT(-x, -y);
	}
};


/*
 *
 *	SIZ class
 * 
 *	D2D1 floating point size object, with some convenience features added on
 * 
 */


struct SIZ : public D2D1_SIZE_F
{
public:

	inline SIZ(void) 
	{ 
	}

	inline SIZ(const D2D1_SIZE_F& siz) 
	{
		width = siz.width; height = siz.height;
	}

	inline SIZ(float dx, float dy) 
	{
		width = dx;
		height = dy;
	}
};


/*
 *
 *	RC class
 * 
 *	D2D1 floating point rectangle object, with a few convenience features built in.
 *
 */


struct RC : public D2D1_RECT_F 
{

public:

	inline RC(void) 
	{ 
	}

	inline RC(const D2D1_RECT_F& rect)
	{
		left = rect.left;
		top = rect.top;
		right = rect.right;
		bottom = rect.bottom;
	}

	inline RC(float xLeft, float yTop, float xRight, float yBot)
	{
		left = xLeft;
		top = yTop;
		right = xRight;
		bottom = yBot;
	}

	inline RC(const PT& ptTopLeft, const SIZ& siz) 
	{
		left = ptTopLeft.x;
		top = ptTopLeft.y;
		right = left + siz.width;
		bottom = top + siz.height;
	}

	inline RC& Offset(float dx, float dy)
	{
		left += dx;
		right += dx;
		top += dy;
		bottom += dy;
		return *this;
	}

	inline RC Offset(float dx, float dy) const
	{
		return RC(left + dx, top + dy, right + dx, bottom + dy);
	}

	inline RC& Offset(const PT& pt)
	{
		return Offset(pt.x, pt.y);
	}

	inline RC Offset(const PT& pt) const
	{
		return RC(left + pt.x, top + pt.y, right + pt.x, bottom + pt.y);
	}

	RC& Inflate(float dx, float dy)
	{
		left -= dx;
		right += dx;
		top -= dy;
		bottom += dy;
		return *this;
	}

	inline RC& Inflate(const PT& pt)
	{
		return Inflate(pt.x, pt.y);
	}

	RC& Union(const RC& rc)
	{
		if (rc.left < left)
			left = rc.left;
		if (rc.right > right)
			right = rc.right;
		if (rc.top < top)
			top = rc.top;
		if (rc.bottom > bottom)
			bottom = rc.bottom;
		return *this;
	}

	RC& Intersect(const RC& rc)
	{
		if (rc.left > left)
			left = rc.left;
		if (rc.right < right)
			right = rc.right;
		if (rc.top > top)
			top = rc.top;
		if (rc.bottom < bottom)
			bottom = rc.bottom;
		return *this;
	}

	inline bool FEmpty(void) const
	{
		return left >= right || top >= bottom;
	}

	inline bool FContainsPt(const PT& pt) const
	{
		return pt.x >= left && pt.x < right && pt.y >= top && pt.y < bottom;
	}

	inline float DxWidth(void) const 
	{
		return right - left;
	}

	inline float DyHeight(void) const 
	{
		return bottom - top;
	}

	inline float XCenter(void) const 
	{
		return (left + right) / 2.0f;
	}

	inline float YCenter(void) const {
		return (top + bottom) / 2.0f;
	}

	inline PT PtCenter(void) const
	{
		return PT(XCenter(), YCenter());
	}

	inline SIZ Siz(void) const 
	{
		return SIZ(DxWidth(), DyHeight());
	}

	inline PT PtTopLeft(void) const 
	{
		return PT(left, top);
	}

	inline PT PtBotRight(void) const
	{
		return PT(right, bottom);
	}

	inline RC& SetSize(const SIZ& siz) 
	{
		right = left + siz.width;
		bottom = top + siz.height;
		return *this;
	}

	inline RC& Move(const PT& ptTopLeft) 
	{
		return Offset(ptTopLeft.x - left, ptTopLeft.y - top);
	}

	inline RC operator+(const PT& pt) const 
	{
		return Offset(pt);
	}

	inline RC& operator+=(const PT& pt) 
	{
		return Offset(pt);
	}
	
	inline RC operator-(const PT& pt) const 
	{
		return Offset(-pt);
	}

	inline RC& operator-=(const PT& pt) 
	{
		return Offset(-pt);
	}

	inline RC operator|(const RC& rc) const 
	{		
		RC rcT = *this;
		return rcT.Union(rc);
	}

	inline RC& operator|=(const RC& rc) 
	{
		return Union(rc);
	}
	
	inline RC operator&(const RC& rc) const 
	{
		RC rcT = *this;
		return rcT.Intersect(rc);
	}

	inline RC& operator&=(const RC& rc) 
	{
		return Intersect(rc);
	}

	inline operator int() const 
	{
		return !FEmpty();
	}
	
	inline bool operator!() const 
	{
		return FEmpty();
	}
};


/*
 *
 *	D2D1 rounded rectangle class with convenience features.
 * 
 */


class RR : public D2D1_ROUNDED_RECT
{
public:
	RR(const RC& rc)
	{
		rect = rc;
		radiusX = 0.0f;
		radiusY = 0.0f;
	}
};


/*
 *
 *	ELL convenience class
 * 
 *	D2D1 ellipse class with some convenience features
 * 
 */


class ELL : public D2D1_ELLIPSE
{
public:
	ELL(void) 
	{ 
	}

	ELL(const PT& ptCenter, const SIZ& sizRadius)
	{
		point.x = ptCenter.x;
		point.y = ptCenter.y;
		radiusX = sizRadius.width;
		radiusY = sizRadius.height;
	}

	ELL(const PT& ptCenter, float dxyRadius)
	{
		point.x = ptCenter.x;
		point.y = ptCenter.y;
		radiusX = dxyRadius;
		radiusY = dxyRadius;
	}

	ELL& Offset(float dx, float dy)
	{
		point.x += dx;
		point.y += dy;
		return *this;
	}

	ELL& Offset(const PT& pt)
	{
		return Offset(pt.x, pt.y);
	}

	ELL Offset(const PT& pt) const
	{
		ELL ell = *this;
		ell.point.x += pt.x;
		ell.point.y += pt.y;
		return ell;
	}
};


/*
 *
 *	Hungarianize some commonly used system types
 * 
 */


typedef ID2D1DeviceContext DC;
typedef ID2D1Factory1 FACTD2;
typedef IDWriteFactory1 FACTDWR;
typedef IWICImagingFactory2 FACTWIC;
typedef ID2D1Brush BR;
typedef ID2D1SolidColorBrush BRS;
typedef IDWriteTextFormat TX;
typedef ID2D1Bitmap1 BMP;
typedef ID2D1PathGeometry GEOM;
typedef IDXGISwapChain1 SWCH;
typedef UINT_PTR TID;



/*
 *
 *	Our random number generator. Generates 64-bit random numbers
 *	using a Mersenne twister.
 *
 */

extern mt19937_64 rgen;


template<class T> inline bool in_range(const T& t, const T& tFirst, const T& tLast)
{
	return t >= tFirst && t <= tLast;
}


/*
 *
 *	Some classes for saving/restoring various Direct2D state using constructors
 *	and destructors so we get automatic restore of state on exit. State will be
 *	restored even in the event of exceptions. We're only adding these as we need 
 *	them, so the list isn't complete.
 * 
 */


/*
 *
 *	RESTORE template
 *
 *	Used to craete a little saver/restorer for a member variable within a class.
 * 
 */


template<typename T>
class RESTORE {
	T& t;
	T tSav;
public:
	RESTORE(T& tNew) : t(tNew), tSav(t) {};
	~RESTORE() { t = tSav; }
};

/*
 *	DC antialias mode 
 */


struct AADC 
{
	DC* pdc;
	D2D1_ANTIALIAS_MODE dcaaSav;

	AADC(DC* pdc, D2D1_ANTIALIAS_MODE aa) : pdc(pdc), dcaaSav(pdc->GetAntialiasMode()) 
	{
		Set(aa);
	}

	void Set(D2D1_ANTIALIAS_MODE aa) 
	{
		pdc->SetAntialiasMode(aa);
	}

	~AADC(void) 
	{ 
		Set(dcaaSav); 
	}
};


/*
 *	solid brush color 
 */


struct COLORBRS 
{
	BRS* pbrs;
	D2D1_COLOR_F colorSav;

	COLORBRS(BRS* pbrs, D2D1_COLOR_F color) : pbrs(pbrs), colorSav(pbrs->GetColor()) 
	{
		Set(color); 
	}

	void Set(D2D1_COLOR_F color) 
	{ 
		pbrs->SetColor(color);
	}

	~COLORBRS(void) 
	{ 
		Set(colorSav); 
	}
};


/*
 *	text format text alignment 
 */


struct TATX 
{
	TX* ptx;
	DWRITE_TEXT_ALIGNMENT taSav;

	TATX(TX* ptx, DWRITE_TEXT_ALIGNMENT ta) : ptx(ptx), taSav(ptx->GetTextAlignment()) 
	{
		Set(ta);
	}

	void Set(DWRITE_TEXT_ALIGNMENT ta) 
	{
		ptx->SetTextAlignment(ta);
	}

	~TATX(void) 
	{
		ptx->SetTextAlignment(taSav);
	}
};


/*
 *	Brush opacity 
 */


struct OPACITYBR 
{
	BR* pbr;
	float opacitySav;

	OPACITYBR(BR* pbr, float opacity) : pbr(pbr), opacitySav(pbr->GetOpacity()) 
	{
		Set(opacity);
	}

	void Set(float opacity) 
	{
		pbr->SetOpacity(opacity);
	}

	~OPACITYBR(void) 
	{
		Set(opacitySav);
	}
};


/*	
 *	DC transform - note that this assigns back to the identity, it does
 *  not save and restore 
 */


struct TRANSDC 
{
	DC* pdc;

	TRANSDC(DC* pdc, const D2D1_MATRIX_3X2_F& trans) : pdc(pdc) 
	{
		Set(trans);
	}

	void Set(const D2D1_MATRIX_3X2_F& trans) 
	{
		pdc->SetTransform(trans);
	}

	~TRANSDC(void) 
	{
		Set(Matrix3x2F::Identity());
	}
};


/*	wjoin
 * 
 *	Concatenates array of values into to_string'ed string of space separated words.
 */

inline wstring const& to_wstring(wstring const& s) { return s; }

template<typename... Args>
inline wstring wjoin(Args const&... args)
{
	wstring sz;
	using ::to_wstring;
	using std::to_wstring;
	int unpack[]{ 0, (sz += to_wstring(args) + L" ", 0)... };
	static_cast<void>(unpack);
	return sz;
}


/*
 *
 *	Some handy-dandy unicode characters
 * 
 */


const wchar_t chNull = L'\0';
const wchar_t chSpace = L' ';
const wchar_t chPound = L'#';
const wchar_t chPlus = L'+';
const wchar_t chMinus = L'-';
const wchar_t chEqual = L'=';
const wchar_t chColon = L':';
const wchar_t chPeriod = L'.';
const wchar_t chQuestion = L'?';
const wchar_t chExclamation = L'!';
const wchar_t chMultiply = L'\x00d7';
const wchar_t chEnDash = L'\x2013'; 
const wchar_t chNonBreakingSpace = L'\x202f';
const wchar_t chWhiteKing = L'\x2654';
const wchar_t chWhiteQueen = L'\x2655';
const wchar_t chWhiteRook = L'\x2656';
const wchar_t chWhiteBishop = L'\x2657';
const wchar_t chWhiteKnight = L'\x2658';
const wchar_t chWhitePawn = L'\x2659';
const wchar_t chBlackKing = L'\x265a';
const wchar_t chBlackQueen = L'\x265b'; 
const wchar_t chBlackRook = L'\x265c'; 
const wchar_t chBlackBishop = L'\x265d'; 
const wchar_t chBlackKnight = L'\x265e'; 
const wchar_t chBlackPawn = L'\x265f';


/*	popcount
 *
 *	Returns the "population count" of the 64-bit number. Population count is the number
 *	of 1 bits in the 64-bit word.
 */
inline int popcount(uint64_t grf) noexcept
{
	return (int)__popcnt64(grf);
}


/*	bitscan
 *
 *	Returns the position of the lowest set bit in the 64-bit word, where 0 is the
 *	least significant bit and 63 is the most.
 *
 *	Does not work on 0.
 */

inline int bitscan(uint64_t grf) noexcept
{
	assert(grf);
	unsigned long shf;
	_BitScanForward64(&shf, grf);
	return shf;
}

inline int bitscanRev(uint64_t grf) noexcept
{
	assert(grf);
	unsigned long shf;
	_BitScanReverse64(&shf, grf);
	return shf;
}


/*
 *	IfDebug and IfDebugElse
 * 
 *	Be careful using these! They are not protected by parens or brackets
 */


#ifndef NDEBUG
#define IfDebug(x) x
#define IfDebugElse(x,y) x
#define IfRelease(x)
#define IfReleaseElse(x,y) y
#else
#define IfDebug(x) 
#define IfDebugElse(x,y) y
#define IfRelease(x) x
#define IfReleaseElse(x,y) x
#endif


/*
 *
 *	Time constants
 *
 */


const DWORD usecMsec = 1000L;	// micro-seconds in a millisecond
const DWORD msecSec = 1000L;	// milliseconds in a second
const DWORD msecHalfSec = 500L;
const DWORD secMin = 60L;
const DWORD msecMin = secMin * msecSec;


#include "co.h"