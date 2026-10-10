#ifndef ECLAT_H
#define ECLAT_H

#include "DataUtils.h"
#include <vector>

class Eclat {
public:
    explicit Eclat(int minSupCount, bool collectItemsets = true);

    // Run the Eclat algorithm using vertical TID-list representation
    AlgorithmResult run(const Database& db);

private:
    int minSupCount;
    bool collectItemsets;
    size_t totalFrequentCount;
    std::vector<FrequentItemset> resultItemsets;

    struct ItemTids {
        Item item;
        std::vector<int> tids;
    };

    // Recursive mining of equivalence classes
    void eclatRecurse(Itemset& prefix, const std::vector<ItemTids>& items);

    // Fast intersection of two sorted TID vectors
    static std::vector<int> intersectTids(const std::vector<int>& t1, const std::vector<int>& t2);
};

#endif // ECLAT_H
