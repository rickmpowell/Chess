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
 *	PTF class
 * 
 *	D2D1 floating point point class, with convenience features added on
 * 
 */


class PTF : public D2D1_POINT_2F {
public:
	inline PTF() { }
	inline PTF(float xf, float yf) { x = xf; y = yf; }

	inline PTF& Offset(float dxf, float dyf) {
		x += dxf;
		y += dyf;
		return *this;
	}

	inline PTF& Offset(const PTF& ptf) {
		return Offset(ptf.x, ptf.y);
	}

	inline PTF operator+(const PTF& ptf) const {
		PTF ptfT(*this);
		return ptfT.Offset(ptf);
	}

	inline PTF operator-(void) const {
		return PTF(-x, -y);
	}

};


/*
 *
 *	SIZF class
 * 
 *	D2D1 floating point size object, with some convenience features added on
 * 
 */


class SIZF : public D2D1_SIZE_F
{
public:
	inline SIZF(void) { }
	inline SIZF(const D2D1_SIZE_F& sizf) {
		width = sizf.width; height = sizf.height;
	}
	inline SIZF(float dxf, float dyf) {
		width = dxf;
		height = dyf;
	}
};


/*
 *
 *	RCF class
 * 
 *	D2D1 floating point rectangle object, with a few convenience features built in.
 *
 */


class RCF : public D2D1_RECT_F {
public:
	inline RCF(void) { }
	inline RCF(float xfLeft, float yfTop, float xfRight, float yfBot)
	{
		left = xfLeft;
		top = yfTop;
		right = xfRight;
		bottom = yfBot;
	}

	inline RCF(const PTF& ptfTopLeft, const SIZF& sizf) {
		left = ptfTopLeft.x;
		top = ptfTopLeft.y;
		right = left + sizf.width;
		bottom = top + sizf.height;
	}

	inline RCF& Offset(float dxf, float dyf)
	{
		left += dxf;
		right += dxf;
		top += dyf;
		bottom += dyf;
		return *this;
	}

	inline RCF& Offset(const PTF& ptf)
	{
		return Offset(ptf.x, ptf.y);
	}

	RCF& Inflate(float dxf, float dyf)
	{
		left -= dxf;
		right += dxf;
		top -= dyf;
		bottom += dyf;
		return *this;
	}

	inline RCF& Inflate(const PTF& ptf)
	{
		return Inflate(ptf.x, ptf.y);
	}

	RCF& Union(const RCF& rcf)
	{
		if (rcf.left < left)
			left = rcf.left;
		if (rcf.right > right)
			right = rcf.right;
		if (rcf.top < top)
			top = rcf.top;
		if (rcf.bottom > bottom)
			bottom = rcf.bottom;
		return *this;
	}

	RCF& Intersect(const RCF& rcf)
	{
		if (rcf.left > left)
			left = rcf.left;
		if (rcf.right < right)
			right = rcf.right;
		if (rcf.top > top)
			top = rcf.top;
		if (rcf.bottom < bottom)
			bottom = rcf.bottom;
		return *this;
	}

	inline bool FEmpty(void) const
	{
		return left >= right || top >= bottom;
	}

	inline bool FContainsPtf(const PTF& ptf) const
	{
		return ptf.x >= left && ptf.x < right && ptf.y >= top && ptf.y < bottom;
	}

	inline float DxfWidth(void) const {
		return right - left;
	}

	inline float DyfHeight(void) const {
		return bottom - top;
	}

	inline float XCenter(void) const {
		return (left + right) / 2.0f;
	}

	inline float YCenter(void) const {
		return (top + bottom) / 2.0f;
	}

	inline PTF PtfCenter(void) const
	{
		return PTF(XCenter(), YCenter());
	}

	inline SIZF Sizf(void) const {
		return SIZF(DxfWidth(), DyfHeight());
	}

	inline PTF PtfTopLeft(void) const {
		return PTF(left, top);
	}

	inline RCF& SetSize(const SIZF& sizf) {
		right = left + sizf.width;
		bottom = top + sizf.height;
		return *this;
	}

	inline RCF& Move(const PTF& ptfTopLeft) {
		return Offset(ptfTopLeft.x - left, ptfTopLeft.y - top);
	}

	inline RCF operator+(const PTF& ptf) const {
		RCF rcf = *this;
		return rcf.Offset(ptf);
	}

	inline RCF& operator+=(const PTF& ptf) {
		return Offset(ptf);
	}
	
	inline RCF operator-(const PTF& ptf) const {
		RCF rcf(*this);
		return rcf.Offset(-ptf);
	}

	inline RCF& operator-=(const PTF& ptf) {
		return Offset(-ptf);
	}

	inline RCF operator|(const RCF& rcf) const {		
		RCF rcfT = *this;
		return rcfT.Union(rcf);
	}

	inline RCF& operator|=(const RCF& rcf) {
		return Union(rcf);
	}
	
	inline RCF operator&(const RCF& rcf) const {
		RCF rcfT = *this;
		return rcfT.Intersect(rcf);
	}

	inline RCF& operator&=(const RCF& rcf) {
		return Intersect(rcf);
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


class ELLF : public D2D1_ELLIPSE
{
public:
	ELLF(void) { }
	ELLF(const PTF& ptfCenter, const PTF& ptfRadius)
	{
		point.x = ptfCenter.x;
		point.y = ptfCenter.y;
		radiusX = ptfRadius.x;
		radiusY = ptfRadius.y;
	}

	ELLF& Offset(float dxf, float dyf)
	{
		point.x += dxf;
		point.y += dyf;
		return *this;
	}

	ELLF& Offset(const PTF& ptf)
	{
		return Offset(ptf.x, ptf.y);
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
	if (*ppi != NULL) {
		(*ppi)->Release();
		*ppi = NULL;
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