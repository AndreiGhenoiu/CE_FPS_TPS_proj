cmake_minimum_required (VERSION 3.6.2)
if(${CMAKE_GENERATOR} MATCHES "^Visual Studio")
	ADD_DEFINITIONS("/ZI")
endif()
set(CRYENGINE_DIR "C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/CRYENGINE_5.4")
set(TOOLS_CMAKE_DIR "${CRYENGINE_DIR}/Tools/CMake")

set(PROJECT_BUILD 1)
set(PROJECT_DIR "C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/stream_mixamo5.4 (1)")

include("${TOOLS_CMAKE_DIR}/CommonOptions.cmake")

add_subdirectory("${CRYENGINE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/CRYENGINE")

include("${TOOLS_CMAKE_DIR}/Configure.cmake")
start_sources()

sources_platform(ALL)
add_sources("Code_uber.cpp"
    PROJECTS Game
    SOURCE_GROUP "Root"
		"GamePlugin.cpp"
		"StdAfx.cpp"
		"UserSettings.cpp"
		"GamePlugin.h"
		"StdAfx.h"
		"UserSettings.h"
)
add_sources("Attachments_uber.cpp"
    PROJECTS Game
    SOURCE_GROUP "Attachments"
		"Attachments/Flashlight.cpp"
		"Attachments/Torch.cpp"
		"Attachments/Flashlight.h"
		"Attachments/Torch.h"
)
add_sources("Components_uber.cpp"
    PROJECTS Game
    SOURCE_GROUP "Components"
		"Components/DestroyableComponent.cpp"
		"Components/Player.cpp"
		"Components/PlayerInput.cpp"
		"Components/PlayerMovement.cpp"
		"Components/PlayerUpdate.cpp"
		"Components/SpawnPoint.cpp"
		"Components/SurveillanceCamera.cpp"
		"Components/SurveillanceComponent.cpp"
		"Components/Bullet.h"
		"Components/DestroyableComponent.h"
		"Components/Player.h"
		"Components/SpawnPoint.h"
		"Components/SurveillanceCamera.h"
		"Components/SurveillanceComponent.h"
)

end_sources()

CryEngineModule(Game PCH "StdAfx.cpp" SOLUTION_FOLDER "Project")

target_include_directories(${THIS_PROJECT}
PRIVATE 
    "${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
    "${CRYENGINE_DIR}/Code/CryEngine/CryAction"
	"${CRYENGINE_DIR}/Code/CryEngine/CrySchematyc/Core/Interface"
	"${CRYENGINE_DIR}/Code/CryPlugins/CryDefaultEntities/Module"
)

# Set StartUp project in Visual Studio

add_library(GameLauncher STATIC "${CRYENGINE_DIR}/Code/CryEngine/CryCommon/CryCore/Platform/platform.h")
set_target_properties(GameLauncher PROPERTIES LINKER_LANGUAGE CXX)
if (WIN32)
    set_visual_studio_debugger_command(GameLauncher "${CRYENGINE_DIR}/bin/win_x64/GameLauncher.exe" "-project \"C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/stream_mixamo5.4 (1)/Game.cryproject\"")
endif()

add_library(Sandbox STATIC "${CRYENGINE_DIR}/Code/CryEngine/CryCommon/CryCore/Platform/platform.h")
set_target_properties(Sandbox PROPERTIES LINKER_LANGUAGE CXX)
if (WIN32)
    set_visual_studio_debugger_command(Sandbox "${CRYENGINE_DIR}/bin/win_x64/Sandbox.exe" "-project \"C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/stream_mixamo5.4 (1)/Game.cryproject\"")
endif()

add_library(GameServer STATIC "${CRYENGINE_DIR}/Code/CryEngine/CryCommon/CryCore/Platform/platform.h")
set_target_properties(GameServer PROPERTIES LINKER_LANGUAGE CXX)
if (WIN32)
    set_visual_studio_debugger_command(GameServer "${CRYENGINE_DIR}/bin/win_x64/Game_Server.exe" "-project \"C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/stream_mixamo5.4 (1)/Game.cryproject\"")
endif()

set_solution_startup_target(GameLauncher)

if (WIN32)
    set_visual_studio_debugger_command( ${THIS_PROJECT} "${CRYENGINE_DIR}/bin/win_x64/GameLauncher.exe" "-project \"C:/Program Files (x86)/Crytek/CRYENGINE Launcher/Crytek/stream_mixamo5.4 (1)/Game.cryproject\"" )
endif()

#BEGIN-CUSTOM
# Make any custom changes here, modifications outside of the block will be discarded on regeneration.
#END-CUSTOM