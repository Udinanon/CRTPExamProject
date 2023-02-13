


int makeBorderNonOptimized(unsigned char *image, unsigned char *border, int cols, int rows, int threshold);

int makeBorderOptimized(unsigned char *image, unsigned char *border, int cols, int rows, int threshold);

void findCenter(int radius, unsigned char *buf, int rows, int cols,
    int *retX, int *retY, int *retMax, unsigned char *map);