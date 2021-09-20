#include <gtest/gtest.h>
#include "xacc.hpp"
#include "Optimizer.hpp"
#include "xacc_service.hpp"
#include "Algorithm.hpp"
#include "xacc_observable.hpp"

TEST(DmSimAcceleratorMultipleGpusTester, testMultipleGPUs) {
  auto accelerator =
      xacc::getAccelerator("dm-sim", {{"gpus", 4}, {"shots", 1024}});
  auto staqCompiler = xacc::getCompiler("staq");
  auto program1 = staqCompiler
                      ->compile(R"(
// quantum ripple-carry adder from Cuccaro et al, quant-ph/0410184
OPENQASM 2.0;
include "qelib1.inc";
gate majority a,b,c 
{
    cx c, b;
    cx c, a;
    ccx a, b, c; 
}
gate unmaj a,b,c 
{
    ccx a, b, c;
    cx c, a;
    cx a, b; 
}
qreg cin[1];
qreg a[4];
qreg b[4];
qreg cout[1];
creg ans[5];
// set input states
x a[0]; // a = 0001
x b;    // b = 1111
// add a to b, storing result in b
majority cin[0],b[0],a[0];
majority a[0],b[1],a[1];
majority a[1],b[2],a[2];
majority a[2],b[3],a[3];
cx a[3],cout[0];
unmaj a[2],b[3],a[3];
unmaj a[1],b[2],a[2];
unmaj a[0],b[1],a[1];
unmaj cin[0],b[0],a[0];
measure b[0] -> ans[0];
measure b[1] -> ans[1];
measure b[2] -> ans[2];
measure b[3] -> ans[3];
measure cout[0] -> ans[4];
})",
                                accelerator)
                      ->getComposites()[0];

  auto buffer1 = xacc::qalloc(10);
  accelerator->execute(buffer1, program1);
  buffer1->print();
}

int main(int argc, char **argv) {
  xacc::Initialize();
  ::testing::InitGoogleTest(&argc, argv);
  const auto result = RUN_ALL_TESTS();
  xacc::Finalize();
  return result;
}
