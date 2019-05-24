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
#include <chrono>

#include <System.h>

void rot90(cv::Mat &matImage, int rotflag);

ORB_SLAM2::System *slam_ptr;

void my_handler(int s){
	printf("Caught signal %d\n",s);
    slam_ptr->Shutdown();
    slam_ptr->SaveTrajectoryTUM("CameraTrajectory.txt");
	exit(1); 
}

int main(int argc, char *argv[]) {

	if (argc < 4) {
		fprintf(stderr, "usage: ./hololens [ip] [vocabulary] [configuration file]\n");
		return 1;
	}

	system("clear");

	int left_port = 23944;
	int right_port = 23945;

	system("clear");
	int left_sockfd = 0;
	int right_sockfd = 0;
	int bytesReceived = 0;
	struct sockaddr_in left_serv_addr, right_serv_addr;

	/* Create a socket first */
	if((left_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
	{
		perror("left socket");
		exit(1);
	}

	if((right_sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
	{
		perror("right socket");
		exit(1);
	}

	/* Initialize sockaddr_in data structure */
	left_serv_addr.sin_family = AF_INET;
	left_serv_addr.sin_port = htons(left_port); // port

	right_serv_addr.sin_family = AF_INET;
	right_serv_addr.sin_port = htons(right_port); // port

	left_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	right_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

	/* Attempt a connection */
	if(connect(left_sockfd, (struct sockaddr *)&left_serv_addr, sizeof(left_serv_addr))<0)
	{
		printf("\n Error : Connect Failed \n");
		return 1;
	}

	/* Attempt a connection */
	if(connect(right_sockfd, (struct sockaddr *)&right_serv_addr, sizeof(right_serv_addr))<0)
	{
		printf("\n Error : Connect Failed \n");
		return 1;
	}

	printf("Connected to ip: %s : %d\n", inet_ntoa(left_serv_addr.sin_addr),
			ntohs(left_serv_addr.sin_port));

	printf("Connected to ip: %s : %d\n", inet_ntoa(right_serv_addr.sin_addr),
			ntohs(right_serv_addr.sin_port));

	int targetSize = 640 * 480;

	// Read rectification parameters
	cv::FileStorage fsSettings(argv[3], cv::FileStorage::READ);
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

	slam_ptr = new ORB_SLAM2::System(argv[2],argv[3],ORB_SLAM2::System::STEREO,true,true);

	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	std::cout << "OpenCV Version: " << CV_VERSION << std::endl;

	std::vector<unsigned char> vectorBuff(640*480*2);
	while (1) {
		unsigned char header[32];
		int totalRequired = targetSize;

		if ((bytesReceived = read(left_sockfd, header, 32)) < 0) {
			perror("read");
			exit(1);
		}

		while (totalRequired > 0) {
			if (totalRequired > 1024) {
				totalRequired -= read(left_sockfd, vectorBuff.data() + targetSize - totalRequired, 1024);
			} else {
				totalRequired -= read(left_sockfd, vectorBuff.data() + targetSize - totalRequired, totalRequired);
				break;
			}
		}

		totalRequired = targetSize;

		if ((bytesReceived = read(right_sockfd, header, 32)) < 0) {
			perror("read");
			exit(1);
		}

		while (totalRequired > 0) {
			if (totalRequired > 1024) {
				totalRequired -= read(right_sockfd, vectorBuff.data() + targetSize +  targetSize - totalRequired, 1024);
			} else {
				totalRequired -= read(right_sockfd, vectorBuff.data() + targetSize + targetSize - totalRequired, totalRequired);
				break;
			}
		}

		usleep(200);

		cv::Mat left_img(cv::Size(640, 480), CV_8UC1, &vectorBuff[0]);
		cv::Mat right_img(cv::Size(640, 480), CV_8UC1, &vectorBuff[640*480]);

		rot90(right_img, 1);
		rot90(left_img, 1);

		cv::Mat rightImgRect, leftImgRect;
		cv::remap(left_img,leftImgRect,M1l,M2l,cv::INTER_LINEAR);
		cv::remap(right_img,rightImgRect,M1r,M2r,cv::INTER_LINEAR);

		slam_ptr->TrackStereo(leftImgRect, rightImgRect, 0.0);
		/* cv::imshow("Left", left_img); */
		/* cv::imshow("Right", right_img); */

		/* cv::waitKey(1); */
	}

	close(left_sockfd);

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
