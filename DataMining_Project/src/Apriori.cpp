#include "Apriori.h"
#include <algorithm>
#include <unordered_map>
#include <map>

Apriori::Apriori(int minSupCount, bool collectItemsets)
    : minSupCount(minSupCount), collectItemsets(collectItemsets) {}

std::vector<Itemset> Apriori::generateCandidates(const std::vector<Itemset>& prevLevel, int k) {
    std::vector<Itemset> candidates;
    size_t n = prevLevel.size();

    // Level k requires joining two itemsets of length k-1
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            // Check if first k-2 items match
            bool match = true;
            for (int p = 0; p < k - 2; ++p) {
                if (prevLevel[i][p] != prevLevel[j][p]) {
                    match = false;
                    break;
                }
            }
            if (!match) {
                // Since prevLevel is sorted lexicographically, if prefix doesn't match,
                // subsequent j for the same i will not match if the differing element is smaller
                if (prevLevel[i][0] != prevLevel[j][0]) {
                    break;
                }
                continue;
            }

            // Create candidate
            Itemset cand = prevLevel[i];
            cand.push_back(prevLevel[j][k - 2]);

            // Pruning step: check if all (k-1) subsets are in prevLevel
            if (!hasInfrequentSubsets(cand, prevLevel, k)) {
                candidates.push_back(std::move(cand));
            }
        }
    }
    return candidates;
}

bool Apriori::hasInfrequentSubsets(const Itemset& candidate, const std::vector<Itemset>& prevLevel, int k) {
    // candidate has size k.
    // The two subsets omitting candidate[k-1] and candidate[k-2] are already known to be in prevLevel.
    // We only need to check the subsets omitting candidate[0] .. candidate[k-3].
    Itemset subset(k - 1);
    for (int omit = 0; omit < k - 2; ++omit) {
        int idx = 0;
        for (int i = 0; i < k; ++i) {
            if (i == omit) continue;
            subset[idx++] = candidate[i];
        }
        if (!std::binary_search(prevLevel.begin(), prevLevel.end(), subset)) {
            return true; // Infrequent subset detected
        }
    }
    return false;
}

AlgorithmResult Apriori::run(const Database& db) {
    AlgorithmResult result;
    result.algorithmName = "Apriori";
    result.minSupCount = minSupCount;
    result.frequentItemsetsCount = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Step 1: Scan DB to get L_1
    std::unordered_map<Item, int> itemCounts;
    for (const auto& trans : db) {
        for (Item it : trans) {
            itemCounts[it]++;
        }
    }

    std::vector<Itemset> currentL;
    std::unordered_map<Item, bool> isFrequentItem;
    Item maxItem = 0;

    for (const auto& pair : itemCounts) {
        if (pair.second >= minSupCount) {
            currentL.push_back({pair.first});
            isFrequentItem[pair.first] = true;
            if (pair.first > maxItem) maxItem = pair.first;
            if (collectItemsets) {
                result.frequentItemsets.push_back({{pair.first}, pair.second});
            }
            result.frequentItemsetsCount++;
        }
    }

    // Sort L_1 lexicographically
    std::sort(currentL.begin(), currentL.end());

    // Pre-filter database transactions: keep only frequent items and transactions with >= 2 items
    Database filteredDb;
    filteredDb.reserve(db.size());
    for (const auto& trans : db) {
        Transaction ft;
        for (Item it : trans) {
            if (isFrequentItem[it]) {
                ft.push_back(it);
            }
        }
        if (ft.size() >= 2) {
            filteredDb.push_back(std::move(ft));
        }
    }

    // Step 2: Generate L_k for k >= 2
    int k = 2;
    while (!currentL.empty()) {
        std::vector<Itemset> Ck = generateCandidates(currentL, k);
        if (Ck.empty()) break;

        // Build index of candidates by their first item for fast transaction checking
        std::vector<std::vector<size_t>> candIndex(maxItem + 1);
        for (size_t idx = 0; idx < Ck.size(); ++idx) {
            candIndex[Ck[idx][0]].push_back(idx);
        }

        std::vector<int> counts(Ck.size(), 0);

        // Count candidate support
        for (const auto& trans : filteredDb) {
            if (static_cast<int>(trans.size()) < k) continue;

            for (size_t i = 0; i <= trans.size() - k; ++i) {
                Item it = trans[i];
                if (it > maxItem) continue;

                for (size_t candIdx : candIndex[it]) {
                    const auto& cand = Ck[candIdx];
                    if (std::includes(trans.begin() + i, trans.end(), cand.begin(), cand.end())) {
                        counts[candIdx]++;
                    }
                }
            }
        }

        // Form L_k
        std::vector<Itemset> nextL;
        for (size_t idx = 0; idx < Ck.size(); ++idx) {
            if (counts[idx] >= minSupCount) {
                if (collectItemsets) {
                    result.frequentItemsets.push_back({Ck[idx], counts[idx]});
                }
                result.frequentItemsetsCount++;
                nextL.push_back(std::move(Ck[idx]));
            }
        }

        currentL = std::move(nextL);
        std::sort(currentL.begin(), currentL.end());
        k++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.peakMemoryMB = DataUtils::getPeakMemoryMB();

    return result;
}
