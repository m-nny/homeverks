import sys
import cv2 as cv
import numpy as np

DELAY_CAPTION = 1500
DELAY_BLUR = 100
MIN_KERNEL_LENGTH = 10
MAX_KERNEL_LENGTH = 255

src = None
dst = None
window_name = 'Project 4'


def main(argv):
    cv.namedWindow(window_name, cv.WINDOW_AUTOSIZE)

    # Load the source image
    image_name = argv[0] if len(argv) > 0 else "../data/lena3.tif"
    global src
    src = cv.imread(image_name)
    if src is None:
        print('Error opening image')
        print(
            'Usage: smoothing.py [image_name -- default ../data/lena3.tif] \n')
        return -1

    if display_caption('Original Image') != 0:
        return 0

    global dst
    dst = np.copy(src)
    if display_dst(DELAY_CAPTION) != 0:
        return 0
    # Selective color effect
    if display_caption('Selective color effect') != 0:
        return 0

    for i in range(MIN_KERNEL_LENGTH, MAX_KERNEL_LENGTH, 5):
        dst = selective_color(src, (0, 0, 255), i)
        if display_dst(DELAY_BLUR) != 0:
            return 0

    #  Done
    display_caption('Done!')
    return 0


def selective_color(image, point, radius):
    # n, m, d = image.shape
    mask = np.linalg.norm(image - point, axis=2) > radius
    # mask = (np.sum((image - point) ** 2, axis=2)) > (radius ** 2)
    # n_mask = np.repeat(mask[:, :, np.newaxis], 3, axis=2)
    gray = cv.cvtColor(image, cv.COLOR_BGR2GRAY)
    gray = np.repeat(gray[:, :, np.newaxis], 3, axis=2)
    output = np.copy(image)
    output[mask] = gray[mask]
    return output


def display_caption(caption):
    global dst
    dst = np.zeros(src.shape, src.dtype)
    rows, cols, ch = src.shape
    cv.putText(dst, caption,
               (int(cols / 4), int(rows / 2)),
               cv.FONT_HERSHEY_COMPLEX, 1, (255, 255, 255))
    return display_dst(DELAY_CAPTION)


def display_dst(delay):
    cv.imshow(window_name, dst)
    c = cv.waitKey(delay)
    if c >= 0:
        return -1
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
