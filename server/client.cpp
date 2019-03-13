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

#include <arpa/inet.h>

void send_image(int *arg, char fname[100]) {
	int connfd = (int)*arg;

	write(connfd, fname, 256);

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

	close(connfd);
	shutdown(connfd, SHUT_WR);
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
	/* char ip[50]; */
	/* if (argc < 2) */ 
	/* { */
	/* 	printf("Enter IP address to connect: "); */
	/* 	gets(ip); */
	/* } */
	/* else */
	/* 	strcpy(ip,argv[1]); */
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

	printf("Connected to ip: %s : %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

	send_image(&sockfd, fname);
	close(sockfd);
}
