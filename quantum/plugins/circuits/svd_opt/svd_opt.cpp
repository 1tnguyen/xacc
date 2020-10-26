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
}
}