#include "vega_lite_plot.h"
#include <fstream>
#include <iomanip>
#include <sstream>

void VegaLitePlot::addDataset(const Polygon& poly, const std::string& label) {
    json data = json::array();

    for (const auto& ring : poly.rings) {
        if (ring.vertexCount == 0) continue;

        Vertex* current = ring.head;
        Vertex* firstActive = nullptr;

        if (current) {
            // Traverse the circular linked list
            do {
                if (current->isActive) {
                    if (!firstActive) firstActive = current;
                    
                    data.push_back({
                        {"x", current->x},
                        {"y", current->y},
                        {"ring_id", ring.ring_id},
                        {"type", label}
                    });
                }
                current = current->next;
            } while (current != ring.head);
        }

        // Close the loop for visualization so Vega-Lite draws a closed shape
        if (firstActive) {
            data.push_back({
                {"x", firstActive->x},
                {"y", firstActive->y},
                {"ring_id", ring.ring_id},
                {"type", label}
            });
        }
    }
    snapshots.push_back({label, data});
}

void VegaLitePlot::savePlot(const std::string& filename, double initialArea, double finalArea, double displacement) {
    // 1. Combine all geometric snapshots into one flat dataset
    json allGeometries = json::array();
    for (const auto& s : snapshots) {
        for (const auto& entry : s.geometryData) {
            allGeometries.push_back(entry);
        }
    }

    // 2. Define the Vega-Lite Specification with 3 horizontally concatenated views
    json spec = {
        {"$schema", "https://vega.github.io/schema/vega-lite/v5.json"},
        {"description", "Polygon Simplification: Original, Simplified, and Overlap"},
        {"data", {{"values", allGeometries}}},
        {"hconcat", json::array({
            {
                {"title", "Input Polygon"},
                {"width", 300}, {"height", 300},
                {"transform", json::array({{{"filter", "datum.type == 'Original'"}}})},
                {"mark", {{"type", "line"}, {"point", true}}},
                {"encoding", {
                    {"x", {{"field", "x"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"y", {{"field", "y"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"color", {{"value", "gray"}}},
                    {"detail", {{"field", "ring_id"}, {"type", "nominal"}}}
                }}
            },
            {
                {"title", "Output Polygon"},
                {"width", 300}, {"height", 300},
                {"transform", json::array({{{"filter", "datum.type == 'Simplified'"}}})},
                {"mark", {{"type", "line"}, {"point", true}}},
                {"encoding", {
                    {"x", {{"field", "x"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"y", {{"field", "y"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"color", {{"value", "red"}}},
                    {"detail", {{"field", "ring_id"}, {"type", "nominal"}}}
                }}
            },
            {
                {"title", "Input & Output Overlap"},
                {"width", 300}, {"height", 300},
                {"mark", {{"type", "line"}, {"point", true}, {"opacity", 0.7}}},
                {"encoding", {
                    {"x", {{"field", "x"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"y", {{"field", "y"}, {"type", "quantitative"}, {"scale", {{"zero", false}}}}},
                    {"color", {
                        {"field", "type"}, 
                        {"type", "nominal"},
                        {"scale", {{"domain", {"Original", "Simplified"}}, {"range", {"gray", "red"}}}}
                    }},
                    {"detail", {{"field", "ring_id"}, {"type", "nominal"}}}
                }}
            }
        })}
    };

    // 3. Generate the HTML wrapper
    std::ofstream file(filename);
    
    file << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "  <title>Polygon Simplification Results</title>\n"
         << "  <script src=\"https://cdn.jsdelivr.net/npm/vega@5\"></script>\n"
         << "  <script src=\"https://cdn.jsdelivr.net/npm/vega-lite@5\"></script>\n"
         << "  <script src=\"https://cdn.jsdelivr.net/npm/vega-embed@6\"></script>\n"
         << "  <style>\n"
         << "    body { font-family: sans-serif; margin: 20px; }\n"
         << "    .stats { background: #f4f4f4; padding: 15px; border-radius: 5px; margin-bottom: 20px; display: inline-block; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "  <h2>Polygon Simplification</h2>\n"
         << "  <div class=\"stats\">\n"
         << "    <strong>Initial Area:</strong> " << std::fixed << std::setprecision(6) << initialArea << "<br>\n"
         << "    <strong>Final Area:</strong> " << finalArea << "<br>\n"
         << "    <strong>Total Displacement:</strong> " << displacement << "\n"
         << "  </div>\n"
         << "  <div id=\"vis\"></div>\n"
         << "  <script>\n"
         << "    var spec = " << spec.dump(2) << ";\n"
         << "    vegaEmbed('#vis', spec).then(function(result) {\n"
         << "    }).catch(console.error);\n"
         << "  </script>\n"
         << "</body>\n"
         << "</html>\n";
         
    file.close();
}