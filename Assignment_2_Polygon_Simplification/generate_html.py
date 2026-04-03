import glob
import json
import math
import os
import re
import subprocess
from decimal import Decimal, InvalidOperation

GIVEN_DIR = "output_test_cases"
MY_DIR = "generated_outputs"
INPUT_DIR = "input_test_cases"
HTML_FILE = "Assignment2_Results_Report.html"

NOTES = {
    "output_rectangle_with_two_holes.txt": ("Minimal shell with multiple holes", "Very little slack: one bad collapse can turn the outer shell into a triangle and spike displacement."),
    "output_cushion_with_hexagonal_hole.txt": ("Smooth shell plus angular hole", "The solver must simplify a curved exterior without destabilizing the non-axis-aligned hole."),
    "output_blob_with_two_holes.txt": ("Irregular shell with holes", "Many locally plausible collapses make greedy ordering more error-prone."),
    "output_wavy_with_three_holes.txt": ("Wavy shell and many holes", "Oscillating boundaries and multiple holes increase narrow-clearance conflicts."),
    "output_lake_with_two_islands.txt": ("Large multi-ring topology", "This combines scale with interior-ring preservation in one benchmark."),
    "output_original_01.txt": ("High vertex count shoreline", "Useful for baseline scalability on large single-ring geometry."),
    "output_original_02.txt": ("High vertex count shoreline", "Queue churn and repeated local reevaluation dominate the workload."),
    "output_original_03.txt": ("Very high vertex count shoreline", "A good stress test for asymptotic runtime behavior."),
    "output_original_04.txt": ("High vertex count shoreline", "Tests large-n simplification quality without holes."),
    "output_original_05.txt": ("High vertex count shoreline", "Checks the balance between simplification aggressiveness and global area quality."),
    "output_original_06.txt": ("Large-scale high vertex dataset", "Both runtime and memory pressure rise sharply here."),
    "output_original_07.txt": ("Large shoreline with many candidate moves", "Many near-equivalent local options make cumulative drift easier to trigger."),
    "output_original_08.txt": ("Large shoreline with irregular features", "Sensitive to candidate placement heuristics and displacement regressions."),
    "output_original_09.txt": ("Extreme input size", "The strongest scaling stress test in the suite."),
    "output_original_10.txt": ("High vertex count shoreline", "Checks large-scale behavior while preserving area quality."),
    "output_custom_narrow_gaps_with_three_holes.txt": ("Narrow gaps", "Small clearances make accidental crossings easy if topology checks are weak."),
    "output_custom_comb_bays_with_inner_hole.txt": ("Alternating concave bays", "Repeated concavities create many tempting but globally poor simplification moves."),
    "output_custom_dense_hole_grid.txt": ("Large number of holes", "Many packed holes stress ring bookkeeping and intersection validation."),
    "output_custom_near_collinear_wavy_shell.txt": ("Near-degeneracies", "Tiny perturbations make numerical stability important."),
    "output_custom_pinch_corridor_with_two_holes.txt": ("Pinch corridor topology", "A narrow passage can disappear if collapses are accepted too aggressively."),
    "output_custom_combined.txt": ("Combined stressors", "Useful for checking overall robustness rather than one isolated failure mode."),
    "output_custom_high_hole.txt": ("Large number of holes", "Many interior rings increase the amount of topology checking per collapse."),
    "output_custom_high_vertex.txt": ("High vertex count", "A custom scalability stress test."),
    "output_custom_narrow_gaps.txt": ("Narrow gaps", "The geometry leaves little room for placement error."),
    "output_custom_near_degeneracies.txt": ("Near-degeneracies", "Almost-collinear features expose unstable predicates."),
}

SWEEP_TARGETS = {
    "output_rectangle_with_two_holes.txt": [12, 10, 8, 7],
    "output_blob_with_two_holes.txt": [36, 30, 24, 17],
    "output_original_07.txt": [10596, 4000, 1000, 250, 99],
}


def parse_decimal(v):
    try:
        return Decimal(v)
    except (InvalidOperation, TypeError):
        return None


def fmt_dec(v):
    return "N/A" if v is None else f"{float(v):.6e}"


def title_name(filename):
    return os.path.splitext(filename)[0].replace("output_", "", 1).replace("_", " ").title()


def infer_note(filename):
    if filename in NOTES:
        prop, why = NOTES[filename]
        return {"property": prop, "challenge": why}
    lower = filename.lower()
    if "narrow" in lower and "gap" in lower:
        return {"property": "Narrow gaps", "challenge": "Tight clearances make topology violations easy if the local move validator is too permissive."}
    if "hole" in lower:
        return {"property": "Multi-ring topology", "challenge": "The solver must preserve area while avoiding ring interactions."}
    if "degener" in lower or "collinear" in lower:
        return {"property": "Near-degeneracies", "challenge": "Numerically delicate geometry can expose unstable candidate placement."}
    if "vertex" in lower or "original" in lower:
        return {"property": "High vertex count", "challenge": "The main challenge is scaling runtime and memory while keeping displacement low."}
    return {"property": "General robustness", "challenge": "This case is included to observe overall simplification quality and stability."}


