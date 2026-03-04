// Extract a subgraph connected to a specific node (IP)
// Returns JSON string: { "nodes": [...], "edges": [...] }
#ifndef GRAPH_BUILDER_H
#define GRAPH_BUILDER_H
#include "graph.h"
#include "path.h"
#include "security.h"
#include "star.h"
#include "sorting.h"
void extract_subgraph(Graph* g, const char* target_ip, int max_hops);
#endif