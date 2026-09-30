#pragma once
#include "Core.h"

#ifdef _MYGAME
#define MYGAME_API __declspec(dllexport)
#define MYGAME_C_API extern "C" __declspec(dllexport)
#else
#define MYGAME_API __declspec(dllimport)
#define MYGAME_C_API extern "C" __declspec(dllimport)
#endif
