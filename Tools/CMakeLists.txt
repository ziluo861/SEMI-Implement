cmake_minimum_required(VERSION 3.20.0)
project(Tools VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

set(Target "Tools")

add_library(${Target} STATIC)

target_include_directories(${Target} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(
    ${Target} 
    PRIVATE 
        src/FSM.cpp
)

# 将目标名称设为父作用域可见
set(Tools_TARGET ${Target} PARENT_SCOPE)