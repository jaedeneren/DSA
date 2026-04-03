#include "spatialmap.h"
#include <cstdint>
#include <unordered_set>

namespace {
// Treat undirected segments as the same key so duplicate counting is avoided.
std::uint64_t makeSegmentKey(Vertex* a, Vertex* b) {
    std::uintptr_t lo = reinterpret_cast<std::uintptr_t>(std::min(a, b));
    std::uintptr_t hi = reinterpret_cast<std::uintptr_t>(std::max(a, b));
    return static_cast<std::uint64_t>(lo) ^ (static_cast<std::uint64_t>(hi) << 1);
}
}

// --- Helper Math: Checks if two line segments cross ---
int SpatialMap::orientation(Vertex* p, Vertex* q, Vertex* r) {
    double val = (q->y - p->y) * (r->x - q->x) - (q->x - p->x) * (r->y - q->y);
    
    if (std::abs(val) < 1e-9) return 0; // Collinear
    
    return (val > 0) ? 1 : 2; // Clockwise or Counterclockwise
}

bool SpatialMap::onSegment(Vertex* p, Vertex* q, Vertex* r) {
    return q->x <= std::max(p->x, r->x) && q->x >= std::min(p->x, r->x) &&
           q->y <= std::max(p->y, r->y) && q->y >= std::min(p->y, r->y);
}

bool SpatialMap::checkIntersection(Vertex* p1, Vertex* p2, Vertex* q1, Vertex* q2) {
    int o1 = orientation(p1, p2, q1);
    int o2 = orientation(p1, p2, q2);
    int o3 = orientation(q1, q2, p1);
    int o4 = orientation(q1, q2, p2);

    // 1. General intersection case (crossing)
    if (o1 != o2 && o3 != o4) {
        // If they share an actual vertex pointer, they are adjacent segments forming a valid angle.
        // It causes o1 != o2 && o3 != o4 to be true, but it's not a topological break.
        if (p1 == q1 || p1 == q2 || p2 == q1 || p2 == q2) {
            // Ignore the cross-check, they share an endpoint. We rely on collinear checks below for fold-backs.
        } else {
            return true; // True topological crossing
        }
    }

    // Helper: checks if 'q' lies strictly inside the physical bounds of segment 'p-r'.
    auto strictlyInside = [](Vertex* p, Vertex* q, Vertex* r) {
        // Exact topological endpoints don't count as "inside"
        if (p == q || r == q) return false;
        return q->x <= std::max(p->x, r->x) && q->x >= std::min(p->x, r->x) &&
               q->y <= std::max(p->y, r->y) && q->y >= std::min(p->y, r->y);
    };

    // 2. Collinear overlaps (fold-backs)
    // If they are collinear (o == 0), they intersect if they collinearly overlap, 
    // which means one vertex is drawn strictly over the path of the other segment.
    if (o1 == 0 && strictlyInside(p1, q1, p2)) return true;
    if (o2 == 0 && strictlyInside(p1, q2, p2)) return true;
    if (o3 == 0 && strictlyInside(q1, p1, q2)) return true;
    if (o4 == 0 && strictlyInside(q1, p2, q2)) return true;

    // 3. Degenerate zero-area identical overlapping spikes
    if ((p1 == q1 && p2 == q2) || (p1 == q2 && p2 == q1)) return true;

    return false;
}

// --- Spatial Grid Logic ---

std::vector<std::pair<int, int>> SpatialMap::getCellsForSegment(Vertex* v1, Vertex* v2) {
    std::vector<std::pair<int, int>> cells;
    
    // A segment is stored in every grid cell touched by its axis-aligned bounding box.
    int minX = static_cast<int>(std::floor(std::min(v1->x, v2->x) / cellSize));
    int maxX = static_cast<int>(std::floor(std::max(v1->x, v2->x) / cellSize));
    int minY = static_cast<int>(std::floor(std::min(v1->y, v2->y) / cellSize));
    int maxY = static_cast<int>(std::floor(std::max(v1->y, v2->y) / cellSize));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            cells.push_back({x, y});
        }
    }
    return cells;
}

