ENGINE_ROOT = ""
WORKSPACE_NAME = "JGEngine"
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

-- Release 구성도 PDB 를 만든다(크래시 분석용). 패키징은 PDB 를 패키지에 넣지 않고 따로 둔다(DevOps GP-D0, 2026-10-02).
function ConfirmConfig(defined)
    symbols         "On"
    optimize        "Full"
    defines         {"_RELEASE", "NDEBUG"}
    cppdialect      "C++20"
    staticruntime   "off"
    runtime         "Release"
end

function ReleaseConfig(defined)
    symbols         "On"
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
        -- 프로젝트 안 .cpp 를 병렬로 컴파일한다(/MP). 없으면 한 파일씩 돌아 32스레드에서도 동시 1개 꼴이다(DevOps BS-1, 2026-10-02).
        flags { "MultiProcessorCompile" }
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

    -- 엔진과 무관한 단독 실행 파일(module.json "Standalone": true — 게임 패키지 부트스트랩 JGGameBootstrap).
    -- PCH · Core · 코드젠 없이 그 폴더 소스만. CRT 는 JGBuildTool 이 구성 필터마다 staticruntime "on" 으로 정적 링크한다(DevOps GP-D2, 2026-10-02).
    function SetStandaloneProjectConfig(kind_name, path, defined)
        location  (GEN_PROJECT_FILE_PATH)
        kind (kind_name)
        language "C++"
        debugdir  (BIN_PATH)
        targetdir (BIN_PATH)
        objdir(OBJECT_PATH)
        buildoptions { "/utf-8" }
        flags { "MultiProcessorCompile" }
        if defined ~= nil then
            defines {defined}
        end
        files {
            path .. "**.h",
            path .. "**.cpp",
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
        -- 프로젝트 안 .cpp 를 병렬로 컴파일한다(/MP). 없으면 한 파일씩 돌아 32스레드에서도 동시 1개 꼴이다(DevOps BS-1, 2026-10-02).
        flags { "MultiProcessorCompile" }
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
		group "Engine/Programs"
			project "JGBuildTool"
				includedirs{ "Source/Programs/JGBuildTool/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetCPPProjectConfig("ConsoleApp", "Source/Programs/JGBuildTool/", {"_JGBUILDTOOL", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "JGConsole"
				includedirs{ "Source/Programs/JGConsole/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWork/", "Temp/CodeGen/GameFrameWork/", }
				links{ "Core", "GameFrameWork", }
				SetCPPProjectConfig("ConsoleApp", "Source/Programs/JGConsole/", {"_JGCONSOLE", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "JGGameBootstrap"
				SetStandaloneProjectConfig("WindowedApp", "Source/Programs/JGGameBootstrap/", {"_JGGAMEBOOTSTRAP", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
					staticruntime "on"
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					staticruntime "on"
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					staticruntime "on"
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					staticruntime "on"


			project "JGGameLaunch"
				includedirs{ "Source/Programs/JGGameLaunch/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGGameLaunch/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Programs/JGGameLaunch/", {"_JGGAMELAUNCH", }, "Temp/CodeGen/JGGameLaunch/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "JGHeaderTool"
				includedirs{ "Source/Programs/JGHeaderTool/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetCPPProjectConfig("ConsoleApp", "Source/Programs/JGHeaderTool/", {"_JGHEADERTOOL", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "JGLauncher"
				includedirs{ "Source/Programs/JGLauncher/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetCPPProjectConfig("ConsoleApp", "Source/Programs/JGLauncher/", {"_JGLAUNCHER", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


		group "Engine/Editor"
			project "JGEditor"
				includedirs{ "Source/Editor/JGEditor/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGEditor/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWork/", "Temp/CodeGen/GameFrameWork/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "GameFrameWork", "Graphics", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/JGEditor/", {"_JGEDITOR", }, "Temp/CodeGen/JGEditor/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


		group "Engine/Runtime"
			project "Core"
				includedirs{ "Source/Runtime/Core/", "Source/ThirdParty", "Source/", }
				links{ }
				SetCPPProjectConfig("StaticLib", "Source/Runtime/Core/", {"_CORE", })
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "GameFrameWork"
				includedirs{ "Source/Runtime/GameFrameWork/", "Source/ThirdParty", "Source/", "Temp/CodeGen/GameFrameWork/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", }
				links{ "Core", "Graphics", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/GameFrameWork/", {"_GAMEFRAMEWORK", }, "Temp/CodeGen/GameFrameWork/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "GameHost"
				includedirs{ "Source/Runtime/GameHost/", "Source/ThirdParty", "Source/", "Temp/CodeGen/GameHost/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/GameFrameWork/", "Temp/CodeGen/GameFrameWork/", }
				links{ "Core", "Graphics", "GameFrameWork", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/GameHost/", {"_GAMEHOST", }, "Temp/CodeGen/GameHost/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "Graphics"
				includedirs{ "Source/Runtime/Graphics/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Graphics/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Graphics/", {"_GRAPHICS", }, "Temp/CodeGen/Graphics/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }


			project "GUI"
				includedirs{ "Source/Runtime/GUI/", "Source/ThirdParty", "Source/", "Temp/CodeGen/GUI/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", }
				links{ "Core", "Graphics", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/GUI/", {"_GUI", }, "Temp/CodeGen/GUI/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }




