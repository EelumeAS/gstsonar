# CMakeLists.txt
cmake_minimum_required (VERSION 3.5.1)
project (sonarparse)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-allocators-1.0)


include_directories(parse ${GST_INCLUDE_DIRS})

file(GLOB SOURCES src/*.c)
add_library(gstsonarparse SHARED ${SOURCES})

IF(DEFINED ENV{IMXVPUTEXTOVERLAY_DEBUG})
  add_definitions("-DIMXVPUTEXTOVERLAY_DEBUG=\"${IMXVPUTEXTOVERLAY_DEBUG}\"")
ELSE()
  set(GST_LIBRARIES "${GST_LIBRARIES}")
ENDIF()
target_link_libraries(gstsonarparse ${GST_LIBRARIES})

install(TARGETS gstsonarparse DESTINATION lib/gstreamer-1.0)
