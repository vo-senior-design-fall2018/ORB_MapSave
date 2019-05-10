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
void rot90(cv::Mat &matImage, int rotflag);

ORB_SLAM2::System *slam_ptr;

void my_handler(int s){
	printf("Caught signal %d\n",s);
    slam_ptr->Shutdown();
    slam_ptr->SaveTrajectoryTUM("CameraTrajectory.txt");
	exit(1); 
}

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

	char timeBuffer[18];
	memset(timeBuffer, '0', sizeof(timeBuffer));

	unsigned char recvBuff[1024];
	memset(recvBuff, '0', sizeof(recvBuff));

	std::vector<unsigned char> vectorBuff(640*480*2);

	// Read rectification parameters
	cv::FileStorage fsSettings(argv[2], cv::FileStorage::READ);
	if(!fsSettings.isOpened())
	{
		cerr << "ERROR: Wrong path to settings" << endl;
		return -1;
	}

	cv::Mat K_l, K_r, P_l, P_r, R_l, R_r, D_l, D_r;
	fsSettings["LEFT.K"] >> K_l;
	fsSettings["RIGHT.K"] >> K_r;

	fsSettings["LEFT.P"] >> P_l;
	fsSettings["RIGHT.P"] >> P_r;

	fsSettings["LEFT.R"] >> R_l;
	fsSettings["RIGHT.R"] >> R_r;

	fsSettings["LEFT.D"] >> D_l;
	fsSettings["RIGHT.D"] >> D_r;

	int rows_l = fsSettings["LEFT.height"];
	int cols_l = fsSettings["LEFT.width"];
	int rows_r = fsSettings["RIGHT.height"];
	int cols_r = fsSettings["RIGHT.width"];

	if(K_l.empty() || K_r.empty() || P_l.empty() || P_r.empty() || R_l.empty() || R_r.empty() || D_l.empty() || D_r.empty() ||
			rows_l==0 || rows_r==0 || cols_l==0 || cols_r==0)
	{
		cerr << "ERROR: Calibration parameters to rectify stereo are missing!" << endl;
		return -1;
	}

	cv::Mat M1l,M2l,M1r,M2r;
	cv::initUndistortRectifyMap(K_l,D_l,R_l,P_l.rowRange(0,3).colRange(0,3),cv::Size(cols_l,rows_l),CV_32F,M1l,M2l);
	cv::initUndistortRectifyMap(K_r,D_r,R_r,P_r.rowRange(0,3).colRange(0,3),cv::Size(cols_r,rows_r),CV_32F,M1r,M2r);

	slam_ptr = new ORB_SLAM2::System(argv[1],argv[2],ORB_SLAM2::System::STEREO,true,true);

	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

	connfd = accept(listenfd, (struct sockaddr*)&c_addr, &clen);
	while(1) {
		clen=sizeof(c_addr);

		int bytesReceived = 0, totalReceived = 0;

		if ((bytesReceived = read(connfd, timeBuffer, 18)) < 0) {
			perror("read");
			exit(1);
		}
		/* printf("%d\n", bytesReceived); */

		double timeStamp = parseTime(timeBuffer, 18);

		/* std::cout << "Timestamp: " << timeStamp << std::endl; */
		int targetSize = 640*480;

		while (totalReceived < targetSize) {
			bytesReceived = read(connfd, vectorBuff.data() + totalReceived, PACKET_SIZE);
			totalReceived += bytesReceived;
		}

		unsigned char *sockData = &vectorBuff[0];

		cv::Mat leftImg(cv::Size(640, 480), CV_8UC1, sockData);
		rot90(leftImg, 1);
		/* cv::imshow("Left", leftImg); */

		totalReceived = 0, bytesReceived = 0;

		while (totalReceived < targetSize) {
			bytesReceived = read(connfd, vectorBuff.data()+640*480+totalReceived, 1024);
			totalReceived += bytesReceived;
		}
		sockData = &vectorBuff[640*480];
		cv::Mat rightImg(cv::Size(640, 480), CV_8UC1, sockData);
		rot90(rightImg, 1);

		/* cv::imshow("Right", rightImg); */

		cv::Mat rightImgRect, leftImgRect;
		cv::remap(leftImg,leftImgRect,M1l,M2l,cv::INTER_LINEAR);
		cv::remap(rightImg,rightImgRect,M1r,M2r,cv::INTER_LINEAR);

		write(connfd, "1", 1);
		slam_ptr->TrackStereo(leftImgRect, rightImgRect, timeStamp);
		/* cv::waitKey(1); */
	}
	close(connfd);
	return 0;
}

void rot90(cv::Mat &matImage, int rotflag){
	//1=CW, 2=CCW, 3=180
	if (rotflag == 1){
		transpose(matImage, matImage);  
		flip(matImage, matImage,1); //transpose+flip(1)=CW
	} else if (rotflag == 2) {
		transpose(matImage, matImage);  
		flip(matImage, matImage,0); //transpose+flip(0)=CCW     
	} else if (rotflag ==3){
		flip(matImage, matImage,-1);    //flip(-1)=180          
	} else if (rotflag != 0){ //if not 0,1,2,3:
		cout  << "Unknown rotation flag(" << rotflag << ")" << endl;
	}
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

	return std::stod(strTime) / pow(10, 7);
}
