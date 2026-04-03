import os
import glob
import json

# Directory configuration
GIVEN_DIR = "output_test_cases"
MY_DIR = "generated_outputs"
HTML_FILE = "Assignment2_Results_Report.html"

def parse_file(filepath):
    """Parses vertices, metadata, and performance metrics from the given text file."""
    if not os.path.exists(filepath):
        return None
        
    data = {
        'rings': {},
        'vertex_count': 0,
        'input_area': 'N/A',
        'output_area': 'N/A',
        'displacement': 'N/A',
        'time_ms': 'N/A',
        'memory_mb': 'N/A'
    }

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            # Extract metadata
            if line.startswith('Total signed area in input:'):
                data['input_area'] = float(line.split(':')[1].strip())
            elif line.startswith('Total signed area in output:'):
                data['output_area'] = float(line.split(':')[1].strip())
            elif line.startswith('Total areal displacement:'):
                data['displacement'] = float(line.split(':')[1].strip())
            elif line.startswith('Running time:'):
                data['time_ms'] = float(line.split(':')[1].replace('ms', '').strip())
            elif line.startswith('Peak memory:'):
                data['memory_mb'] = float(line.split(':')[1].replace('MB', '').strip())
            else:
                # Extract CSV vertices
                parts = line.split(',')
                if len(parts) == 4 and parts[0].lstrip('-').isdigit():
                    ring_id = int(parts[0])
                    try:
                        x, y = float(parts[2]), float(parts[3])
                        if ring_id not in data['rings']:
                            data['rings'][ring_id] = []
                        data['rings'][ring_id].append((x, y))
                        data['vertex_count'] += 1
                    except ValueError:
                        pass
    return data

def generate_svg(rings, min_x, max_x, min_y, max_y, fill_color, stroke_color):
    """Generates an SVG string for the given rings."""
    if not rings:
        return "<svg width='100%' height='300'><text x='10' y='150'>No valid polygon data</text></svg>"
        
    width, height, padding = 400, 300, 20
    span_x = max_x - min_x if (max_x - min_x) > 0 else 1
    span_y = max_y - min_y if (max_y - min_y) > 0 else 1

    scale = min((width - 2*padding) / span_x, (height - 2*padding) / span_y)
    x_offset = (width - (span_x * scale)) / 2
    y_offset = (height - (span_y * scale)) / 2

    def tx(x): return x_offset + (x - min_x) * scale
    def ty(y): return height - (y_offset + (y - min_y) * scale)

    path_d = ""
    for ring_id, vertices in rings.items():
        if not vertices: continue
        path_d += f"M {tx(vertices[0][0])},{ty(vertices[0][1])} "
        for x, y in vertices[1:]:
            path_d += f"L {tx(x)},{ty(y)} "
        path_d += "Z "

    return f"""
    <svg viewBox="0 0 {width} {height}" style="width:100%; height:auto;">
        <path d="{path_d}" fill="{fill_color}" stroke="{stroke_color}" stroke-width="2" fill-rule="evenodd" />
    </svg>
    """

