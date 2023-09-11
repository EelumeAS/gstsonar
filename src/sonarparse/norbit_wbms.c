// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// Copyright (c) https://github.com/magnuan
// All rights reserved
//
// Parts of this code come from https://github.com/magnuan/wbms_georef/blob/main/src/bathy_packet.h
// Parts of this code is licensed under the MIT license. 
//
// Other parts licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#include "norbit_wbms.h"

GstSonarMeasurementType wbms_get_intensity_type(uint32_t dtype)
{
    switch (dtype)
    {
        case 0:
            return GST_SONAR_MEASUREMENT_TYPE_INT8;
        case 1:
            return GST_SONAR_MEASUREMENT_TYPE_INT8;
        case 2:
            return GST_SONAR_MEASUREMENT_TYPE_INT16;
        case 3:
            return GST_SONAR_MEASUREMENT_TYPE_INT16;
        case 4:
            return GST_SONAR_MEASUREMENT_TYPE_INT32;
        case 5:
            return GST_SONAR_MEASUREMENT_TYPE_INT32;
        // TODO
        // case 6:
        //  return GST_SONAR_MEASUREMENT_TYPE_INT64;
        // case 7:
        //  return GST_SONAR_MEASUREMENT_TYPE_INT64;
        case 0x15:
            return GST_SONAR_MEASUREMENT_TYPE_FLOAT32;
        case 0x17:
            return GST_SONAR_MEASUREMENT_TYPE_FLOAT64;
        default:
            return GST_SONAR_MEASUREMENT_TYPE_INVALID;
    }
}
