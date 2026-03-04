#ifndef ADVANCED_ANALYSIS_H
#define ADVANCED_ANALYSIS_H

#include "graph_builder.h"
#include <stdbool.h>

// --- New Feature: Community Detection ---
// Populates the `community_ids` array (size = num_nodes) with a group ID for each node.
void detect_communities(Graph* g, int* community_ids);

// --- New Feature: Botnet/Suspicious Node Detection ---
// Returns true if a node shows suspicious behavior (high out-degree, short flows, etc.)
bool is_suspicious_node(Graph* g, int node_id);

// --- Web Integration: Export Full Graph to JSON ---
// Prints the entire graph structure with metadata (community, suspicious status, traffic) to stdout.
void export_graph_json(Graph* g);

#endif