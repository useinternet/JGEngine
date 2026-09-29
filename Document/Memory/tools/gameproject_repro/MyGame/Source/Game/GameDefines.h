#pragma once
#include "Core.h"

#ifdef _GAME
#define GAME_API __declspec(dllexport)
#define GAME_C_API extern "C" __declspec(dllexport)
#else
#define GAME_API __declspec(dllimport)
#define GAME_C_API extern "C" __declspec(dllimport)
#endif
