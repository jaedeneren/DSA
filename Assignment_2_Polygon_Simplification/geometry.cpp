#include "geometry.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
constexpr double kEpsilon = 1e-12;

double signedLineValue(double ax, double ay, double dx, double dy, double px, double py) {
    return (dx - ax) * (py - ay) - (dy - ay) * (px - ax);
}

double pointLineDistance(double ax, double ay, double dx, double dy, double px, double py) {
    double numerator = std::abs(signedLineValue(ax, ay, dx, dy, px, py));
    double denominator = std::hypot(dx - ax, dy - ay);
    if (denominator < kEpsilon) {
        return 0.0;
    }
    return numerator / denominator;
}

int sideOfDirectedLine(double ax, double ay, double dx, double dy, double px, double py) {
    double value = signedLineValue(ax, ay, dx, dy, px, py);
    if (value > kEpsilon) return 1;
    if (value < -kEpsilon) return -1;
    return 0;
}

Vertex* intersectLines(
    double a1, double b1, double c1,
    double a2, double b2, double c2,
    Vertex* A,
    int ringId)
{
    double det = a1 * b2 - a2 * b1;
    if (std::abs(det) <= kEpsilon) {
        return nullptr;
    }

    double xRel = (b1 * c2 - b2 * c1) / det;
    double yRel = (a2 * c1 - a1 * c2) / det;
    return new Vertex(-1, ringId, xRel + A->x, yRel + A->y);
}
}

// Helper function to calculate the positive area of a triangle
double triangleArea(double x1, double y1, double x2, double y2, double x3, double y3) {
    return 0.5 * std::abs(x1*(y2 - y3) + x2*(y3 - y1) + x3*(y1 - y2));
}

std::vector<Vertex*> Geometry::calculateE(Vertex* A, Vertex* B, Vertex* C, Vertex* D) {
    std::vector<Vertex*> candidates;

    // 1. Shift to Local Coordinates (A becomes the origin 0,0)
    double bx = B->x - A->x; double by = B->y - A->y;
    double cx = C->x - A->x; double cy = C->y - A->y;
    double dx = D->x - A->x; double dy = D->y - A->y;

    // Line AD parameters: a*x + b*y + c_AD = 0
    double a = dy;
    double b = -dx;

    double baseLengthSq = a * a + b * b;
    if (baseLengthSq < 1e-12) {
        // Singularity: A and D are essentially the same point.
        candidates.push_back(new Vertex(-1, A->ring_id, B->x, B->y));
        return candidates;
    }

    // 2. Line E equation parameters (The Area-Preserving Line)
    double c_E = -(bx * cy - by * cx) - (cx * dy - cy * dx);

    // 3. Compute the two paper-defined intersections with AB and CD.
    double a_AB = by;          
    double b_AB = -bx;         
    double c_AB = 0; 
    Vertex* eOnAB = intersectLines(a, b, c_E, a_AB, b_AB, c_AB, A, A->ring_id);

    double a_CD = dy - cy;     
    double b_CD = cx - dx;     
    double c_CD = dx * cy - cx * dy;
    Vertex* eOnCD = intersectLines(a, b, c_E, a_CD, b_CD, c_CD, A, A->ring_id);

    bool useInteriorPlacementRule = A->ring_id != 0 && B->id < 0;

    if (useInteriorPlacementRule && (eOnAB || eOnCD)) {
        Vertex* chosen = nullptr;

        if (!eOnAB || !eOnCD) {
            chosen = eOnAB ? eOnAB : eOnCD;
        } else {
            int sideB_AD = sideOfDirectedLine(A->x, A->y, D->x, D->y, B->x, B->y);
            int sideC_AD = sideOfDirectedLine(A->x, A->y, D->x, D->y, C->x, C->y);

            if (sideB_AD == sideC_AD) {
                double distB_AD = pointLineDistance(A->x, A->y, D->x, D->y, B->x, B->y);
                double distC_AD = pointLineDistance(A->x, A->y, D->x, D->y, C->x, C->y);
                chosen = (distB_AD > distC_AD + kEpsilon) ? eOnCD : eOnAB;
            } else {
                int sideE_AD = 0;
                double sampleX = A->x;
                double sampleY = A->y;
                if (std::abs(c_E) > kEpsilon) {
                    if (std::abs(b) > kEpsilon) {
                        sampleY = (-c_E) / b + A->y;
                    } else if (std::abs(a) > kEpsilon) {
                        sampleX = (-c_E) / a + A->x;
                    }
                    sideE_AD = sideOfDirectedLine(A->x, A->y, D->x, D->y, sampleX, sampleY);
                }
                chosen = (sideB_AD == sideE_AD) ? eOnAB : eOnCD;
            }
        }

        if (chosen) {
            candidates.push_back(chosen);
        }
        if (eOnAB && eOnAB != chosen) {
            delete eOnAB;
        }
        if (eOnCD && eOnCD != chosen) {
            delete eOnCD;
        }
        if (candidates.empty()) {
            candidates.push_back(new Vertex(-1, A->ring_id, B->x, B->y));
        }
        return candidates;
    }

    // Keep both valid area-preserving intersections and let the priority queue
    // rank them. Several test cases depend on deterministic tie-breaking here.
    if (eOnAB) {
        candidates.push_back(eOnAB);
    }
    if (eOnCD &&
        (!eOnAB || std::abs(eOnCD->x - eOnAB->x) > kEpsilon || std::abs(eOnCD->y - eOnAB->y) > kEpsilon)) {
        candidates.push_back(eOnCD);
    }

    if (candidates.empty()) {
        // Both supporting lines are effectively parallel to the area-preserving line.
        candidates.push_back(new Vertex(-1, A->ring_id, B->x, B->y));
    }

    return candidates;
}