def parse_target_map():
    path = os.path.join(GIVEN_DIR, "README.md")
    if not os.path.exists(path):
        return {}
    pat = re.compile(r"\|\s*`input_(.+?)\.csv`\s*\|.*\|\s*(\d+)\s*\|\s*`output_.+?\.txt`\s*\|")
    out = {}
    for line in open(path, "r", encoding="utf-8"):
        m = pat.search(line)
        if m:
            out[f"output_{m.group(1)}.txt"] = int(m.group(2))
    return out


def input_path(output_name):
    core = os.path.splitext(output_name)[0].replace("output_", "", 1)
    return os.path.join(INPUT_DIR, f"input_{core}.csv")


def input_stats(output_name):
    path = input_path(output_name)
    if not os.path.exists(path):
        return {"vertices": "N/A", "holes": "N/A"}
    count, rings = 0, set()
    for line in open(path, "r", encoding="utf-8"):
        p = line.strip().split(",")
        if len(p) == 4 and p[0].lstrip("-").isdigit():
            rings.add(int(p[0]))
            count += 1
    return {"vertices": count, "holes": max(0, len(rings) - 1)}


def parse_lines(lines):
    d = {"rings": {}, "vertex_count": 0, "input_area": "N/A", "input_area_raw": "N/A", "output_area": "N/A", "output_area_raw": "N/A", "displacement": "N/A", "displacement_raw": "N/A", "target_vertices": "N/A", "time_ms": None, "memory_mb": None}
    for raw in lines:
        line = raw.strip()
        if not line:
            continue
        if line.startswith("Total signed area in input:"):
            v = line.split(":", 1)[1].strip()
            d["input_area"], d["input_area_raw"] = float(v), v
        elif line.startswith("Total signed area in output:"):
            v = line.split(":", 1)[1].strip()
            d["output_area"], d["output_area_raw"] = float(v), v
        elif line.startswith("Total areal displacement:"):
            v = line.split(":", 1)[1].strip()
            d["displacement"], d["displacement_raw"] = float(v), v
        elif line.startswith("Running time:"):
            d["time_ms"] = float(line.split(":", 1)[1].replace("ms", "").strip())
        elif line.startswith("Peak memory:"):
            d["memory_mb"] = float(line.split(":", 1)[1].replace("MB", "").strip())
        elif line.startswith("Target vertices:"):
            d["target_vertices"] = int(line.split(":", 1)[1].strip())
        else:
            p = line.split(",")
            if len(p) == 4 and p[0].lstrip("-").isdigit():
                try:
                    ring, x, y = int(p[0]), float(p[2]), float(p[3])
                except ValueError:
                    continue
                d["rings"].setdefault(ring, []).append((x, y))
                d["vertex_count"] += 1
    return d


def parse_file(path):
    if not os.path.exists(path):
        return None
    return parse_lines(open(path, "r", encoding="utf-8").read().splitlines())


def detect_executable():
    for c in ("simplify_win.exe", "simplify.exe", "simplify"):
        if os.path.exists(c):
            return os.path.abspath(c)
    return None


def run_solver(exe, csv_path, target):
    try:
        r = subprocess.run([exe, csv_path, str(target)], capture_output=True, text=True, timeout=120, check=True)
    except (OSError, subprocess.SubprocessError):
        return None
    return parse_lines(r.stdout.splitlines())


def build_sweep(results, target_map):
    exe = detect_executable()
    if not exe:
        return []
    out = []
    for name, targets in SWEEP_TARGETS.items():
        if not any(r["name"] == name for r in results):
            continue
        csv = input_path(name)
        if not os.path.exists(csv):
            continue
        pts = []
        for t in sorted(set(targets)):
            parsed = run_solver(exe, csv, t)
            if parsed and parsed["displacement"] != "N/A":
                pts.append({"x": t, "y": parsed["displacement"], "actual_vertices": parsed["vertex_count"]})
        if pts:
            out.append({"label": title_name(name), "points": pts, "reference_target": target_map.get(name)})
    return out


