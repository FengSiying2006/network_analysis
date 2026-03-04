#include "graph.h"
#include <stdlib.h>
#include <stdio.h>

// 用于构建图的内部结构：缓冲边
typedef struct {
    int src_id;
    int dst_id;
    uint64_t bytes;
    double duration;
    bool has_https;
} EdgeBuffer;

// IP 排序比较函数 (用于去重)
static int cmp_uint32(const void* a, const void* b) {
    if (*(uint32_t*)a < *(uint32_t*)b) return -1;
    if (*(uint32_t*)a > *(uint32_t*)b) return 1;
    return 0;
}

// 边排序比较函数 (先源ID，后目的ID)
static int cmp_edge_src_dst(const void* a, const void* b) {
    EdgeBuffer* ea = (EdgeBuffer*)a;
    EdgeBuffer* eb = (EdgeBuffer*)b;
    if (ea->src_id != eb->src_id) return ea->src_id - eb->src_id;
    return ea->dst_id - eb->dst_id;
}

// 边排序比较函数 (先目的ID，后源ID，用于CSC)
static int cmp_edge_dst_src(const void* a, const void* b) {
    EdgeBuffer* ea = (EdgeBuffer*)a;
    EdgeBuffer* eb = (EdgeBuffer*)b;
    if (ea->dst_id != eb->dst_id) return ea->dst_id - eb->dst_id;
    return ea->src_id - eb->src_id;
}

// 二分查找获取 IP 的分配 ID
static int get_node_id(uint32_t* ip_map, size_t v_count, uint32_t ip) {
    uint32_t* res = (uint32_t*)bsearch(&ip, ip_map, v_count, sizeof(uint32_t), cmp_uint32);
    if (res) return (int)(res - ip_map);
    return -1;
}

Graph* build_graph(SessionRaw* sessions, size_t num_sessions) {
    Graph* g = (Graph*)calloc(1, sizeof(Graph));

    // 1. 提取所有 IP 并去重以分配 ID
    uint32_t* temp_ips = (uint32_t*)malloc(num_sessions * 2 * sizeof(uint32_t));
    for (size_t i = 0; i < num_sessions; i++) {
        temp_ips[i*2] = ip_str_to_uint32(sessions[i].src_ip);
        temp_ips[i*2+1] = ip_str_to_uint32(sessions[i].dst_ip);
    }
    qsort(temp_ips, num_sessions * 2, sizeof(uint32_t), cmp_uint32);
    
    // 去重
    size_t v_count = 0;
    for (size_t i = 0; i < num_sessions * 2; i++) {
        if (i == 0 || temp_ips[i] != temp_ips[i-1]) {
            temp_ips[v_count++] = temp_ips[i];
        }
    }
    g->num_nodes = v_count;
    g->ip_map = temp_ips; // 复用数组

    // 2. 将会话转化为 EdgeBuffer
    EdgeBuffer* edges = (EdgeBuffer*)malloc(num_sessions * sizeof(EdgeBuffer));
    for (size_t i = 0; i < num_sessions; i++) {
        edges[i].src_id = get_node_id(g->ip_map, v_count, ip_str_to_uint32(sessions[i].src_ip));
        edges[i].dst_id = get_node_id(g->ip_map, v_count, ip_str_to_uint32(sessions[i].dst_ip));
        edges[i].bytes = sessions[i].data_size;
        edges[i].duration = sessions[i].duration;
        edges[i].has_https = (sessions[i].protocol == 6 && (sessions[i].src_port == 443 || sessions[i].dst_port == 443));
    }

    // 3. 排序并聚合边 (构建 CSR 需要)
    qsort(edges, num_sessions, sizeof(EdgeBuffer), cmp_edge_src_dst);
    EdgeBuffer* unique_edges = (EdgeBuffer*)malloc(num_sessions * sizeof(EdgeBuffer));
    size_t e_count = 0;
    
    for (size_t i = 0; i < num_sessions; i++) {
        if (i == 0 || edges[i].src_id != unique_edges[e_count-1].src_id || edges[i].dst_id != unique_edges[e_count-1].dst_id) {
            unique_edges[e_count++] = edges[i];
        } else {
            // 合并相同源目的的会话
            unique_edges[e_count-1].bytes += edges[i].bytes;
            unique_edges[e_count-1].duration += edges[i].duration;
            unique_edges[e_count-1].has_https |= edges[i].has_https;
        }
    }
    g->num_edges = e_count;

    // 4. 构建正向 CSR
    g->csr_row_ptr = (int*)calloc(v_count + 1, sizeof(int));
    g->csr_col_idx = (int*)malloc(e_count * sizeof(int));
    g->csr_edge_data = (EdgeData*)malloc(e_count * sizeof(EdgeData));

    // 计算度数
    for (size_t i = 0; i < e_count; i++) {
        g->csr_row_ptr[unique_edges[i].src_id + 1]++;
    }
    // 前缀和
    for (size_t i = 0; i < v_count; i++) {
        g->csr_row_ptr[i + 1] += g->csr_row_ptr[i];
    }
    // 填充数据
    for (size_t i = 0; i < e_count; i++) {
        g->csr_col_idx[i] = unique_edges[i].dst_id;
        g->csr_edge_data[i].total_bytes = unique_edges[i].bytes;
        g->csr_edge_data[i].total_duration = unique_edges[i].duration;
        g->csr_edge_data[i].has_https = unique_edges[i].has_https;
    }

    // 5. 构建逆向 CSC
    qsort(unique_edges, e_count, sizeof(EdgeBuffer), cmp_edge_dst_src);
    g->csc_col_ptr = (int*)calloc(v_count + 1, sizeof(int));
    g->csc_row_idx = (int*)malloc(e_count * sizeof(int));
    g->csc_edge_data = (EdgeData*)malloc(e_count * sizeof(EdgeData));

    for (size_t i = 0; i < e_count; i++) {
        g->csc_col_ptr[unique_edges[i].dst_id + 1]++;
    }
    for (size_t i = 0; i < v_count; i++) {
        g->csc_col_ptr[i + 1] += g->csc_col_ptr[i];
    }
    for (size_t i = 0; i < e_count; i++) {
        g->csc_row_idx[i] = unique_edges[i].src_id;
        g->csc_edge_data[i].total_bytes = unique_edges[i].bytes;
        g->csc_edge_data[i].total_duration = unique_edges[i].duration;
        g->csc_edge_data[i].has_https = unique_edges[i].has_https;
    }

    free(edges);
    free(unique_edges);
    return g;
}

void free_graph(Graph* g) {
    if (!g) return;
    free(g->ip_map);
    free(g->csr_row_ptr); free(g->csr_col_idx); free(g->csr_edge_data);
    free(g->csc_col_ptr); free(g->csc_row_idx); free(g->csc_edge_data);
    free(g);
}