#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

// 原始会话记录结构 (用于从CSV读取)
typedef struct {
    char src_ip[16];
    char dst_ip[16];
    int protocol;
    int src_port;
    int dst_port;
    uint64_t data_size;
    double duration;
} SessionRaw;

// 聚合后的边属性结构 (存储在 CSR/CSC 中)
typedef struct {
    uint64_t total_bytes;
    double total_duration;
    bool has_https; // 是否包含 HTTPS 流量 (端口 443)
} EdgeData;

// 工具函数声明
uint32_t ip_str_to_uint32(const char* ip);
void uint32_to_ip_str(uint32_t ip, char* buf);
int cmp_uint32_search(const void* a, const void* b);
#endif // COMMON_H