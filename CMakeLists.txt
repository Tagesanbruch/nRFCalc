# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(display)

# Automatically find all C source files in src/ and subdirectories
file(GLOB_RECURSE APP_SOURCES 
    src/*.c
)

# Add all discovered source files to the target
target_sources(app PRIVATE ${APP_SOURCES})

# Optional: Print discovered source files for debugging (can be removed later)
message(STATUS "Discovered source files: ${APP_SOURCES}")
