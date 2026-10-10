#include "CompactFPGrowth.h"
#include <algorithm>

CompactFPGrowth::CompactFPGrowth(int minSupCount, bool collectItemsets)
    : minSupCount(minSupCount), collectItemsets(collectItemsets), totalFrequentCount(0), globalNodeCount(0) {}

void CompactFPGrowth::CompactFPTree::insertTransaction(const std::vector<Item>& transaction, int count) {
    CompactNode* current = root;
    size_t tIdx = 0;
    size_t n = transaction.size();

    while (tIdx < n) {
        Item it = transaction[tIdx];
        CompactNode* child = current->findChildStartingWith(it);

        if (child == nullptr) {
            // Case 1: No child starts with transaction[tIdx].
            // Create a single compact node containing all remaining items in the transaction.
            std::vector<Item> remItems(transaction.begin() + tIdx, transaction.end());
            CompactNode* newNode = createNode(std::move(remItems), count, current);
            current->children.push_back(newNode);
            return;
        }

        // Case 2: Child starts with transaction[tIdx]. Find common prefix length.
        size_t commonLen = 0;
        size_t childLen = child->items.size();
        while (commonLen < childLen && tIdx + commonLen < n &&
               child->items[commonLen] == transaction[tIdx + commonLen]) {
            commonLen++;
        }

        if (commonLen == childLen) {
            // Subcase 2A: The entire child node matches.
            child->count += count;
            current = child;
            tIdx += commonLen;
        } else {
            // Subcase 2B: Partial match. Split the child node (Radix/Patricia trie split).
            // 1. Create suffix node for the unshared items of child
            std::vector<Item> suffixItems(child->items.begin() + commonLen, child->items.end());
            CompactNode* suffixNode = createNode(std::move(suffixItems), child->count, child);
            suffixNode->children = std::move(child->children);
            for (auto* subChild : suffixNode->children) {
                subChild->parent = suffixNode;
            }

            // 2. Truncate child to the common prefix
            child->items.resize(commonLen);
            child->count += count;
            child->children.clear();
            child->children.push_back(suffixNode);

            tIdx += commonLen;
            if (tIdx < n) {
                // 3. Create a new branch for the unshared items of the new transaction
                std::vector<Item> branchItems(transaction.begin() + tIdx, transaction.end());
                CompactNode* branch = createNode(std::move(branchItems), count, child);
                child->children.push_back(branch);
            }
            return;
        }
    }
}

void CompactFPGrowth::CompactFPTree::buildHeaderReferences() {
    // Single-pass DFS to populate header table occurrences
    struct StackFrame {
        CompactNode* node;
    };
    std::vector<StackFrame> stack;
    stack.push_back({root});

    while (!stack.empty()) {
        CompactNode* curr = stack.back().node;
        stack.pop_back();

        for (size_t i = 0; i < curr->items.size(); ++i) {
            Item it = curr->items[i];
            auto itHeader = itemToHeaderIndex.find(it);
            if (itHeader != itemToHeaderIndex.end()) {
                headerTable[itHeader->second].occurrences.push_back({curr, static_cast<int>(i)});
            }
        }

        for (auto* ch : curr->children) {
            stack.push_back({ch});
        }
    }
}

void CompactFPGrowth::mineCompactTree(CompactFPTree& tree, const Itemset& prefix) {
    // Process items in header table bottom-up (reverse order)
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
        for (const auto& occ : entry.occurrences) {
            CompactNode* node = occ.node;
            int itemIdx = occ.itemIndex;
            int pathCount = node->count;

            std::vector<Item> path;
            // Items in the current node prior to itemIdx
            for (int k = itemIdx - 1; k >= 0; --k) {
                path.push_back(node->items[k]);
            }

            // Items in ancestor nodes up to root
            CompactNode* p = node->parent;
            while (p != nullptr && !p->items.empty()) {
                for (int k = static_cast<int>(p->items.size()) - 1; k >= 0; --k) {
                    path.push_back(p->items[k]);
                }
                p = p->parent;
            }

            if (!path.empty()) {
                std::reverse(path.begin(), path.end()); // Orient root-down
                condPatterns.emplace_back(std::move(path), pathCount);
            }
        }

        // 2. Count frequency of items in conditional pattern base
        std::unordered_map<Item, int> condCounts;
        for (const auto& cp : condPatterns) {
            for (Item it : cp.first) {
                condCounts[it] += cp.second;
            }
        }

        // Filter frequent items in conditional base
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

        // 3. Build conditional compact FP-tree
        CompactFPTree condTree;
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

        condTree.buildHeaderReferences();

        // 4. Recursive mining
        mineCompactTree(condTree, newPattern);
    }
}

AlgorithmResult CompactFPGrowth::run(const Database& db) {
    AlgorithmResult result;
    result.algorithmName = "Compact-FP-Growth";
    result.minSupCount = minSupCount;
    totalFrequentCount = 0;
    globalNodeCount = 0;
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

    // Step 3: Build global compact FP-tree
    CompactFPTree globalTree;
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

    globalNodeCount = globalTree.nodePool.size();
    globalTree.buildHeaderReferences();

    // Step 4: Mine global compact FP-tree
    Itemset prefix;
    mineCompactTree(globalTree, prefix);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.frequentItemsetsCount = totalFrequentCount;
    result.frequentItemsets = std::move(resultItemsets);
    result.peakMemoryMB = DataUtils::getPeakMemoryMB();

    return result;
}
