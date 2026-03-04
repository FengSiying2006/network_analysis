#include "path.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stdbool.h>

// 二分查找 IP 获取节点 ID (O(log V))
static int get_node_id(Graph* g, uint32_t ip) {
    uint32_t* res = (uint32_t*)bsearch(&ip, g->ip_map, g->num_nodes, sizeof(uint32_t), cmp_uint32_search);
    return res ? (int)(res - g->ip_map) : -1;
}

// 打印路径回溯 (用于 JSON 输出)
static void print_path_json(Graph* g, int* parent, int target_id) {
    // 动态分配，防止路径长度超过 1024 导致栈溢出
    int* path = (int*)malloc(g->num_nodes * sizeof(int));
    int depth = 0;
    int curr = target_id;
    
    // 从目的节点往前回溯到源节点
    while (curr != -1) { 
        path[depth++] = curr; 
        curr = parent[curr]; 
    }
    
    printf("\"path\":[");
    // 倒序打印，因为回溯是反向的
    for (int i = depth - 1; i >= 0; i--) {
        char ip_str[16]; 
        uint32_to_ip_str(g->ip_map[path[i]], ip_str);
        printf("\"%s\"%s", ip_str, i == 0 ? "" : ", ");
    }
    printf("]");
    
    free(path);
}

// Dijkstra：查找拥塞程度最小路径
void find_path_dijkstra(Graph* g, const char* src_ip, const char* dst_ip) {
    int src_id = get_node_id(g, ip_str_to_uint32(src_ip));
    int dst_id = get_node_id(g, ip_str_to_uint32(dst_ip));

    if (src_id == -1 || dst_id == -1) {
        printf("{\"error\": \"IP not found in graph\"}\n"); 
        return;
    }

    double* dist = (double*)malloc(g->num_nodes * sizeof(double));
    int* parent = (int*)malloc(g->num_nodes * sizeof(int));
    bool* visited = (bool*)calloc(g->num_nodes, sizeof(bool));

    for (size_t i = 0; i < g->num_nodes; i++) { 
        dist[i] = DBL_MAX; 
        parent[i] = -1; 
    }
    dist[src_id] = 0.0;

    // 简单 Dijkstra O(V^2) - 适用于万级及以下节点网络
    for (size_t i = 0; i < g->num_nodes; i++) {
        int u = -1; double min_dist = DBL_MAX;
        
        // 寻找当前未访问且距离最小的节点
        for (size_t j = 0; j < g->num_nodes; j++) {
            if (!visited[j] && dist[j] < min_dist) { 
                min_dist = dist[j]; 
                u = j; 
            }
        }
        
        // 若找不到可达节点，或已找到目标节点，提前跳出
        if (u == -1 || u == dst_id) break;
        visited[u] = true;

        // 利用 CSR 极速遍历出边弛豫
        for (int e = g->csr_row_ptr[u]; e < g->csr_row_ptr[u+1]; e++) {
            int v = g->csr_col_idx[e];
            double dur = g->csr_edge_data[e].total_duration;
            if (dur < 0.001) dur = 0.001; // 防除零，设定最小微秒级阈值
            
            // 计算边权重：拥塞程度 = 流量 / 时间
            double congestion = g->csr_edge_data[e].total_bytes / dur;
            
            // 弛豫更新最小拥塞累计值
            if (dist[u] + congestion < dist[v]) {
                dist[v] = dist[u] + congestion;
                parent[v] = u;
            }
        }
    }

    // 格式化输出为 JSON
    printf("{");
    if (dist[dst_id] == DBL_MAX) {
        printf("\"error\": \"No congestion path found\"");
    } else {
        printf("\"congestion\": %.2f, ", dist[dst_id]);
        print_path_json(g, parent, dst_id);
    }
    printf("}\n");

    free(dist); 
    free(parent); 
    free(visited);
}

// BFS：查找跳数最小路径 (最少中转节点)
void find_path_bfs(Graph* g, const char* src_ip, const char* dst_ip) {
    int src_id = get_node_id(g, ip_str_to_uint32(src_ip));
    int dst_id = get_node_id(g, ip_str_to_uint32(dst_ip));

    if (src_id == -1 || dst_id == -1) {
        printf("{\"error\": \"IP not found in graph\"}\n"); 
        return;
    }

    // 初始化 BFS 队列
    int* queue = (int*)malloc(g->num_nodes * sizeof(int));
    int head = 0, tail = 0;

    int* parent = (int*)malloc(g->num_nodes * sizeof(int));
    int* hops = (int*)malloc(g->num_nodes * sizeof(int));
    bool* visited = (bool*)calloc(g->num_nodes, sizeof(bool));

    for (size_t i = 0; i < g->num_nodes; i++) { 
        parent[i] = -1; 
        hops[i] = -1; 
    }

    // 源节点入队
    queue[tail++] = src_id;
    visited[src_id] = true;
    hops[src_id] = 0;

    bool found = false;

    // 队列不为空时循环
    while (head < tail) {
        int u = queue[head++]; // 出队

        // 若找到目的节点，直接跳出 (BFS保证首次到达即为最小跳数)
        if (u == dst_id) {
            found = true;
            break;
        }

        // 利用 CSR 遍历当前节点的所有出边邻居
        for (int e = g->csr_row_ptr[u]; e < g->csr_row_ptr[u+1]; e++) {
            int v = g->csr_col_idx[e];
            
            // 若邻居未被访问过，则记录状态并入队
            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                hops[v] = hops[u] + 1;
                queue[tail++] = v;
            }
        }
    }

    // 格式化输出为 JSON
    printf("{");
    if (!found) {
        printf("\"error\": \"No hop path found\"");
    } else {
        printf("\"hops\": %d, ", hops[dst_id]);
        print_path_json(g, parent, dst_id);
    }
    printf("}\n");

    free(queue);
    free(parent);
    free(hops);
    free(visited);
}