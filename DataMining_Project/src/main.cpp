#include "DataUtils.h"
#include "Apriori.h"
#include "Eclat.h"
#include "FPGrowth.h"
#include "CompactFPGrowth.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

void printBanner() {
    std::cout << "====================================================================\n";
    std::cout << "        FREQUENT ITEMSET MINING BENCHMARK (C++17)                   \n";
    std::cout << "        Algorithms: Apriori [AS94b], Eclat [Zak00], FP-Growth [HPY00]\n";
    std::cout << "        Optimized Variant: Compact FP-Growth (Array-Pointer Hybrid) \n";
    std::cout << "====================================================================\n";
}

void printUsage(const char* progName) {
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " -benchmark\n";
    std::cout << "  " << progName << " -data <path> -minsup <val> [-alg <apriori|eclat|fpgrowth|compactfp|all>] [-out <file>]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -benchmark       Run automated benchmark suite (including FP-Tree optimization study)\n";
    std::cout << "  -data <path>     Path to dataset file (.dat / .txt)\n";
    std::cout << "  -minsup <val>    Minimum support threshold (e.g., 0.90 for 90% or 2000 for count)\n";
    std::cout << "  -alg <name>      Algorithm: 'apriori', 'eclat', 'fpgrowth', 'compactfp', or 'all' (default: all)\n";
    std::cout << "  -out <file>      Optional path to output frequent itemsets to file\n\n";
}

struct BenchmarkConfig {
    std::string datasetPath;
    std::string datasetName;
    std::vector<double> minSupRatios;
};

void runSingleTest(const Database& db, const DatasetStats& stats, int minSupCount, double minSupRatio,
                   const std::string& algChoice, const std::string& outFile) {
    std::cout << "\n--------------------------------------------------------------------\n";
    std::cout << " Running with min_sup = " << minSupCount << " (" << std::fixed << std::setprecision(2) << (minSupRatio * 100.0) << "%)\n";
    std::cout << "--------------------------------------------------------------------\n";

    AlgorithmResult rApriori, rEclat, rFP, rCompactFP;
    bool runAp = (algChoice == "all" || algChoice == "apriori");
    bool runEc = (algChoice == "all" || algChoice == "eclat");
    bool runFp = (algChoice == "all" || algChoice == "fpgrowth");
    bool runCompact = (algChoice == "all" || algChoice == "compactfp");

    if (runAp) {
        std::cout << ">> Running Apriori...\n";
        Apriori apriori(minSupCount, !outFile.empty() || algChoice == "all");
        rApriori = apriori.run(db);
        std::cout << "   [Apriori]       Time: " << std::fixed << std::setprecision(2) << rApriori.executionTimeMs << " ms"
                  << " | Patterns: " << rApriori.frequentItemsetsCount
                  << " | Peak Mem: " << rApriori.peakMemoryMB << " MB\n";
    }

    if (runEc) {
        std::cout << ">> Running Eclat...\n";
        Eclat eclat(minSupCount, !outFile.empty() || algChoice == "all");
        rEclat = eclat.run(db);
        std::cout << "   [Eclat]         Time: " << std::fixed << std::setprecision(2) << rEclat.executionTimeMs << " ms"
                  << " | Patterns: " << rEclat.frequentItemsetsCount
                  << " | Peak Mem: " << rEclat.peakMemoryMB << " MB\n";
    }

    size_t stdNodes = 0;
    if (runFp) {
        std::cout << ">> Running Standard FP-Growth...\n";
        FPGrowth fpgrowth(minSupCount, !outFile.empty() || algChoice == "all");
        rFP = fpgrowth.run(db);
        stdNodes = fpgrowth.getGlobalNodeCount();
        std::cout << "   [FP-Growth]     Time: " << std::fixed << std::setprecision(2) << rFP.executionTimeMs << " ms"
                  << " | Patterns: " << rFP.frequentItemsetsCount
                  << " | Tree Nodes: " << stdNodes
                  << " | Peak Mem: " << rFP.peakMemoryMB << " MB\n";
    }

    size_t compactNodes = 0;
    if (runCompact) {
        std::cout << ">> Running Compact FP-Growth (Array-Pointer Hybrid)...\n";
        CompactFPGrowth compactFp(minSupCount, !outFile.empty() || algChoice == "all");
        rCompactFP = compactFp.run(db);
        compactNodes = compactFp.getGlobalNodeCount();
        std::cout << "   [Compact-FP]    Time: " << std::fixed << std::setprecision(2) << rCompactFP.executionTimeMs << " ms"
                  << " | Patterns: " << rCompactFP.frequentItemsetsCount
                  << " | Tree Nodes: " << compactNodes
                  << " | Peak Mem: " << rCompactFP.peakMemoryMB << " MB\n";
        if (runFp && stdNodes > 0) {
            double nodeReduction = 100.0 * (1.0 - static_cast<double>(compactNodes) / stdNodes);
            std::cout << "   >>> Node Count Reduction: " << std::fixed << std::setprecision(2)
                      << nodeReduction << "% (From " << stdNodes << " -> " << compactNodes << " nodes)\n";
        }
    }

    if (algChoice == "all") {
        if (rApriori.frequentItemsetsCount == rEclat.frequentItemsetsCount &&
            rEclat.frequentItemsetsCount == rFP.frequentItemsetsCount &&
            rFP.frequentItemsetsCount == rCompactFP.frequentItemsetsCount) {
            std::cout << "   >>> [OK] Verification Passed: All algorithms found exactly "
                      << rFP.frequentItemsetsCount << " frequent itemsets!\n";
        } else {
            std::cout << "   >>> [WARNING] Pattern count mismatch across algorithms!\n";
        }
    }

    if (!outFile.empty()) {
        const auto& toSave = runCompact ? rCompactFP.frequentItemsets :
                             (runFp ? rFP.frequentItemsets :
                             (runEc ? rEclat.frequentItemsets : rApriori.frequentItemsets));
        DataUtils::saveItemsets(outFile, toSave, stats.numTransactions);
        std::cout << "Saved " << toSave.size() << " itemsets to: " << outFile << "\n";
    }
}

