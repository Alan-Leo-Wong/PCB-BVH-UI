//
// Created by Lei on 10/6/2024.
//

#ifndef PCB_OFFSET_SNAP_TO_FIXED_UP_H
#define PCB_OFFSET_SNAP_TO_FIXED_UP_H

#include <Eigen/Dense>
#include <Eigen/Geometry>

/// Snap an arbitrary rotation to a rotation resulting from a rotation about
/// the y-axis then the x-axis (maintaining fixed up like
/// two_axis_valuator_fixed_up.)
///
/// @param[in] q  General rotation as quaternion
/// @param[out] s the resulting rotation (as quaternion)
///
/// \see two_axis_valuator_fixed_up
template<typename Qtype>
inline void snap_to_fixed_up(
        const Eigen::Quaternion <Qtype> &q,
        Eigen::Quaternion <Qtype> &s) {
    using namespace Eigen;
    typedef Eigen::Matrix<Qtype,3,1> Vector3Q;
    const Vector3Q up = q.matrix() * Vector3Q(0,1,0);
    Vector3Q proj_up(0,up(1),up(2));
    if(proj_up.norm() == 0)
    {
        proj_up = Vector3Q(0,1,0);
    }
    proj_up.normalize();
    Quaternion<Qtype> dq;
    dq = Quaternion<Qtype>::FromTwoVectors(up,proj_up);
    s = dq * q;
}

#endif //PCB_OFFSET_SNAP_TO_FIXED_UP_H
