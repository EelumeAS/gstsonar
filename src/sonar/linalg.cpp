
#include <Eigen/Dense>
#include "linalg.h"

linalg_rotation_vector_t linalg_calculate_rotation_vector(float roll, float pitch, float yaw)
{
  Eigen::AngleAxisf axis_angle = 
    Eigen::AngleAxisf(Eigen::Quaternionf
    (
      Eigen::AngleAxisf{-roll, Eigen::Vector3f::UnitX()} *
      Eigen::AngleAxisf{-pitch, Eigen::Vector3f::UnitY()} *
      Eigen::AngleAxisf{-yaw, Eigen::Vector3f::UnitZ()}
    ).normalized()
  );

  Eigen::Vector3f rotation_vector = axis_angle.axis() * axis_angle.angle();

  return linalg_rotation_vector_t
  {
    .x = rotation_vector.x(),
    .y = rotation_vector.y(),
    .z = rotation_vector.z(),
  };
}
