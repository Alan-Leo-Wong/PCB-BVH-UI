//
// Created by Lei on 10/6/2024.
//

#ifndef PCB_OFFSET_NULL_H
#define PCB_OFFSET_NULL_H

/// Standard value for double epsilon
const double DOUBLE_EPS = 1.0e-14;
/// Standard value for double epsilon²
const double DOUBLE_EPS_SQ = 1.0e-28;
/// Standard value for single epsilon
const float FLOAT_EPS = 1.0e-7f;
/// Standard value for single epsilon²
const float FLOAT_EPS_SQ = 1.0e-14f;

template<typename T>
T EPS();

template<typename T>
T EPS_SQ();

template<>
inline float EPS() {
    return FLOAT_EPS;
}

template<>
inline double EPS() {
    return DOUBLE_EPS;
}

template<>
inline float EPS_SQ() {
    return FLOAT_EPS_SQ;
}

template<>
inline double EPS_SQ() {
    return DOUBLE_EPS_SQ;
}

template<typename DerivedA, typename DerivedN>
inline void null(
        const Eigen::PlainObjectBase <DerivedA> &A,
        Eigen::PlainObjectBase <DerivedN> &N) {
    using namespace Eigen;
    typedef typename DerivedA::Scalar Scalar;
    JacobiSVD <Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>> svd(A, ComputeFullV);
    svd.setThreshold(A.cols() * svd.singularValues().maxCoeff() * EPS<Scalar>());
    N = svd.matrixV().rightCols(A.cols() - svd.rank());
}

#endif //PCB_OFFSET_NULL_H
