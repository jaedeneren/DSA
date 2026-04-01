#pragma once
#include <vector>
#include <string>
#include "polygon.h"
#include "nlohmann_json.h" // Ensure this is in the same directory

using json = nlohmann::json;

class VegaLitePlot {
public:
    // Stores a snapshot of a polygon state (e.g., "Original" or "Simplified")
    void addDataset(const Polygon& poly, const std::string& label);

    // Generates the final JSON file containing geometries, summary stats, and the Vega-Lite spec
    void savePlot(const std::string& filename, double initialArea, double finalArea, double displacement);

private:
    struct PolySnapshot {
        std::string label;
        json geometryData;
    };
    std::vector<PolySnapshot> snapshots;
};