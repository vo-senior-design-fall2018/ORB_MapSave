#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main() {
	int connfd = 0,err;
	pthread_t tid; 
	struct sockaddr_in serv_addr;
	int listenfd = 0,ret;
	char sendBuff[1025];
	int numrv;
	size_t clen=0;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd<0)
	{
		printf("Error in socket creation\n");
		exit(2);
	}

	printf("Socket retrieve success\n");

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
	struct sockaddr_in c_addr;

	while(1)
	{
		clen=sizeof(c_addr);
		printf("Waiting...\n");
		connfd = accept(listenfd, (struct sockaddr*)&c_addr,&clen);
		if(connfd<0)
		{
			printf("Error in accept\n");
			continue;	
		}
		err = pthread_create(&tid, NULL, &SendFileToClient, &connfd);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
	}
	close(connfd);
	return 0;
}
