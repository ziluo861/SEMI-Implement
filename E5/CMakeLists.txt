cmake_minimum_required(VERSION 3.20.0)
project(E5 VERSION 0.1.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

set(Target "E5")

add_library(${Target} STATIC)

target_include_directories(${Target} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(${Target} PRIVATE 
    src/DataType/FormatCode.cpp
    src/SECSItems/ASCIIItem.cpp
    src/SECSItems/BinaryItem.cpp
    src/SECSItems/BooleanItem.cpp
    src/SECSItems/CharactorItem.cpp
    src/SECSItems/DoubleItem.cpp
    src/SECSItems/FloatItem.cpp
    src/SECSItems/ListItem.cpp
    src/SECSItems/Int8Item.cpp
    src/SECSItems/Int16Item.cpp
    src/SECSItems/Int32Item.cpp
    src/SECSItems/Int64Item.cpp
    src/SECSItems/UInt8Item.cpp
    src/SECSItems/UInt16Item.cpp
    src/SECSItems/UInt32Item.cpp
    src/SECSItems/UInt64Item.cpp
    src/SECS/SECSItem.cpp
    src/SECS/SECSFactory.cpp
    src/SECSHead/SECSHead.cpp
    src/SECSMessageHandleItem.cpp
)

# 将目标名称设为父作用域可见
set(E5_TARGET ${Target} PARENT_SCOPE)