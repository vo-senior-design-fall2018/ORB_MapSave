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

    print("INFO: Initiating send")

    sender.connect(server_address)

    img_length = len(np_bytes)

    i = 0

    while i < img_length:
        check = sender.sendall(np_bytes[i : i + PACKET_SIZE])
        i += PACKET_SIZE if i + PACKET_SIZE < img_length else img_length - i

        if check is not None:
            print("Failed to send s_data")
            sys.exit()

    print("Total sent: {}".format(i))

    sender.close()


def main(argv):
    """Receiver main"""
    parser = argparse.ArgumentParser()
    required_named_group = parser.add_argument_group("named arguments")

    required_named_group.add_argument(
        "-a", "--host", help="Host address to connect", required=True
    )
    args = parser.parse_args(argv)

    HOST = args.host
    # Create a TCP Stream socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        t = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
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

    # Try connecting to the address
    s.connect((HOST, LEFT_FRONT_PORT))
    print("INFO: Socket Connected to " + args.host + " on port " + str(LEFT_FRONT_PORT))

    t.connect((HOST, RIGHT_FRONT_PORT))
    print(
        "INFO: Socket Connected to " + args.host + " on port " + str(RIGHT_FRONT_PORT)
    )

    # Try receive data
    try:
        quit = False
        while not quit:
            s_reply = s.recv(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT))
            if not s_reply:
                print("ERROR: Failed to receive data")
                sys.exit()

            t_reply = t.recv(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT))
            if not t_reply:
                print("ERROR: Failed to receive data")
                sys.exit()

            s_data = struct.unpack(SENSOR_STREAM_HEADER_FORMAT, s_reply)
            t_data = struct.unpack(SENSOR_STREAM_HEADER_FORMAT, t_reply)

            # Parse the header
            s_header = SENSOR_FRAME_STREAM_HEADER(*s_data)
            t_header = SENSOR_FRAME_STREAM_HEADER(*t_data)

            # read the image in chunks
            s_image_size_bytes = s_header.ImageHeight * s_header.RowStride
            s_image_data = bytes()

            t_image_size_bytes = t_header.ImageHeight * t_header.RowStride
            t_image_data = bytes()

            s_bytes_sent = 0
            t_bytes_sent = 0

            sender.connect(server_address)

            s_total_sent = 0

            while len(s_image_data) < s_image_size_bytes:
                remaining_bytes = s_image_size_bytes - len(s_image_data)
                image_data_chunk = s.recv(remaining_bytes)
                if not image_data_chunk:
                    print("ERROR: Failed to receive image data")
                    sys.exit()
                s_image_data += image_data_chunk

            while len(t_image_data) < t_image_size_bytes:
                remaining_bytes = t_image_size_bytes - len(t_image_data)
                image_data_chunk = t.recv(remaining_bytes)
                if not image_data_chunk:
                    print("ERROR: Failed to receive image data")
                    sys.exit()
                t_image_data += image_data_chunk

            image_array = np.frombuffer(image_data, dtype=np.uint8).reshape(
                (header.ImageHeight, header.ImageWidth, header.PixelStride)
            )

            print(len(s_image_data))
            send_image(s_image_data)
            # send_image(t_image_data)

            break
    except KeyboardInterrupt:
        pass

    s.close()
    t.close()


if __name__ == "__main__":
    main(sys.argv[1:])
