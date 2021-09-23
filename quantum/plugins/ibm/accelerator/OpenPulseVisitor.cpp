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
 *   Alexander J. McCaskey - initial API and implementation
 *******************************************************************************/
#include "OpenPulseVisitor.hpp"
#include "xacc.hpp"
namespace {
std::shared_ptr<xacc::CompositeInstruction>
constructUCmdDefComposite(size_t qIdx, const std::string &uGateType) {

  const auto cmdDefName = "pulse::" + uGateType + "_" + std::to_string(qIdx);
  // If we have a pulse cmd-def defined for U3:
  if (xacc::hasContributedService<xacc::Instruction>(cmdDefName)) {
    return xacc::ir::asComposite(
        xacc::getContributedService<xacc::Instruction>(cmdDefName));
  }
  return nullptr;
}
} // namespace
namespace xacc {
namespace quantum {
PulseMappingVisitor::PulseMappingVisitor() {
  auto provider = xacc::getService<IRProvider>("quantum");
  pulseComposite = provider->createComposite("PulseComposite");
}

void PulseMappingVisitor::visit(Hadamard &h) {
  const auto commandDef = constructPulseCommandDef(h);
  // If there is a custom pulse provided for Hadamard gate:
  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    // Just keep the gate as-is (digital gate)
    pulseComposite->addInstruction(h.clone());
  }
}

void PulseMappingVisitor::visit(CNOT &cnot) {
  const auto commandDef = constructPulseCommandDef(cnot);
  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(cnot.clone());
  }
}

void PulseMappingVisitor::visit(Rz &rz) {
  const auto commandDef = constructPulseCommandDef(rz);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(rz.clone());
  }
}

void PulseMappingVisitor::visit(Ry &ry) {
  const auto commandDef = constructPulseCommandDef(ry);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(ry.clone());
  }
}

void PulseMappingVisitor::visit(Rx &rx) {
  const auto commandDef = constructPulseCommandDef(rx);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(rx.clone());
  }
}

void PulseMappingVisitor::visit(X &x) {
  const auto commandDef = constructPulseCommandDef(x);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(x.clone());
  }
}

void PulseMappingVisitor::visit(Y &y) {
  const auto commandDef = constructPulseCommandDef(y);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(y.clone());
  }
}

void PulseMappingVisitor::visit(Z &z) {
  const auto commandDef = constructPulseCommandDef(z);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
   pulseComposite->addInstruction(z.clone());
  }
}

void PulseMappingVisitor::visit(CY &cy) {
  // CY(a,b) = sdg(b); cx(a,b); s(b);
  auto sdg = std::make_shared<Sdg>(cy.bits()[1]);
  auto cx = std::make_shared<CNOT>(cy.bits());
  auto s = std::make_shared<S>(cy.bits()[1]);
  visit(*sdg);
  visit(*cx);
  visit(*s);
}

void PulseMappingVisitor::visit(CZ &cz) {
  // CZ(a,b) =  H(b); CX(a,b); H(b);
  auto h1 = std::make_shared<Hadamard>(cz.bits()[1]);
  auto cx = std::make_shared<CNOT>(cz.bits());
  auto h2 = std::make_shared<Hadamard>(cz.bits()[0]);
  visit(*h1);
  visit(*cx);
  visit(*h2);
}

void PulseMappingVisitor::visit(Swap &s) {
  // SWAP(a,b) =  CX(a,b); CX(b,a); CX(a,b);
  auto cx1 = std::make_shared<CNOT>(s.bits());
  auto cx2 = std::make_shared<CNOT>(s.bits()[1], s.bits()[0]);
  auto cx3 = std::make_shared<CNOT>(s.bits());
  visit(*cx1);
  visit(*cx2);
  visit(*cx3);
}

void PulseMappingVisitor::visit(CRZ &crz) {
  // CRZ(theta)(a,b) = U1(theta/2)(b); CX(a,b); U3(-theta/2)(b);
  // CX(a,b);
  const double theta = crz.getParameter(0).as<double>();
  {
    Rz rz1(crz.bits()[1], theta / 2);
    visit(rz1);
  }
  {
    auto cx = std::make_shared<CNOT>(crz.bits());
    visit(*cx);
  }
  {
    Rz rz2(crz.bits()[1], -theta / 2);
    visit(rz2);
  }
  {
    auto cx = std::make_shared<CNOT>(crz.bits());
    visit(*cx);
  }
}

