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

	void Offset(float dxf, float dyf)
	{
		left += dxf;
		right += dxf;
		top += dyf;
		bottom += dyf;
	}

	void Inflate(const PTF& ptf) 
	{
		left -= ptf.x;
		right += ptf.x;
		top -= ptf.y;
		bottom += ptf.y;
	}

	inline void Inflate(float dxf, float dyf) { Inflate(PTF(dxf, dyf)); }

	bool FContainsPtf(const PTF& ptf) const
	{
		return ptf.x >= left && ptf.x < right && ptf.y >= top && ptf.y < bottom;
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