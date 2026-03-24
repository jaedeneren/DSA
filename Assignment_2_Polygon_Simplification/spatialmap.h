#pragma once
#include "polygon.h"
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>

// Hash function so we can use an x,y coordinate pair as a map key
struct CellHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

class SpatialMap {
private:
    double cellSize = 500.0; // Grid cell size. Tune this based on your dataset!
    
    // The Spatial Grid: Maps a Grid Coordinate (X, Y) to a list of Line Segments
    std::unordered_map<std::pair<int, int>, std::vector<std::pair<Vertex*, Vertex*>>, CellHash> grid;

    // Helper math functions for intersection testing
    std::vector<std::pair<int, int>> getCellsForSegment(Vertex* v1, Vertex* v2);
    bool checkIntersection(Vertex* p1, Vertex* p2, Vertex* q1, Vertex* q2);
    int orientation(Vertex* p, Vertex* q, Vertex* r);
    bool onSegment(Vertex* p, Vertex* q, Vertex* r);

public:
    void buildIndex(const Polygon& poly);
    bool isTopologyValid(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E);
    void updateIndex(Vertex* A, Vertex* D, Vertex* E);
};