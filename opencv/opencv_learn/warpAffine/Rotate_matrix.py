import cv2
import numpy as np

width = 1080
height = 1920
center = (width / 2, height / 2)
angle = 15
scale = 1.0
rotation_matrix = cv2.getRotationMatrix2D(center, angle, scale)
print("rotate matrix:")
print(rotation_matrix)