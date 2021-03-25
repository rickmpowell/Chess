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

	RCF& Offset(float dxf, float dyf)
	{
		left += dxf;
		right += dxf;
		top += dyf;
		bottom += dyf;
		return *this;
	}

	RCF& Inflate(const PTF& ptf) 
	{
		left -= ptf.x;
		right += ptf.x;
		top -= ptf.y;
		bottom += ptf.y;
		return *this;
	}

	inline RCF& Inflate(float dxf, float dyf) { return Inflate(PTF(dxf, dyf)); }

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