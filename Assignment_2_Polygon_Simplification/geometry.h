#pragma once
#include "polygon.h"

class Geometry {
public:
    // Return one or more valid placements for the replacement vertex E.
    static std::vector<Vertex*> calculateE(Vertex* A, Vertex* B, Vertex* C, Vertex* D);
    
    // Estimate the local areal displacement introduced by replacing B and C with E.
    static double calculateDisplacementCost(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E);

    // Compute the full symmetric-difference area between two polygon states.
    static double calculateSymmetricDifferenceArea(const Polygon& lhs, const Polygon& rhs);
    
    // Compute the signed area of the polygon, with clockwise holes subtracting naturally.
    static double calculateTotalArea(const Polygon& poly);
};
