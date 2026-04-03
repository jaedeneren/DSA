# Test Cases for Area-and-Topology-Preserving Polygon Simplification

Each test case consists of an **input CSV** and the corresponding **expected output**.

## Simple cases (polygons with holes)

| Input file | Vertices | Holes | Target | Output file |
|---|---|---|---|---|
| `input_rectangle_with_two_holes.csv` | 12 | 2 | 7 | `output_rectangle_with_two_holes.txt` |
| `input_cushion_with_hexagonal_hole.csv` | 22 | 1 | 13 | `output_cushion_with_hexagonal_hole.txt` |
| `input_blob_with_two_holes.csv` | 36 | 2 | 17 | `output_blob_with_two_holes.txt` |
| `input_wavy_with_three_holes.csv` | 43 | 3 | 21 | `output_wavy_with_three_holes.txt` |
| `input_lake_with_two_islands.csv` | 81 | 2 | 17 | `output_lake_with_two_islands.txt` |

## Lake cases (single polygon, no holes)

| Input file | Target | Output file |
|---|---|---|
| `input_original_01.csv` | 99 | `output_original_01.txt` |
| `input_original_02.csv` | 99 | `output_original_02.txt` |
| `input_original_03.csv` | 99 | `output_original_03.txt` |
| `input_original_04.csv` | 99 | `output_original_04.txt` |
| `input_original_05.csv` | 99 | `output_original_05.txt` |
| `input_original_06.csv` | 99 | `output_original_06.txt` |
| `input_original_07.csv` | 99 | `output_original_07.txt` |
| `input_original_08.csv` | 99 | `output_original_08.txt` |
| `input_original_09.csv` | 99 | `output_original_09.txt` |
| `input_original_10.csv` | 99 | `output_original_10.txt` |

## Custom cases for your report

These are extra datasets for the "generate your own meaningful test datasets" part of the assignment. They do **not** have instructor-provided expected outputs; the point is to stress specific geometric situations and discuss how your algorithm behaves.

| Input file | Vertices | Holes | Suggested target | What it stresses |
|---|---|---|---|---|
| `input_custom_narrow_gaps_with_three_holes.csv` | 16 | 3 | 10 | Narrow clearances between holes and the outer shell |
| `input_custom_comb_bays_with_inner_hole.csv` | 18 | 1 | 10 | Alternating concave bays and a hole near the base |
| `input_custom_dense_hole_grid.csv` | 40 | 9 | 24 | Many small holes packed into one exterior ring |
| `input_custom_near_collinear_wavy_shell.csv` | 25 | 0 | 12 | Near-collinear edges and small vertical perturbations |
| `input_custom_pinch_corridor_with_two_holes.csv` | 20 | 2 | 12 | A narrow pinch corridor that can easily cause topology errors |

### Why these help

- `input_custom_narrow_gaps_with_three_holes.csv`
  Tests whether simplification respects small separations between holes and the exterior boundary instead of creating accidental intersections.
- `input_custom_comb_bays_with_inner_hole.csv`
  Gives the algorithm several sharp concavities, so you can discuss whether the greedy collapse order handles repeated bays well.
- `input_custom_dense_hole_grid.csv`
  Stresses ring bookkeeping and topology validation because there are many interior rings in a relatively small space.
- `input_custom_near_collinear_wavy_shell.csv`
  Targets numerical stability: many vertices are almost collinear, so small placement errors can noticeably change displacement.
- `input_custom_pinch_corridor_with_two_holes.csv`
  Useful for checking whether collapses preserve a narrow passage and avoid merging nearby boundaries.

## Usage

```sh
./simplify <input_file> <target_vertices>
```

The program reads a CSV with columns `ring_id,vertex_id,x,y` and writes simplified output to stdout. If you are only reviewing the results in this repository, you do not need to run this command locally.

## Generate Outputs

```sh
make run
```

This is only needed if you want to regenerate the C++ program outputs yourself. It processes every `input_test_cases/input_*.csv` file and writes the results into `generated_outputs/my_output_*.txt`.

## HTML Report

```sh
python3 generate_html.py
```

This regenerates [Assignment_2_Results_Report.html](Assignment_2_Results_Report.html) from the current generated outputs.

## Test Results

The current implementation has been checked against all 15 instructor-provided reference outputs in `output_test_cases/`.

- Area is preserved to floating-point tolerance in the generated outputs.
- Topology is preserved: ring counts are unchanged, with no self-intersections or ring crossings in the validated outputs.
- For all 15 provided benchmark cases, the generated output is equal to or lower than the provided areal displacement.
- For `rectangle_with_two_holes` with target `7`, the solver stops at `11` vertices because no further valid collapse is accepted without violating the required constraints.

## Dependencies

- Python 3 for `generate_html.py`
- A C++17-compatible compiler and `make` only if you want to rebuild and rerun the simplifier locally

## Implementation Summary

- The polygon is represented as a set of circular doubly linked rings so local collapses can update neighbors in constant time.
- Candidate collapses are stored in a priority queue ordered by estimated areal displacement, which avoids rescanning all vertices after every accepted move.
- A lightweight spatial grid is used for intersection checks so topology validation does not require comparing against every edge globally.
- Only the local neighborhood around an accepted collapse is recomputed, rather than rebuilding the full candidate set after each step.

## Enhancements Beyond Basic APSC

- The implementation keeps the paper-style area-preserving segment collapse as the core operation, but adds a stronger bias toward boundary-intersection candidates instead of overusing interpolated fallback placements.
- Stale queue entries are filtered out by checking that the affected `A-B-C-D` sequence is still consecutive before a collapse is applied.
- The simplifier prevents the exterior ring from collapsing below four vertices, which avoids pathological outer-shell triangles on multi-hole benchmarks such as `rectangle_with_two_holes`.
- On the current instructor suite, these choices keep the generated displacement equal to or lower than the provided outputs across all 15 reference cases.
