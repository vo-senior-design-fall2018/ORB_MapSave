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
            
            print("this is the image_data: " + str(s_image_data))
            print("this is the image_size_bytes: " + str(s_image_size_bytes))
            
            s_bytes_sent = 0
            t_bytes_sent = 0
            print(s_image_size_bytes)
            while len(s_image_data) < s_image_size_bytes:
                d = s.recv(1024)
                s_bytes_sent += len(d)
                if(s_bytes_sent > s_image_size_bytes): 
                    s_bytes_sent = s_bytes_sent - s_image_size_bytes
                    d = s.recv(s_bytes_sent)
                    s_image_data += d
                    
                s_image_data += d
                
                # Serialize frame
                s_data = pickle.dumps(s_image_data)
                s_check = s.sendall(s_data)
                
                if s_check is not None:
                    print("Failed to send s_data")
                    sys.exit()

                print("1024 bytes of left front image array sent")
                
            while len(t_image_data) < t_image_size_bytes:
                d = t.recv(1024)
                t_bytes_sent += len(t)
                print(t_bytes_sent)
                t_image_data += t
                if(t_bytes_sent > t_image_size_bytes): 
                    t_bytes_sent = t_bytes_sent - t_image_size_bytes
                    d = t.recv(s_bytes_sent)
                    t_image_data += d
                
                # Serialize frame
                t_data = pickle.dumps(t_image_data)
                t_check = t.sendall(t_data)
                
                if t_check is not None:
                    print("Failed to send s_data")
                    sys.exit()

                print("1024 bytes of right front image array sent")

            if PROCESS:
                # process image
                gray = cv2.cvtColor(image_array, cv2.COLOR_BGR2GRAY)
                image_array = cv2.Canny(gray, 50, 150, apertureSize=3)

            #             cv2.imwrite('left_image.png', s_image_array)
            #             cv2.imwrite('right_image.png', t_image_array)

            break
    #             if cv2.waitKey(1) & 0xFF == ord('q'):
    #                 break
    except KeyboardInterrupt:
        pass

    s.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main(sys.argv[1:])
