# AI Usage Report

## Introduction

This project implements area-and-topology-preserving polygon simplification based on the Area-Preserving Segment Collapse (APSC) idea discussed by Kronenfeld et al. The assignment required much more than a basic simplifier. The implementation had to preserve polygon structure, maintain ring validity, respect the stopping conditions in the specification, and keep areal displacement low across both provided and custom datasets. In practice, this meant that the work combined algorithm design, geometry implementation, debugging, benchmarking, and report generation.

Generative AI was used throughout the project as an assistant for design discussion, implementation scaffolding, debugging, and documentation. The team did not use AI as an unquestioned source of truth. Instead, AI was treated as a fast technical assistant whose suggestions had to be checked against the assignment brief, the expected outputs, and the actual behavior of the code. This distinction was especially important because the project deals with computational geometry, where a suggestion can appear reasonable at a high level while still being wrong in a subtle but important way.

The nature of AI use changed over time. Early prompts focused on understanding the problem and converting the paper into implementable engineering tasks. Later prompts became narrower and more concrete, such as how to represent polygon rings for repeated local edits, how to manage stale priority-queue entries safely, how to update a spatial index efficiently after a collapse, and how to build the HTML report required for evaluation. As development progressed, AI support shifted further toward debugging mismatches against benchmark outputs and improving the clarity of the final deliverables.

## How AI Was Used

AI assistance was used in four main areas.

First, it helped break the assignment into manageable engineering components. The project needed a clear separation between geometry utilities, polygon and ring data structures, simplification logic, topology checking, command-line execution, and report generation. AI was useful for turning the paper-level description of APSC into a concrete implementation plan with modules and responsibilities.

Second, AI helped propose implementation patterns. Examples include using circular doubly linked lists for repeated local vertex removal, lazy invalidation for priority-queue entries, and a lightweight spatial grid for accelerating edge-intersection checks. These suggestions were not copied blindly, but they were valuable as starting points because they matched the local-update nature of the algorithm better than more naive alternatives.

Third, AI was used as a debugging assistant. When outputs differed from expected benchmark behavior, AI was asked to rank likely causes and suggest what invariants should be rechecked. This was particularly useful for diagnosing stale-neighborhood problems, update-order bugs in linked-list rewiring, and cases where fallback simplification candidates led to worse displacement.

Fourth, AI supported documentation and presentation work. It helped structure the HTML report, organize rubric-oriented sections, suggest charting strategies for difficult scales, and improve the clarity of the explanation surrounding datasets, performance, and displacement behavior. AI also helped with utility code for parsing results and formatting tables or charts cleanly.

## Analysis of AI Suggestions

Some AI suggestions were clearly useful and remained close to the final implementation.

One strong suggestion was to represent each polygon ring as a circular doubly linked list. This fit the assignment well because accepted collapses only affect a small neighborhood at a time. A linked representation makes local rewiring much more practical than repeated removal from a plain array, while still keeping predecessor and successor access straightforward.

Another useful suggestion was the lazy-deletion approach for candidate management. Because each collapse changes only a local part of the polygon, it would be wasteful to rebuild the entire priority queue after every accepted operation. AI suggested storing candidates in the queue and revalidating them only when they reach the top. That pattern was appropriate for this project and influenced the final design.

The recommendation to use a lightweight spatial grid for topology checks was also productive. The assignment did not require or encourage heavy external geometry libraries, so a simple self-contained spatial structure was a good fit. AI helped identify the grid as a better engineering compromise than a more elaborate spatial hierarchy for this scale of project.

However, several suggestions were only partially correct and had to be revised.

One early simplification idea for choosing the replacement point `E` preserved area in a broad sense but did not faithfully reflect the intended APSC behavior for minimizing displacement. That suggestion was not sufficient for the assignment and had to be replaced by a more careful implementation aligned with the project requirements and the paper's intent.

Another issue arose in linked-list updates. One AI-assisted version of the logic deactivated vertices too early, which interfered with proper neighbor rewiring. The code looked plausible, but careful tracing showed that the update order mattered. This was a clear example of AI producing code that was syntactically acceptable while still violating important invariants.

