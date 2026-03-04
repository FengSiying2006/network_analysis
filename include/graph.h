#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"
#include <stddef.h>

// 图结构体定义
typedef struct {
    size_t num_nodes;       // 节点数 V
    size_t num_edges;       // 去重后的有向边数 E
    
    uint32_t* ip_map;       // 节点ID到IP整数的映射 (长度V)

    // 正向图 CSR (用于出度、寻找目的节点)
    int* csr_row_ptr;       // 长度 V + 1
    int* csr_col_idx;       // 长度 E
    EdgeData* csr_edge_data;// 长度 E

    // 逆向图 CSC (用于入度、寻找来源节点)
    int* csc_col_ptr;       // 长度 V + 1
    int* csc_row_idx;       // 长度 E
    EdgeData* csc_edge_data;// 长度 E
} Graph;

// 构建图结构
Graph* build_graph(SessionRaw* sessions, size_t num_sessions);

// 释放图内存
void free_graph(Graph* g);

#endif