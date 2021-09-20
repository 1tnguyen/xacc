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
#pragma one
#include "xacc.hpp"

namespace xacc {
namespace quantum {
class DmSimAccelerator : public Accelerator {
public:
  // Identifiable interface impls
  virtual const std::string name() const override { return "dm-sim"; }
  virtual const std::string description() const override {
    return "XACC Simulation Accelerator based on DM-Sim library.";
  }

  // Accelerator interface impls
  virtual void initialize(const HeterogeneousMap &params = {}) override;
  virtual void updateConfiguration(const HeterogeneousMap &config) override {
    initialize(config);
  };
  virtual const std::vector<std::string> configurationKeys() override {
    return {};
  }
  virtual BitOrder getBitOrder() override { return BitOrder::LSB; }
  virtual void execute(std::shared_ptr<AcceleratorBuffer> buffer,
                       const std::shared_ptr<CompositeInstruction>
                           compositeInstruction) override;
  virtual void execute(std::shared_ptr<AcceleratorBuffer> buffer,
                       const std::vector<std::shared_ptr<CompositeInstruction>>
                           compositeInstructions) override;

private:
  int m_ngpus;
  int m_shots;
};
} // namespace quantum
} // namespace xacc
