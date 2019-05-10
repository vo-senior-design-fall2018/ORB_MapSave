from __future__ import print_function

import argparse
import socket
import sys
import binascii
import struct
from collections import namedtuple
import cv2
import numpy as np
import pickle

PROCESS = False

# Definitions

# Protocol Header Format
# Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth
# ImageHeight PixelStride RowStride
SENSOR_STREAM_HEADER_FORMAT = "@IBBHqIIII"

SENSOR_FRAME_STREAM_HEADER = namedtuple(
    "SensorFrameStreamHeader",
    "Cookie VersionMajor VersionMinor FrameType Timestamp ImageWidth ImageHeight PixelStride RowStride",
)

# Each port corresponds to a single stream type
# Port for obtaining Photo Video Camera stream
PV_STREAM_PORT = 23940
LEFT_FRONT_PORT = 23944
RIGHT_FRONT_PORT = 23945

PACKET_SIZE = 1024


def send_image(np_bytes):
    # Create a TCP Stream socket
    try:
        sender = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = ("localhost", 5000)
    except (socket.error, msg):
        print(
            "ERROR: Failed to create socket. Code: "
            + str(msg[0])
            + ", Message: "
            + msg[1]
        )
        sys.exit()

    print("INFO: socket created")

    sender.connect(server_address)

    img_length = len(np_bytes)

    i = 0

    while i < img_length:
        check = sender.sendall(np_bytes[i : i + PACKET_SIZE])
        i += (
            PACKET_SIZE
            if i + PACKET_SIZE < img_length
            else img_length - i
        )

        if check is not None:
            print("Failed to send s_data")
            sys.exit()

    sender.close()

def main():
    """Receiver main"""

    # Try receive data
    try:
        quit = False
        while not quit:

            i = 0
            img = cv2.imread('tcp_server/test_image.png', 0)
            image_size_bytes = img.shape[0] * img.shape[1]
            np_bytes = img.tobytes()

            send_image(np_bytes)
            # send_image(np_bytes)

            break
    except KeyboardInterrupt:
        pass

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
