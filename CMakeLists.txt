# CMakeLists.txt
cmake_minimum_required (VERSION 3.5.1)
project (sonar)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-allocators-1.0)


include_directories(src ${GST_INCLUDE_DIRS})

file(GLOB SOURCES src/*.c)
add_library(gstsonar SHARED ${SOURCES})

IF(DEFINED ENV{IMXVPUTEXTOVERLAY_DEBUG})
  add_definitions("-DIMXVPUTEXTOVERLAY_DEBUG=\"${IMXVPUTEXTOVERLAY_DEBUG}\"")
ELSE()
  set(GST_LIBRARIES "${GST_LIBRARIES}")
ENDIF()
target_link_libraries(gstsonar ${GST_LIBRARIES})

install(TARGETS gstsonar DESTINATION lib/gstreamer-1.0)
