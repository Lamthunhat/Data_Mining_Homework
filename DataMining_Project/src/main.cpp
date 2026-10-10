#include "DataUtils.h"
#include "Apriori.h"
#include "Eclat.h"
#include "FPGrowth.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

void printBanner() {
    std::cout << "====================================================================\n";
    std::cout << "        FREQUENT ITEMSET MINING BENCHMARK (C++17)                   \n";
    std::cout << "        Algorithms: Apriori [AS94b], Eclat [Zak00], FP-Growth [HPY00]\n";
    std::cout << "====================================================================\n";
}

void printUsage(const char* progName) {
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " -benchmark\n";
    std::cout << "  " << progName << " -data <path> -minsup <val> [-alg <apriori|eclat|fpgrowth|all>] [-out <file>]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -benchmark       Run automated benchmark suite on chess, mushroom, retail datasets\n";
    std::cout << "  -data <path>     Path to dataset file (.dat / .txt)\n";
    std::cout << "  -minsup <val>    Minimum support threshold (e.g., 0.90 for 90% or 2000 for count)\n";
    std::cout << "  -alg <name>      Algorithm to execute: 'apriori', 'eclat', 'fpgrowth', or 'all' (default: all)\n";
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

    AlgorithmResult rApriori, rEclat, rFP;
    bool runAp = (algChoice == "all" || algChoice == "apriori");
    bool runEc = (algChoice == "all" || algChoice == "eclat");
    bool runFp = (algChoice == "all" || algChoice == "fpgrowth");

    if (runAp) {
        std::cout << ">> Running Apriori...\n";
        Apriori apriori(minSupCount, !outFile.empty() || algChoice == "all");
        rApriori = apriori.run(db);
        std::cout << "   [Apriori]   Time: " << std::fixed << std::setprecision(2) << rApriori.executionTimeMs << " ms"
                  << " | Patterns: " << rApriori.frequentItemsetsCount
                  << " | Peak Mem: " << rApriori.peakMemoryMB << " MB\n";
    }

    if (runEc) {
        std::cout << ">> Running Eclat...\n";
        Eclat eclat(minSupCount, !outFile.empty() || algChoice == "all");
        rEclat = eclat.run(db);
        std::cout << "   [Eclat]     Time: " << std::fixed << std::setprecision(2) << rEclat.executionTimeMs << " ms"
                  << " | Patterns: " << rEclat.frequentItemsetsCount
                  << " | Peak Mem: " << rEclat.peakMemoryMB << " MB\n";
    }

    if (runFp) {
        std::cout << ">> Running FP-Growth...\n";
        FPGrowth fpgrowth(minSupCount, !outFile.empty() || algChoice == "all");
        rFP = fpgrowth.run(db);
        std::cout << "   [FP-Growth] Time: " << std::fixed << std::setprecision(2) << rFP.executionTimeMs << " ms"
                  << " | Patterns: " << rFP.frequentItemsetsCount
                  << " | Peak Mem: " << rFP.peakMemoryMB << " MB\n";
    }

    if (algChoice == "all") {
        if (rApriori.frequentItemsetsCount == rEclat.frequentItemsetsCount &&
            rEclat.frequentItemsetsCount == rFP.frequentItemsetsCount) {
            std::cout << "   >>> [OK] Verification Passed: All 3 algorithms found exactly "
                      << rFP.frequentItemsetsCount << " frequent itemsets!\n";
        } else {
            std::cout << "   >>> [WARNING] Pattern count mismatch across algorithms!\n";
        }
    }

    if (!outFile.empty()) {
        const auto& toSave = runFp ? rFP.frequentItemsets : (runEc ? rEclat.frequentItemsets : rApriori.frequentItemsets);
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

            // If ratio is very low on dense dataset or retail, Apriori may take too long, we track safely
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
                std::cout << "   - Apriori:   " << std::setw(9) << std::fixed << std::setprecision(2) << tAp << " ms"
                          << " | Patterns: " << cAp << "\n";
            } else {
                std::cout << "   - Apriori:     (Skipped - candidate space too large for BFS)\n";
            }

            Eclat ec(minSupCount, false);
            auto resEc = ec.run(db);
            std::cout << "   - Eclat:     " << std::setw(9) << std::fixed << std::setprecision(2) << resEc.executionTimeMs << " ms"
                      << " | Patterns: " << resEc.frequentItemsetsCount << "\n";

            FPGrowth fp(minSupCount, false);
            auto resFp = fp.run(db);
            std::cout << "   - FP-Growth: " << std::setw(9) << std::fixed << std::setprecision(2) << resFp.executionTimeMs << " ms"
                      << " | Patterns: " << resFp.frequentItemsetsCount << "\n";

            results.push_back({config.datasetName, ratio, resFp.frequentItemsetsCount, tAp, resEc.executionTimeMs, resFp.executionTimeMs});
        }
    }

    std::cout << "\n\n========================================================================================\n";
    std::cout << "                           SUMMARY BENCHMARK REPORT TABLE                               \n";
    std::cout << "========================================================================================\n";
    std::cout << "| Dataset               | MinSup (%) | Patterns | Apriori (ms) | Eclat (ms) | FP-Growth (ms) |\n";
    std::cout << "|-----------------------|------------|----------|--------------|------------|----------------|\n";

    for (const auto& r : results) {
        std::string shortName = r.dataset.substr(0, r.dataset.find(' '));
        std::cout << "| " << std::left << std::setw(21) << shortName
                  << " | " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << (r.minSup * 100.0) << "%"
                  << " | " << std::setw(8) << r.count << " | ";
        if (r.timeApriori >= 0) {
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << r.timeApriori << " | ";
        } else {
            std::cout << std::setw(12) << "N/A" << " | ";
        }
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << r.timeEclat << " | "
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.timeFPGrowth << " |\n";
    }
    std::cout << "========================================================================================\n";
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
