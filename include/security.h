#ifndef SECURITY_H
#define SECURITY_H
#include "graph.h"
void security_filter(Graph* g, const char* addr1, const char* addr2, const char* addr3, bool is_allow);
#endif