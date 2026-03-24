#pragma once
#include "polygon.h"

class Geometry {
public:
    // Calculates the new vertex E based on the Kronenfeld paper
    static std::vector<Vertex*> calculateE(Vertex* A, Vertex* B, Vertex* C, Vertex* D);
    
    // Calculates the symmetric difference area
    static double calculateDisplacementCost(Vertex* A, Vertex* B, Vertex* C, Vertex* D, Vertex* E);
    
    // Calculates total signed area of the whole polygon
    static double calculateTotalArea(const Polygon& poly);
};