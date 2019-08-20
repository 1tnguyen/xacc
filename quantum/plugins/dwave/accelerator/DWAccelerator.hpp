/***********************************************************************************
 * Copyright (c) 2017, UT-Battelle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the xacc nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Initial API and implementation - Alex McCaskey
 *
 **********************************************************************************/
#ifndef QUANTUM_GATE_ACCELERATORS_DWACCELERATOR_HPP_
#define QUANTUM_GATE_ACCELERATORS_DWACCELERATOR_HPP_

#include "AcceleratorBuffer.hpp"
#include "AnnealingProgram.hpp"
#include "DWQMI.hpp"
#include "RemoteAccelerator.hpp"

using namespace xacc;

namespace xacc {
namespace quantum {

/**
 * Wrapper for information related to the remote
 * D-Wave solver.
 */
struct DWSolver {
  std::string name;
  std::string description;
  double jRangeMin;
  double jRangeMax;
  double hRangeMin;
  double hRangeMax;
  int nQubits;
  std::vector<std::pair<int, int>> edges;
};

// class AnnealScheduleGenerator {
// public:
//   std::vector<std::pair<double, double>>
//   generate(std::shared_ptr<Anneal> annealInst) {
//     std::vector<std::pair<double, double>> as;
//     double s = 1;

//     auto ta = mpark::get<double>(annealInst->getParameter(0));
//     auto tp = mpark::get<double>(annealInst->getParameter(1));
//     auto tq = mpark::get<double>(annealInst->getParameter(2));
//     auto direction = mpark::get<std::string>(annealInst->getParameter(3));

//     auto ti = ta + tp;
//     auto tf = ta + tp + tq;
//     auto end = std::make_pair(tf, 1.0);
//     auto midpt = std::make_pair(tq, (s / tf) * tp);
//     auto midpause = std::make_pair(ti, (s / tf) * ti);

//     if (direction == "forward") {
//       as.push_back({0, 0});
//     } else {
//       as.push_back({0, 1});
//     }

//     if (tp == 0 && tq == 0) {
//       as.push_back(end);
//     } else if (tp == 0) {
//       if (end.first == midpt.first && end.second == midpt.second) {
//         as.push_back(end);
//       } else {
//         as.push_back(midpt);
//         as.push_back(end);
//       }
//     } else {
//       if (end.first == midpause.first && end.second == midpause.second) {
//         as.push_back(midpt);
//         as.push_back(end);
//       } else {
//         as.push_back(midpt);
//         as.push_back(midpause);
//         as.push_back(end);
//       }
//     }

//     return as;
//   }

//   const std::string getAsString(std::vector<std::pair<double, double>> &as) {
//     std::stringstream ss;
//     ss << "[";
//     for (auto e : as) {
//       ss << "[" << e.first << "," << e.second << "],";
//     }
//     return ss.str().substr(0, ss.str().length() - 1) + "]";
//   }
// };

/**
 * The DWAccelerator is an XACC Accelerator that
 * takes D-Wave IR and executes the quantum machine
 * instructions via remote HTTP invocations.
 */
class DWAccelerator : public RemoteAccelerator {
public:
  DWAccelerator() : RemoteAccelerator() {}
  DWAccelerator(std::shared_ptr<Client> client) : RemoteAccelerator(client) {}

  std::vector<std::pair<int, int>> getConnectivity();

  const std::string processInput(
      std::shared_ptr<AcceleratorBuffer> buffer,
      std::vector<std::shared_ptr<CompositeInstruction>> functions) override;

  void processResponse(std::shared_ptr<AcceleratorBuffer> buffer,
                       const std::string &response) override;

  void execute(std::shared_ptr<AcceleratorBuffer> buffer,
               const std::vector<std::shared_ptr<CompositeInstruction>>
                   functions) override {
    int counter = 0;
    std::vector<std::shared_ptr<AcceleratorBuffer>> tmpBuffers;
    for (auto f : functions) {
      auto tmpBuffer = std::make_shared<AcceleratorBuffer>(
          buffer->name() + std::to_string(counter), buffer->size());
      RemoteAccelerator::execute(tmpBuffer, f);
      buffer->appendChild(buffer->name() + std::to_string(counter), tmpBuffer);
      counter++;
    }

    return;
  }

  virtual std::vector<std::shared_ptr<IRTransformation>>
  getIRTransformations() {
    std::vector<std::shared_ptr<IRTransformation>> v;
    return v;
  }

  void initialize(const HeterogeneousMap &params = {}) override;
  void updateConfiguration(const HeterogeneousMap &config) override {
    if (config.keyExists<int>("shots")) {
      shots = config.get<int>("shots");
    }
    if (config.stringExists("backend")) {
      backend = config.getString("backend");
    }
  }

  const std::vector<std::string> configurationKeys() override {
    return {"shots", "backend"};
  }

  const std::string name() const override { return "dwave"; }

  const std::string description() const override {
    return "The D-Wave Accelerator executes Ising Hamiltonian parameters "
           "on a remote D-Wave QPU.";
  }

  virtual ~DWAccelerator() {}
  /**
   * Return all relevant RigettiAccelerator runtime options.
   * Users can set the api-key, execution type, and number of triels
   * from the command line with these options.
   */
  virtual OptionPairs getOptions() {
    OptionPairs desc{
        {"dwave-api-key", "Provide the D-Wave API key. This is used if "
                          "$HOME/.dwave_config is not found"},
        {"dwave-api-url", "The D-Wave SAPI URL, "
                          "https://qubist.dwavesys.com/sapi "
                          "used by default."},
        {"dwave-solver", "The name of the solver to run on."},
        {"dwave-num-reads",
         "The number of executions on the chip for the given problem."},
        {"dwave-anneal-time",
         "The time to evolve the chip - an integer in microseconds."},
        {"dwave-thermalization", "The thermalization..."},
        {"dwave-list-solvers", "List the available solvers at the Qubist URL."},
        {"dwave-solve-type", "The solve type, qubo or ising"},
        {"dwave-skip-initialization", ""}};
    return desc;
  }

  virtual bool handleOptions(const std::map<std::string, std::string> &map) {
    if (map.count("dwave-list-solvers")) {
      initialize();

      for (auto s : availableSolvers) {
        xacc::info("Available D-Wave Solver: " + std::string(s.first));
      }
      return true;
    }
    return false;
  }

protected:
  int shots = 1000;
  std::string backend = "";

  std::string apiKey;
  std::string url;
  std::map<std::string, DWSolver> availableSolvers;

  void searchAPIKey(std::string &key, std::string &url);
  void findApiKeyInFile(std::string &key, std::string &url,
                        const std::string &p);
};

} // namespace quantum
} // namespace xacc

#endif
