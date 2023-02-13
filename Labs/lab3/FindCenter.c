#include <string.h>
#include <math.h>

/* Black threshold:
   a pixel value less than the threshold is considered black. */
#define BLACK_LIMIT 10
#define MAX_RADIUS 200

void findCenter(int radius,unsigned char *buf, int rows, int cols,
    int *retX, int *retY, int *retMax, unsigned char *map)
{
    int x, y, l, m, currCol, currRow, maxCount = 0;
    int maxI  = 0, maxJ = 0;
    /* Square roots needed for computation are computed only once
       and maintained in array sqr */
    static int sqr[2 * MAX_RADIUS];
    static int sqrInitialized = 0;
    /* Hit counter, used to normalize the returned quality indicator */
    double totCounts = 0;
    /* The matrix is initially set to 0 */
    memset(map, 0, rows * cols);
    /* If square root values not yet initialized, compute them */
    if(!sqrInitialized)
    {
        sqrInitialized = 1;
        for(l = -radius; l <= radius; l++)
            /*integer approximation of sqrt(radius^2 - l^2) */
            sqr[l+radius] = sqrt(radius*radius - l*l) + 0.5;
    }
    for(currRow = 0; currRow < rows; currRow++)
    {
        for(currCol = 0; currCol < cols; currCol++)
        {
            /* Consider only pixels corresponding to borders of the image
               Such pixels are set by makeBorder as dark ones*/
            if(buf[currRow*cols + currCol] <= BLACK_LIMIT)
            {
                x = currCol;
                y = currRow;
                /* Increment the value of the pixels in map buffer which corresponds to
                   a circle of the given radius centered in (currCol, currRow) */
                for(l = x - radius; l <= x+radius; l++)
                {
                    if(l < 0 || l >= cols)
                        continue; // Out of image X range
                    m = sqr[l-x+radius];
                    if(y-m < 0 || y+m >= rows)
                        continue; //Out of image Y range
                    map[(y-m)*cols + l]++;
                    map[(y+m)*cols + l]++;	
                    totCounts += 2;	//Two more pixels incremented
                    /* Update current maximum */	
                    if(maxCount < map[(y+m)*cols + l])
                    {
                        maxCount = map[(y+m)*cols + l];
                        maxI = y + m;
                        maxJ = l;
                    }
                    if(maxCount < map[(y-m)*cols + l])
                    {
                        maxCount = map[(y-m)*cols + l];
                        maxI = y - m;
                        maxJ = l;
                    }
                }
            }
        }
    }
    /* Return the (X,y) position in the map which yields the largest value */
    *retX = maxJ;
    *retY = maxI;
    /* The returned quality indicator is expressed as maximum pixel
       value in map matrix */
    *retMax = maxCount;
}