The queue-management logic also needed human correction. In this project, a candidate is not safe merely because its vertices are still marked active. A nearby accepted collapse can change the actual local sequence, meaning a stored `A-B-C-D` neighborhood may no longer be the same neighborhood when the candidate resurfaces. AI pointed toward lazy invalidation as a useful pattern, but the final implementation still required explicit revalidation of adjacency and neighborhood structure before accepting a collapse.

AI suggestions related to stopping conditions also needed scrutiny. In some cases, continuing to simplify toward the target vertex count was technically possible in a narrow sense but produced much worse displacement or undesirable degradation of the exterior ring. This appeared in the `rectangle_with_two_holes` case, where benchmark interpretation and assignment intent mattered more than simply forcing the count downward. Human review was required to translate the assignment rules into a principled stopping policy.

## Reflection on AI Assistance

AI improved development speed most on tasks where a strong programming pattern already existed. Examples include modular code organization, queue invalidation patterns, CSV parsing utilities, result summarization, chart generation, and HTML report structure. In these areas, AI reduced the time spent on boilerplate and helped the team move more quickly toward testing and validation.

At the same time, the project also demonstrated the limits of AI very clearly. Geometry-heavy code is unusually sensitive to hidden assumptions. A suggestion can sound convincing while still failing to preserve the exact local structure needed by the algorithm. Similarly, a debugging explanation can be directionally correct without being sufficient to produce an acceptable final solver. The difference between a valid result and a low-quality result often depended on small implementation details, not on broad conceptual summaries.

This meant that the team's effort shifted away from raw code drafting and toward critical evaluation. AI-generated ideas were treated as hypotheses to test, not as finished solutions. The most important human responsibilities were checking invariants, comparing produced outputs against the provided reference outputs, interpreting the assignment rules accurately, and deciding when an apparently valid simplification was still the wrong engineering choice because of displacement quality or topology risk.

The project therefore reinforced a useful lesson about responsible AI use in software engineering. AI is most effective when used to accelerate exploration, drafting, and iteration. It is much less reliable when treated as an authority on correctness in a specialized domain. Final responsibility for algorithm design, validation, testing, and honest reporting remained with the team.

## Distribution of Work with AI and Within the Team

AI contributed to the project in several concrete ways:

- breaking the assignment and paper into implementable engineering tasks
- proposing data-structure choices such as circular doubly linked rings and lazy queue invalidation
- suggesting an efficient self-contained spatial-grid approach for topology checks
- assisting with utility code for parsing results, formatting tables, and generating the HTML report
- supporting debugging by surfacing likely causes of benchmark mismatches and over-simplification behavior
- improving the structure and presentation of the final documentation

The team remained responsible for the substantive technical work:

- deciding which AI suggestions were worth adopting and which needed to be rejected
- implementing and correcting the algorithmic details of the simplifier
- validating all nontrivial design decisions against the assignment brief, the paper, and the benchmark outputs
- debugging incorrect local updates, stale candidates, and stopping-condition behavior
- evaluating the quality of simplification results rather than relying only on vertex-count reduction
- ensuring that the final report described the real implementation and results accurately

Because this was a group effort, AI use should be understood as support applied across shared development tasks rather than as authorship of the project. The final code, debugging decisions, testing judgments, and deliverable preparation were all filtered through team review. AI accelerated parts of the workflow, but it did not replace the team's responsibility for correctness or academic integrity.

## Conclusion

Overall, AI made a meaningful positive contribution to the project, especially in planning, scaffolding, debugging direction, and report-generation support. Its suggestions often saved time and exposed useful implementation patterns. However, the project also made it clear that AI assistance is only valuable when paired with careful human verification. For this assignment, the most important work was not simply generating code, but judging whether the generated ideas preserved topology correctly, matched the intended APSC behavior, and satisfied the actual grading requirements. That judgment remained a team responsibility throughout the project.

Word count: 1299
