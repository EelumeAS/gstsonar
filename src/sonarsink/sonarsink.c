// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
/**
 * SECTION:element-gst_sonarsink
 *
 * Sonarsink visualizes sonar data
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  TODO
 * </refsect2>
 */

#include "sonarsink.h"
#include "openglWp.h"

#include <stdio.h>
#include <math.h>

GST_DEBUG_CATEGORY_STATIC(sonarsink_debug);
#define GST_CAT_DEFAULT sonarsink_debug

#define gst_sonarsink_parent_class parent_class
G_DEFINE_TYPE(GstSonarsink, gst_sonarsink, GST_TYPE_BASE_SINK);

enum
{
    PROP_0,
    PROP_ZOOM,
    PROP_GAIN,
};

#define DEFAULT_PROP_ZOOM 1
#define DEFAULT_PROP_GAIN 1

const double rad2deg = 180.0/M_PI;
const double deg2rad = M_PI/180.0;
bool metadata_warning_shown = false;

static GstStaticPadTemplate gst_sonarsink_sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("sonar/multibeam ; sonar/bathymetry"));

static GstFlowReturn gst_sonarsink_render(GstBaseSink* basesink, GstBuffer* buf)
{
    GstSonarsink* sonarsink = GST_SONARSINK(basesink);

    GST_OBJECT_LOCK(sonarsink);

    // GstSonarMeta *meta = GST_SONAR_META_GET(buf);
    const GstSonarMeta* meta     = GST_SONAR_META_GET(buf);
    const GstSonarFormat* format = &meta->format;
    const GstSonarParams* params = &meta->params;

    //GST_DEBUG_OBJECT(sonarsink, "%lu: rendering buffer: %p, width n_beams = %d, resolution = %d, sound_speed = %f, sample_rate = %f, t0 = %d, gain = %f", buf->pts, buf, sonarsink->n_beams,
    //    sonarsink->resolution, params->sound_speed, params->sample_rate, params->t0, params->gain);

    GstMapInfo mapinfo;
    if (!gst_buffer_map(buf, &mapinfo, GST_MAP_READ))
    {
        GST_OBJECT_UNLOCK(sonarsink);
        return GST_FLOW_ERROR;
    }

    GstTelemetryMeta* tele_meta = GST_TELEMETRY_META_GET(buf);
    if(tele_meta != NULL)
    {
        GST_INFO_OBJECT(sonarsink, "telemetry: lat: %.6f  long: %.6f  roll: %.2f  pitch: %.2f  heading: %.2f  depth: %.2f m  altitude: %.2f m", 
            tele_meta->tel.latitude,  tele_meta->tel.longitude,
             tele_meta->tel.roll*rad2deg, tele_meta->tel.pitch*rad2deg,  tele_meta->tel.yaw*rad2deg,
             tele_meta->tel.depth, tele_meta->tel.altitude);
    }
    else
    {
        if(!metadata_warning_shown)
        {
            GST_INFO_OBJECT(sonarsink, "telemetry not available");
        }
        metadata_warning_shown = true;
    }

    //GST_TRACE_OBJECT(sonarsink, "timestamp TX: %lu     timestamp net: %lu   ping_number: %i", params->time, params->network_time, params->ping_number);

    switch (sonarsink->sonar_type)
    {
        case GST_SONAR_TYPE_FLS:
        {
            const float max_range = ((params->t0 + sonarsink->resolution) * params->sound_speed) / (2 * params->sample_rate);

            const float total_gain = sonarsink->gain / params->gain;

            for (int range_index = 0; range_index < sonarsink->resolution; ++range_index)
            {
                for (int beam_index = 0; beam_index < sonarsink->n_beams; ++beam_index)
                {
                    float beam_intensity = gst_sonar_format_get_measurement(format, mapinfo.data, beam_index, range_index);
                    float beam_angle     = gst_sonar_format_get_angle(format, mapinfo.data, beam_index);
                    float range          = ((params->t0 + range_index) * params->sound_speed) / (2 * params->sample_rate);

                    int vertex_index = 3 * (beam_index * sonarsink->resolution + range_index);
                    float* vertex    = sonarsink->vertices + vertex_index;

                    float range_norm = range / max_range;

                    vertex[0] = -sin(beam_angle) * range_norm * sonarsink->zoom;
                    vertex[1] = -1 + cos(beam_angle) * range_norm * sonarsink->zoom;
                    vertex[2] = -1;
                
                    float I = total_gain * beam_intensity;
                    if (I > 1)
                    {
                        //GST_TRACE_OBJECT(sonarsink, "intensity too large: %f > %f", beam_intensity, total_gain);
                        I = 1;
                        // gst_buffer_unmap (buf, &mapinfo);
                        // GST_OBJECT_UNLOCK (sonarsink);
                        // return GST_FLOW_ERROR;
                    }

                    float* color = sonarsink->colors + vertex_index;
                    if (sonarsink->detected)
                    {
                        // the index of the detected first point of contact is stored in the first range_index for each beam
                        float first_contact = gst_sonar_format_get_measurement(format, mapinfo.data, beam_index, 0);
                        if (range_index >= first_contact)
                        {
                            // red
                            color[0] = I;
                            color[1] = 0;
                            color[2] = 0;
                        }
                        else if (range_index == 0)
                        {
                            // black
                            color[0] = 0;
                            color[1] = 0;
                            color[2] = 0;
                        }
                        else
                        {
                            // yellow
                            color[0] = I;
                            color[1] = I;
                            color[2] = 0;
                        }
                    }
                    else
                    {
                        // white
                        color[0] = I;
                        color[1] = I;
                        color[2] = I;
                    }
                }
            }
            break;
        }
        case GST_SONAR_TYPE_BATHYMETRY:
        {
            g_assert(sonarsink->resolution == 1);

            for (int beam_index = 0; beam_index < sonarsink->n_beams; ++beam_index)
            {
                float sample_number = gst_sonar_format_get_measurement(format, mapinfo.data, beam_index, 0);
                float angle         = gst_sonar_format_get_angle(format, mapinfo.data, beam_index);
                uint8_t quality     = gst_sonar_format_get_quality_val(format, mapinfo.data, beam_index);
                float intensity     = gst_sonar_format_get_intensity(format, mapinfo.data, beam_index);

                float range = (sample_number * params->sound_speed) / (2 * params->sample_rate);

                int vertex_index = 3 * beam_index;
                float* vertex    = sonarsink->vertices + vertex_index;

                vertex[0] = sin(angle) * range * sonarsink->zoom;
                vertex[1] = -cos(angle) * range * sonarsink->zoom;
                vertex[2] = -1;

                const float quality_color = (quality <= 1 ? 1.0f : 0.0f) ;  // Color bad quality points red 
                float* color = sonarsink->colors + vertex_index;
                color[0]     = 1.0f * fmin(40.0f, intensity) / 40.0f;
                color[1]     = quality_color * fmin(40.0f, intensity) / 40.0f;
                color[2]     = quality_color * fmin(40.0f, intensity) / 40.0f;
            }
            break;
        }
    }

    gst_buffer_unmap(buf, &mapinfo);

    // update graphic
    updateWp(sonarsink->vertices, sonarsink->colors, sonarsink->n_beams * sonarsink->resolution);

    // take input from window
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                GST_OBJECT_UNLOCK(sonarsink);
                return GST_FLOW_EOS;
                break;
            case SDL_KEYDOWN:
            {
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        GST_OBJECT_UNLOCK(sonarsink);
                        return GST_FLOW_EOS;
                    case SDLK_SPACE:
                    {
                        GstMessage* msg = gst_message_new_request_state(GST_OBJECT_CAST(sonarsink), sonarsink->playpause);
                        gst_element_post_message(GST_ELEMENT_CAST(sonarsink), msg);
                        sonarsink->playpause = sonarsink->playpause == GST_STATE_PAUSED ? GST_STATE_PLAYING : GST_STATE_PAUSED;
                    }
                    break;
                }
                break;
            }
        }
    }

    GST_OBJECT_UNLOCK(sonarsink);

    return GST_FLOW_OK;
}

