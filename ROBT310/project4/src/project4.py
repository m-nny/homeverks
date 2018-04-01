import sys
import cv2 as cv
import numpy as np

image_window = "Frame"
config_window = "Config"
cap = None

MouseX, MouseY = (0, 0)


def nothing(val):
    print(val)


def setup():
    print("Setup")
    global cap
    cap = cv.VideoCapture(0)
    cv.namedWindow(image_window, cv.WINDOW_AUTOSIZE)
    cv.namedWindow(config_window, cv.WINDOW_NORMAL)
    cv.resizeWindow(config_window, 720, 80)

    cv.createTrackbar('R', config_window, 0, 255, nothing)
    cv.createTrackbar('G', config_window, 0, 255, nothing)
    cv.createTrackbar('B', config_window, 0, 255, nothing)
    cv.createTrackbar('Radius', config_window, 0, 255, nothing)

    # cap.set(cv.CAP_PROP_FPS, 1)
    cap.set(cv.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv.CAP_PROP_FRAME_WIDTH, 640)
    cv.setMouseCallback(image_window, get_pixel, None)
    print("Setup ended")


def run():
    print("run")
    global cap, frame
    global MouseX, MouseY
    while True:
        _, frame = cap.read()

        # get current positions of four trackbars
        r = cv.getTrackbarPos('R', config_window)
        g = cv.getTrackbarPos('G', config_window)
        b = cv.getTrackbarPos('B', config_window)
        radius = cv.getTrackbarPos('Radius', config_window)

        f_frame = selective_color(frame, (b, g, r), radius)

        cv.imshow(image_window, f_frame)

        k = cv.waitKey(5) & 0xFF
        if k == 27:
            break

        # print("{},{}".format((b, g, r), radius))
        # print("({},{})={}".format(MouseX, MouseY, frame[MouseY][MouseX]))
    print("Finished running")


def main(argv):
    global cap
    setup()
    run()
    cap.release()
    return 0


def get_pixel(event, x, y, flags, param):
    global MouseX, MouseY, frame
    if event == cv.EVENT_LBUTTONDOWN:
        MouseX, MouseY = x, y
        b, g, r = frame[y][x]
        cv.setTrackbarPos('R', config_window, r)
        cv.setTrackbarPos('G', config_window, g)
        cv.setTrackbarPos('B', config_window, b)
        print("Set RGB to {}".format((r, g, b)))


def selective_color(image, point, radius):
    # n, m, d = image.shape
    # mask = np.linalg.norm(image - point, axis=2) > radius
    # mask = 0
    mask = (np.sum((image - point) ** 2, axis=2)) > (radius ** 2)
    # n_mask = np.repeat(mask[:, :, np.newaxis], 3, axis=2)
    gray = cv.cvtColor(image, cv.COLOR_BGR2GRAY)
    gray = cv.merge((gray, gray, gray))
    # gray = np.repeat(gray[:, :, np.newaxis], 3, axis=2)
    output = np.copy(image)
    output[mask] = gray[mask]
    return output


if __name__ == "__main__":
    main(sys.argv[1:])
