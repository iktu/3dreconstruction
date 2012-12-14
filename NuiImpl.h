#pragma once

#include "NuiApi.h"

HRESULT Nui_Init( );
void Nui_UnInit( );
static DWORD WINAPI Nui_ProcessThread( LPVOID pParam );
bool Nui_GotSkeletonAlert( );
bool Nui_GotDepthAlert( );
bool Nui_GotColorAlert( );
LONG* Nui_MapColorToDepth();
DWORD WINAPI Nui_ProcessThread( );

