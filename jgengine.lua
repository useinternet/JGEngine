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
        objdir(OBJECT_PATH)
        pchheader (PCH_HEADER)
        pchsource (PCH_CPP_PATH)
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
        objdir(OBJECT_PATH)
        pchheader (PCH_HEADER)
        pchsource (PCH_CPP_PATH)
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
				includedirs{ "Source/Programs/JGConsole/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", }
				links{ "Core", }
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
			project "JGDev_Graphics"
				includedirs{ "Source/Editor/JGDev_Graphics/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGDev_Graphics/", "Source/Runtime/Core/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", "Source/Runtime/Asset/", "Temp/CodeGen/Asset/", }
				links{ "Core", "Graphics", "Asset", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/JGDev_Graphics/", {"_JGDEV_GRAPHICS", }, "Temp/CodeGen/JGDev_Graphics/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
					
				postbuildcommands {
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\JGDev_Graphics_Dynamic.dll"
}
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\JGDev_Graphics_Dynamic.dll"
}
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\JGDev_Graphics_Dynamic.dll"
}
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGDev_Graphics.dll $(TargetDir)..\\JGDev_Graphics_Dynamic.dll"
}


			project "JGEditor"
				includedirs{ "Source/Editor/JGEditor/", "Source/ThirdParty", "Source/", "Temp/CodeGen/JGEditor/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWorks/", "Source/Runtime/Graphics/", "Temp/CodeGen/Graphics/", }
				links{ "Core", "GameFrameWorks", "Graphics", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Editor/JGEditor/", {"_JGEDITOR", }, "Temp/CodeGen/JGEditor/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
					
				postbuildcommands {
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\JGEditor_Dynamic.dll"
}
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\JGEditor_Dynamic.dll"
}
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\JGEditor_Dynamic.dll"
}
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.lib $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.exp $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.pdb $(TargetDir)..\\",
					"copy $(TargetDir)JGEditor.dll $(TargetDir)..\\JGEditor_Dynamic.dll"
}


		group "Engine/Runtime"
			project "Asset"
				includedirs{ "Source/Runtime/Asset/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Asset/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Asset/", {"_ASSET", }, "Temp/CodeGen/Asset/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
					
				postbuildcommands {
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\",
					"copy $(TargetDir)Asset.lib $(TargetDir)..\\",
					"copy $(TargetDir)Asset.exp $(TargetDir)..\\",
					"copy $(TargetDir)Asset.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\Asset_Dynamic.dll"
}
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\",
					"copy $(TargetDir)Asset.lib $(TargetDir)..\\",
					"copy $(TargetDir)Asset.exp $(TargetDir)..\\",
					"copy $(TargetDir)Asset.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\Asset_Dynamic.dll"
}
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\",
					"copy $(TargetDir)Asset.lib $(TargetDir)..\\",
					"copy $(TargetDir)Asset.exp $(TargetDir)..\\",
					"copy $(TargetDir)Asset.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\Asset_Dynamic.dll"
}
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\",
					"copy $(TargetDir)Asset.lib $(TargetDir)..\\",
					"copy $(TargetDir)Asset.exp $(TargetDir)..\\",
					"copy $(TargetDir)Asset.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Asset.dll $(TargetDir)..\\Asset_Dynamic.dll"
}


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


			project "Game"
				includedirs{ "Source/Runtime/Game/", "Source/ThirdParty", "Source/", "Temp/CodeGen/Game/", "Source/Runtime/Core/", "Source/Runtime/GameFrameWorks/", }
				links{ "Core", "GameFrameWorks", }
				SetDynamicCPPProjectConfig("SharedLib", "Source/Runtime/Game/", {"_GAME", }, "Temp/CodeGen/Game/")
				filter "configurations:DevelopEngine"
					DebugConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPENGINE", }
					
				postbuildcommands {
					"copy $(TargetDir)Game.dll $(TargetDir)..\\",
					"copy $(TargetDir)Game.lib $(TargetDir)..\\",
					"copy $(TargetDir)Game.exp $(TargetDir)..\\",
					"copy $(TargetDir)Game.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Game.dll $(TargetDir)..\\Game_Dynamic.dll"
}
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Game.dll $(TargetDir)..\\",
					"copy $(TargetDir)Game.lib $(TargetDir)..\\",
					"copy $(TargetDir)Game.exp $(TargetDir)..\\",
					"copy $(TargetDir)Game.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Game.dll $(TargetDir)..\\Game_Dynamic.dll"
}
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Game.dll $(TargetDir)..\\",
					"copy $(TargetDir)Game.lib $(TargetDir)..\\",
					"copy $(TargetDir)Game.exp $(TargetDir)..\\",
					"copy $(TargetDir)Game.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Game.dll $(TargetDir)..\\Game_Dynamic.dll"
}
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Game.dll $(TargetDir)..\\",
					"copy $(TargetDir)Game.lib $(TargetDir)..\\",
					"copy $(TargetDir)Game.exp $(TargetDir)..\\",
					"copy $(TargetDir)Game.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Game.dll $(TargetDir)..\\Game_Dynamic.dll"
}


			project "GameFrameWorks"
				includedirs{ "Source/Runtime/GameFrameWorks/", "Source/ThirdParty", "Source/", "Source/Runtime/Core/", }
				links{ "Core", }
				SetCPPProjectConfig("StaticLib", "Source/Runtime/GameFrameWorks/", {"_GAMEFRAMEWORKS", })
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
					
				postbuildcommands {
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\Graphics_Dynamic.dll"
}
				filter "configurations:DevelopGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_DEVELOPGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\Graphics_Dynamic.dll"
}
				filter "configurations:ConfirmGame"
					ConfirmConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_CONFIRMGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\Graphics_Dynamic.dll"
}
				filter "configurations:ReleaseGame"
					ReleaseConfig()
					defines{"_PLATFORM_WINDOWS", "_DIRECTX12", "_JGPROJECT", "_RELEASEGAME", }
					
				postbuildcommands {
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.lib $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.exp $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.pdb $(TargetDir)..\\",
					"copy $(TargetDir)Graphics.dll $(TargetDir)..\\Graphics_Dynamic.dll"
}