static gboolean gst_sonarsink_set_caps(GstBaseSink* basesink, GstCaps* caps)
{
    GstSonarsink* sonarsink = GST_SONARSINK(basesink);

    GstStructure* s = gst_caps_get_structure(caps, 0);

    GST_DEBUG_OBJECT(sonarsink, "caps structure: %s\n", gst_structure_to_string(s));

    gint n_beams, resolution;
    const gchar* caps_name;

    if ((caps_name = gst_structure_get_name(s)) && gst_structure_get_int(s, "n_beams", &n_beams) && gst_structure_get_int(s, "resolution", &resolution))
    {
        GST_OBJECT_LOCK(sonarsink);
        guint32 old_n_beams    = sonarsink->n_beams;
        guint32 old_resolution = sonarsink->resolution;

        GST_DEBUG_OBJECT(sonarsink, "got caps details caps_name: %s, n_beams: %d, resolution: %d", caps_name, n_beams, resolution);
        sonarsink->n_beams    = (guint32)n_beams;
        sonarsink->resolution = (guint32)resolution;

        if ((sonarsink->n_beams != old_n_beams) || (sonarsink->resolution != old_resolution))
        {
            const int size = sonarsink->n_beams * sonarsink->resolution * 3 * sizeof(sonarsink->vertices[0]);

            free(sonarsink->vertices);
            free(sonarsink->colors);
            sonarsink->vertices = (float*)malloc(size);
            sonarsink->colors   = (float*)malloc(size);
        }

        if (strcmp(caps_name, "sonar/multibeam") == 0)
            sonarsink->sonar_type = GST_SONAR_TYPE_FLS;
        else if (strcmp(caps_name, "sonar/bathymetry") == 0)
            sonarsink->sonar_type = GST_SONAR_TYPE_BATHYMETRY;
        else
            g_assert_not_reached();

        gboolean detected;
        if (gst_structure_get_boolean(s, "detected", &detected))
            sonarsink->detected = detected;

        GST_OBJECT_UNLOCK(sonarsink);

        // initialize visualization once
        if (sonarsink->init_wp)
        {
            sonarsink->init_wp = 0;
            int rc             = initWp();
            g_assert(rc == 0);
        }

        return TRUE;
    }
    else
    {
        GST_DEBUG_OBJECT(sonarsink, "no details in caps\n");

        return FALSE;
    }
}

