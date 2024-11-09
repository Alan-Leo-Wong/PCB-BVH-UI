// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2015 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//
// Created by Lei on 10/6/2024.
//

#ifndef PCB_OFFSET_FRUSTUM_H
#define PCB_OFFSET_FRUSTUM_H

#include <Eigen/Dense>

/// Implementation of the deprecated glFrustum function.
///
/// @param[in] left  coordinate of left vertical clipping plane
/// @param[in] right  coordinate of right vertical clipping plane
/// @param[in] bottom  coordinate of bottom vertical clipping plane
/// @param[in] top  coordinate of top vertical clipping plane
/// @param[in] nearVal  distance to near plane
/// @param[in] farVal  distance to far plane
/// @param[out] P  4x4 perspective matrix
template<typename DerivedP>
inline void frustum(
        const typename DerivedP::Scalar left,
        const typename DerivedP::Scalar right,
        const typename DerivedP::Scalar bottom,
        const typename DerivedP::Scalar top,
        const typename DerivedP::Scalar nearVal,
        const typename DerivedP::Scalar farVal,
        Eigen::PlainObjectBase<DerivedP> &P) {
    P.setConstant(4, 4, 0.);
    P(0, 0) = (2.0 * nearVal) / (right - left);
    P(1, 1) = (2.0 * nearVal) / (top - bottom);
    P(0, 2) = (right + left) / (right - left);
    P(1, 2) = (top + bottom) / (top - bottom);
    P(2, 2) = -(farVal + nearVal) / (farVal - nearVal);
    P(3, 2) = -1.0;
    P(2, 3) = -(2.0 * farVal * nearVal) / (farVal - nearVal);
}

#endif //PCB_OFFSET_FRUSTUM_H
