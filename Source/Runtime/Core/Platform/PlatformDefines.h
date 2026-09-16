#pragma once


// Platform 별 Include
// Window
#ifdef _PLATFORM_WINDOWS
#include <Windows.h>
#endif

// Platform 별 Define

#ifdef _PLATFORM_WINDOWS

using HJModule   = HMODULE;
using HJInstance = HINSTANCE;
using HJWHandle  = HWND;
#endif