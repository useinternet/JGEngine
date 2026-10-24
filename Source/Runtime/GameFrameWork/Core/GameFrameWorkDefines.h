#pragma once
#include "Core.h"

#ifdef _GAMEFRAMEWORK
#define GAMEFRAMEWORK_API __declspec(dllexport)
#define GAMEFRAMEWORK_C_API extern "C" __declspec(dllexport)
#else
#define GAMEFRAMEWORK_API __declspec(dllimport)
#define GAMEFRAMEWORK_C_API extern "C" __declspec(dllimport)
#endif
