#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <vector>
#include <string>
#include <iostream>
#include <chrono>

using Item = int;
using Itemset = std::vector<Item>;
using Transaction = std::vector<Item>;
using Database = std::vector<Transaction>;

struct FrequentItemset {
    Itemset items;
    int support;
};

struct DatasetStats {
    int numTransactions = 0;
    int numDistinctItems = 0;
    int maxItem = 0;
    double avgTransactionLength = 0.0;
    int maxTransactionLength = 0;
    int minTransactionLength = 0;
    double density = 0.0; // (avgTransactionLength / numDistinctItems) * 100%
};

struct AlgorithmResult {
    std::string algorithmName;
    std::string datasetName;
    double minSupRatio = 0.0;
    int minSupCount = 0;
    double executionTimeMs = 0.0;
    size_t frequentItemsetsCount = 0;
    double peakMemoryMB = 0.0;
    std::vector<FrequentItemset> frequentItemsets;
};

namespace DataUtils {
    // Load transaction database from space-separated .dat/.txt file
    Database loadDatabase(const std::string& filepath, DatasetStats& stats);

    // Display formatted dataset statistics
    void printDatasetStats(const std::string& datasetName, const DatasetStats& stats);

    // Save frequent itemsets to text file
    bool saveItemsets(const std::string& filepath, const std::vector<FrequentItemset>& itemsets, int totalTransactions);

    // Get current process peak memory usage in MB
    double getPeakMemoryMB();

    // Verify if two result sets contain the exact same frequent itemsets and supports
    bool verifyResults(const std::vector<FrequentItemset>& r1, const std::vector<FrequentItemset>& r2);
}

#endif // DATA_UTILS_H
