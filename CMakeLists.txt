# CMakeLists.txt
cmake_minimum_required (VERSION 3.5.1)
project (sonar)


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
ELSE()
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0)
include_directories(${GST_INCLUDE_DIRS})
set(EXT_LIBS ${GST_LIBRARIES} -lGLEW -lGL -lSDL2)
ENDIF()

file(GLOB SONAR_SRC src/sonar/*.c*)
add_library(gstsonar SHARED ${SONAR_SRC})
target_link_libraries(gstsonar ${EXT_LIBS})
install(TARGETS gstsonar DESTINATION lib/gstreamer-1.0)

file(GLOB PLAYSONAR_SRC src/playsonar/*.c*)
add_executable(playsonar ${PLAYSONAR_SRC})
target_link_libraries(playsonar ${EXT_LIBS})
install(TARGETS playsonar DESTINATION bin)
