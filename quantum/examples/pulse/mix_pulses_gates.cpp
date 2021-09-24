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
// Demonstrates pulse+gate circuit:
// A composite can contain pulses and gates (e.g., measure, x,y,z)
// All digital gates will be kept as-is in the QOBJ (aka default vendor pulse sequences)
// Pulses are converted to **custom** calibration gates referencing the pulse samples.
#include "xacc.hpp"
#include "xacc_service.hpp"

// Running pulse-level simulation.
int main(int argc, char **argv) {
  xacc::set_verbose(true);
  xacc::Initialize(argc, argv);
  auto accelerator = xacc::getAccelerator("ibm:ibmq_armonk");
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
  // Create pulse instructions to send to all D channels.
  auto pulseInst0 = provider->createInstruction(
      "gaussian", {0}, {}, {{"channel", "d0"}, {"samples", samples}});
  auto x_gate = provider->createInstruction("X", {0});
  auto meas = provider->createInstruction("Measure", {0});
  // Add the X - Pulse - Measure:
  // X and Measure are kept as gates.
  auto f = provider->createComposite("tmp");
  f->addInstructions({x_gate, pulseInst0, meas});
  accelerator->execute(buffer, f);
  std::cout << "Experiment result:\n";
  buffer->print();
  xacc::Finalize();
}