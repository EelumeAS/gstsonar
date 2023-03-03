#include "norbit_wbms.h"

GstSonarMeasurementType wbms_get_intensity_type(uint32_t dtype)
{
  switch(dtype)
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
    //case 6:
    //  return GST_SONAR_MEASUREMENT_TYPE_INT64;
    //case 7:
    //  return GST_SONAR_MEASUREMENT_TYPE_INT64;
    case 0x15:
      return GST_SONAR_MEASUREMENT_TYPE_FLOAT32;
    case 0x17:
      return GST_SONAR_MEASUREMENT_TYPE_FLOAT64;
    default:
      return GST_SONAR_MEASUREMENT_TYPE_INVALID;
  }
}