def svg(rings, min_x, max_x, min_y, max_y, fill, stroke):
    if not rings:
        return "<svg width='100%' height='300'><text x='10' y='150'>No valid polygon data</text></svg>"
    w, h, pad = 400, 300, 20
    span_x = max(max_x - min_x, 1)
    span_y = max(max_y - min_y, 1)
    scale = min((w - 2 * pad) / span_x, (h - 2 * pad) / span_y)
    xo, yo = (w - span_x * scale) / 2, (h - span_y * scale) / 2
    tx = lambda x: xo + (x - min_x) * scale
    ty = lambda y: h - (yo + (y - min_y) * scale)
    fill_path_d, stroke_paths, circles = "", [], []
    for ring_id, verts in sorted(rings.items()):
        if not verts:
            continue
        sx, sy = tx(verts[0][0]), ty(verts[0][1])
        ring_path = f"M {sx},{sy} "
        vertex_color = "#e74c3c" if ring_id == 0 else "#ff7f11"
        ring_stroke = stroke if ring_id == 0 else "#1d4e89"
        ring_dash = "" if ring_id == 0 else " stroke-dasharray='6 4'"
        circles.append(f"<circle cx='{sx}' cy='{sy}' r='3' fill='{vertex_color}' stroke='#ffffff' stroke-width='1' />")
        for x, y in verts[1:]:
            cx, cy = tx(x), ty(y)
            ring_path += f"L {cx},{cy} "
            circles.append(f"<circle cx='{cx}' cy='{cy}' r='3' fill='{vertex_color}' stroke='#ffffff' stroke-width='1' />")
        fill_path_d += ring_path + "Z "
        stroke_paths.append(f"<path d=\"{ring_path}Z\" fill='none' stroke='{ring_stroke}' stroke-width='2' fill-rule='evenodd'{ring_dash} />")
    return f"<svg viewBox='0 0 {w} {h}' style='width:100%;height:auto;border-radius:4px;background:#fafafa;border:1px dashed #bdc3c7;'><path d=\"{fill_path_d}\" fill='{fill}' stroke='none' fill-rule='evenodd' />{''.join(stroke_paths)}{''.join(circles)}</svg>"


def ring_summary(rings):
    if not rings:
        return "N/A"
    parts = []
    for ring_id, verts in sorted(rings.items()):
        label = "outer" if ring_id == 0 else f"hole {ring_id}"
        parts.append(f"{label}: {len(verts)}")
    return ", ".join(parts)


def fit(points, basis):
    if not points:
        return {"coefficient": 0.0, "r2": 0.0, "predicted": []}
    pairs = [(basis(p["x"]), p["y"]) for p in points if basis(p["x"]) > 0]
    if not pairs:
        return {"coefficient": 0.0, "r2": 0.0, "predicted": []}
    c = sum(y / b for b, y in pairs) / len(pairs)
    predicted = [{"x": p["x"], "y": c * basis(p["x"])} for p in points]
    ys = [p["y"] for p in points]
    mean = sum(ys) / len(ys)
    ss_tot = sum((y - mean) ** 2 for y in ys)
    ss_res = sum((p["y"] - c * basis(p["x"])) ** 2 for p in points)
    r2 = 1.0 if ss_tot == 0 else max(0.0, 1.0 - ss_res / ss_tot)
    return {"coefficient": c, "r2": r2, "predicted": predicted}


def results_table(cases, title):
    if not cases:
        return ""
    rows = [f"<h3>{title}</h3><div style='overflow-x:auto;'><table><thead><tr><th>Test Case</th><th>Input Vertices</th><th>Holes</th><th>Target Vertices</th><th>Actual Vertices</th><th>Input Area</th><th>Given Output Area</th><th>Actual Displacement</th><th>Given Displacement</th><th>Diff (Displacement)</th><th>Time (ms)</th><th>Memory (MB)</th></tr></thead><tbody>"]
    for r in cases:
        target = r["target_vertices"] if r["target_vertices"] != "N/A" else r["my"]["target_vertices"]
        m_disp = r["my"]["displacement"]
        g_disp = r["given"]["displacement"] if r["has_given"] else "N/A"
        diff_html = "N/A"
        if m_disp != "N/A" and g_disp != "N/A":
            if r["my"]["vertex_count"] != r["given"]["vertex_count"]:
                diff_html = f"<span style='color:#c97b00;font-weight:bold;'>N/A ({r['my']['vertex_count']} vs {r['given']['vertex_count']} verts)</span>"
            else:
                md, gd = parse_decimal(r["my"]["displacement_raw"]), parse_decimal(r["given"]["displacement_raw"])
                diff = None if md is None or gd is None else md - gd
                s = fmt_dec(diff)
                color = "#2b9348" if diff is not None and diff <= Decimal("0.00001") else "#c0392b"
                diff_html = f"<span style='color:{color};font-weight:bold;'>{s}</span>"
        time_val = f"{r['my']['time_ms']:.2f}" if r["my"]["time_ms"] is not None else "N/A"
        mem_val = f"{r['my']['memory_mb']:.2f}" if r["my"]["memory_mb"] is not None else "N/A"
        rows.append(
            "<tr>"
            f"<td><a href='#{r['name']}'>{r['name']}</a></td><td>{r['input_size']}</td><td>{r['holes']}</td><td>{target}</td><td>{r['my']['vertex_count']}</td>"
            f"<td>{r['my']['input_area_raw'] if isinstance(r['my']['input_area'], float) else 'N/A'}</td>"
            f"<td>{r['given']['output_area_raw'] if r['has_given'] and isinstance(r['given']['output_area'], float) else 'N/A'}</td>"
            f"<td>{r['my']['displacement_raw'] if m_disp != 'N/A' else 'N/A'}</td>"
            f"<td>{r['given']['displacement_raw'] if g_disp != 'N/A' else 'N/A'}</td>"
            f"<td>{diff_html}</td><td>{time_val}</td><td>{mem_val}</td></tr>"
        )
    rows.append("</tbody></table></div>")
    return "".join(rows)


