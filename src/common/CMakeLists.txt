set(SRC
    linalg.cpp
    sonarmeta.c
	sonarshared.c
    )

set(HDR
    )

set(PUBLIC_HDR
    linalg.h
    sonarmeta.h
	sonarshared.h
    )

target_sources(${TARGET_LIB}
    PRIVATE ${SRC} ${HDR}
    PUBLIC ${PUBLIC_HDR}
)