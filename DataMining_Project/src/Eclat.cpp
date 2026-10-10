#include "Eclat.h"
#include <algorithm>
#include <map>

Eclat::Eclat(int minSupCount, bool collectItemsets)
    : minSupCount(minSupCount), collectItemsets(collectItemsets), totalFrequentCount(0) {}

std::vector<int> Eclat::intersectTids(const std::vector<int>& t1, const std::vector<int>& t2) {
    std::vector<int> result;
    result.reserve(std::min(t1.size(), t2.size()));

    size_t i = 0, j = 0;
    size_t n1 = t1.size(), n2 = t2.size();

    while (i < n1 && j < n2) {
        if (t1[i] == t2[j]) {
            result.push_back(t1[i]);
            ++i;
            ++j;
        } else if (t1[i] < t2[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}

void Eclat::eclatRecurse(Itemset& prefix, const std::vector<ItemTids>& items) {
    size_t n = items.size();

    for (size_t i = 0; i < n; ++i) {
        const auto& itemA = items[i];
        std::vector<ItemTids> nextEquivClass;

        for (size_t j = i + 1; j < n; ++j) {
            const auto& itemB = items[j];

            std::vector<int> interTids = intersectTids(itemA.tids, itemB.tids);
            if (static_cast<int>(interTids.size()) >= minSupCount) {
                totalFrequentCount++;
                if (collectItemsets) {
                    Itemset newItemset = prefix;
                    newItemset.push_back(itemA.item);
                    newItemset.push_back(itemB.item);
                    resultItemsets.push_back({std::move(newItemset), static_cast<int>(interTids.size())});
                }
                nextEquivClass.push_back({itemB.item, std::move(interTids)});
            }
        }

        if (!nextEquivClass.empty()) {
            prefix.push_back(itemA.item);
            eclatRecurse(prefix, nextEquivClass);
            prefix.pop_back();
        }
    }
}

AlgorithmResult Eclat::run(const Database& db) {
    AlgorithmResult result;
    result.algorithmName = "Eclat";
    result.minSupCount = minSupCount;
    totalFrequentCount = 0;
    resultItemsets.clear();

    auto startTime = std::chrono::high_resolution_clock::now();

    // Step 1: Scan database once to build initial TID lists for all 1-items
    std::map<Item, std::vector<int>> itemTidMap;
    for (int tid = 0; tid < static_cast<int>(db.size()); ++tid) {
        for (Item it : db[tid]) {
            itemTidMap[it].push_back(tid);
        }
    }

    // Step 2: Filter 1-itemsets by minSupCount
    std::vector<ItemTids> frequent1Items;
    for (auto& pair : itemTidMap) {
        if (static_cast<int>(pair.second.size()) >= minSupCount) {
            totalFrequentCount++;
            if (collectItemsets) {
                resultItemsets.push_back({{pair.first}, static_cast<int>(pair.second.size())});
            }
            frequent1Items.push_back({pair.first, std::move(pair.second)});
        }
    }

    // Step 3: DFS exploration on vertical TID equivalence classes
    Itemset prefix;
    eclatRecurse(prefix, frequent1Items);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.frequentItemsetsCount = totalFrequentCount;
    result.frequentItemsets = std::move(resultItemsets);
    result.peakMemoryMB = DataUtils::getPeakMemoryMB();

    return result;
}
