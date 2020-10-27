#include <Eigen/Dense>

namespace xacc {
namespace utils {
inline bool isSquare(const Eigen::MatrixXcd &in_mat) {
  return in_mat.rows() == in_mat.cols();
}

// If the matrix is finite: no NaN elements
template <typename Derived>
inline bool isFinite(const Eigen::MatrixBase<Derived> &x) {
  return ((x - x).array() == (x - x).array()).all();
}

inline bool isDiagonal(const Eigen::MatrixXcd &in_mat, double in_tol = 1e-9) {
  if (!isFinite(in_mat)) {
    return false;
  }

  for (int i = 0; i < in_mat.rows(); ++i) {
    for (int j = 0; j < in_mat.cols(); ++j) {
      if (i != j) {
        if (std::abs(in_mat(i, j)) > in_tol) {
          return false;
        }
      }
    }
  }

  return true;
}

inline bool allClose(const Eigen::MatrixXcd &in_mat1,
                     const Eigen::MatrixXcd &in_mat2, double in_tol = 1e-9) {
  if (!isFinite(in_mat1) || !isFinite(in_mat2)) {
    return false;
  }

  if (in_mat1.rows() == in_mat2.rows() && in_mat1.cols() == in_mat2.cols()) {
    for (int i = 0; i < in_mat1.rows(); ++i) {
      for (int j = 0; j < in_mat1.cols(); ++j) {
        if (std::abs(in_mat1(i, j) - in_mat2(i, j)) > in_tol) {
          return false;
        }
      }
    }

    return true;
  }
  return false;
}

inline bool isHermitian(const Eigen::MatrixXcd &in_mat) {
  if (!isSquare(in_mat) || !isFinite(in_mat)) {
    return false;
  }
  return allClose(in_mat, in_mat.adjoint());
}

inline bool isUnitary(const Eigen::MatrixXcd &in_mat) {
  if (!isSquare(in_mat) || !isFinite(in_mat)) {
    return false;
  }

  Eigen::MatrixXcd Id =
      Eigen::MatrixXcd::Identity(in_mat.rows(), in_mat.cols());

  return allClose(in_mat * in_mat.adjoint(), Id);
}

inline bool isOrthogonal(const Eigen::MatrixXcd &in_mat, double in_tol = 1e-9) {
  if (!isSquare(in_mat) || !isFinite(in_mat)) {
    return false;
  }

  // Is real
  for (int i = 0; i < in_mat.rows(); ++i) {
    for (int j = 0; j < in_mat.cols(); ++j) {
      if (std::abs(in_mat(i, j).imag()) > in_tol) {
        return false;
      }
    }
  }
  // its transpose is its inverse
  return allClose(in_mat.inverse(), in_mat.transpose(), in_tol);
}
} // namespace utils
} // namespace xacc