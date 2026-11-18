#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;

    if (strcmp(argv[1], "null") == 0) {
        int value = 0;
        int *target = &value;
        *target = 42;
        printf("value: %d\n", value);
    } else if (strcmp(argv[1], "overflow") == 0) {
        struct { int data[4]; int limit; } block = {{0}, 100};
        for (size_t i = 0; i < sizeof block.data / sizeof block.data[0]; i++)
            block.data[i] = 7;
        printf("last: %d, limit: %d\n", block.data[3], block.limit);
    } else if (strcmp(argv[1], "heap") == 0) {
        const size_t count = 4;
        int *data = malloc(count * sizeof *data);
        if (!data)
            return 1;
        for (size_t i = 0; i < count; i++)
            data[i] = 7;
        printf("wrote %zu numbers, last: %d\n", count, data[count - 1]);
        free(data);
    } else if (strcmp(argv[1], "uaf") == 0) {
        const char text[] = "agent";
        char *name = malloc(sizeof text);
        if (!name)
            return 1;
        memcpy(name, text, sizeof text);
        printf("name: %s\n", name);
        free(name);
    } else {
        return 1;
    }
    return 0;
}
