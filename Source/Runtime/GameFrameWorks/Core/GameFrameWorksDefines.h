#pragma once
#include "Core.h"

#ifdef _GAMEFRAMEWORKS
#define GAMEFRAMEWORKS_API __declspec(dllexport)
#define GAMEFRAMEWORKS_C_API extern "C" __declspec(dllexport)
#else
#define GAMEFRAMEWORKS_API __declspec(dllimport)
#define GAMEFRAMEWORKS_C_API extern "C" __declspec(dllimport)
#endif
