
// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#include "linalg.h"

#include <Eigen/Dense>

Eigen::Vector3f linalg_calculate_rotation_vector(const linalg_euler_angles_t* angles)
{
    Eigen::Quaternionf q =
        Eigen::AngleAxisf{angles->roll, Eigen::Vector3f::UnitX()} * Eigen::AngleAxisf{angles->pitch, Eigen::Vector3f::UnitY()} * Eigen::AngleAxisf{angles->yaw, Eigen::Vector3f::UnitZ()};

    Eigen::AngleAxisf axis_angle(q);

    return axis_angle.axis() * axis_angle.angle();
}

void linalg_calculate_euler_angles(linalg_euler_angles_t* angles, const Eigen::Vector3f& rotation_vector)
{
    Eigen::AngleAxisf axis_angle = Eigen::AngleAxisf(rotation_vector.norm(), rotation_vector.normalized());

    Eigen::Quaternionf q = Eigen::Quaternionf(axis_angle);

    auto euler = q.toRotationMatrix().eulerAngles(0, 1, 2);

    angles->roll  = euler.x();
    angles->pitch = euler.y();
    angles->yaw   = euler.z();
}

void linalg_interpolate_euler_angles(linalg_euler_angles_t* out, const linalg_euler_angles_t* first, const linalg_euler_angles_t* second, uint64_t interpolation_time)
{
    assert((interpolation_time >= first->time) && (interpolation_time <= second->time));

    auto first_rotation  = linalg_calculate_rotation_vector(first);
    auto second_rotation = linalg_calculate_rotation_vector(second);

    Eigen::Vector3f out_rotation;
    if (first->time == second->time)
        out_rotation = .5 * (first_rotation + second_rotation);
    else
        out_rotation = ((interpolation_time - first->time) * first_rotation + (second->time - interpolation_time) * second_rotation) * (1.f / (second->time - first->time));

    linalg_calculate_euler_angles(out, out_rotation);
}

float linalg_interpolate_scalar(float first, uint64_t first_time, float second, uint64_t second_time, uint64_t interpolation_time)
{
    assert((interpolation_time >= first_time) && (interpolation_time <= second_time));

    return ((interpolation_time - first_time) * first + (second_time - interpolation_time) * second) / (second_time - first_time);
}
