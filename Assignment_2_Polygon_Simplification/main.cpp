#include <iostream>
#include <string>
#include <iomanip>
#include "polygon.h"
#include "simplifier.h"
#include "geometry.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./simplify <input.csv> <target_vertices>\n";
        return 1;
    }

    std::string filename = argv[1];
    int targetVertices = std::stoi(argv[2]);

    Polygon poly;
    poly.loadFromCSV(filename.c_str());

    // 1. Calculate area BEFORE simplification
    double initialArea = Geometry::calculateTotalArea(poly);

    Simplifier simplifier(poly);
    simplifier.buildInitialQueue();
    simplifier.run(targetVertices);

    // 2. Calculate area AFTER simplification
    double finalArea = Geometry::calculateTotalArea(poly);

    // 3. Print the vertices
    poly.printToCSV();

    // 4. Print the required summary in scientific notation
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Total signed area in input: " << initialArea << "\n";
    std::cout << "Total signed area in output: " << finalArea << "\n";
    std::cout << "Total areal displacement: " << simplifier.totalDisplacement << "\n";

    return 0;
}
