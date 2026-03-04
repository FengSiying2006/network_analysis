#include "graph_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Helper to check if a node is already in the list
static bool is_in_array(int* arr, int count, int val) {
    for (int i=0; i<count; i++) if (arr[i] == val) return true;
    return false;
}

// Helper for IP string to ID (reused)
static int get_id(Graph* g, const char* ip) {
    uint32_t ip_int = ip_str_to_uint32(ip);
    uint32_t* res = (uint32_t*)bsearch(&ip_int, g->ip_map, g->num_nodes, sizeof(uint32_t), cmp_uint32_search); // Assume cmp_uint32_search is available
    return res ? (int)(res - g->ip_map) : -1;
}

void extract_subgraph(Graph* g, const char* target_ip, int max_hops) {
    int start_node = get_id(g, target_ip);
    if (start_node == -1) {
        printf("{\"error\": \"IP not found\"}\n");
        return;
    }

    // BFS Queue
    int* queue = (int*)malloc(g->num_nodes * sizeof(int));
    int head = 0, tail = 0;
    
    // Visited set (using array for simplicity/speed)
    bool* visited = (bool*)calloc(g->num_nodes, sizeof(bool));
    int* nodes_in_subgraph = (int*)malloc(g->num_nodes * sizeof(int));
    int node_count = 0;

    // Start BFS
    queue[tail++] = start_node;
    visited[start_node] = true;
    nodes_in_subgraph[node_count++] = start_node;
    
    int current_hop_nodes = 1;
    int next_hop_nodes = 0;
    int hops = 0;

    while (head < tail && hops < max_hops) {
        int u = queue[head++];
        current_hop_nodes--;

        // Explore Outgoing Edges (CSR)
        for (int i = g->csr_row_ptr[u]; i < g->csr_row_ptr[u+1]; i++) {
            int v = g->csr_col_idx[i];
            if (!visited[v]) {
                visited[v] = true;
                queue[tail++] = v;
                nodes_in_subgraph[node_count++] = v;
                next_hop_nodes++;
            }
        }
        
        // Explore Incoming Edges (CSC) - Important for full connectivity!
        for (int i = g->csc_col_ptr[u]; i < g->csc_col_ptr[u+1]; i++) {
            int v = g->csc_row_idx[i];
            if (!visited[v]) {
                visited[v] = true;
                queue[tail++] = v;
                nodes_in_subgraph[node_count++] = v;
                next_hop_nodes++;
            }
        }

        if (current_hop_nodes == 0) {
            current_hop_nodes = next_hop_nodes;
            next_hop_nodes = 0;
            hops++;
        }
    }

    // Output JSON
    printf("{\n  \"nodes\": [");
    for (int i=0; i<node_count; i++) {
        char ip[16]; uint32_to_ip_str(g->ip_map[nodes_in_subgraph[i]], ip);
        printf("{\"id\": \"%s\", \"label\": \"%s\"}%s", ip, ip, (i==node_count-1)?"":", ");
    }
    printf("],\n  \"edges\": [");
    
    bool first_edge = true;
    // Collect edges only between collected nodes
    for (int i=0; i<node_count; i++) {
        int u = nodes_in_subgraph[i];
        
        // Outgoing edges
        for (int j = g->csr_row_ptr[u]; j < g->csr_row_ptr[u+1]; j++) {
            int v = g->csr_col_idx[j];
            // Only output if v is also in our subgraph (it should be, but good to check)
            if (visited[v]) {
                if (!first_edge) printf(", ");
                char src[16], dst[16];
                uint32_to_ip_str(g->ip_map[u], src);
                uint32_to_ip_str(g->ip_map[v], dst);
                printf("{\"from\": \"%s\", \"to\": \"%s\", \"value\": %llu}", 
                       src, dst, g->csr_edge_data[j].total_bytes);
                first_edge = false;
            }
        }
    }
    printf("]\n}\n");

    free(queue); free(visited); free(nodes_in_subgraph);
}