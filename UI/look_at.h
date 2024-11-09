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

#ifndef PCB_OFFSET_LOOK_AT_H
#define PCB_OFFSET_LOOK_AT_H

#include <Eigen/Dense>

template<
        typename Derivedeye,
        typename Derivedcenter,
        typename Derivedup,
        typename DerivedR>
inline void look_at(
        const Eigen::PlainObjectBase <Derivedeye> &eye,
        const Eigen::PlainObjectBase <Derivedcenter> &center,
        const Eigen::PlainObjectBase <Derivedup> &up,
        Eigen::PlainObjectBase <DerivedR> &R) {
    typedef Eigen::Matrix<typename DerivedR::Scalar, 3, 1> Vector3S;
    Vector3S f = (center - eye).normalized();
    Vector3S s = f.cross(up).normalized();
    Vector3S u = s.cross(f);
    R = Eigen::Matrix<typename DerivedR::Scalar, 4, 4>::Identity();
    R(0, 0) = s(0);
    R(0, 1) = s(1);
    R(0, 2) = s(2);
    R(1, 0) = u(0);
    R(1, 1) = u(1);
    R(1, 2) = u(2);
    R(2, 0) = -f(0);
    R(2, 1) = -f(1);
    R(2, 2) = -f(2);
    R(0, 3) = -s.transpose() * eye;
    R(1, 3) = -u.transpose() * eye;
    R(2, 3) = f.transpose() * eye;
}

#endif //PCB_OFFSET_LOOK_AT_H
