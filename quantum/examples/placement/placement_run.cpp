#include "xacc.hpp"
#include <filesystem>

std::pair<size_t, size_t> countGates(std::shared_ptr<xacc::CompositeInstruction>& in_composite) 
{
    size_t nbSingleQubitGates = 0;
    size_t nbTwoQubitGates = 0;
    for (const auto& inst : in_composite->getInstructions())
    {
        if (inst->bits().size() == 1) {
            ++nbSingleQubitGates;
        }
        if (inst->bits().size() == 2) {
            ++nbTwoQubitGates;
        }
    }
    return std::make_pair(nbSingleQubitGates, nbTwoQubitGates);
}

int main(int argc, char **argv) {
    xacc::Initialize(argc, argv);
    // const std::vector<std::string> PLACEMENT_PLUGINS { "swap-shortest-path", "enfield", "triQ" };
    const std::vector<std::string> PLACEMENT_PLUGINS { "swap-shortest-path", "enfield" };
    auto rotationFolding = xacc::getIRTransformation("rotation-folding");

    // const std::vector<std::string> ENFIELD_ALLOCATORS{
    //     "Q_dynprog",  "Q_grdy",
    //     "Q_bmt",      "Q_simplified_bmt",
    //     "Q_ibmt",     "Q_simplified_ibmt",
    //     "Q_opt_bmt",  "Q_layered_bmt",
    //     "Q_ibm",      "Q_wpm",
    //     "Q_random",   "Q_qubiter",
    //     "Q_wqubiter", "Q_jku",
    //     "Q_sabre",    "Q_chw"};
    const std::vector<std::string> ENFIELD_ALLOCATORS{
        "Q_ibm", "Q_wpm", "Q_sabre", "Q_chw" };
    auto compiler = xacc::getCompiler("staq");
    // May need to choose a larger chip to make sure it can accommodate these circuits.
    auto accelerator = xacc::getAccelerator("ibm:ibmq_manhattan");
    const std::string srcDirRath = std::string(QASM_SOURCE_DIR);
    for (const auto& entry : std::filesystem::directory_iterator(srcDirRath)) {
        const std::string sourceFile = entry.path().c_str();
        std::cout << "File: " << sourceFile << "\n";
        // Read source file:
        std::ifstream inFile;
        inFile.open(sourceFile);
        std::stringstream strStream;
        strStream << inFile.rdbuf();
        const std::string qasmSrcStr = strStream.str();
        for (const auto& placement :  PLACEMENT_PLUGINS) {
            auto irt = xacc::getIRTransformation(placement);
            if (placement == "enfield") {
                for (const auto& allocator : ENFIELD_ALLOCATORS) 
                {
                    std::cout << "Placement: " << placement << "::" << allocator << "\n";
                    // Compile:
                    auto IR = compiler->compile(qasmSrcStr);
                    auto program = IR->getComposites()[0];
                    rotationFolding->apply(program, nullptr);
                    auto [before_1q, before_2q] = countGates(program);
                    std::cout << "BEFORE: " << program->nInstructions() << "(" << before_1q << ", " << before_2q << ")\n";
                    irt->apply(program, accelerator,
                               {{"allocator", allocator}});
                    auto [after_1q, after_2q] = countGates(program);
                    std::cout << "AFTER: " << program->nInstructions() << "(" << after_1q << ", " << after_2q << ")\n";
                }
            }
            else 
            {
                std::cout << "Placement: " << placement << "\n";
                // Compile:
                auto IR = compiler->compile(qasmSrcStr);
                auto program = IR->getComposites()[0];
                rotationFolding->apply(program, nullptr);
                auto [before_1q, before_2q] = countGates(program);
                std::cout << "BEFORE: " << program->nInstructions() << "(" << before_1q << ", " << before_2q << ")\n";
                irt->apply(program, accelerator);
                // TODO: analyze the post-placement circuit
                auto [after_1q, after_2q] = countGates(program);
                std::cout << "AFTER: " << program->nInstructions() << "(" << after_1q << ", " << after_2q << ")\n";
            }
        }       
    }   
    xacc::Finalize();
}