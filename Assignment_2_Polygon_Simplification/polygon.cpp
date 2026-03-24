#include "polygon.h"

Ring::Ring(int id) : ring_id(id), head(nullptr), vertexCount(0) {}

Ring::~Ring()
{
    for (Vertex* v : allVertices) {
        delete v;
    }
}

Ring::Ring(Ring &&other) noexcept : ring_id(other.ring_id), head(other.head), 
    vertexCount(other.vertexCount), allVertices(std::move(other.allVertices)) {
    other.head = nullptr; 
    other.vertexCount = 0;
}

bool Ring::isExterior() const
{
    return ring_id == 0; // By convention, ring_id 0 is the exterior ring
}

Vertex *Ring::addVertex(int id, double x, double y)
{
    Vertex* newVertex = new Vertex(id, ring_id, x, y);
    allVertices.push_back(newVertex);
    if (!head) {
        head = newVertex;
        head->next = head; // Point to itself to form a circular list
        head->prev = head;
    } else {
        // Insert new vertex before head (at the end of the list)
        Vertex* tail = head->prev;
        tail->next = newVertex;
        newVertex->prev = tail;
        newVertex->next = head;
        head->prev = newVertex;
    }
    vertexCount++;
    return newVertex;
}

void Ring::removeVertex(Vertex *v)
{
    if(!v || !v->isActive || vertexCount == 0) return; // Invalid vertex or already removed

    if(vertexCount == 1) {
        // Only one vertex, just remove it
        head = nullptr;
    } else {
        // Update the linked list to bypass v
        v->prev->next = v->next;
        v->next->prev = v->prev;
        if (v == head) {
            head = v->next; // Move head if we're removing the current head
        }
    }
    v->isActive = false; // Mark as inactive for lazy deletion
    vertexCount--;
}

void Ring::insertVertexAfter(Vertex *v, Vertex *newV)
{
    if(!v || !newV) return; // Invalid vertices
    Vertex* nextV = v->next;
    v->next = newV;
    newV->prev = v;
    newV->next = nextV;
    nextV->prev = newV;
    vertexCount++;
}

void Ring::printToCSV() const {
    if (!head) return;
    
    Vertex* startVertex = head;
    int min_id = 1e9;
    
    Vertex* current = head;
    do {
        // IGNORE negative IDs (newly created E vertices)
        if (current->isActive && current->id >= 0 && current->id < min_id) {
            min_id = current->id;
            startVertex = current;
        }
        current = current->next;
    } while (current != head);

    // Fallback just in case
    if (min_id == 1e9) {
        startVertex = head;
        while (!startVertex->isActive) startVertex = startVertex->next;
    }

    int new_vertex_id = 0;
    current = startVertex;
    do {
        if (current->isActive) {
            std::cout << ring_id << "," << new_vertex_id << "," 
                      << current->x << "," << current->y << "\n";
            new_vertex_id++;
        }
        current = current->next;
    } while (current != startVertex);
}

// --- Polygon Class Implementations ---

void Polygon::loadFromCSV(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        exit(1);
    }

    std::string line;
    // Skip the header line
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        
        int r_id, v_id;
        double x, y;

        std::getline(ss, token, ','); r_id = std::stoi(token);
        std::getline(ss, token, ','); v_id = std::stoi(token);
        std::getline(ss, token, ','); x = std::stod(token);
        std::getline(ss, token, ','); y = std::stod(token);

        // If we found a new ring ID, create a new Ring object
        while (r_id >= (int)rings.size()) {
            rings.emplace_back(rings.size());
        }

        rings[r_id].addVertex(v_id, x, y);
        totalVertices++;
    }
    file.close();
}

void Polygon::printToCSV() const {
    // Print the header exactly as requested
    std::cout << "ring_id,vertex_id,x,y\n";
    
    std::cout << std::fixed << std::setprecision(3);

    for (const auto& ring : rings) {
        ring.printToCSV();
    }
    
    // Note: You will need to print the Area summaries in main.cpp
    // after you call this function!
}

void Polygon::removeVertex(Vertex* v) {
    if (!v || !v->isActive) return;
    rings[v->ring_id].removeVertex(v);
    totalVertices--;
}

void Polygon::insertVertexAfter(Vertex* target, Vertex* newV) {
    rings[target->ring_id].insertVertexAfter(target, newV);
    totalVertices++;
}