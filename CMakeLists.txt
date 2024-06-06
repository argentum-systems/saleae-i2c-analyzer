cmake_minimum_required (VERSION 3.11)
project(i2c_analyzer_attie)

add_definitions( -DLOGIC2 )

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
	src/Analyzer.cpp
	src/Analyzer.h
	src/Results.cpp
	src/Results.h
	src/Settings.cpp
	src/Settings.h
)

add_analyzer_plugin(i2c_analyzer_attie SOURCES ${SOURCES})
