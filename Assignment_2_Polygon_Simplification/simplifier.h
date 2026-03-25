#pragma once
#include <cmath>
#include <queue>
#include "polygon.h"
#include "spatialmap.h"

struct CollapseCandidate {
    Vertex *A, *B, *C, *D;
    Vertex *E; // The proposed replacement
    double cost;
    int candidateRank;
    int evalVersion;
    
    bool operator>(const CollapseCandidate& other) const {
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
    SpatialMap spatialMap;
    std::priority_queue<CollapseCandidate, std::vector<CollapseCandidate>, std::greater<CollapseCandidate>> pq;

    void evaluateAndPush(Vertex* A);

public:
    double totalDisplacement = 0.0;
    Simplifier(Polygon& p) : poly(p) {}
    ~Simplifier();
    void buildInitialQueue();
    void run(int targetVertices);
};
