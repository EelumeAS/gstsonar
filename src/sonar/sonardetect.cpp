#include "sonardetect.h"

#include "stdint.h"

void sonardetect_detect(uint64_t timestamp, uint8_t* sonar_data, int n_beams, int resolution, const GstSonarMetaData *meta_data, const GstSonarTelemetry* tel)
{
  int16_t* beam_intensities = (int16_t*)(sonar_data + sizeof(wbms_packet_header_t) + sizeof(wbms_fls_data_header_t));
  float* beam_angles = (float*)(beam_intensities + n_beams * resolution);

  for (int beam_index=0; beam_index < n_beams; ++beam_index)
  {
    int total_intensity = 0;

    for (int range_index=0; range_index < resolution; ++range_index)
    {
      int16_t* beam_intensity = beam_intensities + range_index * n_beams + beam_index;
      total_intensity += *beam_intensity;

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
      int16_t* beam_intensity = beam_intensities + range_index * n_beams + beam_index;

      if ((*beam_intensity > limit) && (beam_intensity[-n_beams] < limit))
      {
        // store the index of the first detected point in the first range index for each beam
        beam_intensities[beam_index] = range_index;
        break;
      }
    }
  }
}
