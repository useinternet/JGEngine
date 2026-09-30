#pragma once
#include "Core.h"

#ifdef _DEVCONSOLE
#define DEVCONSOLE_API __declspec(dllexport)
#define DEVCONSOLE_C_API extern "C" __declspec(dllexport)
#else
#define DEVCONSOLE_API __declspec(dllimport)
#define DEVCONSOLE_C_API extern "C" __declspec(dllimport)
#endif
