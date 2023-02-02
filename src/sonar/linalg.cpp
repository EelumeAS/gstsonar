
#include <Eigen/Dense>
#include "linalg.h"

Eigen::Vector3f linalg_calculate_rotation_vector(float roll, float pitch, float yaw)
{
  Eigen::Quaternionf q = 
      Eigen::AngleAxisf{roll, Eigen::Vector3f::UnitX()} *
      Eigen::AngleAxisf{pitch, Eigen::Vector3f::UnitY()} *
      Eigen::AngleAxisf{yaw, Eigen::Vector3f::UnitZ()};

  Eigen::AngleAxisf axis_angle(q);

  return axis_angle.axis() * axis_angle.angle();
}

void linalg_calculate_euler_angles(float* roll, float* pitch, float* yaw, const Eigen::Vector3f& rotation_vector)
{

  Eigen::AngleAxisf axis_angle = Eigen::AngleAxisf(rotation_vector.norm(), rotation_vector.normalized());

  Eigen::Quaternionf q = Eigen::Quaternionf(axis_angle);

  auto euler = q.toRotationMatrix().eulerAngles(0, 1, 2);

  *roll = euler.x();
  *pitch = euler.y();
  *yaw = euler.z();
}

void linalg_interpolate_euler_angles(
  float* out_roll, float* out_pitch, float* out_yaw
  , float first_roll, float first_pitch, float first_yaw
  , float second_roll, float second_pitch, float second_yaw
  )
{
  auto first_rotation = linalg_calculate_rotation_vector(first_roll, first_pitch, first_yaw);
  printf("rotation vector: %f, %f, %f", first_rotation.x(), first_rotation.y(), first_rotation.z());
  auto second_rotation = linalg_calculate_rotation_vector(second_roll, second_pitch, second_yaw);

  auto out_rotation = .5 * first_rotation + .5 * second_rotation;

  linalg_calculate_euler_angles(out_roll, out_pitch, out_yaw, out_rotation);
}
