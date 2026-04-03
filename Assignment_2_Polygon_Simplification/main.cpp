#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>           // Added for timing
#ifndef _WIN32
#include <sys/resource.h>   // Added for peak memory (Linux/macOS)
#endif
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

    // Keep a second copy so the final symmetric difference can be measured exactly.
    Polygon originalPoly;
    originalPoly.loadFromCSV(filename.c_str());

    // 1. Calculate area BEFORE simplification
    double initialArea = Geometry::calculateTotalArea(poly);

    // ==========================================
    // START TIMING
    // ==========================================
    auto start_time = std::chrono::high_resolution_clock::now();

    Simplifier simplifier(poly);
    simplifier.buildInitialQueue();
    simplifier.run(targetVertices);

    // ==========================================
    // STOP TIMING
    // ==========================================
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> running_time = end_time - start_time;

    // 2. Calculate area AFTER simplification
    double finalArea = Geometry::calculateTotalArea(poly);

    // 3. Report the actual displacement between input and final output
    simplifier.totalDisplacement = Geometry::calculateSymmetricDifferenceArea(originalPoly, poly);

    // Print the simplified polygon first so the output file stays in the required format.
    poly.printToCSV();

    // Follow with the required summary block.
    std::cout << "Target vertices: " << targetVertices << "\n";

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Total signed area in input: " << initialArea << "\n";
    std::cout << "Total signed area in output: " << finalArea << "\n";
    std::cout << "Total areal displacement: " << simplifier.totalDisplacement << "\n";

    // ==========================================
    // CALCULATE & PRINT PERFORMANCE METRICS
    // ==========================================
    double peakMemoryMB = 0.0;
#ifndef _WIN32
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    // Note: ru_maxrss is in Kilobytes on Linux, but Bytes on macOS.
    // Assuming a standard Linux environment, divide by 1024.0 to get Megabytes.
    peakMemoryMB = usage.ru_maxrss / 1024.0;
#endif

    // Switch back to fixed notation for cleaner ms/MB formatting
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Running time: " << running_time.count() << " ms\n";
    std::cout << "Peak memory: " << peakMemoryMB << " MB\n";

    return 0;
}
