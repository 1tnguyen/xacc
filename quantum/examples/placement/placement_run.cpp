#include "xacc.hpp"
#include <filesystem>

int main(int argc, char **argv) {
    xacc::Initialize(argc, argv);
    const std::vector<std::string> PLACEMENT_PLUGINS { "swap-shortest-path", "triQ", "enfield" };
    const std::vector<std::string> ENFIELD_ALLOCATORS{
        "Q_dynprog",  "Q_grdy",
        "Q_bmt",      "Q_simplified_bmt",
        "Q_ibmt",     "Q_simplified_ibmt",
        "Q_opt_bmt",  "Q_layered_bmt",
        "Q_ibm",      "Q_wpm",
        "Q_random",   "Q_qubiter",
        "Q_wqubiter", "Q_jku",
        "Q_sabre",    "Q_chw"};

    auto compiler = xacc::getCompiler("staq");
    // May need to choose a larger chip to make sure it can accommodate these circuits.
    auto accelerator = xacc::getAccelerator("ibm:ibmq_16_melbourne");
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
                    irt->apply(program, accelerator,
                               {{"allocator", allocator}});
                    std::cout << "HOWDY: \n" << program->nInstructions() << "\n";
                }
            }
            else 
            {
                std::cout << "Placement: " << placement << "\n";
                // Compile:
                auto IR = compiler->compile(qasmSrcStr);
                auto program = IR->getComposites()[0];
                irt->apply(program, accelerator);
                // TODO: analyze the post-placement circuit
                std::cout << "HOWDY: \n" << program->nInstructions() << "\n";
            }
        }       
    }   
    xacc::Finalize();
}