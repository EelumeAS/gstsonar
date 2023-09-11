// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#include "sbd.h"
#include "norbit_wbms.h"
#include "common/sonarshared.h"

#include "stdio.h"

GstBuffer* sbd_entry(GstBuffer* payload)
{
    if (!payload)
        return NULL;

    GstMapInfo mapinfo;
    if (!gst_buffer_map(payload, &mapinfo, GST_MAP_READ))
        return NULL;

    sbd_entry_header_t* header = (sbd_entry_header_t*)malloc(sizeof(sbd_entry_header_t));
    *header                    = (sbd_entry_header_t){
        .relative_time = payload->pts / 1e6,
        .absolute_time =
            {
                (payload->pts + gst_sonarshared_get_initial_time()) / 1e9,
                (payload->pts % (guint64)1e9) / 1e3,
            },
        .entry_size = mapinfo.size,
    };
    _Static_assert(sizeof(*header) == 20);

    int padding_size = 0;

    g_assert(mapinfo.size > 8);
    if (*(guint32*)mapinfo.data == 0xdeadbeef)
    {
        switch (((wbms_packet_header_t*)mapinfo.data)->type)
        {
            case WBMS_BATH:
                header->entry_type = SBD_WBMS_BATH;
                break;
            case WBMS_FLS:
                header->entry_type = SBD_WBMS_FLS;
                break;
            default:
                g_assert_not_reached();
        }
    }
    else if (strncmp(mapinfo.data, "$EIHEA", 6) == 0)
    {
        header->entry_type = SBD_NMEA_EIHEA;
        padding_size       = 20;
    }
    else if (strncmp(mapinfo.data, "$EIPOS", 6) == 0)
    {
        header->entry_type = SBD_NMEA_EIPOS;
        padding_size       = 20;
    }
    else if (strncmp(mapinfo.data, "$EIORI", 6) == 0)
    {
        header->entry_type = SBD_NMEA_EIORI;
        padding_size       = 16;
    }
    else if (strncmp(mapinfo.data, "$EIDEP", 6) == 0)
    {
        header->entry_type = SBD_NMEA_EIDEP;
        padding_size       = 16;
    }
    else
    {
        // assume sbd header
        header->entry_type = SBD_HEADER;
        padding_size       = 0;
    }

    if (padding_size)
    {
        header->entry_size += padding_size;

        char* padding                                   = (char*)malloc(padding_size);
        ((int*)padding)[padding_size / sizeof(int) - 1] = mapinfo.size + 1;

        GstMemory* mysterious_padding = gst_memory_new_wrapped(0,    // flags (GstMemoryFlags)
            padding,                                                 // data
            padding_size,                                            // maxsize
            0,                                                       // offset
            padding_size,                                            // size
            NULL,                                                    // user_data
            NULL);                                                   // notify (GDestroyNotify)

        gst_buffer_prepend_memory(payload, mysterious_padding);
    }

    gst_buffer_unmap(payload, &mapinfo);

    GstMemory* header_mem = gst_memory_new_wrapped(0,    // flags (GstMemoryFlags)
        header,                                          // data
        sizeof(*header),                                 // maxsize
        0,                                               // offset
        sizeof(*header),                                 // size
        NULL,                                            // user_data
        NULL);                                           // notify (GDestroyNotify)

    gst_buffer_prepend_memory(payload, header_mem);

    return payload;
}