void SpatialMap::buildIndex(const Polygon& poly) {
    grid.clear();
    for (const auto& ring : poly.rings) {
        if (ring.vertexCount < 2) continue;
        
        Vertex* current = ring.head;
        do {
            if (current->isActive) {
                Vertex* nextV = current->next;
                // Note: assuming initial polygon has no inactive vertices yet
                auto cells = getCellsForSegment(current, nextV);
                for (const auto& cell : cells) {
                    grid[cell].push_back({current, nextV});
                }
            }
            current = current->next;
        } while (current != ring.head);
    }
}

bool SpatialMap::isTopologyValid(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E) {
    // We only need to check the cells that the new segments AE and ED will touch!
    auto cellsAE = getCellsForSegment(A, E);
    auto cellsED = getCellsForSegment(E, D);
    
    // Combine them to check all relevant local segments
    std::vector<std::pair<int, int>> cellsToCheck = cellsAE;
    cellsToCheck.insert(cellsToCheck.end(), cellsED.begin(), cellsED.end());

    for (const auto& cell : cellsToCheck) {
        for (const auto& segment : grid[cell]) {
            // Ignore inactive segments
            if (!segment.first->isActive || !segment.second->isActive) continue;

            // ADDED: Ignore the segments we are actively trying to collapse!
            if (segment.first == A && segment.second == B) continue;
            if (segment.first == B && segment.second == C) continue;
            if (segment.first == C && segment.second == D) continue;
            
            // If our new lines intersect an existing line, topology is broken!
            if (checkIntersection(A, E, segment.first, segment.second)) return false;
            if (checkIntersection(E, D, segment.first, segment.second)) return false;
        }
    }
    return true; // Safe to collapse!
}

int SpatialMap::countInactiveOriginalCrossings(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E) {
    if (A->ring_id == 0 || A->id >= 0 || B->id < 0 || C->id < 0) {
        return 0;
    }

    auto cellsAE = getCellsForSegment(A, E);
    auto cellsED = getCellsForSegment(E, D);
    std::vector<std::pair<int, int>> cellsToCheck = cellsAE;
    cellsToCheck.insert(cellsToCheck.end(), cellsED.begin(), cellsED.end());

    std::unordered_set<std::uint64_t> countedSegments; // Prevent double-counting segments seen in multiple cells
    int conflicts = 0;

    for (const auto& cell : cellsToCheck) {
        for (const auto& segment : grid[cell]) {
            if (segment.first->isActive || segment.second->isActive) continue;
            if (segment.first->ring_id != A->ring_id || segment.second->ring_id != A->ring_id) continue;
            if (segment.first->id < 0 || segment.second->id < 0) continue;
            if ((segment.first->id == B->id && segment.second->id == C->id) ||
                (segment.first->id == C->id && segment.second->id == B->id)) {
                continue;
            }

            std::uint64_t key = makeSegmentKey(segment.first, segment.second);
            if (!countedSegments.insert(key).second) continue;

            if (checkIntersection(A, E, segment.first, segment.second) ||
                checkIntersection(E, D, segment.first, segment.second)) {
                ++conflicts;
            }
        }
    }

    return conflicts;
}

void SpatialMap::updateIndex(Vertex* A, Vertex* D, Vertex* E) {
    // 1. We don't need to manually remove AB, BC, CD. 
    // They are marked 'inactive' in the polygon, and isTopologyValid ignores inactive vertices!
    // This is another form of lazy deletion that saves immense CPU time.

    // 2. Just insert the two new segments into the grid
    auto cellsAE = getCellsForSegment(A, E);
    for (const auto& cell : cellsAE) {
        grid[cell].push_back({A, E});
    }

    auto cellsED = getCellsForSegment(E, D);
    for (const auto& cell : cellsED) {
        grid[cell].push_back({E, D});
    }
}
