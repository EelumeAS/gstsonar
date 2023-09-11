set(SRC
    nmeaparse.c
    norbit_wbms.c
    sbd.c
    sonarparse.c
    )

set(HDR    
    norbit_wbms.h
    sbd.h
    )

set(PUBLIC_HDR
    nmeaparse.h
    sonarparse.h
    )

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)