def goal_table(cases, title):
    if not cases:
        return ""
    rows = [f"<h3>{title}</h3><div style='overflow-x:auto;'><table><thead><tr><th>Dataset</th><th>Input Vertices</th><th>Holes</th><th>Target Vertices</th><th>Targeted Property</th><th>Why It Is Challenging</th></tr></thead><tbody>"]
    for r in cases:
        rows.append(f"<tr><td>{r['name']}</td><td>{r['input_size']}</td><td>{r['holes']}</td><td>{r['target_vertices']}</td><td>{r['property']}</td><td>{r['challenge']}</td></tr>")
    rows.append("</tbody></table></div>")
    return "".join(rows)


def custom_cards(cases):
    if not cases:
        return "<p class='placeholder-text'>No custom outputs were found.</p>"
    out = ["<div class='card-grid'>"]
    for r in cases:
        time_val = f"{r['my']['time_ms']:.2f}" if r["my"]["time_ms"] is not None else "N/A"
        mem_val = f"{r['my']['memory_mb']:.2f}" if r["my"]["memory_mb"] is not None else "N/A"
        out.append(f"<div class='info-card'><h4>{r['name']}</h4><p><strong>Targets:</strong> {r['property']}</p><p><strong>Why difficult:</strong> {r['challenge']}</p><p><strong>Observed outcome:</strong> {r['my']['vertex_count']} final vertices, {r['my']['displacement_raw']} displacement, {time_val} ms runtime, {mem_val} MB peak memory.</p></div>")
    out.append("</div>")
    return "".join(out)


def fit_cards(time_fit, mem_fit):
    return (
        "<div class='formula-grid'>"
        f"<div class='formula-card'><h4>Runtime Fit</h4><p><code>T(n) ≈ {time_fit['coefficient']:.6e} · n log₂ n</code></p><p><strong>R² = {time_fit['r2']:.4f}</strong></p></div>"
        f"<div class='formula-card'><h4>Memory Fit</h4><p><code>M(n) ≈ {mem_fit['coefficient']:.6e} · n</code> MB</p><p><strong>R² = {mem_fit['r2']:.4f}</strong></p></div>"
        "</div>"
    )


def discussion(provided, custom, time_fit, mem_fit):
    comparable = [r for r in provided if r["has_given"] and r["my"]["displacement"] != "N/A" and r["given"]["displacement"] != "N/A" and r["my"]["vertex_count"] == r["given"]["vertex_count"]]
    improved = [r for r in comparable if r["my"]["displacement"] <= r["given"]["displacement"] + 1e-12]
    best = max(comparable, key=lambda r: r["given"]["displacement"] - r["my"]["displacement"]) if comparable else None
    worst_rt = max(provided + custom, key=lambda r: r["my"]["time_ms"] or 0.0)
    worst_mem = max(provided + custom, key=lambda r: r["my"]["memory_mb"] or 0.0)
    hardest_custom = max(custom, key=lambda r: (r["my"]["time_ms"] or 0.0, r["my"]["displacement"] if r["my"]["displacement"] != "N/A" else -1.0)) if custom else None
    parts = [
        f"<p>The provided benchmark set contains <strong>{len(comparable)}</strong> directly comparable cases, and the current implementation is <strong>equal to or better than the given displacement on {len(improved)}</strong> of them.</p>",
        f"<p>{'The largest observed improvement is on <strong>' + best['name'] + '</strong>, where displacement drops from ' + best['given']['displacement_raw'] + ' to ' + best['my']['displacement_raw'] + '.' if best else 'No directly comparable benchmark cases were found.'}</p>",
        f"<p>The scaling plots are consistent with an <code>O(n log n)</code> runtime trend and an <code>O(n)</code> memory trend. The fitted coefficients are <code>{time_fit['coefficient']:.6e}</code> for runtime and <code>{mem_fit['coefficient']:.6e}</code> for memory, with R² values of {time_fit['r2']:.4f} and {mem_fit['r2']:.4f}.</p>",
        f"<p>The heaviest workload in the current report is <strong>{worst_rt['name']}</strong> at {worst_rt['my']['time_ms']:.2f} ms, while the largest peak memory is on <strong>{worst_mem['name']}</strong> at {worst_mem['my']['memory_mb']:.2f} MB.</p>",
    ]
    if hardest_custom:
        parts.append(f"<p>Among the custom datasets, <strong>{hardest_custom['name']}</strong> appears to be the toughest in practice. It targets <strong>{hardest_custom['property']}</strong> and finishes with {hardest_custom['my']['displacement_raw']} displacement after {hardest_custom['my']['time_ms']:.2f} ms.</p>")
    return "".join(parts)


