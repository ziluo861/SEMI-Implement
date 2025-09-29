cmake_minimum_required(VERSION 3.20.0)
project(Semi VERSION 0.1.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

set(TARGET ${PROJECT_NAME}-${CMAKE_SYSTEM_NAME})

if(APPLE)
  set(CMAKE_AR     "/usr/bin/ar"     CACHE FILEPATH "Apple ar"     FORCE)
  set(CMAKE_RANLIB "/usr/bin/ranlib" CACHE FILEPATH "Apple ranlib" FORCE)
endif()

option(SEMI_ENABLE_E5 "Build SEMI E5 Library" true)

if (SEMI_ENABLE_E5) 
    message(STATUS "Build SEMI E5 Library")
    add_subdirectory(E5)
endif()

add_subdirectory(Tools)

add_executable(${TARGET} main.cpp)


if (SEMI_ENABLE_E5) 
    message(STATUS "Linking SEMI E5 Library")
    target_link_libraries(${TARGET} PRIVATE E5)
endif()

target_link_libraries(${TARGET} PRIVATE Tools)