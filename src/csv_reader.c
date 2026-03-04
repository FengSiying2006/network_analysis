#include "csv_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 1024
#define MAX_LINE_LEN 512

bool read_csv(const char* filepath, SessionRaw** sessions, size_t* count) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filepath);
        return false;
    }

    size_t capacity = INITIAL_CAPACITY;
    *sessions = (SessionRaw*)malloc(capacity * sizeof(SessionRaw));
    if (!*sessions) return false;
    *count = 0;

    char line[MAX_LINE_LEN];
    // 读取表头
    if (!fgets(line, MAX_LINE_LEN, file)) {
        fclose(file);
        return false;
    }

    while (fgets(line, MAX_LINE_LEN, file)) {
        // 倍增扩容
        if (*count >= capacity) {
            capacity *= 2;
            SessionRaw* temp = (SessionRaw*)realloc(*sessions, capacity * sizeof(SessionRaw));
            if (!temp) {
                fprintf(stderr, "Error: Memory allocation failed during CSV reading.\n");
                fclose(file);
                return false;
            }
            *sessions = temp;
        }

        SessionRaw* s = &(*sessions)[*count];
        // 默认值初始化 (防 ICMP 无端口)
        s->protocol = -1; s->src_port = 0; s->dst_port = 0;
        
        // 简单 CSV 解析 (假设格式规范，逗号分隔)
        char* token = strtok(line, ",");
        if(token) strncpy(s->src_ip, token, 15); s->src_ip[15]='\0';
        token = strtok(NULL, ","); if(token) strncpy(s->dst_ip, token, 15); s->dst_ip[15]='\0';
        token = strtok(NULL, ","); if(token) s->protocol = atoi(token);
        token = strtok(NULL, ","); if(token) s->src_port = atoi(token);
        token = strtok(NULL, ","); if(token) s->dst_port = atoi(token);
        token = strtok(NULL, ","); if(token) s->data_size = strtoull(token, NULL, 10);
        token = strtok(NULL, "\r\n"); if(token) s->duration = atof(token);

        (*count)++;
    }

    fclose(file);
    return true;
}