#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

extern int makeBorderNonOptimized(unsigned char *image, unsigned char *border, int cols, int rows, int threshold);
extern int makeBorderOptimized(unsigned char *image, unsigned char *border, int cols, int rows, int threshold);

int main(int argc, char **argv)
{
  struct timeval time1, time2;
  FILE *inF, *outF;
  int rows, cols, col, row, pix, threshold, numBlackPixels, usecs, mode;

  unsigned char *pixels, *border_pixels;

  if (argc != 4)
  {
    perror("Usage: test_gradient <input pixel file> <output pixel file> <mode(0: non optimized; 1: optimized)>\n");
    return 1;
  }

  sscanf(argv[3], "%d", &mode);
  if (mode != 0 && mode != 1)
  {
    perror("mode argument shall be either 0 (non optimized) or 1 (optimized)\n");
    return 1;
  }

  threshold = 100;
  inF = fopen(argv[1], "r");
  if (!inF)
  {
    perror("file not found\n");
    return 1;
  }

  fscanf(inF, "%d", &rows);
  fscanf(inF, "%d", &cols);
  printf("Rows: %d  Cols: %d\n", rows, cols);
  pixels = (unsigned char *)malloc(rows * cols);
  border_pixels = (unsigned char *)malloc(rows * cols);
  for (row = 0; row < rows; row++)
  {
    for (col = 0; col < cols; col++)
    {
      fscanf(inF, "%d ", &pix);
      pixels[row * cols + col] = pix;
    }
  }
  fclose(inF);

  struct timespec t_start, t_end;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_start);

  gettimeofday(&time1, NULL);

  if (mode == 1)
    numBlackPixels = makeBorderOptimized(pixels, border_pixels, cols, rows, threshold);
  else
    numBlackPixels = makeBorderNonOptimized(pixels, border_pixels, cols, rows, threshold);
  gettimeofday(&time2, NULL);

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_end);
  double t_process_us = (t_end.tv_sec - t_start.tv_sec) * 1e6 +
                      (t_end.tv_nsec - t_start.tv_nsec) / 1e3;

  printf("Num Pixels: %d\tNum  Black Pixels: %d\n", rows * cols, numBlackPixels);
  usecs = (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec);
  printf("Elapsed system time(us) : %d\n", usecs);
  printf("Elapsed process time(us): %f\n", t_process_us);
  outF = fopen(argv[2], "w");
  fprintf(outF, "%d %d\n", rows, cols);
  for (row = 0; row < rows; row++)
  {
    for (col = 0; col < cols; col++)
    {
      if (col < cols - 1)
        fprintf(outF, "%d ", border_pixels[row * cols + col]);
      else
        fprintf(outF, "%d\n", border_pixels[row * cols + col]);
    }
  }
  fclose(outF);
  return 0;
}
