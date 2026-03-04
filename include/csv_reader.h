#ifndef CSV_READER_H
#define CSV_READER_H

#include "common.h"
#include <stddef.h>

// 从 CSV 文件读取会话记录
// 成功返回 true，失败返回 false
bool read_csv(const char* filepath, SessionRaw** sessions, size_t* count);

#endif