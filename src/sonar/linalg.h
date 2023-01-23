
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  float x;
  float y;
  float z;
} linalg_rotation_vector_t;

linalg_rotation_vector_t linalg_calculate_rotation_vector(float roll, float pitch, float yaw);


#ifdef __cplusplus
} // extern "C"
#endif