void PulseMappingVisitor::visit(CH &ch) {
  // CH(a,b) = S(b); H(b); T(b); CX(a,b); Tdg(b); H(b); Sdg(b);
  {
    auto s = std::make_shared<S>(ch.bits()[1]);
    visit(*s);
  }
  {
    auto h = std::make_shared<Hadamard>(ch.bits()[1]);
    visit(*h);
  }
  {
    auto t = std::make_shared<T>(ch.bits()[1]);
    visit(*t);
  }
  {
    auto cx = std::make_shared<CNOT>(ch.bits());
    visit(*cx);
  }
  {
    auto tdg = std::make_shared<Tdg>(ch.bits()[1]);
    visit(*tdg);
  }
  {
    auto h = std::make_shared<Hadamard>(ch.bits()[1]);
    visit(*h);
  }
  {
    auto sdg = std::make_shared<Sdg>(ch.bits()[1]);
    visit(*sdg);
  }
}

void PulseMappingVisitor::visit(S &s) {
  const auto commandDef = constructPulseCommandDef(s);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(s.clone());
  }
}

void PulseMappingVisitor::visit(Sdg &sdg) {
  const auto commandDef = constructPulseCommandDef(sdg);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(sdg.clone());
  }
}

void PulseMappingVisitor::visit(T &t) {
  const auto commandDef = constructPulseCommandDef(t);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(t.clone());
  }
}

void PulseMappingVisitor::visit(Tdg &tdg) {
  const auto commandDef = constructPulseCommandDef(tdg);

  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(tdg.clone());
  }
}

void PulseMappingVisitor::visit(CPhase &cphase) {
  // Ref:
  // gate cu1(lambda) a, b {
  //   u1(lambda / 2) a;
  //   cx a, b;
  //   u1(-lambda / 2) b;
  //   cx a, b;
  //   u1(lambda / 2) b;
  // }
  const double lambda = cphase.getParameter(0).as<double>();
  {
    Rz rz1(cphase.bits()[0], lambda / 2.0);
    visit(rz1);
  }
  {
    auto cx = std::make_shared<CNOT>(cphase.bits());
    visit(*cx);
  }
  {
    Rz rz1(cphase.bits()[1], -lambda / 2.0);
    visit(rz1);
  }
  {
    auto cx = std::make_shared<CNOT>(cphase.bits());
    visit(*cx);
  }
  {
    Rz rz1(cphase.bits()[1], lambda / 2.0);
    visit(rz1);
  }
}

void PulseMappingVisitor::visit(Identity &i) {
  // Ignore for now
}

void PulseMappingVisitor::visit(U &u) {
  pulseComposite->addInstruction(u.clone());
}

void PulseMappingVisitor::visit(Measure &measure) {
  const auto commandDef = constructPulseCommandDef(measure);
  if (xacc::hasContributedService<xacc::Instruction>(commandDef)) {
    auto pulseInst = xacc::getContributedService<xacc::Instruction>(commandDef);
    pulseComposite->addInstruction(pulseInst);
  } else {
    pulseComposite->addInstruction(measure.clone());
  }
}

std::string
PulseMappingVisitor::constructPulseCommandDef(xacc::quantum::Gate &in_gate) {
  const auto getGateCommandDefName =
      [](xacc::quantum::Gate &in_gate) -> std::string {
    std::string gateName = in_gate.name();
    std::transform(gateName.begin(), gateName.end(), gateName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (gateName == "cnot") {
      return "cx";
    } else {
      return gateName;
    }
  };

  std::string result = "pulse::" + getGateCommandDefName(in_gate);

  for (const auto &qIdx : in_gate.bits()) {
    result += ("_" + std::to_string(qIdx));
  }

  return result;
}
} // namespace quantum
} // namespace xacc