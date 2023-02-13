
static inline int _abs(int x) { return (x>0)?x:-x; }

int makeBorderOptimized(unsigned char *image, unsigned char *border, int cols, int rows, int threshold)
{
    int x = 0, y, sumX, sumY, sum, numBlackPixels = 0;
/* Variables to hold the 3x3 portion of the image used in the computation
of the Sobel filter output */
    int c11,c12,c13,c21,c22,c23,c31,c32,c33;

    for(y = 0; y <= (rows-1); y++)
    {
    /* First image row: the first row of cij is zero */
        if(y == 0)
        {
            c11 = c12 = c13 = 0;
        }
        else 
    /* First image column: the first column of cij matrix is zero */
        {
            c11=0;
            c12 = *(image + (y - 1) * cols);
            c13 = *(image + 1 + (y - 1)*cols);
        }
        c21 = 0;
        c22 = *(image + y*cols);
        c23 = *(image + 1 + y*cols);
        if(y == rows - 1)
    /* Last image row: the third row of cij matrix is zero */
        {
            c31 = c32 = c33 = 0;
        }
        else
        {
            c31=0;
            c32 = *(image + (y + 1)*cols);
            c33 = *(image + 1 + (y + 1)*cols);
        }
/* The 3x3 matrix corresponding to the first pixel of the current image
   row has been loaded in program variables.
   The following iterations will only load
   from memory the rightmost column of such matrix */
        for(x = 0; x <= (cols-1); x++)
        {
            sumX = sumY = 0;
/* Skip image boundaries */
            if(y == 0 || y == rows-1)
                sum = 0;
            else if(x == 0 || x == cols-1)
                sum = 0;
/* Convolution starts here.
   GX and GY parameters are now "cabled" in the code */
            else
            {
                sumX = sumX - c11;
                sumY = sumY + c11;
                sumY = sumY + 2*c12;
                sumX = sumX + c13;
                sumY = sumY + c13;
                sumX = sumX - 2 * c21;
                sumX = sumX + 2*c23;
                sumX = sumX - c31;
                sumY = sumY - c31;
                sumY = sumY - 2*c32;
                sumX = sumX + c33;
                sumY = sumY - c33;
                sum = _abs(sumX) + _abs(sumY);
            }
/* Move one pixel on the right in the current row.
   Update the first/last row only if not in the first/last image row */
            if(y > 0)
            {
                c11 = c12;
                c12 = c13;
                c13 = *(image + x + 2 + (y - 1) * cols);
            }
            c21 = c22;
            c22 = c23;
            c23 = *(image + x +2 + y * cols);
            if(y < cols - 1)
            {
                c31 = c32;
                c32 = c33;
                c33 = *(image + x + 2 + (y + 1) * cols);
            }
            if(sum > 255) sum = 255;
            if(sum < threshold)
              sum=0;
            else
              numBlackPixels++;
/* Report the new pixel in the output image */
            *(border + x + y*cols) = 255 - (unsigned char)(sum);
        }
    }
    return numBlackPixels;
}
