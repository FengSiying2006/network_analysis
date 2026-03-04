#include "common.h"
#include <stdio.h>

// 将点分十进制字符串转换为32位无符号整数 (大端序处理，保证比较时的数学连续性)
uint32_t ip_str_to_uint32(const char* ip) {
    uint32_t a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        return (a << 24) | (b << 16) | (c << 8) | d;
    }
    return 0;
}

// 将32位无符号整数转换回点分十进制字符串
void uint32_to_ip_str(uint32_t ip, char* buf) {
    sprintf(buf, "%u.%u.%u.%u", 
            (ip >> 24) & 0xFF, 
            (ip >> 16) & 0xFF, 
            (ip >> 8) & 0xFF, 
            ip & 0xFF);
}