#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>

#include <vector>
#include <opencv2/core/core.hpp>

#include <System.h>

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: ./os2_server_cli [port] [vocabulary] [settings]\n");
    return 1;
  }

  addrinfo hints, *servinfo;

  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, argv[1], &hints, &servinfo);

  int localSocket,
      remoteSocket;

  sockaddr_storage remoteAddr;
  socklen_t remoteAddrLen;

  localSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  if (localSocket == -1) {
    fprintf(stderr, "socket() failed\n");
    return 1;
  }

  if (bind(localSocket, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    fprintf(stderr, "bind() on localSocket failed\n");
    return 1;
  }

  listen(localSocket, 3);
  fprintf(stderr, "listening on port %s\n", argv[1]);
  freeaddrinfo(servinfo);

  while (1) {
    if ((remoteSocket = accept(localSocket, (sockaddr *)&remoteAddr, &remoteAddrLen)) < 0) {
      fprintf(stderr, "accept() failed\n");
      return 1;
    }

    if (!fork()) {
/*
      if (send(remoteSocket, "Hello\n", 6, 0) < 0) {
        fprintf(stderr, "send() failed\n");
        return 1;
      }
*/

      ORB_SLAM2::System SLAM(argv[2], argv[3], ORB_SLAM2::System::RGBD, true, 0);

      cv::Mat imRGB = cv::Mat::zeros(480, 640, CV_8UC3),
              imD   = cv::Mat::zeros(480, 640, CV_16UC1);

      int sizeRGB = imRGB.total() * imRGB.elemSize(),
          sizeD   = imD.total() * imD.elemSize();

      int nBytes = 0;

      vector<uchar> buffer;
      buffer.resize(sizeRGB+sizeD+sizeof(double));

      uchar *ptrRGB = &buffer[0],
            *ptrD   = &buffer[sizeRGB];

      double *ptrTFrame = (double*)&buffer[sizeRGB+sizeD];

      double tFrame = 0.0;

      if (!imRGB.isContinuous()) {
        imRGB = imRGB.clone();
      }

      while (1) {
        nBytes = recv(remoteSocket, &buffer[0], buffer.size(), MSG_WAITALL);
        if (nBytes < 0) {
          fprintf(stderr, "recv() failed\n");
          return 1;
        }

        if (nBytes == 0) break;

        memcpy(imRGB.data, ptrRGB, sizeRGB);
        memcpy(imD.data, ptrD, sizeD);
        tFrame = *ptrTFrame;

        SLAM.TrackRGBD(imRGB, imD, tFrame);
      }

      close(localSocket);
      close(remoteSocket);
      return 0;
    } else {
      wait(NULL);
      close(localSocket);
      close(remoteSocket);
      break;
    }
  }

  return 0;
}
