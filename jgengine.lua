local GEN_PROJECT_FILE_PATH = "Temp/ProjectFiles/"
local BIN_PATH        = "Bin/%{cfg.buildcfg}/"
local OBJECT_PATH     = "Temp/Obj/%{cfg.buildcfg}/"
local PCH_HEADER      = "PCH/PCH.h"
local PCH_HEADER_PATH = "Source/PCH/PCH.h"
local PCH_CPP_PATH    = "Source/PCH/PCH.cpp"

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

workspace "JGEngine"
    architecture "x64"

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
				includedirs{ "Source/Programs/JGConsole/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWorks/", "Temp/CodeGen/GameFrameWorks/", }
				links{ "Core", "GameFrameWorks", }
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
			project "DevConsole"
				includedirs{ "Source/Editor/DevConsole/", "Source/ThirdParty", "Source/", "Temp/CodeGen/DevConsole/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "Graphics", "Asset", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/DevConsole/", {"_DEVCONSOLE", }, "Temp/CodeGen/DevConsole/")
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


			project "DevStatistics"
				includedirs{ "Source/Editor/DevStatistics/", "Source/ThirdParty", "Source/", "Temp/CodeGen/DevStatistics/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "Graphics", "Asset", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/DevStatistics/", {"_DEVSTATISTICS", }, "Temp/CodeGen/DevStatistics/")
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


			project "JGDev_Graphics"
				includedirs{ "Source/Editor/JGDev_Graphics/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGDev_Graphics/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/Devkit/", "Temp/CodeGen/Devkit/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "Graphics", "Asset", "Devkit", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/JGDev_Graphics/", {"_JGDEV_GRAPHICS", }, "Temp/CodeGen/JGDev_Graphics/")
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


			project "JGEditor"
				includedirs{ "Source/Editor/JGEditor/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGEditor/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWorks/", "Temp/CodeGen/GameFrameWorks/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "GameFrameWorks", "Graphics", "GUI", }
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
			project "AI"
				includedirs{ "Source/Runtime/AI/", "Source/ThirdParty", "Source/", "Temp/CodeGen/AI/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", }
				links{ "Core", "Graphics", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/AI/", {"_AI", }, "Temp/CodeGen/AI/")
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


			project "Asset"
				includedirs{ "Source/Runtime/Asset/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Asset/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Asset/", {"_ASSET", }, "Temp/CodeGen/Asset/")
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


			project "Devkit"
				includedirs{ "Source/Runtime/Devkit/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Devkit/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "Graphics", "Asset", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Devkit/", {"_DEVKIT", }, "Temp/CodeGen/Devkit/")
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


			project "Game"
				includedirs{ "Source/Runtime/Game/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Game/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWorks/", "Temp/CodeGen/GameFrameWorks/", }
				links{ "Core", "GameFrameWorks", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Game/", {"_GAME", }, "Temp/CodeGen/Game/")
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


			project "GameFrameWorks"
				includedirs{ "Source/Runtime/GameFrameWorks/", "Source/ThirdParty", "Source/", "Temp/CodeGen/GameFrameWorks/", "Source/Runtime/Core/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/GUI/", "Temp/CodeGen/GUI/", }
				links{ "Core", "Asset", "Graphics", "GUI", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/GameFrameWorks/", {"_GAMEFRAMEWORKS", }, "Temp/CodeGen/GameFrameWorks/")
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
				includedirs{ "Source/Runtime/Graphics/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Graphics/", "Source/Runtime/Core/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", }
				links{ "Core", "Asset", }
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
				includedirs{ "Source/Runtime/GUI/", "Source/ThirdParty", "Source/", "Temp/CodeGen/GUI/", "Source/Runtime/Core/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", }
				links{ "Core", "Asset", "Graphics", }
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




