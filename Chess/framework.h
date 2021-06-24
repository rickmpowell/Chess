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

#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <d2d1effects.h>
#include <d2d1effecthelpers.h>

using namespace D2D1;

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <assert.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <shlwapi.h>
#include <d2d1_3.h>
#include <libloaderapi.h>

using namespace std;
using namespace chrono;


#ifndef NDEBUG
extern bool fValidate;
#endif


/*
 *
 *	PT class
 * 
 *	D2D1 floating point point class, with convenience features added on
 * 
 */


struct PT : public D2D1_POINT_2F {
	inline PT() { }
	inline PT(float x, float y) { this->x = x; this->y = y; }

	inline PT& Offset(float dx, float dy) {
		x += dx;
		y += dy;
		return *this;
	}

	inline PT& Offset(const PT& pt) {
		return Offset(pt.x, pt.y);
	}

	inline PT operator+(const PT& pt) const {
		PT ptT(*this);
		return ptT.Offset(pt);
	}

	inline PT operator-(void) const {
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
	inline SIZ(void) { }
	inline SIZ(const D2D1_SIZE_F& siz) {
		width = siz.width; height = siz.height;
	}
	inline SIZ(float dx, float dy) {
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


struct RC : public D2D1_RECT_F {
public:
	inline RC(void) { }
	inline RC(float xLeft, float yTop, float xRight, float yBot)
	{
		left = xLeft;
		top = yTop;
		right = xRight;
		bottom = yBot;
	}

	inline RC(const PT& ptTopLeft, const SIZ& siz) {
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

	inline RC& Offset(const PT& pt)
	{
		return Offset(pt.x, pt.y);
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

	inline float DxWidth(void) const {
		return right - left;
	}

	inline float DyHeight(void) const {
		return bottom - top;
	}

	inline float XCenter(void) const {
		return (left + right) / 2.0f;
	}

	inline float YCenter(void) const {
		return (top + bottom) / 2.0f;
	}

	inline PT PtCenter(void) const
	{
		return PT(XCenter(), YCenter());
	}

	inline SIZ Siz(void) const {
		return SIZ(DxWidth(), DyHeight());
	}

	inline PT PtTopLeft(void) const {
		return PT(left, top);
	}

	inline RC& SetSize(const SIZ& siz) {
		right = left + siz.width;
		bottom = top + siz.height;
		return *this;
	}

	inline RC& Move(const PT& ptTopLeft) {
		return Offset(ptTopLeft.x - left, ptTopLeft.y - top);
	}

	inline RC operator+(const PT& pt) const {
		RC rc = *this;
		return rc.Offset(pt);
	}

	inline RC& operator+=(const PT& pt) {
		return Offset(pt);
	}
	
	inline RC operator-(const PT& pt) const {
		RC rc(*this);
		return rc.Offset(-pt);
	}

	inline RC& operator-=(const PT& pt) {
		return Offset(-pt);
	}

	inline RC operator|(const RC& rc) const {		
		RC rcT = *this;
		return rcT.Union(rc);
	}

	inline RC& operator|=(const RC& rc) {
		return Union(rc);
	}
	
	inline RC operator&(const RC& rc) const {
		RC rcT = *this;
		return rcT.Intersect(rc);
	}

	inline RC& operator&=(const RC& rc) {
		return Intersect(rc);
	}

	inline operator int() const {
		return !FEmpty();
	}
	
	inline bool operator!() const {
		return FEmpty();
	}
};


/*
 *
 *	ELLF convenience class
 * 
 *	D2D1 ellipse class with some convenience features
 * 
 */


class ELL : public D2D1_ELLIPSE
{
public:
	ELL(void) { }
	ELL(const PT& ptCenter, const PT& ptRadius)
	{
		point.x = ptCenter.x;
		point.y = ptCenter.y;
		radiusX = ptRadius.x;
		radiusY = ptRadius.y;
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


/*
 *
 *	Useful C/C++ extensions
 * 
 */

#define CArray(rg) (sizeof(rg) / sizeof((rg)[0]))

template<class Interface>
inline void SafeRelease(Interface** ppi)
{
	if (*ppi != nullptr) {
		(*ppi)->Release();
		*ppi = nullptr;
	}
}

string SzFlattenWsz(const wstring& wsz);


/*
 *
 *	Our random number generator. Generates 32-bit random numbers
 *	using a Mersenne twister.
 *
 */

extern mt19937 rgen;


/*	peg
 *
 *	A little helper function for pegging a value between first and last values.
 *	After pegging, the return value is guaranteed to be greater or equal to
 *	tFirst and less than or equal to tLast.
 */
template<class T> inline T peg(const T& t, const T& tFirst, const T& tLast)
{
	return min(max(t, tFirst), tLast);
}


/*
 *
 *	Some classes for saving/restoring various Direct2D state using constructors
 *	and destructors so we get automatic restore of state on exit. State will be
 *	restored even in the event of exceptions. We're only adding these as we need 
 *	them, so the list isn't complete.
 * 
 */

/*	DC antialias mode */

struct AADC {
	DC* pdc;
	D2D1_ANTIALIAS_MODE dcaaSav;
	AADC(DC* pdc, D2D1_ANTIALIAS_MODE aa) : pdc(pdc), dcaaSav(pdc->GetAntialiasMode()) {
		Set(aa);
	}
	void Set(D2D1_ANTIALIAS_MODE aa) {
		pdc->SetAntialiasMode(aa);
	}
	~AADC(void) { 
		Set(dcaaSav); 
	}
};

/*	solid brush color */

struct COLORBRS {
	BRS* pbrs;
	D2D1_COLOR_F colorSav;
	COLORBRS(BRS* pbrs, D2D1_COLOR_F color) : pbrs(pbrs), colorSav(pbrs->GetColor()) {
		Set(color); 
	}
	void Set(D2D1_COLOR_F color) { 
		pbrs->SetColor(color);
	}
	~COLORBRS(void) { 
		Set(colorSav); 
	}
};

/*	text format text alignment */

struct TATX {
	TX* ptx;
	DWRITE_TEXT_ALIGNMENT taSav;
	TATX(TX* ptx, DWRITE_TEXT_ALIGNMENT ta) : ptx(ptx), taSav(ptx->GetTextAlignment()) {
		Set(ta);
	}
	void Set(DWRITE_TEXT_ALIGNMENT ta) {
		ptx->SetTextAlignment(ta);
	}
	~TATX(void) {
		ptx->SetTextAlignment(taSav);
	}
};

/*	Brush opacity */

struct OPACITYBR {
	BR* pbr;
	float opacitySav;
	OPACITYBR(BR* pbr, float opacity) : pbr(pbr), opacitySav(pbr->GetOpacity()) {
		Set(opacity);
	}
	void Set(float opacity) {
		pbr->SetOpacity(opacity);
	}
	~OPACITYBR(void) {
		Set(opacitySav);
	}
};

/*	DC transform - note that this assigns back to the identity, it does
    not save and restore */

struct TRANSDC {
	DC* pdc;
	TRANSDC(DC* pdc, const D2D1_MATRIX_3X2_F& trans) : pdc(pdc) {
		Set(trans);
	}
	void Set(const D2D1_MATRIX_3X2_F& trans) {
		pdc->SetTransform(trans);
	}
	~TRANSDC(void) {
		Set(Matrix3x2F::Identity());
	}
};