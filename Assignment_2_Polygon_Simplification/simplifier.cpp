#include "simplifier.h"
#include "geometry.h"

void Simplifier::evaluateAndPush(Vertex* A) {
    if (!A || !A->isActive) return;

    Vertex* B = A->next;
    Vertex* C = B->next;
    Vertex* D = C->next;

    if (!B->isActive || !C->isActive || !D->isActive) return;
    if (A == C || A == D) return; // Skip if ring is too small

    // Get ALL valid geometric candidates
    std::vector<Vertex*> possible_Es = Geometry::calculateE(A, B, C, D);

    // Evaluate the displacement cost for each and push them to the PQ
    for (Vertex* E : possible_Es) {
        CollapseCandidate candidate;
        candidate.A = A;
        candidate.B = B;
        candidate.C = C;
        candidate.D = D;
        candidate.E = E;
        candidate.cost = Geometry::calculateDisplacementCost(A, B, C, D, E);

        pq.push(candidate);
    }
}

Simplifier::~Simplifier()
{
    while (!pq.empty()) {
        CollapseCandidate leftover = pq.top();
        pq.pop();
        delete leftover.E;
    }
}

void Simplifier::buildInitialQueue()
{
    // Build the spatial index FIRST so topology checks work!
    spatialMap.buildIndex(poly);

    for (auto& ring : poly.rings) {
        if (ring.vertexCount < 4) continue;

        Vertex* current = ring.head;
        do {
            if (current->isActive) {
                evaluateAndPush(current);
            }
            current = current->next;
        } while (current != ring.head);
    }
}

void Simplifier::run(int targetVertices)
{
    while (poly.totalVertices > targetVertices && !pq.empty()) {
        CollapseCandidate best = pq.top();
        pq.pop();

        // 1. LAZY DELETION CHECK: Are all 4 vertices still part of the polygon?
        if (!best.A->isActive || !best.B->isActive || !best.C->isActive || !best.D->isActive) {
            // One of these was already removed in a previous collapse. 
            // Discard this candidate and move to the next one.
            delete best.E; // Clean up the memory we allocated for the proposed E
            continue; 
        }

        // 2. TOPOLOGY CHECK: Does this move break the shape?
        if (!spatialMap.isTopologyValid(best.A, best.B, best.C, best.D, best.E)) {
            // It intersects something! Discard it.
            delete best.E;
            continue; 
        }

        // 3. EXECUTE THE COLLAPSE
        // Update the spatial index first
        spatialMap.updateIndex(best.A, best.D, best.E);

        // Update the linked list (remove B and C, insert E)
        poly.removeVertex(best.B);
        poly.removeVertex(best.C);

        static int stable_id_counter = -1000; // Start negative to avoid clashing with original IDs
        best.E->id = stable_id_counter--;
        
        poly.insertVertexAfter(best.A, best.E);
        poly.rings[best.A->ring_id].allVertices.push_back(best.E);

        totalDisplacement += best.cost;

        // 4. LOCAL UPDATES ONLY
        // We only need to recalculate the costs for the sequences immediately 
        // surrounding our new vertex E. 
        // Note: You'll need to write evaluateAndPush to calculate the math and push to the queue.
        evaluateAndPush(best.A->prev->prev); // Sequence starting two before A
        evaluateAndPush(best.A->prev);       // Sequence starting one before A
        evaluateAndPush(best.A);             // Sequence starting at A
        evaluateAndPush(best.E);             // Sequence starting at E
    }
}