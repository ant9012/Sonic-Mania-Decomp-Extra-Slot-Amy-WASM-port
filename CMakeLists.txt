cmake_minimum_required(VERSION 3.7)

project(SonicMania)
set(GAME_NAME SonicMania CACHE STRING "The game directory to look into")
set(OUTPUT_NAME "Game" CACHE STRING "The name of the built library")

# 1. Define the source files here so both builds can use them
set(GAME_SOURCES
    ${GAME_NAME}/Game.c
    ${GAME_NAME}/Objects/All.c
)

# 2. Logic: If building for Web (Emscripten), use the helper; otherwise use standard library
if(EMSCRIPTEN)
    # This includes the WASM-specific setup
    include(RSDK-Library-GameAPI.cmake)
else()
    # Standard PC Build
    add_library(${GAME_NAME} SHARED ${GAME_SOURCES})
endif()

# 3. Common Settings (Applied to both Web and PC)
target_include_directories(${GAME_NAME} PRIVATE
    ${GAME_NAME}/
    ${GAME_NAME}/Objects/
)

# Your specific compile options
target_compile_options(${GAME_NAME} PRIVATE -DRETRO_REVISION=3 -DMANIA_PREPLUS=0)

# Ensure the output file is named "Game.wasm" (Web) or "Game.dll" (PC)
set_target_properties(${GAME_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

unset(GAME_NAME CACHE)
unset(OUTPUT_NAME CACHE)
