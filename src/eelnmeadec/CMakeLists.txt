set(SRC
    eelnmeadec.c
    )

set(HDR        
    )

set(PUBLIC_HDR
    eelnmeadec.h
    )

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)