#ifndef FPGROWTH_H
#define FPGROWTH_H

#include "DataUtils.h"
#include <vector>
#include <deque>
#include <unordered_map>

class FPGrowth {
public:
    explicit FPGrowth(int minSupCount, bool collectItemsets = true);

    // Run the FP-Growth algorithm using FP-Tree and conditional pattern bases
    AlgorithmResult run(const Database& db);

private:
    int minSupCount;
    bool collectItemsets;
    size_t totalFrequentCount;
    std::vector<FrequentItemset> resultItemsets;

    struct FPNode {
        Item item = -1;
        int count = 0;
        FPNode* parent = nullptr;
        FPNode* nodeLink = nullptr;
        std::vector<FPNode*> children;

        FPNode() = default;
        FPNode(Item it, int cnt, FPNode* par)
            : item(it), count(cnt), parent(par), nodeLink(nullptr) {}

        FPNode* findChild(Item it) const {
            for (auto* child : children) {
                if (child->item == it) return child;
            }
            return nullptr;
        }
    };

    struct HeaderEntry {
        Item item;
        int count = 0;
        FPNode* head = nullptr;
        FPNode* tail = nullptr;
    };

    struct FPTree {
        FPNode* root = nullptr;
        std::vector<HeaderEntry> headerTable;
        std::unordered_map<Item, size_t> itemToHeaderIndex;
        // Arena pool using std::deque to ensure stable memory addresses
        std::deque<FPNode> nodePool;

        FPTree() {
            nodePool.emplace_back(-1, 0, nullptr);
            root = &nodePool.back();
        }

        FPNode* createNode(Item item, int count, FPNode* parent) {
            nodePool.emplace_back(item, count, parent);
            return &nodePool.back();
        }

        void addHeaderItem(Item item, int count) {
            itemToHeaderIndex[item] = headerTable.size();
            headerTable.push_back({item, count, nullptr, nullptr});
        }

        void insertTransaction(const std::vector<Item>& transaction, int count);
    };

    // Recursive pattern growth on conditional FP-tree
    void mineFPTree(FPTree& tree, const Itemset& prefix);
};

#endif // FPGROWTH_H
