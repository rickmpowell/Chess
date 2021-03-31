// header.h : include file for standard system include files,
// or project specific include files
//


#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <string.h>


#include <d2d1.h>
#include <d2d1helper.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <d2d1effects.h>

using namespace D2D1;

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
#include <shlwapi.h>
#include <d2d1_3.h>
#include <libloaderapi.h>

using namespace std;


/*
 *	Abbreviations for some long system types
 */

class PTF : public D2D1_POINT_2F {
public:
	PTF() { x = 0.0f; y = 0.0f; }
	PTF(float xf, float yf) { x = xf; y = yf; }

	PTF& Offset(float dxf, float dyf) {
		x += dxf;
		y += dyf;
		return *this;
	}

	PTF& Offset(const PTF& ptf) {
		return Offset(ptf.x, ptf.y);
	}

	PTF operator+(const PTF& ptf) const {
		PTF ptfT(*this);
		return ptfT.Offset(ptf);
	}

	PTF operator-(void) const {
		return PTF(-x, -y);
	}
};

class SIZF : public D2D1_SIZE_F
{
public:
	SIZF(float dxf, float dyf) {
		width = dxf;
		height = dyf;
	}
};

class RCF : public D2D1_RECT_F {
public:
	RCF(void) { }
	RCF(float xfLeft, float yfTop, float xfRight, float yfBot)
	{
		left = xfLeft;
		top = yfTop;
		right = xfRight;
		bottom = yfBot;
	}

	RCF(const PTF& ptfTopLeft, const SIZF& sizf) {
		left = ptfTopLeft.x;
		top = ptfTopLeft.y;
		right = left + sizf.width;
		bottom = top + sizf.height;
	}

	RCF& Offset(float dxf, float dyf)
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

	RCF& Inflate(const PTF& ptf)
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

	bool FEmpty(void) const
	{
		return left >= right || top >= bottom;
	}

	bool FContainsPtf(const PTF& ptf) const
	{
		return ptf.x >= left && ptf.x < right && ptf.y >= top && ptf.y < bottom;
	}

	float DxfWidth(void) const {
		return right - left;
	}

	float DyfHeight(void) const {
		return bottom - top;
	}

	SIZF Sizf(void) const {
		return SIZF(DxfWidth(), DyfHeight());
	}

	PTF PtfTopLeft(void) const {
		return PTF(left, top);
	}

	RCF& SetSize(const SIZF& sizf) {
		right = left + sizf.width;
		bottom = top + sizf.height;
	}

	RCF& Move(const PTF& ptfTopLeft) {
		return Offset(ptfTopLeft.x - left, ptfTopLeft.y - top);
	}

	RCF operator+(const PTF& ptf) const {
		RCF rcf = *this;
		return rcf.Offset(ptf);
	}

	RCF& operator+=(const PTF& ptf) {
		return Offset(ptf);
	}
	
	RCF operator-(const PTF& ptf) const {
		RCF rcf(*this);
		return rcf.Offset(-ptf);
	}

	RCF& operator-=(const PTF& ptf) {
		return Offset(-ptf);
	}

	RCF operator|(const RCF& rcf) const {		
		RCF rcfT = *this;
		return rcfT.Union(rcf);
	}

	RCF& operator|=(const RCF& rcf) {
		return Union(rcf);
	}
	
	RCF operator&(const RCF& rcf) const {
		RCF rcfT = *this;
		return rcfT.Intersect(rcf);
	}

	RCF& operator&=(const RCF& rcf) {
		return Intersect(rcf);
	}

	operator int() const {
		return !FEmpty();
	}
	
	bool operator!() const {
		return FEmpty();
	}
};

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
		point.x += ptf.x;
		point.y += ptf.y;
		return *this;
	}
};


#define CArray(rg) (sizeof(rg) / sizeof((rg)[0]))


template<class Interface>
inline void SafeRelease(Interface** ppi)
{
	if (*ppi != NULL) {
		(*ppi)->Release();
		*ppi = NULL;
	}
}