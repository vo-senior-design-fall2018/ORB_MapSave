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

	std::vector<unsigned char> vectorBuff;

	while(1) {
		connfd = accept(listenfd, (struct sockaddr*)&c_addr, &clen);
		clen=sizeof(c_addr);
		printf("Waiting...\n");

		int bytesReceived = 0, totalReceived = 0;

		while ((bytesReceived = read(connfd, recvBuff, 1024)) > 0) {
			totalReceived += bytesReceived;
			std::copy(recvBuff, recvBuff + bytesReceived, std::back_inserter(vectorBuff));
		}

		printf("Total received left: %d; Vector size: %d\n", totalReceived, vectorBuff.size());

		unsigned char *sockData = &vectorBuff[0];

		cv::Mat leftImg(cv::Size(640, 480), CV_8UC1, sockData);
		cv::imshow("left img", leftImg);
		cv::waitKey(0);

		vectorBuff.clear();

/* 		totalReceived = 0, bytesReceived = 0; */

/* 		while ((bytesReceived = read(connfd, recvBuff, 1024)) > 0) { */
/* 			totalReceived += bytesReceived; */
/* 			std::copy(recvBuff, recvBuff + bytesReceived, std::back_inserter(vectorBuff)); */
/* 		} */

/* 		sockData = &vectorBuff[0]; */

/* 		cv::Mat rightImg(cv::Size(640, 480), CV_8UC1, sockData); */
/* 		cv::imshow("right img", rightImg); */
/* 		cv::waitKey(0); */

/* 		printf("Total received right: %d\n", totalReceived); */

		close(connfd);
	}
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
