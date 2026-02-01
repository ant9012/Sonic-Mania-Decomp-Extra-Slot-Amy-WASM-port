cmake_minimum_required(VERSION 3.7)

project(SonicMania)
set(GAME_NAME SonicMania CACHE STRING "The game directory to look into")
set(OUTPUT_NAME "Game" CACHE STRING "The name of the built library")

# 1. Define Sources
set(GAME_SOURCES
    ${GAME_NAME}/Game.c
    ${GAME_NAME}/Objects/All.c
)

# 2. CREATE TARGET FIRST (This must happen before target_include_directories)
if(EMSCRIPTEN)
    # This creates the target as an Executable (WASM)
    include(RSDK-Library-GameAPI.cmake)
else()
    # This creates the target as a Library (PC)
    add_library(${GAME_NAME} SHARED ${GAME_SOURCES})
endif()

# 3. Configure Includes (Now that the target exists, this will work)
target_include_directories(${GAME_NAME} PRIVATE
    ${GAME_NAME}/
    ${GAME_NAME}/Objects/
)

# 4. Add Definitions (From vcxproj)
add_definitions(
    -DRETRO_REVISION=3
    -DMANIA_USE_PLUS=1    # <--- CRITICAL FIX FOR AMY/PLUS MODS
    -DRETRO_STANDALONE=1
    -DSONICMANIA_EXPORTS
    -DNDEBUG
    -D_CRT_SECURE_NO_WARNINGS
)

# 5. Set Output Name
set_target_properties(${GAME_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

unset(GAME_NAME CACHE)
unset(OUTPUT_NAME CACHE)
