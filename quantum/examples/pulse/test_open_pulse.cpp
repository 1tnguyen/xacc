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
#include "xacc_config.hpp"

int main(int argc, char **argv) {
  xacc::Initialize(argc, argv);
  std::string config_dir = std::string(GATEIR_TEST_FILE_DIR);

  std::ifstream backendFile(config_dir + "/test_backends.json");
  std::string jjson((std::istreambuf_iterator<char>(backendFile)), std::istreambuf_iterator<char>());
  // Contribute pulse instructions to the service registry
  if (xacc::hasAccelerator("ibm")) {
    auto ibm = xacc::getService<xacc::Accelerator>("ibm");
    ibm->updateConfiguration(
        xacc::HeterogeneousMap{std::make_pair("backend", "ibmq_johannesburg")});
    ibm->contributeInstructions(jjson);
  }
  
  auto buffer = xacc::qalloc(2);

  xacc::qasm(R"(
.compiler xasm
.circuit bell
.qbit q
pulseu3(q[0], pi/2, 0, pi);
pulsecx(q[0],q[1]);
pulseMEAS(q[0]);
pulseMEAS(q[1]);
)");

 auto bell = xacc::getCompiled("bell");
 std::cout << "Compiled: \n" << bell->toString();
}