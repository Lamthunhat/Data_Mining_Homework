#ifndef COMPACT_FP_GROWTH_H
#define COMPACT_FP_GROWTH_H

#include "DataUtils.h"
#include <vector>
#include <deque>
#include <unordered_map>

/**
 * CompactFPGrowth: Optimized FP-Growth using Hybrid Array-and-Pointer FP-Tree (Compressed FP-Tree).
 * 
 * In standard FP-Tree, each item requires an individual node with pointers (parent, children, nodeLink).
 * In sparse datasets, transactions form long non-branching chains.
 * CompactNode stores an array of sequential items in a single node if there is no branching point.
 * Nodes only split (like a Radix Trie / Patricia Trie) when transactions branch off.
 * This significantly reduces memory consumption and node count on sparse data.
 */
class CompactFPGrowth {
public:
    explicit CompactFPGrowth(int minSupCount, bool collectItemsets = true);

    // Run the Compact (Array-Pointer Hybrid) FP-Growth algorithm
    AlgorithmResult run(const Database& db);

    // Get total nodes created in the global Compact FP-Tree
    size_t getGlobalNodeCount() const { return globalNodeCount; }

private:
    int minSupCount;
    bool collectItemsets;
    size_t totalFrequentCount;
    size_t globalNodeCount;
    std::vector<FrequentItemset> resultItemsets;

    // Compact Node storing multiple sequential items along a non-branching path
    struct CompactNode {
        std::vector<Item> items; // Array of sequential items
        int count = 0;
        CompactNode* parent = nullptr;
        std::vector<CompactNode*> children;

        CompactNode() = default;
        CompactNode(std::vector<Item> its, int cnt, CompactNode* par)
            : items(std::move(its)), count(cnt), parent(par) {}

        CompactNode* findChildStartingWith(Item it) const {
            for (auto* child : children) {
                if (!child->items.empty() && child->items[0] == it) {
                    return child;
                }
            }
            return nullptr;
        }
    };

    // Reference to an item inside a CompactNode's items vector
    struct NodeItemRef {
        CompactNode* node;
        int itemIndex;
    };

    struct HeaderEntry {
        Item item;
        int count = 0;
        std::vector<NodeItemRef> occurrences;
    };

    struct CompactFPTree {
        CompactNode* root = nullptr;
        std::vector<HeaderEntry> headerTable;
        std::unordered_map<Item, size_t> itemToHeaderIndex;
        // Arena pool using std::deque to ensure stable memory addresses
        std::deque<CompactNode> nodePool;

        CompactFPTree() {
            nodePool.emplace_back(std::vector<Item>{}, 0, nullptr);
            root = &nodePool.back();
        }

        CompactNode* createNode(std::vector<Item> items, int count, CompactNode* parent) {
            nodePool.emplace_back(std::move(items), count, parent);
            return &nodePool.back();
        }

        void addHeaderItem(Item item, int count) {
            itemToHeaderIndex[item] = headerTable.size();
            headerTable.push_back({item, count, {}});
        }

        void insertTransaction(const std::vector<Item>& transaction, int count);
        void buildHeaderReferences();
    };

    void mineCompactTree(CompactFPTree& tree, const Itemset& prefix);
};

#endif // COMPACT_FP_GROWTH_H
