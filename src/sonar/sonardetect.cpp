#include "sonardetect.h"

#include "stdio.h"

void sonardetect_detect(uint64_t timestamp, uint8_t* sonar_data, uint32_t n_beams, uint32_t resolution, const GstSonarMetaData *meta_data, const GstSonarTelemetry* tel)
{
  int16_t* beam_intensities = (int16_t*)(sonar_data + sizeof(packet_header_t) + sizeof(fls_data_header_t));
  float* beam_angles = (float*)(beam_intensities + n_beams * resolution);

  for (int beam_index=0; beam_index < n_beams; ++beam_index)
  {
    int limit = 0;

    for (int range_index=0; range_index < resolution; ++range_index)
    {
      int16_t* beam_intensity = beam_intensities + range_index * n_beams + beam_index;
      limit += *beam_intensity;

      // draw cross
      //if ((beam_index == (int)(n_beams / 2))
      //  || range_index == (int)(resolution / 2))
      //  *beam_intensity = 5000;

      
    }
    limit /= resolution;

    limit *= 1.7;

    bool found_largest_intensity = false;
    for (int range_index=1; range_index < resolution; ++range_index)
    {
      int16_t* beam_intensity = beam_intensities + range_index * n_beams + beam_index;

      if ((*beam_intensity > limit) && (beam_intensities[(range_index - 1) * n_beams + beam_index] < limit))
        found_largest_intensity = true;
        

      //if (*beam_intensity == largest_signal)
      //  found_largest_intensity = true;

      if (found_largest_intensity)
        *beam_intensity *= 3;
    }
  }
}
