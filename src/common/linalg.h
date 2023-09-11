// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float roll;
        float pitch;
        float yaw;
        uint64_t time;
    } linalg_euler_angles_t;

    // linear interpolation
    void linalg_interpolate_euler_angles(linalg_euler_angles_t* out, const linalg_euler_angles_t* first, const linalg_euler_angles_t* second, uint64_t interpolation_time);
    float linalg_interpolate_scalar(float first, uint64_t first_time, float second, uint64_t second_time, uint64_t interpolation_time);

#ifdef __cplusplus
}    // extern "C"
#endif
