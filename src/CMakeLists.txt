set(SRC
    plugin.c
    )

set(HDR
    )

set(PUBLIC_HDR
    )


add_library(${TARGET_LIB} SHARED ${SRC})

set_target_properties( ${TARGET_LIB}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(${TARGET_LIB} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(
    ${TARGET_LIB}
    PRIVATE
        ${EXT_LIBS})

add_subdirectory(common)
add_subdirectory(sonarparse)
add_subdirectory(sonarsink)