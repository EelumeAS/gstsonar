// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************

// global object for synchronization during initialization
// FIXME: This limits the scalability of the sonar elements
#pragma once

#include <glib.h>

typedef struct
{
    GMutex m;
    guint64 initial_time;
} GstSonarSharedData;

extern GstSonarSharedData gst_sonar_shared_data;

void gst_sonarshared_init();

void gst_sonarshared_finalize();

guint64 gst_sonarshared_set_initial_time(guint64 timestamp);

guint64 gst_sonarshared_get_initial_time();
