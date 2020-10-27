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
}
namespace xacc {
namespace circuits {
bool SVD::expand(const xacc::HeterogeneousMap& runtimeOptions) 
{
    /// TODO:
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
}
}