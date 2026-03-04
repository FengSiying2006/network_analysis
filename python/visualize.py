import sys
import json
import networkx as nx
from pyvis.network import Network
import webbrowser
import os

def visualize_graph(json_data, output_file="network_graph.html"):
    """
    Visualizes a subgraph or path based on JSON data from the C engine.
    Expected JSON format: 
    { "nodes": [{"id": "IP", "label": "IP", "value": size}, ...], 
      "edges": [{"from": "IP", "to": "IP", "value": traffic}, ...] }
    OR simplified star/path output.
    """
    
    # Create Pyvis network
    net = Network(height="750px", width="100%", bgcolor="#222222", font_color="white", directed=True)
    net.force_atlas_2based()
    
    try:
        data = json.loads(json_data)
        
        # Handling Star Topology Output
        if isinstance(data, list): # List of star centers
             for star in data:
                center = star.get('center')
                if center:
                    net.add_node(center, label=center, color='red', size=20)
                    for edge in star.get('edges', []):
                        net.add_node(edge, label=edge, color='#00ff1e', size=10)
                        net.add_edge(center, edge)

        # Handling Path Output
        elif 'path' in data:
            path = data['path']
            for i in range(len(path)):
                net.add_node(path[i], label=path[i], color='cyan' if i==0 or i==len(path)-1 else 'white')
                if i < len(path) - 1:
                    net.add_edge(path[i], path[i+1], color='yellow', width=2)

        # Handling Subgraph (Custom JSON structure from C 'SUBGRAPH' cmd)
        elif 'nodes' in data and 'edges' in data:
            for node in data['nodes']:
                net.add_node(node['id'], label=node['id'], value=node.get('traffic', 1))
            for edge in data['edges']:
                net.add_edge(edge['from'], edge['to'], value=edge.get('traffic', 1))
        
        else:
            print("Unknown JSON format for visualization.")
            return

        net.show_buttons(filter_=['physics'])
        net.save_graph(output_file)
        print(f"Graph saved to {output_file}")
        
        # Automatically open in browser
        webbrowser.open('file://' + os.path.realpath(output_file))

    except json.JSONDecodeError:
        print("Invalid JSON data provided.")

if __name__ == "__main__":
    # Example usage for testing: passing a JSON string as an argument
    if len(sys.argv) > 1:
        visualize_graph(sys.argv[1])