import argparse
import socket
import sys
import binascii
import struct
from collections import namedtuple
import cv2
import numpy as np
import pickle
import time
import math

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

THRESHHOLD = 0.1
DIVISOR = 10 ** 7


def send_image(sender, np_bytes):
    # Create a TCP Stream socket
    img_length = len(np_bytes)
    i = 0

    while i < img_length:
        check = sender.sendall(np_bytes[i : i + PACKET_SIZE])
        i += PACKET_SIZE if i + PACKET_SIZE < img_length else img_length - i

        if check is not None:
            print("Failed to send data")
            sys.exit()


def send_timestamp(sender, time):
    # print("INFO: Initiating send")
    if sender.sendall(str(time)) != None:
        print("Failed to send timestamp")
        sys.exit()


def recv_data(socket):
    reply = socket.recv(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT)) # 32 length
    print(struct.calcsize(SENSOR_STREAM_HEADER_FORMAT))
    if not reply:
        print("ERROR: Failed to receive data")
        sys.exit()

    data = struct.unpack(SENSOR_STREAM_HEADER_FORMAT, reply)
    print(len(data))
    header = SENSOR_FRAME_STREAM_HEADER(*data)

    time = header.Timestamp
    image_size_bytes = header.ImageHeight * header.RowStride
    image_data = bytes()
    bytes_sent = 0

    while len(image_data) < image_size_bytes:
        remaining_bytes = image_size_bytes - len(image_data)
        image_data_chunk = socket.recv(remaining_bytes)
        if not image_data_chunk:
            print("ERROR: Failed to receive image data")
            sys.exit()
        image_data += image_data_chunk

    return image_data, time


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
        start = time.time()
        img_count = 0
        quit = False
        s_time, t_time = 0, 0
        s_bool, t_bool = True, True
        s_data, t_data = None, None
        img_count = 0
        # sender.connect(server_address)

        while not quit:

            if s_bool:
                s_data, s_time = recv_data(s)
            if t_bool:
                t_data, t_time = recv_data(t)

            # image_array = np.frombuffer(s_data, dtype=np.uint8).reshape(
            #     (480, 640, 1)
            # )
            # cv2.imshow("Left", image_array)
            # if cv2.waitKey(1) & 0xFF == ord('q'):
            #     break
            if within_threshhold(s_time, t_time):
                """
                Send Image if the threshholds are aligned and reset bools
                """
                s_bool, t_bool = True, True
                s_image_array = np.frombuffer(s_data, dtype=np.uint8).reshape(
                    (480, 640, 1)
                )
                t_image_array = np.frombuffer(t_data, dtype=np.uint8).reshape(
                    (480, 640, 1)
                )
                # send_timestamp(sender, s_time)
                # send_image(sender, s_data)
                # send_image(sender, t_data)
                # data = sender.recv(1)
                cv2.imshow('Left', s_image_array)
                cv2.imshow('Right', t_image_array)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                if s_time > t_time:
                    s_bool = False
                    t_bool = True
                elif t_time > s_time:
                    t_bool = False
                    s_bool = True

    except KeyboardInterrupt:
        pass

    s.close()
    t.close()
    sender.close()

    cv2.destroyAllWindows()


def within_threshhold(s_time, t_time):

    s_float_time = float(s_time) / DIVISOR
    t_float_time = float(t_time) / DIVISOR

    return abs(s_float_time - t_float_time) < THRESHHOLD


if __name__ == "__main__":
    main(sys.argv[1:])
