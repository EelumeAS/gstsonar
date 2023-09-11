# ***************************************************************************
# Gstsonar - Gstreamer sonar processing plugins
# 
# Copyright (c) 2023 Eelume AS <opensource@eelume.com>
# All rights reserved
#
# Licensed under the LGPL v2.1 License. 
# See LICENSE file in the project root for full license information.
# ***************************************************************************

# CMakeLists.txt
cmake_minimum_required (VERSION 3.15)

# Set CMAKE_TOOLCHAIN_FILE to point to vcpkg folder
# set(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/vcpkg")
# if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#     # CMAKE_TOOLCHAIN_FILE not defined on commandline, use default location   
#     set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")     
# endif()
# if(NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
#     message("== Delete CMakeCache.txt before retrying...")
#     message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE does not exist: ${CMAKE_TOOLCHAIN_FILE}")
# endif()

project(gstsonar)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

if (WIN32)
    #TODO: don't assume where gstreamer is installed
    set(GSTREAMER_ROOT C:\\gstreamer\\1.0\\msvc_x86_64)
    include_directories(${GSTREAMER_ROOT}\\include)
    include_directories(${GSTREAMER_ROOT}\\include\\glib-2.0)
    include_directories(${GSTREAMER_ROOT}\\lib\\glib-2.0\\include)
    include_directories(${GSTREAMER_ROOT}\\include\\gstreamer-1.0)
    link_directories(${GSTREAMER_ROOT}\\lib)

    set(SDL_ROOT C:\\SDL2-2.26.2)
    include_directories(${SDL_ROOT}\\include)
    link_directories(${SDL_ROOT}\\lib\\x64)

    set(GLEW_ROOT C:\\glew-2.2.0)
    include_directories(${GLEW_ROOT}\\include)
    link_directories(${GLEW_ROOT}\\lib\\Release\\x64)

    set(EXT_LIBS gstbase-1.0 gstreamer-1.0 gobject-2.0 glib-2.0 glew32s OpenGL32 SDL2)

    set(CMAKE_INSTALL_PREFIX ${GSTREAMER_ROOT})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0)
    include_directories(${GST_INCLUDE_DIRS})
    pkg_check_modules(EIGEN REQUIRED eigen3)
    include_directories(${EIGEN_INCLUDE_DIRS})
    set(EXT_LIBS ${GST_LIBRARIES} ${EIGEN_LIBRARIES} -lGLEW -lGL -lSDL2)
endif()


set(TARGET_LIB gstsonar)
add_subdirectory(src)


install(TARGETS ${TARGET_LIB} DESTINATION lib/gstreamer-1.0)
