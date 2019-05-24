#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <arpa/inet.h>

void send_image(int *arg, char fname[100]) {
	int connfd = (int)*arg;

	/* write(connfd, fname, 256); */

	FILE *fp = fopen(fname, "rb");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	int totalSent = 0;

	while (1) {
		unsigned char buff[1024] = {0};
		int nread = fread(buff, 1, 1024, fp);

		totalSent += nread;

		if (nread > 0) write(connfd, buff, nread);
		if (nread < 1024) {
			if (feof(fp)) {
				printf("End of file\n");
			}
			if (ferror(fp)) perror("read");
			break;
		}
	}

	printf("Total sent: %d\n", totalSent);
}

void send_opencv_image(int *arg, char fname[100]) {
	int connfd = (int)*arg;

	cv::Mat img = cv::imread(fname, 0);

	if (!img.data) {
		std::cout <<  "Could not open or find the image" << std::endl;
	}

	int imgSize = img.total() * img.elemSize();
	cv::imshow("hello", img);
	cv::waitKey(0);
	int totalSent = 0;

	img = (img.reshape(0,1));
	int bytes = send(connfd, img.data, imgSize, 0);

	printf("Total sent: %d\n", bytes);
}

void send_sixteen_bits(int *arg, char *data[16]) {
	int connfd = (int)*arg;
	write(connfd, data, 16);
}

int main(int argc, char *argv[]) {
	system("clear");
	int sockfd = 0;
	int bytesReceived = 0;
	char buf[1025];
	struct sockaddr_in serv_addr;
	size_t clen = 0;

	/* Create a socket first */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
	{
		perror("socket");
		exit(1);
	}

	/* Initialize sockaddr_in data structure */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000); // port

	char fname[100];
	if (argc < 2) {
		printf("Enter the filename to send: ");
		std::cin >> fname;
	} else strcpy(fname, argv[1]);

	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* Attempt a connection */
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("\n Error : Connect Failed \n");
		return 1;
	}

	printf("Connected to ip: %s : %d\n", inet_ntoa(serv_addr.sin_addr),
			ntohs(serv_addr.sin_port));

	send_image(&sockfd, fname);
	close(sockfd);
	return 0;
}
