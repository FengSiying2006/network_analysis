#include "common.h"
#include "graph.h"
#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// 二分查找获取节点 ID
static int get_node_id(Graph* g, uint32_t ip) {
    uint32_t* res = (uint32_t*)bsearch(&ip, g->ip_map, g->num_nodes, sizeof(uint32_t), cmp_uint32_search);
    return res ? (int)(res - g->ip_map) : -1;
}

// 检查对端 IP 是否违反规则
static bool is_violation(uint32_t target_ip, uint32_t range_start, uint32_t range_end, bool is_allow_mode) {
    bool in_range = (target_ip >= range_start && target_ip <= range_end);
    
    if (is_allow_mode) {
        // Allow 模式：只允许范围内的 IP。如果在范围外，则违规
        return !in_range;
    } else {
        // Deny 模式：禁止范围内的 IP。如果在范围内，则违规
        return in_range;
    }
}

void security_filter(Graph* g, const char* addr1, const char* addr2, const char* addr3, bool is_allow) {
    uint32_t a1 = ip_str_to_uint32(addr1);
    uint32_t start = ip_str_to_uint32(addr2);
    uint32_t end = ip_str_to_uint32(addr3);
    
    // 确保 start <= end
    if (start > end) { 
        uint32_t temp = start; 
        start = end; 
        end = temp; 
    }

    int id1 = get_node_id(g, a1);
    
    printf("[\n"); // 输出 JSON 数组开始

    if (id1 == -1) {
        // 目标 IP 不在图谱中，自然没有违规会话
        printf("]\n");
        return;
    }

    bool has_printed = false;
    char src_ip_str[16];
    char dst_ip_str[16];

    // 1. 检查作为源节点发出的会话 (出边)
    uint32_to_ip_str(a1, src_ip_str); // addr1 是源
    for (int i = g->csr_row_ptr[id1]; i < g->csr_row_ptr[id1+1]; i++) {
        uint32_t target_ip = g->ip_map[g->csr_col_idx[i]];
        
        if (is_violation(target_ip, start, end, is_allow)) {
            if (has_printed) printf(",\n");
            
            uint32_to_ip_str(target_ip, dst_ip_str);
            printf("  {\"src\": \"%s\", \"dst\": \"%s\", \"traffic\": %llu, \"duration\": %.2f}", 
                   src_ip_str, dst_ip_str, 
                   g->csr_edge_data[i].total_bytes, 
                   g->csr_edge_data[i].total_duration);
            has_printed = true;
        }
    }

    // 2. 检查作为目的节点接收的会话 (入边)
    uint32_to_ip_str(a1, dst_ip_str); // addr1 是目的
    for (int i = g->csc_col_ptr[id1]; i < g->csc_col_ptr[id1+1]; i++) {
        uint32_t target_ip = g->ip_map[g->csc_row_idx[i]];
        
        if (is_violation(target_ip, start, end, is_allow)) {
            if (has_printed) printf(",\n");
            
            uint32_to_ip_str(target_ip, src_ip_str);
            printf("  {\"src\": \"%s\", \"dst\": \"%s\", \"traffic\": %llu, \"duration\": %.2f}", 
                   src_ip_str, dst_ip_str, 
                   g->csc_edge_data[i].total_bytes, 
                   g->csc_edge_data[i].total_duration);
            has_printed = true;
        }
    }

    printf("\n]\n"); // 输出 JSON 数组结束
}