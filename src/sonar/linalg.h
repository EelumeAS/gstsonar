
#ifdef __cplusplus
extern "C" {
#endif

void linalg_interpolate_euler_angles(
  float* out_roll, float* out_pitch, float* out_yaw
  , float first_roll, float first_pitch, float first_yaw
  , float second_roll, float second_pitch, float second_yaw);


#ifdef __cplusplus
} // extern "C"
#endif