static void gst_sonarsink_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstSonarsink* sonarsink = GST_SONARSINK(object);

    GST_OBJECT_LOCK(sonarsink);
    switch (prop_id)
    {
        case PROP_ZOOM:
            sonarsink->zoom = g_value_get_double(value);
            break;
        case PROP_GAIN:
            sonarsink->gain = g_value_get_double(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(sonarsink);
}

static void gst_sonarsink_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstSonarsink* sonarsink = GST_SONARSINK(object);

    switch (prop_id)
    {
        case PROP_ZOOM:
            g_value_set_double(value, sonarsink->zoom);
            break;
        case PROP_GAIN:
            g_value_set_double(value, sonarsink->gain);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_sonarsink_finalize(GObject* object)
{
    GstSonarsink* sonarsink = GST_SONARSINK(object);

    free(sonarsink->vertices);
    free(sonarsink->colors);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_sonarsink_class_init(GstSonarsinkClass* klass)
{
    GObjectClass* gobject_class       = (GObjectClass*)klass;
    GstElementClass* gstelement_class = (GstElementClass*)klass;
    GstBaseSinkClass* basesink_class  = (GstBaseSinkClass*)klass;

    gobject_class->finalize     = gst_sonarsink_finalize;
    gobject_class->set_property = gst_sonarsink_set_property;
    gobject_class->get_property = gst_sonarsink_get_property;

    basesink_class->render   = GST_DEBUG_FUNCPTR(gst_sonarsink_render);
    basesink_class->set_caps = GST_DEBUG_FUNCPTR(gst_sonarsink_set_caps);

    GST_DEBUG_CATEGORY_INIT(sonarsink_debug, "sonarsink", 0, "sonarsink");

    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_ZOOM,
        g_param_spec_double("zoom", "zoom", "Zoom", G_MINDOUBLE, G_MAXDOUBLE, DEFAULT_PROP_ZOOM, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_GAIN,
        g_param_spec_double("gain", "gain", "Gain", G_MINDOUBLE, G_MAXDOUBLE, DEFAULT_PROP_GAIN, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(gstelement_class, "Sonarsink", "Sink", "visualizes sonar data", "Eelume AS <opensource@eelume.com>");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_sonarsink_sink_template);
}

static void gst_sonarsink_init(GstSonarsink* sonarsink)
{
    sonarsink->n_beams    = 0;
    sonarsink->resolution = 0;
    sonarsink->detected   = FALSE;
    sonarsink->vertices   = NULL;
    sonarsink->colors     = NULL;
    sonarsink->init_wp    = 1;
    sonarsink->playpause  = GST_STATE_PAUSED;

    sonarsink->zoom = DEFAULT_PROP_ZOOM;
    sonarsink->gain = DEFAULT_PROP_GAIN;
}
