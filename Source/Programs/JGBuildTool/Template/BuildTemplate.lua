-- ENGINE_ROOT · WORKSPACE_NAME · START_PROJECT 는 JGBuildTool 이 이 파일 앞에 적는다.
-- 엔진 솔루션: ENGINE_ROOT = "" (스크립트가 엔진 루트에 있다), WORKSPACE_NAME = "JGEngine"
-- 게임 프로젝트 솔루션: 스크립트가 프로젝트 루트에 있고 ENGINE_ROOT 는 엔진 절대경로. 아래 산출물 경로는 프로젝트 기준이 된다.
-- 앞에 적힌 값이 없으면(이전 JGBuildTool.exe) 엔진 솔루션 값으로 둔다.
ENGINE_ROOT    = ENGINE_ROOT or ""
WORKSPACE_NAME = WORKSPACE_NAME or "JGEngine"

local GEN_PROJECT_FILE_PATH = "Temp/ProjectFiles/"
local BIN_PATH        = "Bin/%{cfg.buildcfg}/"
local OBJECT_PATH     = "Temp/Obj/%{cfg.buildcfg}/"
local PCH_HEADER      = "PCH/PCH.h"
local PCH_HEADER_PATH = ENGINE_ROOT .. "Source/PCH/PCH.h"
local PCH_CPP_PATH    = ENGINE_ROOT .. "Source/PCH/PCH.cpp"

function DebugConfig(defined)
    symbols       "On"
    optimize      "Off"
    defines       {"_DEBUG"}
    cppdialect    "C++20"
    staticruntime "off"
    runtime       "Debug"
end

function ConfirmConfig(defined)
    optimize        "Full" 
    defines         {"_RELEASE", "NDEBUG"}
    cppdialect      "C++20"
    staticruntime   "off"
    runtime         "Release"
end

function ReleaseConfig(defined)
    optimize        "Full" 
    defines         {"_RELEASE", "NDEBUG"}
    cppdialect      "C++20"
    staticruntime   "off"
    runtime         "Release"
end

workspace (WORKSPACE_NAME)
    architecture "x64"
    if START_PROJECT ~= nil then
        startproject (START_PROJECT)
    end

    configurations
    { 
        "DevelopEngine",  -- Engine : Debug,  Game : Debug,  Profile On
        "DevelopGame",    -- Engine : Release, Game : Debug, Profile On 
        "ConfirmGame",    -- Engine : Release, Game : Release, Profile On
        "ReleaseGame",    -- Engine : Release, Game : Release, Profile Off
    }

    -- "StaticLib"  --
    -- "SharedLib"  --
    -- "ConsoleApp" --
    function SetCPPProjectConfig(kind_name, path, defined)
        location  (GEN_PROJECT_FILE_PATH)
        kind (kind_name)
        language "C++"
        debugdir  (BIN_PATH)
        targetdir (BIN_PATH)
        libdirs(BIN_PATH)
        objdir(OBJECT_PATH)
        pchheader (PCH_HEADER)
        pchsource (PCH_CPP_PATH)
        -- 소스/실행 문자 집합을 UTF-8로 고정. 한글 주석이 있는 UTF-8 파일의 C4819를 없앤다.
        -- (모든 소스 파일은 UTF-8이어야 한다. CP949 파일이 남아 있으면 C4828이 난다.)
        buildoptions { "/utf-8" }
        if defined ~= nil then
            defines {defined}
        end
        -- file
        files {
            path .. "**.h",
            path .. "**.cpp",
            path .. "**.c",
            PCH_HEADER_PATH,
            PCH_CPP_PATH,
        }
    end

    function SetDynamicCPPProjectConfig(kind_name, path, defined, codeGenPath)
        location  (GEN_PROJECT_FILE_PATH)
        kind (kind_name)
        language "C++"
        debugdir  (BIN_PATH)
        targetdir (BIN_PATH)
        libdirs(BIN_PATH)
        objdir(OBJECT_PATH)
        pchheader (PCH_HEADER)
        pchsource (PCH_CPP_PATH)
        -- 소스/실행 문자 집합을 UTF-8로 고정. 한글 주석이 있는 UTF-8 파일의 C4819를 없앤다.
        -- (모든 소스 파일은 UTF-8이어야 한다. CP949 파일이 남아 있으면 C4828이 난다.)
        buildoptions { "/utf-8" }
        if defined ~= nil then
            defines {defined}
        end
       
        -- file
        files {
            path .. "**.h",
            path .. "**.cpp",
            path .. "**.c",
            codeGenPath .. "**.h",
            codeGenPath .. "**.cpp",
            PCH_HEADER_PATH,
            PCH_CPP_PATH,
        }
    end

    group "Engine"