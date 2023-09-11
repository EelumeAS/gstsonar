// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// Copyright (c) https://github.com/magnuan
// All rights reserved
//
// Parts of this code is adapted from https://github.com/magnuan/wbms_georef/blob/main/src/eelume_sbd_nav.c
// Parts of this code is licensed under the MIT license. 
//
// Other parts licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_SBD_H__
#define __GST_SBD_H__

#include <stdint.h>

#include <gst/gst.h>

typedef struct
{
    enum __attribute__((__packed__))    // one byte enum
    {
        SBD_NMEA_EIHEA = 2,
        SBD_NMEA_EIORI = 3,
        SBD_NMEA_EIDEP = 4,
        SBD_NMEA_EIPOS = 8,
        SBD_WBMS_BATH  = 9,
        SBD_WBMS_FLS   = 10,
        SBD_HEADER     = 21,
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
// static_assert(sizeof(SbdEntryHeader) == 20);

GstBuffer* sbd_entry(GstBuffer* payload);

#endif /* __GST_SBD_H__*/