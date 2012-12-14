#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <atlbase.h> 
#include <Windows.h>

#define SKELETON_SCALE /*1.0*/4.0

enum ESHOW_TYPE : int
{
	Type_Mesh = 0,
	Type_PointCloud = 1,
	Type_SkeletonPoint = 2,
};

inline void MyDebugOutput(const char* szOutput, ...)
{
	char szData[512]={0};

	va_list args;
	va_start(args, szOutput);
	_vsnprintf(szData, sizeof(szData) - 1, szOutput, args);
	va_end(args);

	OutputDebugString(/*CA2W*/(szData));
}


