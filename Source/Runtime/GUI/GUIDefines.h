#include "Core.h"

#ifdef _GUI
#define GUI_API __declspec(dllexport)
#define GUI_C_API extern "C" __declspec(dllexport)
#else
#define GUI_API __declspec(dllimport)
#define GUI_C_API extern "C" __declspec(dllimport)
#endif