# CMakeLists.txt
cmake_minimum_required (VERSION 3.5.1)
project (sonar)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-allocators-1.0 gstreamer-video-1.0)


include_directories(${GST_INCLUDE_DIRS})

file(GLOB SONAR_SRC src/sonar/*.c*)
add_library(gstsonar SHARED ${SONAR_SRC})
target_link_libraries(gstsonar ${GST_LIBRARIES} -lGLEW -lGL -lSDL2)
install(TARGETS gstsonar DESTINATION lib/gstreamer-1.0)

file(GLOB PLAYSONAR_SRC src/playsonar/*.c*)
add_executable(playsonar ${PLAYSONAR_SRC})
target_link_libraries(playsonar ${GST_LIBRARIES})
install(TARGETS playsonar DESTINATION bin)
