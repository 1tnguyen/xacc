#include "DWAccelerator.hpp"
// #include "DWQMICompiler.hpp"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/ServiceProperties.h"

#include <memory>
#include <set>

using namespace cppmicroservices;

namespace {

/**
 */
class US_ABI_LOCAL DWaveActivator : public BundleActivator {

public:
  DWaveActivator() {}

  /**
   */
  void Start(BundleContext context) {
    auto acc = std::make_shared<xacc::quantum::DWAccelerator>();
    context.RegisterService<xacc::Accelerator>(acc);
    context.RegisterService<xacc::OptionsProvider>(acc);

    // auto c = std::make_shared<xacc::quantum::DWQMICompiler>();
    // context.RegisterService<xacc::Compiler>(c);
    // context.RegisterService<xacc::OptionsProvider>(c);

  }

  /**
   */
  void Stop(BundleContext /*context*/) {}
};

} // namespace

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(DWaveActivator)
