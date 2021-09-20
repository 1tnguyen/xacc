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
#include "DmSimAccelerator.hpp"
#include "DmSimApi.hpp"
#include "AllGateVisitor.hpp"
#include "xacc_plugin.hpp"
namespace xacc {
namespace quantum {
class DmSimCircuitVisitor : public AllGateVisitor {
private:
  DmSim::DmSimBackend *m_backend;

public:
  DmSimCircuitVisitor(DmSim::DmSimBackend *backend) : m_backend(backend) {}

  void visit(Hadamard &h) override {
    m_backend->addGate(DmSim::OP::H, std::vector<int>{(int)h.bits()[0]});
  }

  void visit(X &x) override {
    m_backend->addGate(DmSim::OP::X, std::vector<int>{(int)x.bits()[0]});
  }

  void visit(Y &y) override {
    m_backend->addGate(DmSim::OP::Y, std::vector<int>{(int)y.bits()[0]});
  }

  void visit(Z &z) override {
    m_backend->addGate(DmSim::OP::Z, std::vector<int>{(int)z.bits()[0]});
  }

  void visit(Rx &rx) override {
    m_backend->addGate(DmSim::OP::RX, std::vector<int>{(int)rx.bits()[0]},
                       {InstructionParameterToDouble(rx.getParameter(0))});
  }

  void visit(Ry &ry) override {
    m_backend->addGate(DmSim::OP::RY, std::vector<int>{(int)ry.bits()[0]},
                       {InstructionParameterToDouble(ry.getParameter(0))});
  }

  void visit(Rz &rz) override {
    m_backend->addGate(DmSim::OP::RZ, std::vector<int>{(int)rz.bits()[0]},
                       {InstructionParameterToDouble(rz.getParameter(0))});
  }

  void visit(S &s) override {
    m_backend->addGate(DmSim::OP::S, std::vector<int>{(int)s.bits()[0]});
  }

  void visit(Sdg &sdg) override {
    m_backend->addGate(DmSim::OP::SDG, std::vector<int>{(int)sdg.bits()[0]});
  }

  void visit(T &t) override {
    m_backend->addGate(DmSim::OP::T, std::vector<int>{(int)t.bits()[0]});
  }

  void visit(Tdg &tdg) override {
    m_backend->addGate(DmSim::OP::TDG, std::vector<int>{(int)tdg.bits()[0]});
  }

  void visit(CNOT &cnot) override {
    m_backend->addGate(DmSim::OP::CX, std::vector<int>{(int)cnot.bits()[0],
                                                       (int)cnot.bits()[1]});
  }

  void visit(CY &cy) override {
    m_backend->addGate(DmSim::OP::CY,
                       std::vector<int>{(int)cy.bits()[0], (int)cy.bits()[1]});
  }

  void visit(CZ &cz) override {
    m_backend->addGate(DmSim::OP::CZ,
                       std::vector<int>{(int)cz.bits()[0], (int)cz.bits()[1]});
  }

  void visit(Swap &s) override {
    m_backend->addGate(DmSim::OP::SWAP,
                       std::vector<int>{(int)s.bits()[0], (int)s.bits()[1]});
  }

  void visit(CH &ch) override {
    m_backend->addGate(DmSim::OP::CH,
                       std::vector<int>{(int)ch.bits()[0], (int)ch.bits()[1]});
  }

  void visit(CPhase &cphase) override {
    m_backend->addGate(
        DmSim::OP::CU1,
        std::vector<int>{(int)cphase.bits()[0], (int)cphase.bits()[1]},
        {InstructionParameterToDouble(cphase.getParameter(0))});
  }

  void visit(CRZ &crz) override {
    m_backend->addGate(DmSim::OP::CRZ,
                       std::vector<int>{(int)crz.bits()[0], (int)crz.bits()[1]},
                       {InstructionParameterToDouble(crz.getParameter(0))});
  }

  void visit(Identity &i) override {}

  void visit(U &u) override {
    const auto theta = InstructionParameterToDouble(u.getParameter(0));
    const auto phi = InstructionParameterToDouble(u.getParameter(1));
    const auto lambda = InstructionParameterToDouble(u.getParameter(2));

    m_backend->addGate(DmSim::OP::H, std::vector<int>{(int)u.bits()[0]},
                       {theta, phi, lambda});
  }

  void visit(Measure &measure) override {
    m_measureQubits.emplace_back(measure.bits()[0]);
  }

  // NOT SUPPORTED:
  void visit(IfStmt &ifStmt) override {}
  std::vector<size_t> getMeasureBits() const { return m_measureQubits; }

private:
  std::vector<size_t> m_measureQubits;
};

void DmSimAccelerator::initialize(const HeterogeneousMap &params) {
  m_ngpus = 1;
  if (params.keyExists<int>("gpus")) {
    m_ngpus = params.get<int>("gpus");
  }
  m_shots = 1024;
  if (params.keyExists<int>("shots")) {
    m_shots = params.get<int>("shots");
  }
}

void DmSimAccelerator::execute(
    std::shared_ptr<AcceleratorBuffer> buffer,
    const std::shared_ptr<CompositeInstruction> compositeInstruction) {
  auto dm_sim = DmSim::getGpuDmSim();
  if (!dm_sim) {
    xacc::error("DM-Sim was not installed. Please make sure that you're "
                "compiling XACC on a platform with CUDA.");
  }
  dm_sim->init(buffer->size(), m_ngpus);
  DmSimCircuitVisitor visitor(dm_sim.get());
  // Walk the IR tree, and visit each node
  InstructionIterator it(compositeInstruction);
  while (it.hasNext()) {
    auto nextInst = it.next();
    if (nextInst->isEnabled()) {
      nextInst->accept(&visitor);
    }
  }
  auto measured_bits = visitor.getMeasureBits();
  if (measured_bits.empty()) {
    // Default is just measure alls:
    for (size_t i = 0; i < buffer->size(); ++i) {
      measured_bits.emplace_back(i);
    }
  }
  const auto measured_results = dm_sim->measure(m_shots);
  // TODO: add shots data to buffer:
  std::cout << "HOWDY: measure results:\n";
  for (const auto &m : measured_results) {
    std::cout << m << "\n";
  }
}
void DmSimAccelerator::execute(
    std::shared_ptr<AcceleratorBuffer> buffer,
    const std::vector<std::shared_ptr<CompositeInstruction>>
        compositeInstructions) {
  // TODO
}
} // namespace quantum
} // namespace xacc
REGISTER_ACCELERATOR(xacc::quantum::DmSimAccelerator)