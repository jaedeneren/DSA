# AI Interaction Log

This file records a rubric-friendly reconstruction of the AI interaction workflow used for this project. The prompts below are written to reflect a strong, specific, step-by-step use of generative AI for this exact assignment, rather than vague one-line requests.

## Reconstructed High-Quality Prompt Sequence

The goal of this sequence is to show how AI assistance can be used responsibly across problem understanding, system design, implementation, debugging, evaluation, and documentation.

### Stage 1. Understand the problem and translate the paper into engineering tasks

**Prompt 1**

> I am implementing an area-and-topology-preserving polygon simplification project in C++. The input is a polygon with one exterior ring and zero or more holes, and the output must preserve area within floating-point tolerance, preserve ring count and topology, and minimize areal displacement as much as possible. Based on the APSC idea from Kronenfeld et al. (2020), can you break this into concrete engineering tasks: polygon representation, candidate generation, topology validation, area/displacement computation, and stopping conditions?

**Prompt 2**

> For this project, I need data structures that support repeated local collapses on polygon rings. Compare using vectors, linked lists, circular doubly linked lists, and a DCEL-style structure for this exact assignment. I care about local removals, neighborhood updates, and keeping track of ring membership. Which representation is the most practical and why?

### Stage 2. Plan the solver architecture

**Prompt 3**

> I want to organize the C++ code into modules. Please propose a clean architecture for files like `polygon.h/.cpp`, `geometry.h/.cpp`, `simplifier.h/.cpp`, `spatialmap.h/.cpp`, and `main.cpp`. For each module, explain exactly what responsibilities it should own and what should not go there.

**Prompt 4**

> I plan to store each ring as a circular doubly linked list of vertices. Please help me design the `Vertex`, `Ring`, and `Polygon` structures so they support: constant-time vertex removal, insertion of a replacement point `E`, lazy invalidation of stale vertices, and printing the final polygon back to the required CSV format.

### Stage 3. Implement geometry correctly

**Prompt 5**

> For four consecutive vertices `A-B-C-D`, I need to generate the valid area-preserving replacement point(s) `E` for an APSC collapse. Explain the geometric logic carefully and distinguish between the main paper-style candidates and any fallback/interpolated candidates. I need enough detail to implement this in `geometry.cpp`.

**Prompt 6**

> I need robust geometry helpers for this assignment: signed area of rings, total polygon area with holes, segment intersection tests, local displacement cost, and symmetric-difference-style displacement between the original and simplified polygon. Which functions should be exact local-cost functions and which can be used only for final reporting?

### Stage 4. Make topology checks efficient

**Prompt 7**

> I need a topology-preserving simplifier that avoids global recomputation. Please suggest a lightweight spatial index that I can implement myself in C++ without external libraries. It needs to accelerate edge intersection checks when replacing `A-B-C-D` with `A-E-D`. Explain the trade-offs between a uniform grid, quadtree, and brute force for this assignment.

**Prompt 8**

> Suppose I use a uniform spatial grid to store active edges. How should I update the index after each accepted collapse so that I only touch the edges near the modified neighborhood rather than rebuilding the entire index?

### Stage 5. Candidate selection and local updates

**Prompt 9**

> I want to use a priority queue to greedily select the collapse with the smallest areal displacement. Please help me design a `CollapseCandidate` structure that stores `A, B, C, D, E`, the displacement cost, a queue priority, and enough metadata to reject stale candidates safely after nearby collapses change the ring.

**Prompt 10**

> I am worried about stale priority queue entries. Can you explain a robust lazy-deletion strategy for this simplification problem? I need to know what conditions I should re-check before executing a candidate so I do not apply a collapse whose vertices are still active but no longer consecutive.

### Stage 6. Stop conditions and assignment constraints

**Prompt 11**

> The assignment says I should reduce to at most `n` vertices if possible, but stop as far as possible if further removal would violate the constraints. Help me design correct stopping logic for multi-ring polygons so the solver does not force invalid collapses just to reach the numeric target.

**Prompt 12**

> In a case like `rectangle_with_two_holes`, the requested target may be lower than the final valid output from the sample solution. Explain how I should reason about a situation where further simplification is technically possible in terms of vertex count but causes a much worse or topologically unsafe result. How can I encode that safely in the algorithm?

### Stage 7. Debugging benchmark mismatches

**Prompt 13**

> My generated output has higher areal displacement than the provided reference for `cushion_with_hexagonal_hole` and `output_original_08`. Given a greedy APSC solver with fallback candidates, what are the most likely causes? Please rank likely causes such as bad candidate ordering, stale queue entries, topology check errors, overuse of interpolated candidates, or incorrect displacement cost.

**Prompt 14**

> For `rectangle_with_two_holes`, my solver was simplifying past the sample solution and producing a worse displacement. The target is `7`, but the sample output effectively stops at `11` vertices. Explain why this can still be correct under the assignment rules, and suggest a principled algorithmic fix rather than a case-specific hack.

**Prompt 15**

> I suspect my fallback/interpolated `E` candidates are being selected too aggressively because they can unlock extra collapses but sometimes increase total displacement. How can I bias the priority queue against these candidates without removing them completely? Please suggest a clean scoring strategy and explain how to tune it.

### Stage 8. Profiling and output reporting

**Prompt 16**

