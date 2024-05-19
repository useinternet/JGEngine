#pragma once

#ifdef _ASSET
#define ASSET_API __declspec(dllexport)
#define ASSET_C_API extern "C" __declspec(dllexport)
#else
#define ASSET_API __declspec(dllimport)
#define ASSET_C_API extern "C" __declspec(dllimport)
#endif


#define JG_ASSET_FORMAT ".jgasset"
#define JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN "/JGEngine/"
#define JG_ASSET_GAME_PATH_RECOGNITION_TOEKN "/JGGame/"