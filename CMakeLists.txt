################################################################################
#
# nerfnet build
#
################################################################################

# Project Settings #############################################################

cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)

project(nerfnet)

# Dependencies #################################################################

enable_testing()

include(FindProtobuf)
include(GoogleTest)

find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(tclap REQUIRED tclap)

message(${GTEST_INCLUDE_DIRS})

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

# Compile Flags ################################################################

add_compile_options(-Wno-psabi)

# Subdirectories ###############################################################

add_subdirectory(nerfnet)
