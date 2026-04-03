#include "geometry.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
constexpr double kEpsilon = 1e-12;

struct Point2D {
    double x;
    double y;
};

struct Segment2D {
    Point2D start;
    Point2D end;
};

using Interval = std::pair<double, double>;

Point2D subtract(const Point2D& lhs, const Point2D& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

double cross(const Point2D& lhs, const Point2D& rhs) {
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

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

void appendRingSegments(const Ring& ring, std::vector<Segment2D>& output, std::vector<double>& xEvents) {
    if (!ring.head || ring.vertexCount < 2) {
        return;
    }

    Vertex* current = ring.head;
    do {
        if (current->isActive) {
            Vertex* next = current->next;
            while (!next->isActive && next != ring.head) {
                next = next->next;
            }

            if (next != current) {
                output.push_back({{current->x, current->y}, {next->x, next->y}});
                xEvents.push_back(current->x);
            }
        }
        current = current->next;
    } while (current != ring.head);
}

std::vector<Segment2D> collectSegments(const Polygon& poly, std::vector<double>& xEvents) {
    std::vector<Segment2D> segments;
    // The symmetric-difference sweep works over explicit active segments from both polygons.
    for (const auto& ring : poly.rings) {
        appendRingSegments(ring, segments, xEvents);
    }
    return segments;
}

void sortAndUnique(std::vector<double>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end(), [](double lhs, double rhs) {
        return std::abs(lhs - rhs) <= 1e-9;
    }), values.end());
}

bool segmentIntersectionX(const Segment2D& lhs, const Segment2D& rhs, double& xIntersection) {
    Point2D p = lhs.start;
    Point2D q = rhs.start;
    Point2D r = subtract(lhs.end, lhs.start);
    Point2D s = subtract(rhs.end, rhs.start);

    double denominator = cross(r, s);
    Point2D qp = subtract(q, p);

    if (std::abs(denominator) <= kEpsilon) {
        return false;
    }

    double t = cross(qp, s) / denominator;
    double u = cross(qp, r) / denominator;

    if (t < -1e-9 || t > 1.0 + 1e-9 || u < -1e-9 || u > 1.0 + 1e-9) {
        return false;
    }

    xIntersection = p.x + t * r.x;
    return true;
}

struct SweepSegment {
    Segment2D segment;
    double minX;
    double maxX;

    double yAt(double x) const {
        return segment.start.y + (x - segment.start.x) * (segment.end.y - segment.start.y) /
            (segment.end.x - segment.start.x);
    }
};

struct SweepState {
    std::vector<SweepSegment> segments;
    std::vector<int> activeIndices;
    std::vector<int> activePositions;
    std::vector<std::pair<double, int>> startEvents;
    std::vector<std::pair<double, int>> endEvents;
    std::size_t nextStart = 0;
    std::size_t nextEnd = 0;
};

SweepState buildSweepState(const std::vector<Segment2D>& segments) {
    SweepState state;

    for (const auto& segment : segments) {
        double minX = std::min(segment.start.x, segment.end.x);
        double maxX = std::max(segment.start.x, segment.end.x);
        if (maxX - minX <= 1e-9) {
            continue;
        }

        int index = static_cast<int>(state.segments.size());
        state.segments.push_back({segment, minX, maxX});
        // Each segment contributes a start event and an end event to the vertical sweep.
        state.startEvents.emplace_back(minX, index);
        state.endEvents.emplace_back(maxX, index);
    }

    std::sort(state.startEvents.begin(), state.startEvents.end());
    std::sort(state.endEvents.begin(), state.endEvents.end());
    state.activePositions.assign(state.segments.size(), -1);
    return state;
}

void activateSegment(SweepState& state, int index) {
    if (state.activePositions[index] >= 0) {
        return;
    }

    state.activePositions[index] = static_cast<int>(state.activeIndices.size());
    state.activeIndices.push_back(index);
}

void deactivateSegment(SweepState& state, int index) {
    int position = state.activePositions[index];
    if (position < 0) {
        return;
    }

    int lastIndex = state.activeIndices.back();
    state.activeIndices[position] = lastIndex;
    state.activePositions[lastIndex] = position;
    state.activeIndices.pop_back();
    state.activePositions[index] = -1;
}

void advanceSweep(SweepState& state, double xEvent) {
    while (state.nextEnd < state.endEvents.size() && std::abs(state.endEvents[state.nextEnd].first - xEvent) <= 1e-9) {
        deactivateSegment(state, state.endEvents[state.nextEnd].second);
        ++state.nextEnd;
    }

    while (state.nextStart < state.startEvents.size() && std::abs(state.startEvents[state.nextStart].first - xEvent) <= 1e-9) {
        activateSegment(state, state.startEvents[state.nextStart].second);
        ++state.nextStart;
    }
}

std::vector<Interval> intervalsAtX(const SweepState& state, double x) {
    std::vector<double> intersections;
    intersections.reserve(state.activeIndices.size());

    for (int index : state.activeIndices) {
        intersections.push_back(state.segments[index].yAt(x));
    }

    std::sort(intersections.begin(), intersections.end());

    std::vector<Interval> intervals;
    // Even/odd filling: sorted y-intersections pair up into inside intervals.
    for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
        if (intersections[i + 1] > intersections[i] + 1e-9) {
            intervals.emplace_back(intersections[i], intersections[i + 1]);
        }
    }

    return intervals;
}

double intervalLength(const std::vector<Interval>& intervals) {
    double length = 0.0;
    for (const auto& interval : intervals) {
        length += interval.second - interval.first;
    }
    return length;
}

