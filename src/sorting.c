#include "sorting.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 内部结构体用于排序
typedef struct {
    uint32_t ip;
    uint64_t total_traffic;
    double out_ratio; // 专门用于单向流量的占比记录
} NodeStat;

// 按总流量降序排列的比较函数 (供 qsort 使用)
static int cmp_traffic_desc(const void* a, const void* b) {
    NodeStat* na = (NodeStat*)a;
    NodeStat* nb = (NodeStat*)b;
    if (na->total_traffic > nb->total_traffic) return -1;
    if (na->total_traffic < nb->total_traffic) return 1;
    return 0;
}

// 1. 计算所有节点的总流量并排序
void sort_all_traffic(Graph* g) {
    NodeStat* stats = (NodeStat*)malloc(g->num_nodes * sizeof(NodeStat));
    
    for (size_t i = 0; i < g->num_nodes; i++) {
        stats[i].ip = g->ip_map[i];
        stats[i].total_traffic = 0;
        
        // 累加出边流量
        for (int j = g->csr_row_ptr[i]; j < g->csr_row_ptr[i+1]; j++) {
            stats[i].total_traffic += g->csr_edge_data[j].total_bytes;
        }
        // 累加入边流量
        for (int j = g->csc_col_ptr[i]; j < g->csc_col_ptr[i+1]; j++) {
            stats[i].total_traffic += g->csc_edge_data[j].total_bytes;
        }
    }

    qsort(stats, g->num_nodes, sizeof(NodeStat), cmp_traffic_desc);

    // 以 JSON 数组格式输出
    printf("[\n");
    for (size_t i = 0; i < g->num_nodes; i++) {
        char ip_str[16]; uint32_to_ip_str(stats[i].ip, ip_str);
        printf("  {\"ip\": \"%s\", \"traffic\": %llu}%s\n", 
               ip_str, stats[i].total_traffic, i == g->num_nodes - 1 ? "" : ",");
    }
    printf("]\n");
    
    free(stats);
}

// 2. 筛选包含 HTTPS 的节点，按总流量排序
void sort_https_traffic(Graph* g) {
    NodeStat* stats = (NodeStat*)malloc(g->num_nodes * sizeof(NodeStat));
    size_t valid_count = 0; // 记录符合条件的节点数

    for (size_t i = 0; i < g->num_nodes; i++) {
        uint64_t total = 0;
        bool has_https = false;

        // 检查出边 (累加流量，并排查是否有HTTPS标识)
        for (int j = g->csr_row_ptr[i]; j < g->csr_row_ptr[i+1]; j++) {
            total += g->csr_edge_data[j].total_bytes;
            if (g->csr_edge_data[j].has_https) has_https = true;
        }
        // 检查入边 (累加流量，并排查是否有HTTPS标识)
        for (int j = g->csc_col_ptr[i]; j < g->csc_col_ptr[i+1]; j++) {
            total += g->csc_edge_data[j].total_bytes;
            if (g->csc_edge_data[j].has_https) has_https = true;
        }

        // 如果该节点涉及 HTTPS 通信，加入排序候选数组
        if (has_https) {
            stats[valid_count].ip = g->ip_map[i];
            stats[valid_count].total_traffic = total;
            valid_count++;
        }
    }

    qsort(stats, valid_count, sizeof(NodeStat), cmp_traffic_desc);

    // 以 JSON 数组格式输出
    printf("[\n");
    for (size_t i = 0; i < valid_count; i++) {
        char ip_str[16]; uint32_to_ip_str(stats[i].ip, ip_str);
        printf("  {\"ip\": \"%s\", \"traffic\": %llu}%s\n", 
               ip_str, stats[i].total_traffic, i == valid_count - 1 ? "" : ",");
    }
    printf("]\n");
    
    free(stats);
}

// 3. 筛选单向发出流量占比 > 80% 的节点，按总流量排序
void sort_unidirectional_traffic(Graph* g) {
    NodeStat* stats = (NodeStat*)malloc(g->num_nodes * sizeof(NodeStat));
    size_t valid_count = 0; // 记录符合条件的节点数

    for (size_t i = 0; i < g->num_nodes; i++) {
        uint64_t out_traffic = 0;
        uint64_t in_traffic = 0;

        // 计算出边流量 (发出的数据)
        for (int j = g->csr_row_ptr[i]; j < g->csr_row_ptr[i+1]; j++) {
            out_traffic += g->csr_edge_data[j].total_bytes;
        }
        // 计算入边流量 (收到的数据)
        for (int j = g->csc_col_ptr[i]; j < g->csc_col_ptr[i+1]; j++) {
            in_traffic += g->csc_edge_data[j].total_bytes;
        }

        uint64_t total = out_traffic + in_traffic;
        
        // 如果该节点有流量，计算发出占比
        if (total > 0) {
            double ratio = (double)out_traffic / (double)total;
            
            // 筛选条件：发出占比 > 0.8 (即 80%)
            if (ratio > 0.8) {
                stats[valid_count].ip = g->ip_map[i];
                stats[valid_count].total_traffic = total;
                stats[valid_count].out_ratio = ratio;
                valid_count++;
            }
        }
    }

    qsort(stats, valid_count, sizeof(NodeStat), cmp_traffic_desc);

    // 以 JSON 数组格式输出，附带占比字段 out_ratio
    printf("[\n");
    for (size_t i = 0; i < valid_count; i++) {
        char ip_str[16]; uint32_to_ip_str(stats[i].ip, ip_str);
        // 保留4位小数，例如 0.8523 代表 85.23%
        printf("  {\"ip\": \"%s\", \"traffic\": %llu, \"out_ratio\": %.4f}%s\n", 
               ip_str, stats[i].total_traffic, stats[i].out_ratio, i == valid_count - 1 ? "" : ",");
    }
    printf("]\n");
    
    free(stats);
}