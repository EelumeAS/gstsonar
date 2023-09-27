set(SRC
    sonarmux.c
    )

set(HDR
    )

set(PUBLIC_HDR
    sonarmux.h
    )

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)