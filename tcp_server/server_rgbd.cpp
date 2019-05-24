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

struct sockaddr_in c_addr;

int parseFileSize(char fileSize[], int n);

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

	unsigned char recvBuff[1024];
	memset(recvBuff, '0', sizeof(recvBuff));

	/* ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::RGBD, true, true); */

	while(1) {
		clen=sizeof(c_addr);
		printf("Waiting...\n");
		connfd = accept(listenfd, (struct sockaddr*)&c_addr, &clen);

		FILE *fp_rgb;
		FILE *fp_depth;

		fp_rgb = fopen("rgb.png", "w+");
		fp_depth = fopen("depth.png", "w+");

		if (NULL == fp_rgb) {
			perror("fopen");
			exit(1);
		}
		if (NULL == fp_depth) {
			perror("fopen");
			exit(1);
		}

		long double sz = 1;
		int bytesReceived = 0, totalReceived = 0;

		if ((bytesReceived = read(connfd, fileSize, 16)) < 0) {
			perror("read");
			exit(1);
		}

		int size = parseFileSize(fileSize, 16);

		std::cout << size << std::endl;

		int depthImageSize = size, rgbImageSize = size;

		while (depthImageSize != 0) {
			bytesReceived = read(connfd, recvBuff, 1024);
			totalReceived += bytesReceived;
			sz++;
			fflush(stdout);
			fwrite(recvBuff, 1, bytesReceived, fp_depth);
			depthImageSize -= bytesReceived;
		}

		printf("Total received depth: %d\n", totalReceived);

		bytesReceived = 0, totalReceived = 0;

		while (rgbImageSize != 0) {
			bytesReceived = read(connfd, recvBuff, 1024);
			totalReceived += bytesReceived;
			sz++;
			fflush(stdout);
			fwrite(recvBuff, 1, bytesReceived, fp_rgb);
			rgbImageSize -= bytesReceived;
		}

		printf("Total received rgb: %d\n", totalReceived);

		if (bytesReceived < 0) {
			perror("read");
			exit(1);
		} else printf("\nFile OK. Transfer Complete\n");
		fclose(fp_rgb);
		fclose(fp_depth);

		cv::Mat img = cv::imread("test.png");

		write(connfd, "Hello", 16);

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
