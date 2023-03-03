#include <stdint.h>
#include <gst/gst.h>

typedef struct
{
    enum __attribute__((__packed__)) // one byte enum
    {
        SBD_NMEA_EIHEA = 2,
        SBD_NMEA_EIORI = 3,
        SBD_NMEA_EIDEP = 4,
        SBD_NMEA_EIPOS = 8,
        SBD_WBMS_BATH = 9,
        SBD_WBMS_FLS = 10,
        SBD_HEADER = 21,
    } entry_type;

    char dont_care[3];
    int32_t relative_time;

    struct
    {
        int32_t tv_sec;
        int32_t tv_usec;
    } absolute_time;

    uint32_t entry_size;
} sbd_entry_header_t;
//static_assert(sizeof(SbdEntryHeader) == 20);

GstBuffer* sbd_entry(GstBuffer *payload);
