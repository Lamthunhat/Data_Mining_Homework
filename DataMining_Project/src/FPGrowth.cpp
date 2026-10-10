#include "FPGrowth.h"
#include <algorithm>

FPGrowth::FPGrowth(int minSupCount, bool collectItemsets)
    : minSupCount(minSupCount), collectItemsets(collectItemsets), totalFrequentCount(0) {}

void FPGrowth::FPTree::insertTransaction(const std::vector<Item>& transaction, int count) {
    FPNode* current = root;
    for (Item item : transaction) {
        FPNode* child = current->findChild(item);
        if (child != nullptr) {
            child->count += count;
            current = child;
        } else {
            FPNode* newNode = createNode(item, count, current);
            current->children.push_back(newNode);

            // Link node in header table
            size_t headerIdx = itemToHeaderIndex[item];
            HeaderEntry& entry = headerTable[headerIdx];
            if (entry.head == nullptr) {
                entry.head = newNode;
                entry.tail = newNode;
            } else {
                entry.tail->nodeLink = newNode;
                entry.tail = newNode;
            }
            current = newNode;
        }
    }
}

void FPGrowth::mineFPTree(FPTree& tree, const Itemset& prefix) {
    // Process items in header table bottom-up (from lowest support to highest support)
    // The header table was constructed with descending support order, so bottom-up is reverse iteration
    for (int idx = static_cast<int>(tree.headerTable.size()) - 1; idx >= 0; --idx) {
        const auto& entry = tree.headerTable[idx];
        Item item = entry.item;
        int support = entry.count;

        Itemset newPattern = prefix;
        newPattern.push_back(item);
        totalFrequentCount++;

        if (collectItemsets) {
            Itemset sortedPattern = newPattern;
            std::sort(sortedPattern.begin(), sortedPattern.end());
            resultItemsets.push_back({std::move(sortedPattern), support});
        }

        // 1. Collect conditional pattern base for item
        std::vector<std::pair<std::vector<Item>, int>> condPatterns;
        FPNode* curr = entry.head;
        while (curr != nullptr) {
            int pathCount = curr->count;
            std::vector<Item> path;
            FPNode* p = curr->parent;
            while (p != nullptr && p->item != -1) {
                path.push_back(p->item);
                p = p->parent;
            }
            if (!path.empty()) {
                condPatterns.emplace_back(std::move(path), pathCount);
            }
            curr = curr->nodeLink;
        }

        // 2. Count frequency of items in conditional patterns
        std::unordered_map<Item, int> condCounts;
        for (const auto& cp : condPatterns) {
            for (Item it : cp.first) {
                condCounts[it] += cp.second;
            }
        }

        // Filter frequent items in conditional pattern base
        std::vector<std::pair<Item, int>> frequentCondItems;
        for (const auto& pair : condCounts) {
            if (pair.second >= minSupCount) {
                frequentCondItems.push_back(pair);
            }
        }

        if (frequentCondItems.empty()) {
            continue;
        }

        // Sort conditional items by descending support (tie-breaker by item ID)
        std::sort(frequentCondItems.begin(), frequentCondItems.end(),
                  [](const std::pair<Item, int>& a, const std::pair<Item, int>& b) {
                      if (a.second != b.second) return a.second > b.second;
                      return a.first < b.first;
                  });

        std::unordered_map<Item, int> condRank;
        for (size_t r = 0; r < frequentCondItems.size(); ++r) {
            condRank[frequentCondItems[r].first] = static_cast<int>(r);
        }

        // 3. Build conditional FP-tree
        FPTree condTree;
        for (const auto& pair : frequentCondItems) {
            condTree.addHeaderItem(pair.first, pair.second);
        }

        for (const auto& cp : condPatterns) {
            std::vector<Item> filteredPath;
            for (Item it : cp.first) {
                if (condRank.find(it) != condRank.end()) {
                    filteredPath.push_back(it);
                }
            }
            if (!filteredPath.empty()) {
                std::sort(filteredPath.begin(), filteredPath.end(),
                          [&condRank](Item a, Item b) {
                              return condRank[a] < condRank[b];
                          });
                condTree.insertTransaction(filteredPath, cp.second);
            }
        }

        // 4. Recursive mining
        mineFPTree(condTree, newPattern);
    }
}

AlgorithmResult FPGrowth::run(const Database& db) {
    AlgorithmResult result;
    result.algorithmName = "FP-Growth";
    result.minSupCount = minSupCount;
    totalFrequentCount = 0;
    resultItemsets.clear();

    auto startTime = std::chrono::high_resolution_clock::now();

    // Step 1: Count support of 1-items across database
    std::unordered_map<Item, int> itemCounts;
    for (const auto& trans : db) {
        for (Item it : trans) {
            itemCounts[it]++;
        }
    }

    // Step 2: Filter and sort frequent items by descending support
    std::vector<std::pair<Item, int>> frequentItems;
    for (const auto& pair : itemCounts) {
        if (pair.second >= minSupCount) {
            frequentItems.push_back(pair);
        }
    }

    if (frequentItems.empty()) {
        result.executionTimeMs = 0.0;
        result.frequentItemsetsCount = 0;
        return result;
    }

    std::sort(frequentItems.begin(), frequentItems.end(),
              [](const std::pair<Item, int>& a, const std::pair<Item, int>& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });

    std::unordered_map<Item, int> itemRank;
    for (size_t i = 0; i < frequentItems.size(); ++i) {
        itemRank[frequentItems[i].first] = static_cast<int>(i);
    }

    // Step 3: Build global FP-tree
    FPTree globalTree;
    for (const auto& pair : frequentItems) {
        globalTree.addHeaderItem(pair.first, pair.second);
    }

    for (const auto& trans : db) {
        std::vector<Item> filteredTrans;
        for (Item it : trans) {
            if (itemRank.find(it) != itemRank.end()) {
                filteredTrans.push_back(it);
            }
        }
        if (!filteredTrans.empty()) {
            std::sort(filteredTrans.begin(), filteredTrans.end(),
                      [&itemRank](Item a, Item b) {
                          return itemRank[a] < itemRank[b];
                      });
            globalTree.insertTransaction(filteredTrans, 1);
        }
    }

    // Step 4: Mine global FP-tree
    Itemset prefix;
    mineFPTree(globalTree, prefix);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.frequentItemsetsCount = totalFrequentCount;
    result.frequentItemsets = std::move(resultItemsets);
    result.peakMemoryMB = DataUtils::getPeakMemoryMB();

    return result;
}
