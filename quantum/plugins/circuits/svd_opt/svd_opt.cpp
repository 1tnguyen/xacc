/*******************************************************************************
 * Copyright (c) 2019 UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompanies this
 * distribution. The Eclipse Public License is available at
 * http://www.eclipse.org/legal/epl-v10.html and the Eclipse Distribution
 *License is available at https://eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *   Thien Nguyen - initial API and implementation
 *******************************************************************************/
#include "svd_opt.hpp"
#include "linalg_utils/utils.hpp"
#include <unsupported/Eigen/CXX11/Tensor>
#include "xacc.hpp"

namespace {
template <class T> 
bool hasDuplicates(const std::vector<T>& in_vec) 
{
  std::unordered_map<T, int> elemToCount;
  for (const auto& e : in_vec) 
  {
    ++elemToCount[e];
    if (elemToCount[e] > 1) 
    {
      return true;
    }
  }
  return false;
}

std::optional<size_t> calcNumQubits(size_t in_uDim)
{
  // Cap the potential size of the unitary matrix:
  constexpr size_t MAX_NUM_QUBITS = 5;
  for (size_t i = 2; i < MAX_NUM_QUBITS; ++i) 
  {
    if ((1ULL << i) == in_uDim) {
      return i;
    }
  }
  return std::nullopt;
}

template <typename T>
using MatrixType = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
template <typename Scalar, int rank, typename sizeType>
auto Tensor_to_Matrix(const Eigen::Tensor<Scalar, rank>& tensor,
                      sizeType rows, sizeType cols) 
{
  return Eigen::Map<const MatrixType<Scalar>>(tensor.data(), rows, cols);
}

template <typename Scalar, typename... Dims>
auto Matrix_to_Tensor(const MatrixType<Scalar>& matrix, Dims... dims) 
{
  constexpr int rank = sizeof...(Dims);
  return Eigen::TensorMap<Eigen::Tensor<const Scalar, rank>>(matrix.data(), {dims...});
}

// Note: the unitary matrix tensors always have even ranks:
using Tensor4d = Eigen::Tensor<std::complex<double>, 4>;
using Tensor6d = Eigen::Tensor<std::complex<double>, 6>;
using Tensor8d = Eigen::Tensor<std::complex<double>, 8>;
using Tensor10d = Eigen::Tensor<std::complex<double>, 10>;

// Eigen doesn't support dynamic Tensor ranks, hence create a wrapper to handle dynamic ranks.
class DynamicTensor 
{
public:
  DynamicTensor(const Eigen::MatrixXcd& in_unitary)
  {
    assert(xacc::utils::isUnitary(in_unitary));
    m_rank = 2 * calcNumQubits(in_unitary.rows()).value();
    assert(m_rank >= 6 && m_rank <= 10);
    LoadFromUnitary(in_unitary);
  }

  enum class ContractDirection { Left, Right };
  void contractGate(const Eigen::MatrixXcd& in_gateMat, const std::vector<size_t>& in_location, ContractDirection in_dir) 
  {
    // Assume two-qubit gate matrix only atm (block size = 2)
    Tensor4d gateTensor = Matrix_to_Tensor(in_gateMat, 2, 2, 2, 2);
    Eigen::array<Eigen::IndexPair<long>, 2> cdim;
    assert(in_location.size() == 2);
    if (in_dir == ContractDirection::Right)  
    {
      assert(in_location[0] < m_rank/2);
      cdim[0] = Eigen::IndexPair<long> { (long) (in_location[0] + m_rank/2),  0 };
      cdim[1] = Eigen::IndexPair<long> { (long) (in_location[1] + m_rank/2),  1 };
    }
    else 
    {
      assert(in_location[0] < m_rank/2);
      cdim[0] = Eigen::IndexPair<long> { (long) in_location[0],  2 };
      cdim[1] = Eigen::IndexPair<long> { (long) in_location[1],  3 };
    }

    if (m_rank == 6) 
    {
      m_tensor6 = m_tensor6.contract(gateTensor, cdim);
    }
    else if (m_rank == 8) 
    {
      m_tensor8 = m_tensor8.contract(gateTensor, cdim);
    }
    else if (m_rank == 10) 
    {
      m_tensor10 = m_tensor10.contract(gateTensor, cdim);
    }
    else 
    {
      xacc::error("Unsupported");
    }
  }

