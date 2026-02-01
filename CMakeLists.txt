cmake_minimum_required(VERSION 3.7)

project(SonicMania)
set(GAME_NAME SonicMania CACHE STRING "The game directory to look into")
set(OUTPUT_NAME "Game" CACHE STRING "The name of the built library")

# 1. Source Files
set(GAME_SOURCES
    ${GAME_NAME}/Game.c
    ${GAME_NAME}/Objects/All.c
)

# 2. Include Directories (From vcxproj "AdditionalIncludeDirectories")
target_include_directories(${GAME_NAME} PRIVATE
    ${GAME_NAME}/
    ${GAME_NAME}/Objects/
)

# 3. CRITICAL FIX: Preprocessor Definitions
# These match the "PreprocessorDefinitions" in your vcxproj.
# RETRO_STANDALONE=1 is required to prevent the main menu crash.
add_definitions(
    -DRETRO_REVISION=3
    -DRETRO_STANDALONE=1
    -DSONICMANIA_EXPORTS
    -DNDEBUG
    -D_CRT_SECURE_NO_WARNINGS
)

# 4. Logic: Web (WASM) vs PC
if(EMSCRIPTEN)
    # Uses the helper file you created earlier
    include(RSDK-Library-GameAPI.cmake)
else()
    # Standard PC Build
    add_library(${GAME_NAME} SHARED ${GAME_SOURCES})
endif()

# 5. Output Name
set_target_properties(${GAME_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

unset(GAME_NAME CACHE)
unset(OUTPUT_NAME CACHE)
