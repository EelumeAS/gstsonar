#include "sonardetect.h"
#include "sonarmeta.h"

#include "stdint.h"

void sonardetect_detect(uint64_t timestamp, char* sonar_data, int n_beams, int resolution, const GstSonarMeta *meta, const GstSonarTelemetry* tel)
{
  const GstSonarFormat *format = &meta->format;
  //const GstSonarParams *params = &meta->params;

  for (int beam_index=0; beam_index < n_beams; ++beam_index)
  {
    int total_intensity = 0;

    for (int range_index=0; range_index < resolution; ++range_index)
    {
      //float beam_angle = gst_sonar_format_get_angle(format, sonar_data, beam_index);
      float beam_intensity = gst_sonar_format_get_measurement(format, (const char*)sonar_data, beam_index, range_index);
      total_intensity += beam_intensity;

      // draw cross
      //if ((beam_index == (int)(n_beams / 2))
      //  || range_index == (int)(resolution / 2))
      //  *beam_intensity = 5000;

      
    }
    const float average_intensity = total_intensity / resolution;
    const float limit = 1.7 * average_intensity;

    bool found_largest_intensity = false;
    for (int range_index=1; range_index < resolution; ++range_index)
    {
      float beam_intensity = gst_sonar_format_get_measurement(format, sonar_data, beam_index, range_index);
      float prev_beam_intensity = gst_sonar_format_get_measurement(format, sonar_data, beam_index, range_index - 1);

      if ((beam_intensity > limit) && (prev_beam_intensity < limit))
      {
        // store the index of the first detected point in the first range index for each beam
        gst_sonar_format_set_measurement(format, sonar_data, beam_index, 0, range_index);
        break;
      }
    }
  }
}