void runBenchmarkSuite() {
    std::cout << "\n====================================================================\n";
    std::cout << "        EXECUTING AUTOMATED BENCHMARK SUITE                         \n";
    std::cout << "====================================================================\n";

    std::vector<BenchmarkConfig> suite = {
        {"data/chess.dat", "chess.dat (Dense, N=3,196, Items=75, AvgLen=37)", {0.95, 0.90, 0.85, 0.80}},
        {"data/mushroom.dat", "mushroom.dat (Medium, N=8,416, Items=119, AvgLen=23)", {0.50, 0.40, 0.30, 0.20}},
        {"data/retail.dat", "retail.dat (Sparse, N=88,162, Items=16,470, AvgLen=10.3)", {0.05, 0.02, 0.01, 0.005}}
    };

    struct BenchmarkRow {
        std::string dataset;
        double minSup;
        size_t count;
        double timeApriori;
        double timeEclat;
        double timeFPGrowth;
        double timeCompactFP;
        size_t stdNodes;
        size_t compactNodes;
    };

    std::vector<BenchmarkRow> results;

    for (const auto& config : suite) {
        DatasetStats stats;
        std::cout << "\nLoading dataset: " << config.datasetPath << " ...\n";
        Database db = DataUtils::loadDatabase(config.datasetPath, stats);
        if (db.empty()) {
            std::cout << "[Skipped] Dataset not found: " << config.datasetPath << "\n";
            continue;
        }
        DataUtils::printDatasetStats(config.datasetName, stats);

        for (double ratio : config.minSupRatios) {
            int minSupCount = std::max(1, static_cast<int>(ratio * stats.numTransactions));
            std::cout << "\n>>> Evaluating min_sup: " << std::fixed << std::setprecision(1) << (ratio * 100.0)
                      << "% (Count: " << minSupCount << ") ...\n";

            bool runApriori = true;
            if (config.datasetPath.find("chess") != std::string::npos && ratio < 0.75) runApriori = false;
            if (config.datasetPath.find("retail") != std::string::npos && ratio < 0.01) runApriori = false;

            double tAp = -1.0;
            size_t cAp = 0;
            if (runApriori) {
                Apriori ap(minSupCount, false);
                auto res = ap.run(db);
                tAp = res.executionTimeMs;
                cAp = res.frequentItemsetsCount;
                std::cout << "   - Apriori:     " << std::setw(9) << std::fixed << std::setprecision(2) << tAp << " ms"
                          << " | Patterns: " << cAp << "\n";
            } else {
                std::cout << "   - Apriori:       (Skipped - candidate space too large for BFS)\n";
            }

            Eclat ec(minSupCount, false);
            auto resEc = ec.run(db);
            std::cout << "   - Eclat:       " << std::setw(9) << std::fixed << std::setprecision(2) << resEc.executionTimeMs << " ms"
                      << " | Patterns: " << resEc.frequentItemsetsCount << "\n";

            FPGrowth fp(minSupCount, false);
            auto resFp = fp.run(db);
            std::cout << "   - FP-Growth:   " << std::setw(9) << std::fixed << std::setprecision(2) << resFp.executionTimeMs << " ms"
                      << " | Patterns: " << resFp.frequentItemsetsCount
                      << " | Nodes: " << fp.getGlobalNodeCount() << "\n";

            CompactFPGrowth compactFp(minSupCount, false);
            auto resCompact = compactFp.run(db);
            std::cout << "   - Compact-FP:  " << std::setw(9) << std::fixed << std::setprecision(2) << resCompact.executionTimeMs << " ms"
                      << " | Patterns: " << resCompact.frequentItemsetsCount
                      << " | Nodes: " << compactFp.getGlobalNodeCount() << "\n";

            results.push_back({config.datasetName, ratio, resFp.frequentItemsetsCount,
                               tAp, resEc.executionTimeMs, resFp.executionTimeMs, resCompact.executionTimeMs,
                               fp.getGlobalNodeCount(), compactFp.getGlobalNodeCount()});
        }
    }

    std::cout << "\n\n===================================================================================================================\n";
    std::cout << "                                SUMMARY BENCHMARK REPORT TABLE (ALL ALGORITHMS)                                    \n";
    std::cout << "===================================================================================================================\n";
    std::cout << "| Dataset       | MinSup (%) | Patterns | Apriori (ms) | Eclat (ms) | FP-Growth (ms) | Compact-FP (ms) |\n";
    std::cout << "|---------------|------------|----------|--------------|------------|----------------|-----------------|\n";

    for (const auto& r : results) {
        std::string shortName = r.dataset.substr(0, r.dataset.find(' '));
        std::cout << "| " << std::left << std::setw(13) << shortName
                  << " | " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << (r.minSup * 100.0) << "%"
                  << " | " << std::setw(8) << r.count << " | ";
        if (r.timeApriori >= 0) {
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << r.timeApriori << " | ";
        } else {
            std::cout << std::setw(12) << "N/A" << " | ";
        }
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << r.timeEclat << " | "
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.timeFPGrowth << " | "
                  << std::setw(15) << std::fixed << std::setprecision(2) << r.timeCompactFP << " |\n";
    }
    std::cout << "===================================================================================================================\n";

    std::cout << "\n===================================================================================================================\n";
    std::cout << "                     FP-TREE OPTIMIZATION COMPARISON: STANDARD vs. COMPACT (CÂU 12.B)                              \n";
    std::cout << "===================================================================================================================\n";
    std::cout << "| Dataset       | MinSup (%) | Standard Nodes | Compact Nodes | Node Reduction (%) | Standard (ms) | Compact (ms) |\n";
    std::cout << "|---------------|------------|----------------|---------------|--------------------|---------------|--------------|\n";

    for (const auto& r : results) {
        std::string shortName = r.dataset.substr(0, r.dataset.find(' '));
        double reduction = r.stdNodes > 0 ? 100.0 * (1.0 - static_cast<double>(r.compactNodes) / r.stdNodes) : 0.0;
        std::cout << "| " << std::left << std::setw(13) << shortName
                  << " | " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << (r.minSup * 100.0) << "%"
                  << " | " << std::setw(14) << r.stdNodes
                  << " | " << std::setw(13) << r.compactNodes
                  << " | " << std::setw(17) << std::fixed << std::setprecision(2) << reduction << " %"
                  << " | " << std::setw(13) << std::fixed << std::setprecision(2) << r.timeFPGrowth
                  << " | " << std::setw(12) << std::fixed << std::setprecision(2) << r.timeCompactFP << " |\n";
    }
    std::cout << "===================================================================================================================\n";
}

