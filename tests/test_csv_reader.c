#include <stdio.h>
#include <stdlib.h>
#include "../include/csv_reader.h"

int main() {
    printf("Testing CSV Reader...\n");
    SessionRaw* sessions = NULL;
    size_t count = 0;
    
    // Ensure you have a small test file at data/network_data_small.csv
    if (read_csv("../data/network_data_small.csv", &sessions, &count)) {
        printf("PASS: Read %zu sessions.\n", count);
        if (count > 0) {
            printf("Sample: %s -> %s (Proto: %d)\n", sessions[0].src_ip, sessions[0].dst_ip, sessions[0].protocol);
        }
        free(sessions);
        return 0;
    } else {
        printf("FAIL: Could not read CSV.\n");
        return 1;
    }
}