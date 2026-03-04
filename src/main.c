#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv_reader.h"
#include "graph.h"
#include "sorting.h"
#include "path.h"
#include "star.h"
#include "security.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("{\"error\": \"Missing arguments. Usage: program.exe <csv_path> <cmd> [...]\"}\n");
        return 1;
    }

    const char* csv_path = argv[1];
    const char* cmd = argv[2];

    SessionRaw* sessions = NULL;
    size_t count = 0;
    if (!read_csv(csv_path, &sessions, &count)) {
        printf("{\"error\": \"Failed to read CSV file\"}\n");
        return 1;
    }

    Graph* g = build_graph(sessions, count);
    free(sessions);

    // 路由分发器
    if (strcmp(cmd, "SORT_ALL") == 0) {
        sort_all_traffic(g);
    } 
    else if (strcmp(cmd, "SORT_HTTPS") == 0) {
        sort_https_traffic(g);
    } 
    else if (strcmp(cmd, "SORT_UNI") == 0) {
        sort_unidirectional_traffic(g);
    } 
    else if (strcmp(cmd, "PATH_HOP") == 0 && argc == 5) {
        find_path_bfs(g, argv[3], argv[4]);
    } 
    else if (strcmp(cmd, "PATH_CONGEST") == 0 && argc == 5) {
        find_path_dijkstra(g, argv[3], argv[4]);
    } 
    else if (strcmp(cmd, "STAR") == 0) {
        detect_star_topology(g);
    } 
    else if (strcmp(cmd, "RULE") == 0 && argc == 7) {
        bool is_allow = (strcmp(argv[6], "allow") == 0);
        security_filter(g, argv[3], argv[4], argv[5], is_allow);
    } 
    else if (strcmp(cmd, "SUBGRAPH") == 0 && argc == 5) {
        extract_subgraph(g, argv[3], atoi(argv[4])); 
    }
    else {
        printf("{\"error\": \"Unknown command\"}\n");
    }

    free_graph(g);
    return 0;
}