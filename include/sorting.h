 #ifndef SORTING_H
#define SORTING_H

#include "graph.h"

// 1. 对所有节点按总流量降序排序
void sort_all_traffic(Graph* g);

// 2. 筛选包含HTTPS的节点，按总流量排序
void sort_https_traffic(Graph* g);

// 3. 筛选单向发出流量占比 > 80% 的节点，按总流量排序
void sort_unidirectional_traffic(Graph* g);

#endif