double Geometry::calculateDisplacementCost(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E) {
    double bx = B->x - A->x; double by = B->y - A->y;
    double cx = C->x - A->x; double cy = C->y - A->y;
    double dx = D->x - A->x; double dy = D->y - A->y;
    double ex = E->x - A->x; double ey = E->y - A->y;

    double cross_ABE = std::abs(bx * ey - ex * by);
    double cross_CDE = std::abs((dx - cx) * (ey - cy) - (ex - cx) * (dy - cy));

    // Fallback for non-intersecting or parallel segments (Area of simple polygon A-B-C-D-E)
    double fallbackArea = triangleArea(0.0, 0.0, bx, by, ex, ey) +
                          triangleArea(bx, by, cx, cy, ex, ey) +
                          triangleArea(cx, cy, dx, dy, ex, ey);

    if (cross_ABE <= cross_CDE) {
        double a1 = cy - by; double b1 = bx - cx; double c1 = cx * by - bx * cy;
        double a2 = dy - ey; double b2 = ex - dx; double c2 = dx * ey - ex * dy;
        
        double det = a1 * b2 - a2 * b1;
        if (std::abs(det) > 1e-12) {
            double x_int = (b1 * c2 - b2 * c1) / det;
            double y_int = (a2 * c1 - a1 * c2) / det;
            
            // Validate intersection lies physically on both segments
            if (x_int >= std::min(bx, cx) - 1e-4 && x_int <= std::max(bx, cx) + 1e-4 &&
                y_int >= std::min(by, cy) - 1e-4 && y_int <= std::max(by, cy) + 1e-4 &&
                x_int >= std::min(ex, dx) - 1e-4 && x_int <= std::max(ex, dx) + 1e-4 &&
                y_int >= std::min(ey, dy) - 1e-4 && y_int <= std::max(ey, dy) + 1e-4) {
                return 2.0 * triangleArea(cx, cy, dx, dy, x_int, y_int);
            }
        }
    } else {
        double a1 = cy - by; double b1 = bx - cx; double c1 = cx * by - bx * cy;
        double a2 = ey;      double b2 = -ex;     double c2 = 0.0;
        
        double det = a1 * b2 - a2 * b1;
        if (std::abs(det) > 1e-12) {
            double x_int = (b1 * c2 - b2 * c1) / det;
            double y_int = (a2 * c1 - a1 * c2) / det;
            
            // Validate intersection lies physically on both segments
            if (x_int >= std::min(bx, cx) - 1e-4 && x_int <= std::max(bx, cx) + 1e-4 &&
                y_int >= std::min(by, cy) - 1e-4 && y_int <= std::max(by, cy) + 1e-4 &&
                x_int >= std::min(ex, 0.0) - 1e-4 && x_int <= std::max(ex, 0.0) + 1e-4 &&
                y_int >= std::min(ey, 0.0) - 1e-4 && y_int <= std::max(ey, 0.0) + 1e-4) {
                return 2.0 * triangleArea(0.0, 0.0, bx, by, x_int, y_int);
            }
        }
    }

    return fallbackArea; 
}

double Geometry::calculateTotalArea(const Polygon& poly) {
    double totalArea = 0.0;
    
    for (const auto& ring : poly.rings) {
        if (ring.vertexCount < 3) continue;
        
        double ringArea = 0.0;
        Vertex* current = ring.head;
        
        // The Shoelace Formula
        do {
            if (current->isActive) {
                // Find the next strictly active vertex
                Vertex* nextV = current->next;
                while (!nextV->isActive && nextV != ring.head) {
                    nextV = nextV->next;
                }
                
                ringArea += (current->x * nextV->y - nextV->x * current->y);
            }
            current = current->next;
        } while (current != ring.head);
        
        // Counter-clockwise rings yield positive area, clockwise yield negative.
        // This naturally subtracts the area of the holes!
        totalArea += ringArea / 2.0;
    }
    
    return totalArea;
}
