#ifndef PATH_H
#define PATH_H

#include "graph.h"

// BFS：查找跳数最小路径
void find_path_bfs(Graph* g, const char* src_ip, const char* dst_ip);

// Dijkstra：查找拥塞程度最小路径
void find_path_dijkstra(Graph* g, const char* src_ip, const char* dst_ip);

#endif