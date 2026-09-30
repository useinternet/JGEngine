#pragma once
#include "Core.h"

#ifdef _MYGAMEEDITOR
#define MYGAMEEDITOR_API __declspec(dllexport)
#define MYGAMEEDITOR_C_API extern "C" __declspec(dllexport)
#else
#define MYGAMEEDITOR_API __declspec(dllimport)
#define MYGAMEEDITOR_C_API extern "C" __declspec(dllimport)
#endif