def main():
    target_map = parse_target_map()
    files = glob.glob(os.path.join(MY_DIR, "my_output_*.txt"))
    if not files:
        return

    results = []
    for my_path in files:
        my_name = os.path.basename(my_path)
        given_name = my_name.replace("my_", "", 1)
        my_data = parse_file(my_path)
        if not my_data:
            continue
        given_data = parse_file(os.path.join(GIVEN_DIR, given_name))
        stats = input_stats(given_name)
        note = infer_note(given_name)
        target = target_map.get(given_name, my_data["target_vertices"])
        results.append({"name": given_name, "pretty_name": title_name(given_name), "input_size": stats["vertices"], "holes": stats["holes"], "target_vertices": target, "property": note["property"], "challenge": note["challenge"], "my": my_data, "given": given_data, "has_given": given_data is not None})

    provided = sorted([r for r in results if r["has_given"]], key=lambda r: (1 if "original" in r["name"].lower() else 0, r["name"]))
    custom = sorted([r for r in results if not r["has_given"]], key=lambda r: r["name"])
    all_cases = provided + custom
    size_cases = sorted([r for r in all_cases if isinstance(r["input_size"], int)], key=lambda r: r["input_size"])

    time_points = [{"x": r["input_size"], "y": r["my"]["time_ms"]} for r in size_cases if r["my"]["time_ms"] is not None and r["input_size"] > 1]
    mem_points = [{"x": r["input_size"], "y": r["my"]["memory_mb"]} for r in size_cases if r["my"]["memory_mb"] is not None and r["input_size"] > 0]
    disp_size_points = [{"x": r["input_size"], "y": r["my"]["displacement"], "label": r["name"]} for r in size_cases if r["my"]["displacement"] != "N/A"]
    time_fit = fit(time_points, lambda n: n * math.log2(n))
    mem_fit = fit(mem_points, lambda n: n)

    comparable = [r for r in provided if r["my"]["displacement"] != "N/A" and r["given"]["displacement"] != "N/A" and r["my"]["vertex_count"] == r["given"]["vertex_count"]]
    disp_labels = [r["name"] for r in comparable]
    my_disp = [r["my"]["displacement"] for r in comparable]
    given_disp = [r["given"]["displacement"] for r in comparable]
    vert_labels = [r["name"] for r in provided]
    my_verts = [r["my"]["vertex_count"] for r in provided]
    given_verts = [r["given"]["vertex_count"] if r["has_given"] else 0 for r in provided]

    collapse_labels, collapse_provided, collapse_custom = [], [], []
    for r in all_cases:
        if r["my"]["time_ms"] is None or not isinstance(r["input_size"], int):
            continue
        collapses = r["input_size"] - r["my"]["vertex_count"]
        if collapses <= 0:
            continue
        avg = r["my"]["time_ms"] / collapses
        collapse_labels.append(r["name"])
        if r["has_given"]:
            collapse_provided.append(avg)
            collapse_custom.append(0)
        else:
            collapse_provided.append(0)
            collapse_custom.append(avg)

    sweep = build_sweep(results, target_map)
    colors = [("#e76f51", "rgba(231,111,81,0.15)"), ("#168aad", "rgba(22,138,173,0.15)"), ("#2b9348", "rgba(43,147,72,0.15)")]
    sweep_datasets = [{"label": s["label"], "data": s["points"], "borderColor": colors[i % len(colors)][0], "backgroundColor": colors[i % len(colors)][1], "showLine": True, "pointRadius": 4, "pointHoverRadius": 6, "tension": 0.15} for i, s in enumerate(sweep)]

    html = """<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>Assignment 2 - Results Report</title><script src='https://cdn.jsdelivr.net/npm/chart.js'></script><style>
    :root{--primary:#20364d;--secondary:#38526b;--accent:#168aad;--bg:#f4f7fb;--muted:#64748b;}body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:#233142;margin:0;padding:0;scroll-behavior:smooth;}
    header{background:linear-gradient(135deg,#20364d,#2b4c68);color:#fff;padding:22px 40px;position:sticky;top:0;z-index:1000;box-shadow:0 2px 12px rgba(0,0,0,.12);}header h1{margin:0 0 10px 0;font-size:24px;}nav{display:flex;gap:12px;flex-wrap:wrap;}nav a{color:#fff;text-decoration:none;padding:8px 12px;background:rgba(255,255,255,.12);border-radius:999px;font-size:14px;}
    .container{max-width:1380px;margin:36px auto;padding:0 20px 40px;}section{background:#fff;margin-bottom:32px;padding:28px;border-radius:14px;box-shadow:0 2px 10px rgba(24,39,75,.08);}h2{color:var(--primary);border-bottom:2px solid #e8edf3;padding-bottom:10px;margin-top:0;}h3{color:var(--secondary);margin-top:28px;}h4{margin-top:0;color:var(--secondary);}p,ul{line-height:1.7;}
    table{width:100%;border-collapse:collapse;margin-top:12px;font-size:13px;white-space:nowrap;}th,td{padding:10px 12px;text-align:left;border-bottom:1px solid #dbe2ea;vertical-align:top;}th{background:#eef3f8;color:var(--secondary);font-weight:600;border-top:1px solid #dbe2ea;}td a{color:var(--accent);text-decoration:none;font-weight:600;}
    .viz-grid,.two-chart-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-top:20px;}.card-grid,.formula-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:18px;margin-top:18px;}
    .viz-card,.info-card,.formula-card{border:1px solid #e5ebf2;border-radius:12px;padding:16px;background:#fff;}.metrics-box{background:#f6f9fc;padding:10px;border-radius:8px;font-family:Consolas,monospace;font-size:12px;margin-top:12px;line-height:1.6;}
    .chart-container{position:relative;height:390px;width:100%;min-width:0;box-sizing:border-box;margin-top:18px;border:1px solid #e5ebf2;padding:10px;border-radius:12px;background:#fff;}.placeholder-text{color:var(--muted);}.instructions-box{background:linear-gradient(135deg,#f5fbff,#eef7fb);border:1px solid #cfe5ef;border-radius:12px;padding:18px 20px;}code{background:#f3f6fa;border-radius:6px;padding:2px 6px;}@media (max-width:960px){.viz-grid,.two-chart-grid{grid-template-columns:1fr;}header{padding:18px 20px;}}</style></head><body>"""
    html += """<header><h1>Assignment 2 - Results Report</h1><nav><a href='#instructions'>Instructions</a><a href='#dataset-goals'>Dataset Goals</a><a href='#overview'>Overview</a><a href='#efficiency'>Efficiency</a><a href='#displacement'>Displacement</a><a href='#custom-cases'>Custom Cases</a><a href='#discussion'>Discussion</a><a href='#correctness'>Visuals</a></nav></header><div class='container'>"""
    html += "<section id='instructions'><h2>Instructions</h2><div class='instructions-box'><p>This report is organized to satisfy the experimental-evaluation requirements of the assignment.</p><ul><li><strong>Dataset Goals</strong> explains what each dataset targets and why it is difficult.</li><li><strong>Overview</strong> keeps the exact values from the output files for area, displacement, running time, and peak memory.</li><li><strong>Efficiency</strong> shows running time vs input size and peak memory vs input size, together with fitted scaling laws.</li><li><strong>Displacement</strong> compares displacement quality, final vertex counts, displacement vs input size, and displacement vs target vertex count.</li><li><strong>Custom Cases</strong> explains the purpose of the custom stress tests.</li><li><strong>Discussion</strong> interprets the main trends rather than only listing raw values.</li></ul></div></section>"
    html += f"<section id='dataset-goals'><h2>Dataset Goals</h2><p class='placeholder-text'>Each benchmark is tagged with the geometric property it targets and why that property is challenging for area-and-topology-preserving simplification.</p>{goal_table(provided, 'Provided Test Datasets')}{goal_table(custom, 'Custom Test Datasets')}</section>"
    html += f"<section id='overview'><h2>Results Overview</h2><p class='placeholder-text'>The tables below preserve the exact scientific-notation values from the output files for input area, output area, displacement, and displacement difference. Running time and peak memory are also listed for every generated output.</p>{results_table(provided, 'Provided Test Cases')}{results_table(custom, 'Custom Test Cases')}</section>"
    html += f"<section id='efficiency'><h2>Efficiency and Scaling</h2><p class='placeholder-text'>Requirement (a) and (b): running time versus input size and peak memory versus input size. The fitted models summarize the approximate asymptotic behavior seen in the measurements.</p>{fit_cards(time_fit, mem_fit)}<div class='two-chart-grid'><div class='chart-container'><canvas id='timeChart'></canvas></div><div class='chart-container'><canvas id='memChart'></canvas></div></div></section>"
    html += "<section id='displacement'><h2>Areal Displacement Analysis</h2><p class='placeholder-text'>Requirement (c): areal displacement is examined both against the provided outputs and against requested target vertex counts. The report therefore includes direct comparisons, final-vertex comparisons, displacement-versus-input-size behavior, and explicit target sweeps.</p><div class='two-chart-grid'><div class='chart-container'><canvas id='dispChart'></canvas></div><div class='chart-container'><canvas id='verticesChart'></canvas></div></div><div class='two-chart-grid'><div class='chart-container'><canvas id='dispSizeChart'></canvas></div><div class='chart-container'><canvas id='targetSweepChart'></canvas></div></div></section>"
    html += f"<section id='custom-cases'><h2>What the Custom Test Cases Target</h2><p class='placeholder-text'>These custom datasets are meant to expose failure modes that are easy to miss on ordinary benchmarks. They are especially useful for discussing robustness under narrow clearances, many holes, high vertex counts, and near-degenerate geometry.</p>{custom_cards(custom)}<div class='chart-container'><canvas id='collapseChart'></canvas></div></section>"
    html += f"<section id='discussion'><h2>Discussion and Interpretation</h2>{discussion(provided, custom, time_fit, mem_fit)}</section>"
    html += "<section id='correctness'><h2>Visual Verification</h2><p class='placeholder-text'>The views below support manual checking of topology preservation. Red dots mark the surviving vertices in each final polygon.</p>"
    for r in all_cases:
        xs, ys = [], []
        if r["has_given"]:
            xs += [v[0] for ring in r["given"]["rings"].values() for v in ring]
            ys += [v[1] for ring in r["given"]["rings"].values() for v in ring]
        xs += [v[0] for ring in r["my"]["rings"].values() for v in ring]
        ys += [v[1] for ring in r["my"]["rings"].values() for v in ring]
        if not xs:
            continue
        min_x, max_x, min_y, max_y = min(xs), max(xs), min(ys), max(ys)
        grid_style = "grid-template-columns:1fr;" if not r["has_given"] else "grid-template-columns:1fr 1fr;"
        time_val = f"{r['my']['time_ms']:.2f}" if r["my"]["time_ms"] is not None else "N/A"
        mem_val = f"{r['my']['memory_mb']:.2f}" if r["my"]["memory_mb"] is not None else "N/A"
        html += f"<div id='{r['name']}' style='margin-top:42px;border-top:2px dashed #e1e8f0;padding-top:26px;'><h3>{r['name']} (Input vertices: {r['input_size']}, holes: {r['holes']})</h3><p class='placeholder-text'><strong>Targets:</strong> {r['property']}. <strong>Why difficult:</strong> {r['challenge']}</p><div class='viz-grid' style='{grid_style}'>"
        if r["has_given"]:
            html += f"<div class='viz-card'><h4>Given Output ({r['given']['vertex_count']} vertices)</h4>{svg(r['given']['rings'], min_x, max_x, min_y, max_y, 'rgba(52,152,219,0.35)', 'rgba(41,128,185,1.0)')}<div class='metrics-box'><strong>Output Area:</strong> {r['given']['output_area_raw']}<br><strong>Displacement:</strong> {r['given']['displacement_raw']}<br><strong>Ring vertices:</strong> {ring_summary(r['given']['rings'])}</div></div>"
        html += f"<div class='viz-card'><h4>My Output ({r['my']['vertex_count']} vertices)</h4>{svg(r['my']['rings'], min_x, max_x, min_y, max_y, 'rgba(46,204,113,0.35)', 'rgba(39,174,96,1.0)')}<div class='metrics-box'><strong>Output Area:</strong> {r['my']['output_area_raw']}<br><strong>Displacement:</strong> {r['my']['displacement_raw']}<br><strong>Ring vertices:</strong> {ring_summary(r['my']['rings'])}<br><strong>Time:</strong> {time_val} ms<br><strong>Peak Memory:</strong> {mem_val} MB</div></div></div></div>"

    html += "</section></div>"
    html += "<script>"
    html += f"const timeData={json.dumps(time_points)},timeTrend={json.dumps(time_fit['predicted'])},memData={json.dumps(mem_points)},memTrend={json.dumps(mem_fit['predicted'])},dispLabels={json.dumps(disp_labels)},myDisp={json.dumps(my_disp)},givenDisp={json.dumps(given_disp)},vertLabels={json.dumps(vert_labels)},myVerts={json.dumps(my_verts)},givenVerts={json.dumps(given_verts)},dispSizePoints={json.dumps(disp_size_points)},sweepDatasets={json.dumps(sweep_datasets)},collapseLabels={json.dumps(collapse_labels)},collapseProvided={json.dumps(collapse_provided)},collapseCustom={json.dumps(collapse_custom)};"
    html += "const sciTick=(value)=>{const n=Number(value);return Number.isFinite(n)?n.toExponential(2):value;};"
    html += "const plainTick=(value)=>{const n=Number(value);return Number.isFinite(n)?n.toLocaleString():value;};"
    html += "new Chart(document.getElementById('timeChart'),{type:'scatter',data:{datasets:[{label:'Measured Runtime (ms)',data:timeData,backgroundColor:'#e76f51',borderColor:'#e76f51'},{label:'Fit: c · n log₂ n',data:timeTrend,type:'line',borderColor:'#b6462a',fill:false,pointRadius:0,borderDash:[6,6]}]},options:{maintainAspectRatio:false,plugins:{title:{display:true,text:'Running Time vs Input Size'}},scales:{x:{type:'linear',title:{display:true,text:'Input size (vertices)'}},y:{title:{display:true,text:'Running time (ms)'}}}}});"
    html += "new Chart(document.getElementById('memChart'),{type:'scatter',data:{datasets:[{label:'Measured Peak Memory (MB)',data:memData,backgroundColor:'#8e5ea2',borderColor:'#8e5ea2'},{label:'Fit: c · n',data:memTrend,type:'line',borderColor:'#68407a',fill:false,pointRadius:0,borderDash:[6,6]}]},options:{maintainAspectRatio:false,plugins:{title:{display:true,text:'Peak Memory vs Input Size'}},scales:{x:{type:'linear',title:{display:true,text:'Input size (vertices)'}},y:{title:{display:true,text:'Peak memory (MB)'}}}}});"
    html += "new Chart(document.getElementById('dispChart'),{type:'bar',data:{labels:dispLabels,datasets:[{label:'My displacement',data:myDisp,backgroundColor:'rgba(43,147,72,0.85)'},{label:'Given displacement',data:givenDisp,backgroundColor:'rgba(22,138,173,0.85)'}]},options:{maintainAspectRatio:false,plugins:{title:{display:true,text:'Direct Displacement Comparison'}},scales:{y:{type:'logarithmic',title:{display:true,text:'Areal displacement'}}}}});"
    html += "new Chart(document.getElementById('verticesChart'),{type:'bar',data:{labels:vertLabels,datasets:[{label:'My final vertices',data:myVerts,backgroundColor:'rgba(87,117,144,0.85)'},{label:'Given final vertices',data:givenVerts,backgroundColor:'rgba(173,181,189,0.85)'}]},options:{maintainAspectRatio:false,plugins:{title:{display:true,text:'Final Vertex Count Comparison'}},scales:{y:{title:{display:true,text:'Vertices remaining'}}}}});"
    html += "new Chart(document.getElementById('dispSizeChart'),{type:'scatter',data:{datasets:[{label:'Displacement vs input size',data:dispSizePoints,backgroundColor:'rgba(244,162,97,0.9)',borderColor:'rgba(244,162,97,0.9)',pointRadius:4,pointHoverRadius:6}]},options:{maintainAspectRatio:false,parsing:false,layout:{padding:{top:8,right:12,bottom:8,left:8}},plugins:{title:{display:true,text:'Areal Displacement vs Input Size'},legend:{position:'bottom'},tooltip:{callbacks:{label:(c)=>`${c.raw.label}: n=${Number(c.raw.x).toLocaleString()}, displacement=${c.raw.y.toExponential(6)}`}}},scales:{x:{type:'logarithmic',title:{display:true,text:'Input size (vertices, log scale)'},ticks:{callback:plainTick}},y:{type:'logarithmic',title:{display:true,text:'Areal displacement (log scale)'},ticks:{callback:sciTick}}}}});"
    html += "new Chart(document.getElementById('targetSweepChart'),{type:'scatter',data:{datasets:sweepDatasets},options:{maintainAspectRatio:false,parsing:false,layout:{padding:{top:8,right:12,bottom:8,left:8}},plugins:{title:{display:true,text:'Areal Displacement vs Target Vertex Count'},legend:{position:'bottom'},tooltip:{callbacks:{label:(c)=>`${c.dataset.label}: target=${Number(c.raw.x).toLocaleString()}, displacement=${c.raw.y.toExponential(6)}, actual vertices=${c.raw.actual_vertices}`}}},scales:{x:{type:'logarithmic',title:{display:true,text:'Requested target vertex count (log scale)'},ticks:{callback:plainTick}},y:{type:'logarithmic',title:{display:true,text:'Areal displacement (log scale)'},ticks:{callback:sciTick}}}}});"
    html += "new Chart(document.getElementById('collapseChart'),{type:'bar',data:{labels:collapseLabels,datasets:[{label:'Provided datasets',data:collapseProvided,backgroundColor:'rgba(22,138,173,0.85)'},{label:'Custom datasets',data:collapseCustom,backgroundColor:'rgba(233,196,106,0.85)'}]},options:{maintainAspectRatio:false,plugins:{title:{display:true,text:'Average Time per Accepted Collapse'}},scales:{y:{title:{display:true,text:'Milliseconds per collapse'}}}}});"
    html += "</script></body></html>"

    with open(HTML_FILE, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"Success! Report generated at: {os.path.abspath(HTML_FILE)}")


if __name__ == "__main__":
    main()
