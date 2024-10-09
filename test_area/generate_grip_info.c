#include <stdio.h>
#include <stdlib.h>

typedef struct {
    float x;  // Destination x-coordinate
    float y;  // Destination y-coordinate
} GridPoint;

void generate_grid(const char* filename, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    GridPoint gridPoint;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Example transformation: identity transformation (no change)
            gridPoint.x = (float)x;
            gridPoint.y = (float)y;

            // Write each grid point to file
            fwrite(&gridPoint, sizeof(GridPoint), 1, file);
        }
    }

    fclose(file);
}

int main() {
    int width = 1280;  // Example width
    int height = 720;  // Example height
    const char* filename = "grid_info.dat";

    generate_grid(filename, width, height);

    printf("Generated grid info file '%s' for an image of size %dx%d.\n", filename, width, height);
    return 0;
}
