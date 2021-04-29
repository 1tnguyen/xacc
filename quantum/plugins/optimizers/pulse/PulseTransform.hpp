#pragma once

#include "IRTransformation.hpp"

namespace xacc {
namespace quantum {
class PulseTransform : public IRTransformation {
public:
  PulseTransform() {}
  void apply(std::shared_ptr<CompositeInstruction> program,
             const std::shared_ptr<Accelerator> accelerator,
             const HeterogeneousMap &options = {}) override;

  const IRTransformationType type() const override {
    return IRTransformationType::Optimization;
  }

  const std::string name() const override { return "quantum-control"; }
  const std::string description() const override { return ""; }
  // Pulse optimization info:
  struct OptInfo {
    // Static H
    std::string H0;
    // Control channels
    std::vector<std::string> controlOps;
    std::vector<std::string> controlChannels;
    double dt;
    std::string ham_json;
    std::vector<double> drive_channel_freqs;
  };

  OptInfo parseDeviceInfo(const HeterogeneousMap &exe_data) const;
};
} // namespace quantum
} // namespace xacc