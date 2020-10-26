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

#pragma once

#include "Circuit.hpp"
#include "IRProvider.hpp"
#include <Eigen/Dense>
namespace xacc {
namespace circuits {
class SVD : public xacc::quantum::Circuit 
{
public:
    SVD(): Circuit("svd") {};
    bool expand(const xacc::HeterogeneousMap& runtimeOptions) override;
    const std::vector<std::string> requiredKeys() override;
    DEFINE_CLONE(SVD);
    // Internal classes:
    // A Gate is a unitary operation applied to a set of qubits.
    class Gate 
    {
    public:
        Gate(const Eigen::MatrixXcd& in_unitary, const std::vector<size_t>& in_location, bool in_fixed = false);
        // Returns the inverse of this gate.
        Gate getInverse() const;
    private:
        friend class CircuitTensor; 
        std::vector<size_t> m_location;
        bool m_isFixed;
        Eigen::MatrixXcd m_unitary;
    };

    // A CircuitTensor tracks an entire circuit as a tensor
    class CircuitTensor 
    {
    public:
        CircuitTensor(const Eigen::MatrixXcd& in_targetUnitary, const std::vector<Gate>& in_gates);
        // Reconstructs the circuit tensor
        void reinitialize();
        void applyRight(const Gate& in_rhsGate);
        void applyLeft(const Gate& in_rhsGate);
        // Calculates the environmental matrix of the tensor with respect to the specified location.
        Eigen::MatrixXcd calcEnvMatrix(const std::vector<size_t>& in_location) const;
    private:
        Eigen::MatrixXcd m_targetUnitary;
        Eigen::MatrixXcd m_tensor;
        size_t m_numQubits;
        std::vector<Gate> m_gateList;
    };
};
} // namespace circuits
} // namespace xacc