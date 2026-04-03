#pragma once
#include <cmath>
#include <queue>
#include "polygon.h"
#include "spatialmap.h"

struct CollapseCandidate {
    Vertex *A, *B, *C, *D;                             // Consecutive active vertices considered for collapse
    Vertex *E; // The proposed replacement
    double cost;                                       // Pure geometric displacement estimate
    double priorityCost;                               // Queue score after optional penalties
    int candidateRank;                                 // Earlier paper-style candidates win ties
    int evalVersion;                                   // Used to reject stale queue entries
    
    bool operator>(const CollapseCandidate& other) const {
        if (std::abs(priorityCost - other.priorityCost) > 1e-12) return priorityCost > other.priorityCost;
        if (std::abs(cost - other.cost) > 1e-12) return cost > other.cost;
        if (candidateRank != other.candidateRank) return candidateRank > other.candidateRank;
        if (std::abs(E->y - other.E->y) > 1e-9) return E->y > other.E->y;
        if (A->id != other.A->id) return A->id < other.A->id;
        if (std::abs(E->x - other.E->x) > 1e-9) return E->x > other.E->x;
        return false;
    }
};

class Simplifier {
private:
    Polygon& poly;
    SpatialMap spatialMap;                             // Accelerates local topology checks
    std::priority_queue<CollapseCandidate, std::vector<CollapseCandidate>, std::greater<CollapseCandidate>> pq;

    void evaluateAndPush(Vertex* A);                   // Rebuild every candidate that starts at A

public:
    double totalDisplacement = 0.0;
    Simplifier(Polygon& p) : poly(p) {}
    ~Simplifier();
    void buildInitialQueue();
    void run(int targetVertices);
};
