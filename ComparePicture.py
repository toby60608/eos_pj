import subprocess
import cv2 
import numpy as np
from skimage.metrics import structural_similarity
import sys

img1 = cv2.imread(sys.argv[1],cv2.IMREAD_GRAYSCALE)
img2 = cv2.imread(sys.argv[2],cv2.IMREAD_GRAYSCALE)

(score,diff) =structural_similarity (img1,img2,full = True)
diff = (diff *255).astype("uint8")
print("{}".format(score*100)) # SSIM value * 100