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
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define HEIGHT 320
#define WIDTH 569

struct sockaddr_in c_addr;

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void gotoxy(int x,int y)
{
	printf("%c[%d;%df",0x1B,y,x);
}

int main(int argc, char *argv[]) {

	system("clear");
	int connfd = 0, err;
	struct sockaddr_in serv_addr;
	int listenfd = 0, re;
	char buf[1025];
	int numrv, ret;
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
	char fname[100];

	char recvBuff[1024];
	memset(recvBuff, '0', sizeof(recvBuff));

	while(1) {
		clen=sizeof(c_addr);
		printf("Waiting...\n");
		connfd = accept(listenfd, (struct sockaddr*)&c_addr, &clen);

		FILE *fp;
		read(connfd, fname, 256);
		printf("File Name: %s\n", fname);
		printf("Receiving file...\n");
		fp = fopen(fname, "ab");
		if (NULL == fp) {
			perror("fopen");
			exit(1);
		}

		long double sz = 1;
		int bytesReceived = 0, totalReceived = 0;

		while ((bytesReceived = read(connfd, recvBuff, 1024)) > 0) {
			totalReceived += bytesReceived;
			sz++;
			fflush(stdout);
			fwrite(recvBuff, 1, bytesReceived, fp);
		}

		printf("Total received: %d\n", totalReceived);

		if (bytesReceived < 0) {
			perror("read");
			exit(1);
		} else printf("\nFile OK. Transfer Complete\n");
		fclose(fp);
		close(connfd);
	}
	return 0;
}