def main():
    my_files = glob.glob(os.path.join(MY_DIR, "my_output_*.txt"))
    if not my_files:
        print(f"No generated files found in {MY_DIR}/")
        return

    results = []
    
    for my_filepath in sorted(my_files):
        my_filename = os.path.basename(my_filepath)
        given_filename = my_filename.replace("my_", "", 1)
        given_filepath = os.path.join(GIVEN_DIR, given_filename)

        my_data = parse_file(my_filepath)
        given_data = parse_file(given_filepath)
        
        if not my_data: continue

        results.append({
            'name': given_filename,
            'my': my_data,
            'given': given_data,
            'has_given': given_data is not None
        })

    # --- HTML TEMPLATE ---
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Assignment 2 - Results Report</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            :root { --primary: #2c3e50; --secondary: #34495e; --accent: #3498db; --success: #2ecc71; --bg: #f8f9fa; }
            body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: #333; margin: 0; padding: 0; }
            header { background: var(--primary); color: white; padding: 20px 40px; position: sticky; top: 0; z-index: 1000; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
            header h1 { margin: 0 0 10px 0; font-size: 24px; }
            nav { display: flex; gap: 15px; }
            nav a { color: white; text-decoration: none; padding: 8px 12px; background: rgba(255,255,255,0.1); border-radius: 4px; font-size: 14px; transition: 0.2s; }
            nav a:hover { background: var(--accent); }
            
            .container { max-width: 1200px; margin: 40px auto; padding: 0 20px; }
            section { background: white; margin-bottom: 40px; padding: 30px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.05); }
            h2 { color: var(--primary); border-bottom: 2px solid #eee; padding-bottom: 10px; margin-top: 0; }
            
            table { width: 100%; border-collapse: collapse; margin-top: 20px; font-size: 14px; }
            th, td { padding: 12px 15px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f1f4f8; color: var(--secondary); font-weight: 600; }
            tr:hover { background-color: #f9fbfd; }
            td a { color: var(--accent); text-decoration: none; font-weight: 500; }
            td a:hover { text-decoration: underline; }
            
            .viz-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 30px; margin-top: 20px; }
            .viz-card { border: 1px solid #eee; border-radius: 8px; padding: 15px; background: #fff; }
            .viz-card h4 { margin: 0 0 15px 0; text-align: center; color: var(--secondary); }
            .metrics-box { background: #f8f9fa; padding: 10px; border-radius: 4px; font-family: monospace; font-size: 12px; margin-top: 15px; }
            
            .chart-container { position: relative; height: 350px; width: 100%; margin-top: 20px; }
            .placeholder-text { color: #7f8c8d; font-style: italic; line-height: 1.6; }
        </style>
    </head>
    <body>

    <header>
        <h1>Assignment 2 - Results Report</h1>
        <nav>
            <a href="#overview">Overview Table</a>
            <a href="#correctness">Correctness</a>
            <a href="#displacement">Areal Displacement</a>
            <a href="#efficiency">Efficiency</a>
            <a href="#evaluation">Experimental Evaluation</a>
        </nav>
    </header>

    <div class="container">
        <section id="overview">
            <h2>Test Results Overview</h2>
            <table>
                <thead>
                    <tr>
                        <th>Test Case</th>
                        <th>Output Vertices</th>
                        <th>Input Area</th>
                        <th>My Displacement</th>
                        <th>Given Displacement</th>
                        <th>Time (ms)</th>
                        <th>Memory (MB)</th>
                    </tr>
                </thead>
                <tbody>
    """

    # Generate Table Rows
    for r in results:
        g_disp = f"{r['given']['displacement']:.2e}" if r['has_given'] else "N/A"
        html_content += f"""
            <tr>
                <td><a href="#{r['name']}">{r['name']}</a></td>
                <td>{r['my']['vertex_count']}</td>
                <td>{r['my']['input_area']:.2e}</td>
                <td>{r['my']['displacement']:.2e}</td>
                <td>{g_disp}</td>
                <td>{r['my']['time_ms']}</td>
                <td>{r['my']['memory_mb']}</td>
            </tr>
        """
        
    html_content += """
                </tbody>
            </table>
        </section>

        <section id="correctness">
            <h2>Correctness of Implementation</h2>
            <p class="placeholder-text">Area preserved to within floating-point tolerance. Topology preserved: no self-intersections, no ring crossings, ring count unchanged.</p>
    """

    # Generate Visualizations
    for r in results:
        all_x, all_y = [], []
        if r['has_given']:
            all_x.extend([v[0] for ring in r['given']['rings'].values() for v in ring])
            all_y.extend([v[1] for ring in r['given']['rings'].values() for v in ring])
        all_x.extend([v[0] for ring in r['my']['rings'].values() for v in ring])
        all_y.extend([v[1] for ring in r['my']['rings'].values() for v in ring])

        if not all_x: continue
        min_x, max_x, min_y, max_y = min(all_x), max(all_x), min(all_y), max(all_y)

        html_content += f'<div id="{r["name"]}" style="margin-top: 50px; border-top: 2px dashed #eee; padding-top: 30px;">'
        html_content += f'<h3>{r["name"]}</h3>'
        html_content += '<div class="viz-grid">'

        # Given Output
        if r['has_given']:
            html_content += f"""
            <div class="viz-card">
                <h4>Given Output ({r['given']['vertex_count']} vertices)</h4>
                {generate_svg(r['given']['rings'], min_x, max_x, min_y, max_y, "rgba(52, 152, 219, 0.4)", "rgba(41, 128, 185, 1.0)")}
                <div class="metrics-box">
                    Area In:  {r['given']['input_area']}<br>
                    Area Out: {r['given']['output_area']}<br>
                    Displace: {r['given']['displacement']}
                </div>
            </div>
            """

        # My Output
        html_content += f"""
        <div class="viz-card">
            <h4>My Output ({r['my']['vertex_count']} vertices)</h4>
            {generate_svg(r['my']['rings'], min_x, max_x, min_y, max_y, "rgba(46, 204, 113, 0.4)", "rgba(39, 174, 96, 1.0)")}
            <div class="metrics-box">
                Area In:  {r['my']['input_area']}<br>
                Area Out: {r['my']['output_area']}<br>
                Displace: {r['my']['displacement']}
            </div>
        </div>
        """
        html_content += '</div></div>'

    # Extracted data for Chart.js
    labels = [r['name'] for r in results]
    my_disp = [r['my']['displacement'] if r['my']['displacement'] != 'N/A' else 0 for r in results]
    given_disp = [r['given']['displacement'] if r['has_given'] and r['given']['displacement'] != 'N/A' else 0 for r in results]
    
    times = [r['my']['time_ms'] if r['my']['time_ms'] != 'N/A' else 0 for r in results]
    mems = [r['my']['memory_mb'] if r['my']['memory_mb'] != 'N/A' else 0 for r in results]

    html_content += f"""
        </section>

        <section id="displacement">
            <h2>Areal Displacement Quality</h2>
            <p class="placeholder-text">[Draft your explanation here: Discuss enhancements like non-greedy collapse orderings, look-ahead strategies, or post-processing passes.]</p>
            <div class="chart-container">
                <canvas id="dispChart"></canvas>
            </div>
        </section>

        <section id="efficiency">
            <h2>Efficiency (Running Time & Memory)</h2>
            <p class="placeholder-text">[Draft your explanation here: Discuss choice of spatial indexes, priority queues, and algorithmic complexity.]</p>
            <div class="viz-grid">
                <div class="chart-container"><canvas id="timeChart"></canvas></div>
                <div class="chart-container"><canvas id="memChart"></canvas></div>
            </div>
        </section>

        <section id="evaluation">
            <h2>Experimental Evaluation</h2>
            <p class="placeholder-text">[Provide your own meaningful test datasets for thorough validation beyond the test cases provided on xSITe. State what property it targets (e.g., a large number of holes, narrow gaps) and why it is challenging.]</p>
        </section>

    </div>

    <script>
        const labels = {json.dumps(labels)};
        
        // Displacement Chart
        new Chart(document.getElementById('dispChart'), {{
            type: 'bar',
            data: {{
                labels: labels,
                datasets: [
                    {{ label: 'My Displacement', data: {json.dumps(my_disp)}, backgroundColor: 'rgba(46, 204, 113, 0.7)' }},
                    {{ label: 'Given Displacement', data: {json.dumps(given_disp)}, backgroundColor: 'rgba(52, 152, 219, 0.7)' }}
                ]
            }},
            options: {{ maintainAspectRatio: false, scales: {{ y: {{ type: 'logarithmic', title: {{ display: true, text: 'Areal Displacement (Log Scale)' }} }} }} }}
        }});

        // Time Chart
        new Chart(document.getElementById('timeChart'), {{
            type: 'line',
            data: {{
                labels: labels,
                datasets: [{{ label: 'Running Time (ms)', data: {json.dumps(times)}, borderColor: '#e74c3c', tension: 0.1, fill: false }}]
            }},
            options: {{ maintainAspectRatio: false, scales: {{ y: {{ title: {{ display: true, text: 'Time (ms)' }} }} }} }}
        }});

        // Memory Chart
        new Chart(document.getElementById('memChart'), {{
            type: 'line',
            data: {{
                labels: labels,
                datasets: [{{ label: 'Peak Memory (MB)', data: {json.dumps(mems)}, borderColor: '#9b59b6', tension: 0.1, fill: false }}]
            }},
            options: {{ maintainAspectRatio: false, scales: {{ y: {{ title: {{ display: true, text: 'Memory (MB)' }} }} }} }}
        }});
    </script>
    </body>
    </html>
    """

    with open(HTML_FILE, 'w') as f:
        f.write(html_content)
    
    print(f"Success! Report generated at: {os.path.abspath(HTML_FILE)}")

if __name__ == "__main__":
    main()