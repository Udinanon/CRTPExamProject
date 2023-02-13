import sys
from PIL import Image
import numpy as np

if len(sys.argv) != 3:
  print('Usage: python from_pixmap.py <input pixmap file> <output image file>')
  sys.exit(0)
inFileName = sys.argv[1]
outFileName = sys.argv[2]

inF = open(inFileName)
lines = inF.readlines()
shape = lines[0].split()
rows = int(shape[0])
cols = int(shape[1])

pixels = np.zeros((rows,cols), dtype=np.uint8)
for row in range(rows):
  currRow = lines[row+1].split()
  for col in range(cols):
    pixels[row,col] = int(currRow[col])

out_img = Image.fromarray(pixels, 'L')
out_img.save(outFileName)
