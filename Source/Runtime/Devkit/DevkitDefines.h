#pragma once
#include "Core.h"

#ifdef _DEVKIT
#define DEVKIT_API __declspec(dllexport)
#define DEVKIT_C_API extern "C" __declspec(dllexport)
#else
#define DEVKIT_API __declspec(dllimport)
#define DEVKIT_C_API extern "C" __declspec(dllimport)
#endif