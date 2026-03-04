#include "advanced_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: Calculate node out-degree using CSR
static int get_out_degree(Graph* g, int u) {
    return g->csr_row_ptr[u+1] - g->csr_row_ptr[u];
}

// Algorithm: Label Propagation for Community Detection
void detect_communities(Graph* g, int* community_ids) {
    // 1. Initialize: Every node is its own community
    for (size_t i = 0; i < g->num_nodes; i++) {
        community_ids[i] = (int)i;
    }

    int max_iter = 5; // 5 iterations is usually enough for convergence on small/medium graphs

    for (int iter = 0; iter < max_iter; iter++) {
        // For every node, adopt the label of its "smallest ID" neighbor (Simplified LPA)
        // A full LPA would count frequencies, but this is faster for C and sufficient for visualization.
        for (size_t i = 0; i < g->num_nodes; i++) {
            int current_label = community_ids[i];
            int best_label = current_label;

            // Check outgoing neighbors
            for (int e = g->csr_row_ptr[i]; e < g->csr_row_ptr[i+1]; e++) {
                int neighbor = g->csr_col_idx[e];
                if (community_ids[neighbor] < best_label) {
                    best_label = community_ids[neighbor];
                }
            }
            // Check incoming neighbors (important for community structure!)
            for (int e = g->csc_col_ptr[i]; e < g->csc_col_ptr[i+1]; e++) {
                int neighbor = g->csc_row_idx[e];
                if (community_ids[neighbor] < best_label) {
                    best_label = community_ids[neighbor];
                }
            }
            community_ids[i] = best_label;
        }
    }
}

// Heuristic: Suspicious Node Detection
bool is_suspicious_node(Graph* g, int node_id) {
    int out_degree = get_out_degree(g, node_id);
    int in_degree = g->csc_col_ptr[node_id+1] - g->csc_col_ptr[node_id];

    // Rule 1: High Out-Degree Scanner (e.g., > 50 connections)
    if (out_degree > 50) return true;

    // Rule 2: "Black Hole" or "Sinkhole" (High In-Degree, Zero Out-Degree)
    if (in_degree > 50 && out_degree == 0) return true;

    // Rule 3: Ratio check (Connects to many, receives from few -> typical scanner)
    if (out_degree > 20 && in_degree == 0) return true;

    return false;
}

// Export Graph to JSON for Web Frontend
void export_graph_json(Graph* g) {
    int* communities = (int*)malloc(g->num_nodes * sizeof(int));
    detect_communities(g, communities);

    printf("{\n");
    printf("  \"nodes\": [\n");
    
    for (size_t i = 0; i < g->num_nodes; i++) {
        char ip[16];
        uint32_to_ip_str(g->ip_map[i], ip);
        
        // Calculate Total Traffic (In + Out) for Visualization Size
        uint64_t total_traffic = 0;
        for (int e = g->csr_row_ptr[i]; e < g->csr_row_ptr[i+1]; e++) 
            total_traffic += g->csr_edge_data[e].total_bytes;
        for (int e = g->csc_col_ptr[i]; e < g->csc_col_ptr[i+1]; e++) 
            total_traffic += g->csc_edge_data[e].total_bytes;

        bool suspicious = is_suspicious_node(g, i);

        // JSON Node Object
        printf("    {\"id\": %zu, \"label\": \"%s\", \"value\": %llu, \"group\": %d, \"suspicious\": %s}%s\n",
               i, 
               ip, 
               total_traffic, 
               communities[i], 
               suspicious ? "true" : "false",
               (i == g->num_nodes - 1) ? "" : ",");
    }
    printf("  ],\n");

    printf("  \"edges\": [\n");
    int edge_count = 0;
    // Iterate over all edges to print them
    bool first_edge = true;
    for (size_t i = 0; i < g->num_nodes; i++) {
        for (int e = g->csr_row_ptr[i]; e < g->csr_row_ptr[i+1]; e++) {
            int target = g->csr_col_idx[e];
            
            if (!first_edge) printf(",\n");
            printf("    {\"from\": %zu, \"to\": %d, \"value\": %llu}",
                   i, target, g->csr_edge_data[e].total_bytes);
            first_edge = false;
            edge_count++;
        }
    }
    printf("\n  ]\n");
    printf("}\n");

    free(communities);
}