double intersectionLength(const std::vector<Interval>& lhs, const std::vector<Interval>& rhs) {
    double overlap = 0.0;
    std::size_t i = 0;
    std::size_t j = 0;

    while (i < lhs.size() && j < rhs.size()) {
        double low = std::max(lhs[i].first, rhs[j].first);
        double high = std::min(lhs[i].second, rhs[j].second);
        if (high > low) {
            overlap += high - low;
        }

        if (lhs[i].second < rhs[j].second) {
            ++i;
        } else {
            ++j;
        }
    }

    return overlap;
}

double xorCrossSectionLength(const SweepState& lhs, const SweepState& rhs, double x) {
    const std::vector<Interval> lhsIntervals = intervalsAtX(lhs, x);
    const std::vector<Interval> rhsIntervals = intervalsAtX(rhs, x);

    const double lhsLength = intervalLength(lhsIntervals);
    const double rhsLength = intervalLength(rhsIntervals);
    const double overlap = intersectionLength(lhsIntervals, rhsIntervals);
    return lhsLength + rhsLength - 2.0 * overlap;
}
}

// Helper function to calculate the positive area of a triangle
double triangleArea(double x1, double y1, double x2, double y2, double x3, double y3) {
    return 0.5 * std::abs(x1*(y2 - y3) + x2*(y3 - y1) + x3*(y1 - y2));
}

std::vector<Vertex*> Geometry::calculateE(Vertex* A, Vertex* B, Vertex* C, Vertex* D) {
    std::vector<Vertex*> candidates;

    // Work in coordinates relative to A to keep the paper's formulas simple.
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

    // The paper's main candidates lie where the area-preserving line meets AB and CD.
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
        // Interior synthetic starts use a single side-consistent placement to avoid unstable hole edits.
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

        if (chosen) candidates.push_back(chosen);
        if (eOnAB && eOnAB != chosen) delete eOnAB;
        if (eOnCD && eOnCD != chosen) delete eOnCD;
        if (candidates.empty()) candidates.push_back(new Vertex(-1, A->ring_id, B->x, B->y));
        return candidates;
    }

    // Enhancement: keep the paper candidates and also sample between them as fallbacks.
    if (eOnAB) {
        candidates.push_back(eOnAB);
    }
    
    if (eOnCD && (!eOnAB || std::abs(eOnCD->x - eOnAB->x) > kEpsilon || std::abs(eOnCD->y - eOnAB->y) > kEpsilon)) {
        candidates.push_back(eOnCD);
    }

    // If both bounding intersections exist, create interpolations on the area-preserving line
    if (eOnAB && eOnCD) {
        // Midpoint
        candidates.push_back(new Vertex(-1, A->ring_id, 
            (eOnAB->x + eOnCD->x) / 2.0, 
            (eOnAB->y + eOnCD->y) / 2.0));
            
        // 25% Mark
        candidates.push_back(new Vertex(-1, A->ring_id, 
            eOnAB->x * 0.75 + eOnCD->x * 0.25, 
            eOnAB->y * 0.75 + eOnCD->y * 0.25));
            
        // 75% Mark
        candidates.push_back(new Vertex(-1, A->ring_id, 
            eOnAB->x * 0.25 + eOnCD->x * 0.75, 
            eOnAB->y * 0.25 + eOnCD->y * 0.75));
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

    // Fallback for parallel or awkward cases: measure the whole local replacement polygon.
    double fallbackArea = triangleArea(0.0, 0.0, bx, by, ex, ey) +
                          triangleArea(bx, by, cx, cy, ex, ey) +
                          triangleArea(cx, cy, dx, dy, ex, ey);

    if (cross_ABE <= cross_CDE) {
        // The lower-cost side is determined by which of the two local triangles survives the collapse.
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
                return triangleArea(cx, cy, dx, dy, x_int, y_int);
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
                return triangleArea(0.0, 0.0, bx, by, x_int, y_int);
            }
        }
    }

    return fallbackArea; 
}

double Geometry::calculateSymmetricDifferenceArea(const Polygon& lhs, const Polygon& rhs) {
    std::vector<double> xEvents;
    const std::vector<Segment2D> lhsSegments = collectSegments(lhs, xEvents);
    const std::vector<Segment2D> rhsSegments = collectSegments(rhs, xEvents);
    SweepState lhsSweep = buildSweepState(lhsSegments);
    SweepState rhsSweep = buildSweepState(rhsSegments);

    for (const auto& lhsSegment : lhsSegments) {
        for (const auto& rhsSegment : rhsSegments) {
            double xIntersection = 0.0;
            if (segmentIntersectionX(lhsSegment, rhsSegment, xIntersection)) {
                xEvents.push_back(xIntersection);
            }
        }
    }

    sortAndUnique(xEvents);
    if (xEvents.size() < 2) {
        return 0.0;
    }

    double totalArea = 0.0;
    for (std::size_t i = 0; i + 1 < xEvents.size(); ++i) {
        double x0 = xEvents[i];
        double x1 = xEvents[i + 1];
        advanceSweep(lhsSweep, x0);
        advanceSweep(rhsSweep, x0);

        double width = x1 - x0;
        if (width <= 1e-9) {
            continue;
        }

        // Two interior samples are enough here because the cross section is piecewise linear between events.
        double leftSample = x0 + width / 3.0;
        double rightSample = x0 + 2.0 * width / 3.0;
        double leftLength = xorCrossSectionLength(lhsSweep, rhsSweep, leftSample);
        double rightLength = xorCrossSectionLength(lhsSweep, rhsSweep, rightSample);
        totalArea += width * (leftLength + rightLength) / 2.0;
    }

    return totalArea;
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
                // Skip over lazily deleted vertices when walking the ring.
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
