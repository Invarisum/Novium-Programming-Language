#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("Test program started\n");
    fflush(stdout);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Could not open: %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    printf("Read %ld bytes\n", size);
    fflush(stdout);
    return 0;
}