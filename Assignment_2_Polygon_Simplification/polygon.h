#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

struct Vertex {
    int id;
    int ring_id;
    double x, y;
    int evalVersion;
    Vertex* prev;
    Vertex* next;
    bool isActive;                                      // For lazy deletion

    Vertex(int _id, int _ring_id, double _x, double _y) 
        : id(_id), ring_id(_ring_id), x(_x), y(_y), evalVersion(0), prev(nullptr), next(nullptr), isActive(true) {}
};

struct Ring {
    int ring_id;
    Vertex* head;
    int vertexCount;
    std::vector<Vertex*> allVertices;
    
    Ring(int _ring_id);
    ~Ring();
    Ring(Ring&& other) noexcept;
    Ring(const Ring&) = delete; // Prevent accidental copying
    Ring& operator=(const Ring&) = delete;

    bool isExterior() const;                            // Determine if this ring is an exterior or interior ring
    Vertex* addVertex(int id, double x, double y);      // Adds a vertex to the end of the ring
    void removeVertex(Vertex* v);                       // Removes a vertex from the ring and updates pointers
    void insertVertexAfter(Vertex* v, Vertex* newV);    // Inserts newV after v and updates pointers
    void printToCSV() const;                            // Print the vertices of this ring in CSV format
};

class Polygon {
public:
    std::vector<Ring> rings;
    int totalVertices = 0;

    void loadFromCSV(const char* filename);
    void printToCSV() const;
    void removeVertex(Vertex* v);                       // Update prev/next pointers here
    void insertVertexAfter(Vertex* v, Vertex* newV);
};
