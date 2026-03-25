#include "simplifier.h"
#include "geometry.h"
#include <cstdlib>

namespace {
bool isTraceEnabled() {
    const char* value = std::getenv("APSC_TRACE");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

bool isCandidateTraceEnabled() {
    const char* value = std::getenv("APSC_TRACE_CANDIDATES");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}
}

void Simplifier::evaluateAndPush(Vertex* A) {
    if (!A || !A->isActive) return;

    Vertex* B = A->next;
    Vertex* C = B->next;
    Vertex* D = C->next;

    if (!B->isActive || !C->isActive || !D->isActive) return;
    if (A == C || A == D) return; // Skip if ring is too small

    ++A->evalVersion;

    // Get ALL valid geometric candidates
    std::vector<Vertex*> possible_Es = Geometry::calculateE(A, B, C, D);

    // Evaluate the displacement cost for each and push them to the PQ
    for (int i = 0; i < static_cast<int>(possible_Es.size()); ++i) {
        Vertex* E = possible_Es[i];
        CollapseCandidate candidate;
        candidate.A = A;
        candidate.B = B;
        candidate.C = C;
        candidate.D = D;
        candidate.E = E;
        candidate.cost = Geometry::calculateDisplacementCost(A, B, C, D, E);
        candidate.candidateRank = i;
        candidate.evalVersion = A->evalVersion;

        if (isCandidateTraceEnabled()) {
            std::cerr << "candidate ring=" << A->ring_id
                      << " A=" << A->id
                      << " B=" << B->id
                      << " C=" << C->id
                      << " D=" << D->id
                      << " E=(" << E->x << "," << E->y << ")"
                      << " cost=" << candidate.cost << "\n";
        }

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
            if (isCandidateTraceEnabled()) {
                std::cerr << "reject inactive ring=" << best.A->ring_id
                          << " A=" << best.A->id
                          << " B=" << best.B->id
                          << " C=" << best.C->id
                          << " D=" << best.D->id << "\n";
            }
            // One of these was already removed in a previous collapse. 
            // Discard this candidate and move to the next one.
            delete best.E; // Clean up the memory we allocated for the proposed E
            continue; 
        }

        if (best.evalVersion != best.A->evalVersion) {
            if (isCandidateTraceEnabled()) {
                std::cerr << "reject superseded ring=" << best.A->ring_id
                          << " A=" << best.A->id
                          << " B=" << best.B->id
                          << " C=" << best.C->id
                          << " D=" << best.D->id << "\n";
            }
            delete best.E;
            continue;
        }

        // 1b. STALE CANDIDATE CHECK: the queue can still contain candidates
        // whose vertices are active but are no longer consecutive after nearby
        // collapses. Those candidates must not be applied.
        if (best.A->next != best.B || best.B->next != best.C || best.C->next != best.D ||
            best.B->prev != best.A || best.C->prev != best.B || best.D->prev != best.C) {
            if (isCandidateTraceEnabled()) {
                std::cerr << "reject stale ring=" << best.A->ring_id
                          << " A=" << best.A->id
                          << " B=" << best.B->id
                          << " C=" << best.C->id
                          << " D=" << best.D->id << "\n";
            }
            delete best.E;
            continue;
        }

        // 2. TOPOLOGY CHECK: Does this move break the shape?
        if (!spatialMap.isTopologyValid(best.A, best.B, best.C, best.D, best.E)) {
            if (isCandidateTraceEnabled()) {
                std::cerr << "reject topology ring=" << best.A->ring_id
                          << " A=" << best.A->id
                          << " B=" << best.B->id
                          << " C=" << best.C->id
                          << " D=" << best.D->id
                          << " E=(" << best.E->x << "," << best.E->y << ")\n";
            }
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

        if (isTraceEnabled()) {
            std::cerr << "collapse ring=" << best.A->ring_id
                      << " A=" << best.A->id
                      << " B=" << best.B->id
                      << " C=" << best.C->id
                      << " D=" << best.D->id
                      << " -> E=(" << best.E->x << "," << best.E->y << ")"
                      << " cost=" << best.cost << "\n";
        }

        // 4. LOCAL UPDATES ONLY
        // We only need to recalculate the costs for the sequences immediately 
        // surrounding our new vertex E. 
        evaluateAndPush(best.A->prev->prev); // Sequence starting two before A
        evaluateAndPush(best.A->prev);       // Sequence starting one before A
        evaluateAndPush(best.A);             // Sequence starting at A
        evaluateAndPush(best.E);             // Sequence starting at E
    }
}