  Eigen::MatrixXcd toMat() const 
  {
    const auto dim = 1ULL << (m_rank/2);
    if (m_rank == 6) 
    {
      return Tensor_to_Matrix(m_tensor6, dim, dim);
    }
    else if (m_rank == 8) 
    {
      return Tensor_to_Matrix(m_tensor8, dim, dim);
    }
    else if (m_rank == 10) 
    {
      return Tensor_to_Matrix(m_tensor10, dim, dim);
    }
    else 
    {
      xacc::error("Unsupported");
    }
  }

  Eigen::MatrixXcd computeEnvForGate(size_t in_idx1, size_t in_idx2) const 
  {
    std::vector<int> traceDims;
    assert((in_idx1 < m_rank/2) && (in_idx2 < m_rank/2));
    for (int i = 0; i < m_rank/2; ++i) 
    {
      if ((i != in_idx1) && (i != in_idx2)) 
      {
        traceDims.emplace_back(i);
        traceDims.emplace_back(m_rank/2 + i);
      }
    }
    
    if (m_rank == 6) 
    {
      assert(traceDims.size() == 2);
      Eigen::array<int, 2> dims({traceDims[0], traceDims[1]});
      Tensor4d traceResult = m_tensor6.trace(dims);
      return Tensor_to_Matrix(traceResult, 4, 4);
    }
    else if (m_rank == 8) 
    {
      assert(traceDims.size() == 4);
      Eigen::array<int, 4> dims({traceDims[0], traceDims[1], traceDims[2], traceDims[3]});
      Tensor4d traceResult = m_tensor8.trace(dims);
      return Tensor_to_Matrix(traceResult, 4, 4);
    }
    else if (m_rank == 10) 
    {
      assert(traceDims.size() == 6);
      Eigen::array<int, 6> dims({traceDims[0], traceDims[1], traceDims[2], traceDims[3], traceDims[4], traceDims[5]});
      Tensor4d traceResult = m_tensor10.trace(dims);
      return Tensor_to_Matrix(traceResult, 4, 4);
    }
    else 
    {
      xacc::error("Unsupported");
    }
  }

private:
  void LoadFromUnitary(const Eigen::MatrixXcd& in_unitary) 
  {
    if (m_rank == 6) 
    {
      m_tensor6 = Matrix_to_Tensor(in_unitary, 2, 2, 2, 2, 2, 2);
    }
    else if (m_rank == 8) 
    {
      m_tensor8 = Matrix_to_Tensor(in_unitary, 2, 2, 2, 2, 2, 2, 2, 2);
    }
    else if (m_rank == 10) 
    {
      m_tensor10 = Matrix_to_Tensor(in_unitary, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2);
    }
    else 
    {
      xacc::error("Unsupported");
    }
  }
private:
  size_t m_rank;
  Tensor6d m_tensor6;
  Tensor8d m_tensor8;
  Tensor10d m_tensor10;
};

using namespace xacc::circuits;
std::vector<SVD::Gate> genRandomGates(size_t nbQubits, size_t length)
{
  std::vector<SVD::Gate> result;
  std::vector<int> qubitIdx(nbQubits) ;
  std::iota(qubitIdx.begin(), qubitIdx.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  
  std::set<size_t> lastPair;
  for (size_t i = 0; i < length; ++i)
  {
    Eigen::Matrix4cd mat = Eigen::Matrix4cd::Random();
    auto QR = mat.householderQr();
    Eigen::Matrix4cd qMat = QR.householderQ() * Eigen::Matrix4cd::Identity();
    // Pick two random location
    std::shuffle(qubitIdx.begin(), qubitIdx.end(), g);
    std::set<size_t> pickedIds { qubitIdx[0], qubitIdx[1] };
    while (pickedIds == lastPair) 
    {
      std::shuffle(qubitIdx.begin(), qubitIdx.end(), g);
      // Re-pick
      pickedIds = { qubitIdx[0], qubitIdx[1] };
    }
    lastPair = pickedIds;
    std::vector<size_t> bitLoc;
    bitLoc.assign(lastPair.begin(), lastPair.end());
    result.emplace_back(qMat, bitLoc);
  }
  
  return result;
}
} 

namespace xacc {
namespace circuits {
bool SVD::expand(const xacc::HeterogeneousMap& runtimeOptions) 
{
  Eigen::MatrixXcd unitary;
  if (runtimeOptions.keyExists<Eigen::MatrixXcd>("unitary"))
  {
    unitary = runtimeOptions.get<Eigen::MatrixXcd>("unitary");
  }

  const int max_iters = 100000;
  std::complex<double> c1 = 0.0;
  std::complex<double> c2 = 1.0;
  int iter = 0;
  const double threshold = 1e-9;
  const size_t num_qubits = calcNumQubits(unitary.rows()).value();
  // !!! Test only !!!
  // Number of blocks
  const size_t gate_length = 7;
  auto initialGateLists = genRandomGates(num_qubits, gate_length);

  const double slowdown_factor = 0.0;
  assert(initialGateLists.size() == gate_length);
  CircuitTensor circuit(unitary, initialGateLists);
  while ((std::abs(c1 - c2) > threshold) && (iter < max_iters))
  {
    iter++;
    // From right to left
    for (size_t k = 0; k < initialGateLists.size(); ++k)
    {
      auto rk = initialGateLists.size() - 1 - k;
      // Remove current gate from right of circuit tensor
      auto inv_gate = initialGateLists[rk].getInverse();
      circuit.applyRight(inv_gate);
      // Update current gate
      if (!initialGateLists[rk].isFixed())
      {
        auto env = circuit.calcEnvMatrix(initialGateLists[rk].getLocation());
        auto svdMat = env + slowdown_factor * inv_gate.getUnitaryMat();
        Eigen::JacobiSVD<Eigen::MatrixXcd> svd(svdMat, Eigen::ComputeThinU | Eigen::ComputeThinV);
        Eigen::MatrixXcd u = svd.matrixU();
        Eigen::MatrixXcd v = svd.matrixV();
        u.adjointInPlace();
        v.adjointInPlace();
        Gate updatedGate(v * u, initialGateLists[rk].getLocation());
        initialGateLists[rk] = updatedGate;
      }
      // Add updated gate to left of circuit tensor
      circuit.applyLeft(initialGateLists[rk]);    
    }
               
    // from left to right
    for (size_t k = 0; k < initialGateLists.size(); ++k)
    {
      // Remove current gate from right of circuit tensor
      auto inv_gate = initialGateLists[k].getInverse();
      circuit.applyLeft(inv_gate);
      // Update current gate
      if (!initialGateLists[k].isFixed())
      {
        auto env = circuit.calcEnvMatrix(initialGateLists[k].getLocation());
        auto svdMat = env + slowdown_factor * inv_gate.getUnitaryMat();
        Eigen::JacobiSVD<Eigen::MatrixXcd> svd(svdMat, Eigen::ComputeThinU | Eigen::ComputeThinV);
        Eigen::MatrixXcd u = svd.matrixU();
        Eigen::MatrixXcd v = svd.matrixV();
        u.adjointInPlace();
        v.adjointInPlace();
        Gate updatedGate(v * u, initialGateLists[k].getLocation());
        initialGateLists[k] = updatedGate;
      }
      // Add updated gate to right of circuit tensor
      circuit.applyRight(initialGateLists[k]);
    }
    
    c2 = c1;
    c1 = circuit.getCurrentUnitary().trace();
    c1 = std::pow(2.0, num_qubits + 1) - (2.0 * c1.real());

    if (iter % 100 == 0)
    {
      xacc::info("iteration: " + std::to_string(iter) + ", cost: " + std::to_string(c1.real()));
    }
           
    if (iter % 1000 == 0)
    {
      circuit.reinitialize();
    }
  }
  
  return true;
}

const std::vector<std::string> SVD::requiredKeys()
{
  // num-gates is the number of two-qubit blocks to optimize for.
  // Note: We only support *two-qubit* blocks since we will need to eventually
  // lower to gates (via KAK).
  return {"unitary", "num-gates"};
}

SVD::Gate::Gate(const Eigen::MatrixXcd& in_unitary, const std::vector<size_t>& in_location, bool in_fixed):
  m_unitary(in_unitary),
  m_location(in_location),
  m_isFixed(in_fixed)
{
  // Validate:
  assert(!in_location.empty());
  assert(utils::isUnitary(in_unitary));
  assert((1ULL << in_location.size()) == in_unitary.rows());
  assert(!hasDuplicates(in_location));
}

SVD::Gate SVD::Gate::getInverse() const
{
  // Inverse of a gate == conjugate transpose
  Eigen::MatrixXcd inverseMat = m_unitary;
  inverseMat.adjointInPlace();
  return SVD::Gate(inverseMat, m_location, m_isFixed);
}

SVD::CircuitTensor::CircuitTensor(const Eigen::MatrixXcd& in_targetUnitary, const std::vector<Gate>& in_gates):
  m_targetUnitary(in_targetUnitary),
  m_gateList(in_gates)
{
  assert(utils::isUnitary(in_targetUnitary));
  const auto validateGates =
      [](size_t numQubits, const std::vector<Gate> &initialGateList) -> bool {
    for (const auto &gate : initialGateList) {
      for (const auto &bitLoc : gate.getLocation()) {
        if (bitLoc >= numQubits) {
          return false;
        }
      }
    }
    return true;
  };
  // Throws if the unitary matrix has invalid dimension.
  m_numQubits = calcNumQubits(in_targetUnitary.rows()).value();
  assert(validateGates(m_numQubits, m_gateList));
  reinitialize();
}

void SVD::CircuitTensor::reinitialize() 
{
  m_tensor = m_targetUnitary;
  m_tensor.adjointInPlace();
  for (const auto &gate : m_gateList) 
  {
    applyRight(gate);
  }
}

void SVD::CircuitTensor::applyRight(const Gate& in_rhsGate) 
{
  // Applies a gate to the right of the current unitary.
  //      .-----.   .------.
  //   0 -|     |---|      |-
  //   1 -|     |---| gate |-
  //      .     .   '------'
  //      .     .
  //      .     .
  // n-1 -|     |------------
  //      '-----'

  DynamicTensor mapped_tensor(m_tensor);
  mapped_tensor.contractGate(in_rhsGate.getUnitaryMat(),
                             in_rhsGate.getLocation(),
                             DynamicTensor::ContractDirection::Right);
  m_tensor = mapped_tensor.toMat();
}

void SVD::CircuitTensor::applyLeft(const Gate& in_rhsGate) 
{
  // Applies a gate to the left of the current unitary.
  //      .------.   .-----.
  //   0 -|      |---|     |-
  //   1 -| gate |---|     |-
  //      '------'   .     .
  //                 .     .
  //                 .     .
  // n-1 ------------|     |-
  //                 '-----'

  DynamicTensor mapped_tensor(m_tensor);
  mapped_tensor.contractGate(in_rhsGate.getUnitaryMat(),
                             in_rhsGate.getLocation(),
                             DynamicTensor::ContractDirection::Left);
  m_tensor = mapped_tensor.toMat();
}

Eigen::MatrixXcd SVD::CircuitTensor::calcEnvMatrix(const std::vector<size_t>& in_location) const
{
  assert(in_location.size() == 2);
  DynamicTensor mapped_tensor(m_tensor);
  return mapped_tensor.computeEnvForGate(in_location[0], in_location[1]);
}
}
}