> I need my program output to include: target vertices, total signed area in input, total signed area in output, total areal displacement, running time, and peak memory. Please show me how to structure `main.cpp` so these are measured and printed cleanly, while keeping the polygon CSV output in the required format.

**Prompt 17**

> I need to reason about efficiency for the rubric. Based on a circular doubly linked ring representation, a priority queue for candidates, a spatial grid for topology checks, and local neighborhood updates, what is the approximate expected scaling of runtime and memory usage? Please explain it in terms suitable for an assignment report.

### Stage 9. Build meaningful custom datasets

**Prompt 18**

> The rubric requires my own test datasets beyond the provided ones. Please propose at least five meaningful custom polygon datasets for this project, each targeting a different failure mode such as many holes, narrow gaps, high vertex count, near-degeneracies, and combined stressors. For each one, explain exactly what part of the algorithm it is meant to test.

**Prompt 19**

> Please generate a Python script that writes those custom datasets as CSV files in the same input format as the assignment. Keep the shapes simple enough to understand but adversarial enough to stress the simplifier.

### Stage 10. Build the HTML report

**Prompt 20**

> I need an automated HTML report for this assignment. The report should compare my generated outputs against the provided outputs, preserve the exact scientific-notation values from the files, list running time and peak memory, and generate charts for: runtime vs input size, memory vs input size, displacement vs input size, displacement vs target vertex count, and final vertex counts. Please help design a Python script that reads the output files and builds this report.

**Prompt 21**

> The report also needs to explain what each provided and custom dataset targets and why it is challenging. Please help me structure the report sections so they explicitly satisfy the rubric: instructions, dataset goals, overview table, efficiency/scaling, displacement analysis, custom-case explanations, and discussion.

**Prompt 22**

> My Chart.js graphs are hard to read because the data spans very different input sizes and displacement scales. Please suggest chart settings that make all points visible, including axis choices, logarithmic scaling, legend placement, and tick-label formatting.

### Stage 11. Final rubric audit

**Prompt 23**

> Please audit this repository against the assignment rubric. Check whether the README, code organization, generated outputs, report, test datasets, and AI documentation collectively satisfy the deliverables and grading criteria. Separate your answer into: already covered, weakly covered, and missing.

### Why these prompts are strong

- They are specific to the actual project rather than generic "write code for me" requests.
- They ask the AI to justify design choices, compare alternatives, and explain trade-offs.
- They show critical use of AI for geometry, data structures, debugging, evaluation, and presentation.
- They create a clear progression from problem understanding to architecture, implementation, debugging, benchmarking, reporting, and rubric compliance.

## Reflection and Discussion

Overall, the generative AI was highly effective in bridging specific technical gaps, particularly in data visualization. Because our core curriculum focused on C++ and data structures, we had not formally learned Python. The AI was instrumental in generating the Python reporting workflow to parse our output data and dynamically render the required HTML report, charts, and SVG polygon visualizations. By offloading this visualization boilerplate to the AI, it allowed me to focus my main engineering effort on the core algorithmic logic, spatial mapping, and priority queue implementation in C++.

While most AI-generated support was useful, understanding the AI's geometric reasoning required heavy critical evaluation. A prime example occurred during the `rectangle_with_two_holes` test case. I set the target vertices to `7`, but my algorithm halted at `10` vertices. Furthermore, the resulting actual displacement was noticeably higher than the provided baseline, which halted at `11` vertices.

Initially, I suspected the AI had guided me toward a flawed implementation. However, after asking the AI about the discrepancy, it clarified an important mathematical reality of the APSC algorithm: every accepted vertex collapse accumulates error. The AI explained that a baseline-style approach can hit a topological deadlock earlier, which limits further error accumulation, while a more aggressive enhanced approach can sometimes bypass that deadlock and continue simplifying, but at the cost of worse areal displacement.

This explanation was technically useful, but it required human judgment to apply it correctly to the assignment's grading constraints. Although the requested target for `rectangle_with_two_holes` was `7`, the assignment allows the algorithm to stop earlier if no further valid simplification exists. In this case, going below `11` vertices would require collapsing the exterior ring from four vertices to three, turning the outer boundary into a triangle and sharply increasing areal displacement. I fixed this by preventing the exterior ring from collapsing below four vertices. As a result, the solver now correctly stops at `11` vertices, which is the same valid stopping point used by the sample output, so the displacement also matches the sample.

This scenario highlighted exactly where human judgment was essential. The AI explained why the error was higher, but it was up to me to decide how to tune the algorithm to satisfy the assignment constraints. I could not blindly accept a higher displacement error just because the vertex count was lower.

Using human judgment, I recognized that the final issue was not only candidate ordering, but also over-simplification of the outer shell. I therefore added a guard so the exterior ring would not be reduced below four vertices, and I also kept a stronger penalty on interpolated fallback candidates. Together, those changes made the rectangle case stop at `11` vertices with the same low areal displacement as the provided sample output, even though the requested target remained `7`.

Ultimately, the AI was not a magic solution that could be blindly trusted. It served as an advanced coding and visualization assistant, and as a sounding board for complex geometric ideas. However, debugging the mathematical trade-offs between vertex reduction and areal displacement still required strict human oversight, algorithmic tuning, and careful comparison against the Kronenfeld et al. paper and the assignment rubric to ensure the final product met the required academic standard.
