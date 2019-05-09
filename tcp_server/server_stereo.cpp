#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <System.h>

#define BACKLOG 10     // how many pending connections queue will hold
#define PACKET_SIZE 1024 // PACKET SIZE

struct sockaddr_in c_addr;

int parseFileSize(char fileSize[], int n);
double parseTime(char time[], int n);

int main(int argc, char *argv[]) {

	if (argc < 3) {
		fprintf(stderr, "usage: ./mono_tcp_server [vocabulary] [configuration file]\n");
		return 1;
	}

	system("clear");
	int connfd = 0, listenfd = 0, ret;
	struct sockaddr_in serv_addr;
	uint clen = 0;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	ret=bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
	if(ret<0)
	{
		printf("Error in bind\n");
		exit(2);
	}

	if(listen(listenfd, 10) == -1)
	{
		printf("Failed to listen\n");
		return -1;
	}

	char fileSize[16];
	memset(fileSize, '0', sizeof(fileSize));

	char timeBuffer[16];
	memset(timeBuffer, '0', sizeof(timeBuffer));

	unsigned char recvBuff[1024];
	memset(recvBuff, '0', sizeof(recvBuff));

	std::vector<unsigned char> leftFrontBuff;
	std::vector<unsigned char> rightFrontBuff;

	ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::STEREO, true, 0);

	connfd = accept(listenfd, (struct sockaddr*)&c_addr, &clen);
	while(1) {
		clen=sizeof(c_addr);
		printf("Waiting...\n");

		int bytesReceived = 0, totalReceived = 0;

		if ((bytesReceived = read(connfd, fileSize, 16)) < 0) {
			perror("read");
			exit(1);
		}

		cv::Mat leftFrontMat = cv::Mat::zeros(480, 640, CV_8UC1),
				rightFrontMat = cv::Mat::zeros(480, 640, CV_8UC1);

		int size = parseFileSize(fileSize, 16);
		std::cout << size << std::endl;

		leftFrontBuff.resize(size);
		rightFrontBuff.resize(size);

		if ((bytesReceived = read(connfd, timeBuffer, 16)) < 0) {
			perror("read");
			exit(1);
		}

		/* double time = parseTime(timeBuffer, 16); */

		while ((bytesReceived = read(connfd, recvBuff, 1024)) > 0) {
			totalReceived += bytesReceived;
			std::copy(recvBuff, recvBuff + PACKET_SIZE, std::back_inserter(leftFrontBuff));
			fflush(stdout);
		}

		printf("Total received for left front: %d\n", totalReceived);
		unsigned char *ptrLeftFront = &leftFrontBuff[0];
		std::memcpy(leftFrontMat.data, ptrLeftFront, totalReceived);

		bytesReceived = 0, totalReceived = 0;

		while ((bytesReceived = read(connfd, recvBuff, 1024)) > 0) {
			totalReceived += bytesReceived;
			std::copy(recvBuff, recvBuff + PACKET_SIZE, std::back_inserter(rightFrontBuff));
			fflush(stdout);
		}

		printf("Total received for right front: %d\n", totalReceived);
		unsigned char *ptrRightFront = &rightFrontBuff[0];
		std::memcpy(rightFrontMat.data, ptrRightFront, totalReceived);

		/* cv::Mat img = cv::imread("test.png"); */

		/* cv::Mat track = SLAM.TrackStereo(img, time); */
		/* std::cout << track << std::endl; */
		leftFrontBuff.clear();
		rightFrontBuff.clear();

	}
	close(connfd);
	return 0;
}

int parseFileSize(char fileSize[], int n) {

	std::string strFileSize;

	for (int i = 0; i < n; i++) {
		if (fileSize[i] == ':') break;
		strFileSize += fileSize[i];
	}

	return std::stoi(strFileSize);
}

double parseTime(char time[], int n) {
	std::string strTime;

	for (int i = 0; i < n; i++) {
		if (time[i] == ':') break;
		strTime += time[i];
	}
	return std::stod(strTime);
}
