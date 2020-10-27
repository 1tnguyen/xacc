#include "xacc.hpp"
#include <gtest/gtest.h>
#include "svd_opt.hpp"
#include "xacc_service.hpp"
#include <Eigen/Dense>
#include "linalg_utils/utils.hpp"
#include <unsupported/Eigen/KroneckerProduct>

namespace {
Eigen::MatrixXcd generateRandomUnitary(size_t dim)
{
  Eigen::MatrixXcd mat = Eigen::MatrixXcd::Random(dim, dim);
  auto QR = mat.householderQr();
  Eigen::MatrixXcd qMat = QR.householderQ() * Eigen::MatrixXcd::Identity(dim, dim);
  return qMat;
}
}

using namespace xacc;
using namespace xacc::circuits;

TEST(SvdOptTester, checkGate) 
{
    auto utry = generateRandomUnitary(8);
    SVD::Gate gate(utry,  { 0, 1, 2 });
    auto inv_gate = gate.getInverse();
   
    Eigen::MatrixXcd test1 = utry * inv_gate.getUnitaryMat();
    Eigen::MatrixXcd test2 = inv_gate.getUnitaryMat() * utry;
    Eigen::MatrixXcd Iden = Eigen::MatrixXcd::Identity(utry.rows(), utry.cols());
    EXPECT_TRUE(utils::allClose(test1, Iden));
    EXPECT_TRUE(utils::allClose(test2, Iden));
    EXPECT_EQ(gate.getLocation().size(), inv_gate.getLocation().size());
    for (size_t i = 0; i < gate.getLocation().size(); ++i)
    {
        EXPECT_EQ(gate.getLocation()[i], inv_gate.getLocation()[i]);
    }
    EXPECT_EQ(gate.isFixed(), inv_gate.isFixed());
}

TEST(SvdOptTester, checkApplyRight) 
{
    const auto u1 = generateRandomUnitary(8);
    const auto u2 = generateRandomUnitary(4); 
    SVD::Gate g( u2, { 0, 1 });
    SVD::CircuitTensor ct(u1, {});
    ct.applyRight(g);
    Eigen::MatrixXcd prod = Eigen::kroneckerProduct(u2, Eigen::Matrix2cd::Identity()) * u1;
    Eigen::MatrixXcd prod_contract = ct.getCurrentUnitary();
    std::cout << "Prod:\n" << prod << "\n";
    std::cout << "Test:\n" << prod_contract << "\n";
    EXPECT_TRUE(utils::allClose(prod, prod_contract));
}

int main(int argc, char **argv) {
  xacc::Initialize(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  auto ret = RUN_ALL_TESTS();
  xacc::Finalize();
  return ret;
}