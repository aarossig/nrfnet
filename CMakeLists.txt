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

include(FindProtobuf)

find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(tclap REQUIRED tclap)

# Subdirectories ###############################################################

add_subdirectory(nerfnet)
