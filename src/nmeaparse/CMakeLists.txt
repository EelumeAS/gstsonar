set(SRC
    nmeaparse.c
    )

set(HDR        
    )

set(PUBLIC_HDR
    nmeaparse.h
    )

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)