int main(int argc, char* argv[]) {
    printBanner();

    if (argc == 1) {
        std::cout << "No arguments provided. Running default benchmark suite...\n";
        std::cout << "(To see command-line options, run with '-h' or '--help')\n\n";
        runBenchmarkSuite();
        return 0;
    }

    std::string dataPath = "";
    double minSupVal = -1.0;
    std::string algChoice = "all";
    std::string outFile = "";
    bool isBenchmark = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-benchmark") {
            isBenchmark = true;
        } else if (arg == "-data" && i + 1 < argc) {
            dataPath = argv[++i];
        } else if (arg == "-minsup" && i + 1 < argc) {
            minSupVal = std::stod(argv[++i]);
        } else if (arg == "-alg" && i + 1 < argc) {
            algChoice = argv[++i];
            std::transform(algChoice.begin(), algChoice.end(), algChoice.begin(), ::tolower);
        } else if (arg == "-out" && i + 1 < argc) {
            outFile = argv[++i];
        }
    }

    if (isBenchmark) {
        runBenchmarkSuite();
        return 0;
    }

    if (dataPath.empty() || minSupVal <= 0) {
        std::cerr << "[Error] Missing required arguments: -data and -minsup\n";
        printUsage(argv[0]);
        return 1;
    }

    DatasetStats stats;
    Database db = DataUtils::loadDatabase(dataPath, stats);
    if (db.empty()) {
        std::cerr << "[Error] Could not load database from: " << dataPath << "\n";
        return 1;
    }

    DataUtils::printDatasetStats(dataPath, stats);

    int minSupCount = 1;
    double minSupRatio = 0.0;
    if (minSupVal < 1.0) {
        minSupRatio = minSupVal;
        minSupCount = std::max(1, static_cast<int>(minSupRatio * stats.numTransactions));
    } else {
        minSupCount = static_cast<int>(minSupVal);
        minSupRatio = static_cast<double>(minSupCount) / stats.numTransactions;
    }

    runSingleTest(db, stats, minSupCount, minSupRatio, algChoice, outFile);

    return 0;
}
