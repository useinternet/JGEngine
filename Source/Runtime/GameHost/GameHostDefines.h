#pragma once
#include "Core.h"

#ifdef _GAMEHOST
#define GAMEHOST_API __declspec(dllexport)
#define GAMEHOST_C_API extern "C" __declspec(dllexport)
#else
#define GAMEHOST_API __declspec(dllimport)
#define GAMEHOST_C_API extern "C" __declspec(dllimport)
#endif
