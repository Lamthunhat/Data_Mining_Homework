#include "DataUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <iomanip>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace DataUtils {

Database loadDatabase(const std::string& filepath, DatasetStats& stats) {
    Database db;
    std::string actualPath = filepath;
    std::ifstream fin(actualPath);
    if (!fin.is_open()) {
        std::string altPath1 = "DataMining_Project/" + filepath;
        fin.open(altPath1);
        if (fin.is_open()) {
            actualPath = altPath1;
        } else {
            std::string altPath2 = "../" + filepath;
            fin.open(altPath2);
            if (fin.is_open()) {
                actualPath = altPath2;
            }
        }
    }
    if (!fin.is_open()) {
        std::cerr << "[Error] Cannot open dataset file: " << filepath << std::endl;
        return db;
    }

    std::string line;
    std::unordered_set<Item> distinctItems;
    long long totalItemsCount = 0;
    stats.minTransactionLength = 1000000;
    stats.maxTransactionLength = 0;
    stats.maxItem = 0;

    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '@') continue;
        std::stringstream ss(line);
        Transaction trans;
        Item item;
        while (ss >> item) {
            trans.push_back(item);
            distinctItems.insert(item);
            if (item > stats.maxItem) {
                stats.maxItem = item;
            }
        }
        if (trans.empty()) continue;

        // Sort items and remove duplicates within transaction
        std::sort(trans.begin(), trans.end());
        trans.erase(std::unique(trans.begin(), trans.end()), trans.end());

        int len = static_cast<int>(trans.size());
        totalItemsCount += len;
        if (len < stats.minTransactionLength) stats.minTransactionLength = len;
        if (len > stats.maxTransactionLength) stats.maxTransactionLength = len;

        db.push_back(std::move(trans));
    }

    stats.numTransactions = static_cast<int>(db.size());
    stats.numDistinctItems = static_cast<int>(distinctItems.size());
    if (stats.numTransactions > 0) {
        stats.avgTransactionLength = static_cast<double>(totalItemsCount) / stats.numTransactions;
    } else {
        stats.minTransactionLength = 0;
    }

    if (stats.numDistinctItems > 0) {
        stats.density = (stats.avgTransactionLength / stats.numDistinctItems) * 100.0;
    }

    return db;
}

void printDatasetStats(const std::string& datasetName, const DatasetStats& stats) {
    std::cout << "========================================================\n";
    std::cout << " Dataset Information: " << datasetName << "\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << " - Transactions (|D|):         " << stats.numTransactions << "\n";
    std::cout << " - Distinct Items (|I|):       " << stats.numDistinctItems << "\n";
    std::cout << " - Max Item ID:                " << stats.maxItem << "\n";
    std::cout << " - Avg Transaction Length:     " << std::fixed << std::setprecision(2) << stats.avgTransactionLength << "\n";
    std::cout << " - Min / Max Length:           " << stats.minTransactionLength << " / " << stats.maxTransactionLength << "\n";
    std::cout << " - Density (AvgLen / |I|):     " << std::fixed << std::setprecision(2) << stats.density << " %\n";
    std::cout << " - Classification:             " << (stats.density > 10.0 ? "Dense Dataset" : "Sparse Dataset") << "\n";
    std::cout << "========================================================\n";
}

bool saveItemsets(const std::string& filepath, const std::vector<FrequentItemset>& itemsets, int totalTransactions) {
    std::ofstream fout(filepath);
    if (!fout.is_open()) {
        std::cerr << "[Error] Cannot create output file: " << filepath << std::endl;
        return false;
    }

    for (const auto& fi : itemsets) {
        for (size_t i = 0; i < fi.items.size(); ++i) {
            fout << fi.items[i] << (i + 1 < fi.items.size() ? " " : "");
        }
        double supRatio = totalTransactions > 0 ? (static_cast<double>(fi.support) / totalTransactions) : 0.0;
        fout << " #SUP: " << fi.support << " (" << std::fixed << std::setprecision(4) << supRatio << ")\n";
    }
    return true;
}

double getPeakMemoryMB() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    }
    return 0.0;
#elif defined(__linux__) || defined(__APPLE__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
        return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
        return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
    }
    return 0.0;
#else
    return 0.0;
#endif
}

bool verifyResults(const std::vector<FrequentItemset>& r1, const std::vector<FrequentItemset>& r2) {
    if (r1.size() != r2.size()) {
        std::cerr << "[Mismatch] Count differs: " << r1.size() << " vs " << r2.size() << "\n";
        return false;
    }

    // Helper map
    auto makeMap = [](const std::vector<FrequentItemset>& list) {
        std::map<Itemset, int> m;
        for (const auto& fi : list) {
            Itemset sorted = fi.items;
            std::sort(sorted.begin(), sorted.end());
            m[sorted] = fi.support;
        }
        return m;
    };

    auto map1 = makeMap(r1);
    auto map2 = makeMap(r2);

    for (const auto& pair : map1) {
        auto it = map2.find(pair.first);
        if (it == map2.end() || it->second != pair.second) {
            std::cerr << "[Mismatch] Itemset or support mismatch!\n";
            return false;
        }
    }
    return true;
}

} // namespace DataUtils
