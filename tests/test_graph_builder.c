#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/graph.h"

int main() {
    printf("Testing Graph Builder...\n");
    
    // Mock Data
    SessionRaw data[3];
    // 1 -> 2
    sprintf(data[0].src_ip, "1.1.1.1"); sprintf(data[0].dst_ip, "2.2.2.2");
    data[0].data_size = 100; data[0].duration = 1.0; data[0].protocol = 6; data[0].src_port=0; data[0].dst_port=0;
    // 1 -> 2 (should merge)
    sprintf(data[1].src_ip, "1.1.1.1"); sprintf(data[1].dst_ip, "2.2.2.2");
    data[1].data_size = 200; data[1].duration = 2.0; data[1].protocol = 6; data[1].src_port=0; data[1].dst_port=0;
    // 2 -> 3
    sprintf(data[2].src_ip, "2.2.2.2"); sprintf(data[2].dst_ip, "3.3.3.3");
    data[2].data_size = 50; data[2].duration = 0.5; data[2].protocol = 17; data[2].src_port=0; data[2].dst_port=0;

    Graph* g = build_graph(data, 3);
    
    // Verify Nodes (1.1.1.1, 2.2.2.2, 3.3.3.3) -> 3 nodes
    if (g->num_nodes != 3) { printf("FAIL: Expected 3 nodes, got %zu\n", g->num_nodes); return 1; }
    
    // Verify Edges (1->2, 2->3) -> 2 edges
    if (g->num_edges != 2) { printf("FAIL: Expected 2 edges, got %zu\n", g->num_edges); return 1; }
    
    printf("PASS: Graph built successfully with correct node/edge counts.\n");
    free_graph(g);
    return 0;
}