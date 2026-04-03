import math
import os

# Write all generated custom datasets into the same folder used by the solver.
os.makedirs("input_test_cases", exist_ok=True)

def write_csv(filename, rings):
    # Each ring is emitted in the assignment's ring_id,vertex_id,x,y format.
    with open(f"input_test_cases/{filename}", "w") as f:
        f.write("ring_id,vertex_id,x,y\n")
        for r_id, ring in enumerate(rings):
            for v_id, (x, y) in enumerate(ring):
                f.write(f"{r_id},{v_id},{x:.8f},{y:.8f}\n")

print("Generating datasets...")

# 1. High Hole Count (Targets spatial index efficiency)
rings_hh = [[(0,0), (100,0), (100,100), (0,100)]]
for i in range(10):
    for j in range(10):
        cx, cy = 5 + i*10, 5 + j*10
        rings_hh.append([(cx-1, cy-1), (cx-1, cy+1), (cx+1, cy+1), (cx+1, cy-1)])
write_csv("input_custom_high_hole.csv", rings_hh)
print("- input_custom_high_hole.csv created.")

# 2. High Vertex Count (Targets O(n log n) scaling)
rings_hv = [[]]
for i in range(15000):
    theta = 2 * math.pi * i / 15000
    rings_hv[0].append((100 * math.cos(theta), 100 * math.sin(theta)))
write_csv("input_custom_high_vertex.csv", rings_hv)
print("- input_custom_high_vertex.csv created.")

# 3. Narrow Gaps (Targets Spatial Index edge-cases)
eps = 0.001
outer, inner = [], []
# Build two nearly touching rectangles so topology checks have very little slack.
for i in range(100): outer.append((i/10.0, 0))
for i in range(100): outer.append((10, i/10.0))
for i in range(100): outer.append((10 - i/10.0, 10))
for i in range(100): outer.append((0, 10 - i/10.0))
for i in range(100): inner.append((eps, eps + i/10.0 * 0.99))
for i in range(100): inner.append((eps + i/10.0 * 0.99, 10-eps))
for i in range(100): inner.append((10-eps, 10-eps - i/10.0 * 0.99))
for i in range(100): inner.append((10-eps - i/10.0 * 0.99, eps))
write_csv("input_custom_narrow_gaps.csv", [outer, inner])
print("- input_custom_narrow_gaps.csv created.")

# 4. Near-Degeneracies (Targets floating point precision)
rings_nd = [[]]
for i in range(2000):
    theta = 2 * math.pi * i / 2000
    r = 100 if i % 2 == 0 else 100.0000001
    rings_nd[0].append((r * math.cos(theta), r * math.sin(theta)))
write_csv("input_custom_near_degeneracies.csv", rings_nd)
print("- input_custom_near_degeneracies.csv created.")

# 5. Combined Requirements (The ultimate stress test)
rings_comb = [[]]
for i in range(5000):
    theta = 2 * math.pi * i / 5000
    r = 100 if i % 2 == 0 else 100.0000001
    rings_comb[0].append((r * math.cos(theta), r * math.sin(theta)))
for i in range(50):
    theta = 2 * math.pi * i / 50
    cx, cy = 99.9 * math.cos(theta), 99.9 * math.sin(theta)
    rings_comb.append([(cx-0.001, cy-0.001), (cx-0.001, cy+0.001), (cx+0.001, cy+0.001), (cx+0.001, cy-0.001)])
write_csv("input_custom_combined.csv", rings_comb)
print("- input_custom_combined.csv created.")

print("\nDone! You can now run your algorithm on these files.")
