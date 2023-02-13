import sys
from PIL import Image
import numpy as np

if len(sys.argv) != 3:
  print('Usage: python to_pixmap.py <input image file> <output pixmap file>')
  sys.exit(0)
inFileName = sys.argv[1]
outFileName = sys.argv[2]

color_pixels = np.asarray(Image.open(inFileName))
#mean_pixels = np.mean(color_pixels, axis = 2)
mean_pixels =0.2126*color_pixels[:,:,0] + 0.7152*color_pixels[:,:,1]+0.0722*color_pixels[:,:,2]
mean_pixels = mean_pixels.astype(np.uint8)
outF = open(outFileName, 'w')
outF.write(str(mean_pixels.shape[0])+' '+str(mean_pixels.shape[1])+'\n')
for row in range(mean_pixels.shape[0]):
  for col in range(mean_pixels.shape[1]):
    if col < mean_pixels.shape[1] - 1:
      outF.write(str(mean_pixels[row,col])+' ')
    else:
      outF.write(str(mean_pixels[row,col])+'\n')
    
outF.close()  


#out_img = Image.fromarray(mean_pixels.astype(np.uint8))
#out_img.save(outFileName)
