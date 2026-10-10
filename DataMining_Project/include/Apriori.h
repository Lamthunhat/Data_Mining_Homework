#ifndef APRIORI_H
#define APRIORI_H

#include "DataUtils.h"
#include <vector>

class Apriori {
public:
    explicit Apriori(int minSupCount, bool collectItemsets = true);

    // Run the Apriori algorithm on transaction database
    AlgorithmResult run(const Database& db);

private:
    int minSupCount;
    bool collectItemsets;

    // Join step: generate candidate k-itemsets from L_{k-1}
    std::vector<Itemset> generateCandidates(const std::vector<Itemset>& prevLevel, int k);

    // Prune step: check if all (k-1) subsets of candidate exist in prevLevel
    bool hasInfrequentSubsets(const Itemset& candidate, const std::vector<Itemset>& prevLevel, int k);
};

#endif // APRIORI_H
