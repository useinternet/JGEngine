#pragma once
#include "Core.h"

#ifdef _EDITOR
#define EDITOR_API __declspec(dllexport)
#define EDITOR_C_API extern "C" __declspec(dllexport)
#else
#define EDITOR_API __declspec(dllimport)
#define EDITOR_C_API extern "C" __declspec(dllimport)
#endif
