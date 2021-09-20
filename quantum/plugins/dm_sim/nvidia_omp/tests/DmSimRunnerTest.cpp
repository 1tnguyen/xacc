#include "DmSimApi.hpp"
#include <iostream>

int main() {
  auto dm_sim = DmSim::getGpuDmSim();
  if (!dm_sim) {
    std::cout << "Failed to find DM-SIM\n";
    return -1;
  }
  dm_sim->init(10, 1);
  dm_sim->addGate(DmSim::OP::H, {0});
  dm_sim->addGate(DmSim::OP::CX, {0, 1});
  auto meas = dm_sim->measure(1024);
  std::cout << "DONE\n";
  return 0;
}