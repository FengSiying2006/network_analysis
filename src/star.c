#include "star.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 辅助函数：精确计算某个节点的无向度数 (即唯一的通信邻居数)
// 因为可能存在 A->B 和 B->A 的双向通信，必须去重
static int get_unique_degree(Graph* g, int u) {
    int count = 0;
    int out_start = g->csr_row_ptr[u];
    int out_end   = g->csr_row_ptr[u+1];
    int in_start  = g->csc_col_ptr[u];
    int in_end    = g->csc_col_ptr[u+1];
    
    // 1. 出边邻居本身在 graph_builder 中已经去重过，直接计入
    count += (out_end - out_start);
    
    // 2. 检查入边邻居。如果入边邻居已经出现在出边邻居中，则不重复计数
    for (int i = in_start; i < in_end; i++) {
        int v = g->csc_row_idx[i];
        bool is_duplicate = false;
        for (int j = out_start; j < out_end; j++) {
            if (g->csr_col_idx[j] == v) { 
                is_duplicate = true; 
                break; 
            }
        }
        if (!is_duplicate) count++;
    }
    return count;
}

// 辅助函数：获取节点的所有唯一邻居放入数组
static void get_unique_neighbors(Graph* g, int u, int* neighbors, int* count) {
    *count = 0;
    // 添加出边邻居
    for (int i = g->csr_row_ptr[u]; i < g->csr_row_ptr[u+1]; i++) {
        neighbors[(*count)++] = g->csr_col_idx[i];
    }
    // 添加入边邻居并去重
    for (int i = g->csc_col_ptr[u]; i < g->csc_col_ptr[u+1]; i++) {
        int v = g->csc_row_idx[i];
        bool is_duplicate = false;
        for (int j = 0; j < *count; j++) {
            if (neighbors[j] == v) { is_duplicate = true; break; }
        }
        if (!is_duplicate) neighbors[(*count)++] = v;
    }
}

void detect_star_topology(Graph* g) {
    bool found_any = false;
    
    // 为了防止栈溢出，动态分配内存存储邻居
    int* neighbors = (int*)malloc(g->num_nodes * sizeof(int));
    int* valid_edges = (int*)malloc(g->num_nodes * sizeof(int));
    
    printf("[\n"); // 输出 JSON 数组开始

    // 遍历所有节点作为候选中心
    for (size_t i = 0; i < g->num_nodes; i++) {
        int center_degree = get_unique_degree(g, i);
        
        // 星型结构定义：中心节点至少有 20 个邻居
        if (center_degree >= 20) {
            int neighbor_count = 0;
            get_unique_neighbors(g, i, neighbors, &neighbor_count);
            
            int valid_count = 0;
            // 验证这些邻居是否为“边缘节点”（只与中心节点相连，即度数为1）
            for (int j = 0; j < neighbor_count; j++) {
                int v = neighbors[j];
                if (get_unique_degree(g, v) == 1) {
                    valid_edges[valid_count++] = v;
                }
            }

            // 满足条件的边缘节点数量 >= 20，确认为星型结构
            if (valid_count >= 20) {
                if (found_any) printf(",\n");
                found_any = true;
                
                char center_ip[16];
                uint32_to_ip_str(g->ip_map[i], center_ip);
                
                printf("  {\n    \"center\": \"%s\",\n    \"edge_count\": %d,\n    \"edges\":[", center_ip, valid_count);
                
                for (int k = 0; k < valid_count; k++) {
                    char edge_ip[16];
                    uint32_to_ip_str(g->ip_map[valid_edges[k]], edge_ip);
                    printf("\"%s\"%s", edge_ip, (k == valid_count - 1) ? "" : ", ");
                }
                printf("]\n  }");
            }
        }
    }
    
    printf("\n]\n"); // 输出 JSON 数组结束

    free(neighbors);
    free(valid_edges);
}