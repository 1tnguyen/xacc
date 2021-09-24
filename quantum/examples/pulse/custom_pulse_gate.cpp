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
#include "xacc.hpp"
#include "xacc_service.hpp"

// Running pulse-level simulation.
int main(int argc, char **argv) {
  xacc::set_verbose(true);
  xacc::Initialize(argc, argv);
  auto accelerator =
      xacc::getAccelerator("ibm:ibmq_armonk", {{"mode", "pulse"}});
  auto buffer = xacc::qalloc(1);
  auto provider = xacc::getService<xacc::IRProvider>("quantum");
  // Helper to compute a Gaussian pulse.
  // In this example, we just want to test a random Gaussian pulse.
  const auto gaussianCalc = [](double in_time, double in_amp, double in_center, double in_sigma){
    return in_amp*std::exp(-std::pow(in_time - in_center, 2) / 2.0 / std::pow(in_sigma, 2.0));
  };

  const size_t nbSamples = 160;
  std::vector<double> samples;
  for (size_t i = 0; i < nbSamples; ++i) {
    samples.emplace_back(gaussianCalc(i, 0.1, nbSamples / 2, nbSamples / 4));
  }
  // Create pulse instruction: a Gaussian pulse on d0
  auto pulseInst0 = provider->createInstruction(
      "gaussian", {0}, {}, {{"channel", "d0"}, {"samples", samples}});


  // Register a **custom** implementation of H gate for qubit 0:
  // (1) The pulse sequence name must conform to the format:
  // pulse::<gate_name>_<qubit>
  // or: pulse::<gate_name>_<qubit>_<qubit> for 2-qubit gate
  const std::string x_cmd_def_name = "pulse::h_0";
  auto cmd_def = provider->createComposite(x_cmd_def_name);
  // (2) Add pulses to the sequence:
  // In this case, we just add a dummy Gaussian pulse.
  cmd_def->addInstruction(pulseInst0);
  // (3) Register the cmd-def 
  xacc::contributeService(x_cmd_def_name, cmd_def);

  // Now, create a circuit using H gate:
  auto h_gate = provider->createInstruction("H", {0});
  auto meas = provider->createInstruction("Measure", {0});
  auto f = provider->createComposite("circuit");
  f->addInstructions({h_gate, meas});
  accelerator->execute(buffer, f);
  std::cout << "Experiment Result:\n";
  buffer->print();
  xacc::Finalize();
}