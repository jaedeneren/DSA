#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include "polygon.h"
#include "simplifier.h"
#include "geometry.h"
#include "vega_lite_plot.h"

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

    // 2. Initialize the Visualization Tool
    VegaLitePlot visualizer;
    
    // 3. Capture the initial state of the polygon for the plot
    visualizer.addDataset(poly, "Original");

    // 4. Perform Simplification
    Simplifier simplifier(poly);
    simplifier.buildInitialQueue();
    simplifier.run(targetVertices);

    // 5. Calculate area AFTER simplification
    double finalArea = Geometry::calculateTotalArea(poly);

    // 6. Add the simplified state to the visualizer
    visualizer.addDataset(poly, "Simplified");

    // 7. Save the visualization data to a JSON file
    visualizer.savePlot("plot_results.html", initialArea, finalArea, simplifier.totalDisplacement);

    // 8. Standard Output requirements (CSV format)
    poly.printToCSV();

    // 9. Print the metrics to standard output
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Total signed area in input: " << initialArea << "\n";
    std::cout << "Total signed area in output: " << finalArea << "\n";
    std::cout << "Total areal displacement: " << simplifier.totalDisplacement << "\n";

    return 0;
}