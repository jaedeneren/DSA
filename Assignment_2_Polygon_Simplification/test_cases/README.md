# Test Cases for Area-and-Topology-Preserving Polygon Simplification

Each test case consists of an **input CSV** and the corresponding **expected output**.

## Simple cases (polygons with holes)

| Input file | Vertices | Holes | Target | Output file |
|---|---|---|---|---|
| `input_rectangle_with_two_holes.csv` | 12 | 2 | 11 | `output_rectangle_with_two_holes.txt` |
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

```
./area_and_topology_preserving_polygon_simplification <input_file> <target_vertices>
```

The program reads a CSV with columns `ring_id,vertex_id,x,y` and writes simplified output to stdout.
