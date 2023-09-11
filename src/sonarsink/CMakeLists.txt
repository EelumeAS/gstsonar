set(SRC
    sonarsink.c
    )

set(HDR
    openglWp.h
    )

set(PUBLIC_HDR
    sonarsink.h)

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)