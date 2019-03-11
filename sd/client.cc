/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

#include <vector>
#include <opencv2/core/core.hpp>

#include <System.h>

void LoadImages(const string &strAssociationFilename, vector<string> &vstrImageFilenamesRGB,
                vector<string> &vstrImageFilenamesD, vector<double> &vTimestamps);

int main(int argc, char **argv)
{
    if(argc != 4)
    {
        cerr << endl << "Usage: ./client [port] [association.txt] [sequence]" << endl;
        return 1;
    }

    // Retrieve paths to images
    vector<string> vstrImageFilenamesRGB;
    vector<string> vstrImageFilenamesD;
    vector<double> vTimestamps;
    string strAssociationFilename = string(argv[2]);
    LoadImages(strAssociationFilename, vstrImageFilenamesRGB, vstrImageFilenamesD, vTimestamps);

    // Check consistency in the number of images and depthmaps
    int nImages = vstrImageFilenamesRGB.size();
    if(vstrImageFilenamesRGB.empty())
    {
        cerr << endl << "No images found in provided path." << endl;
        return 1;
    }
    else if(vstrImageFilenamesD.size()!=vstrImageFilenamesRGB.size())
    {
        cerr << endl << "Different number of images for rgb and depth." << endl;
        return 1;
    }

    addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, argv[1], &hints, &servinfo);

    int serverSocket;

    for (p = servinfo; p; p = p->ai_next) {
      if ((serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        continue;
      }
      if (connect(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
        close(serverSocket);
        continue;
      }
      break;
    }

    if (p == NULL) {
      fprintf(stderr, "connect() to server failed\n");
      return 1;
    }

    freeaddrinfo(servinfo);

    cv::Mat imRGB = cv::Mat::zeros(480, 640, CV_8UC3),
            imD   = cv::Mat::zeros(480, 640, CV_16UC1);

    int sizeRGB = imRGB.total() * imRGB.elemSize(),
        sizeD   = imD.total() * imD.elemSize();

    vector<uchar> buffer;
    buffer.resize(sizeRGB+sizeD+sizeof(double));

    uchar *ptrRGB = &buffer[0],
          *ptrD   = &buffer[sizeRGB];

    double *ptrTFrame = (double*)&buffer[sizeRGB+sizeD];

    double tFrame = 0.0;

    // Main loop
    for(int ni=0; ni<nImages; ni++)
    {
      // Read image and depthmap from file
      imRGB = cv::imread(string(argv[3])+"/"+vstrImageFilenamesRGB[ni],CV_LOAD_IMAGE_UNCHANGED);
      imD = cv::imread(string(argv[3])+"/"+vstrImageFilenamesD[ni],CV_LOAD_IMAGE_UNCHANGED);
      tFrame = vTimestamps[ni];
/*
      fprintf(stderr, "%d, %d\n", imRGB.total() * imRGB.elemSize(), sizeRGB);
      fprintf(stderr, "%d, %d\n", imD.total() * imD.elemSize(), sizeD);
      fprintf(stderr, "%d, %d\n", sizeof(double), buffer.size());
*/
      if(imRGB.empty())
      {
        cerr << endl << "Failed to load image at: "
             << string(argv[3]) << "/" << vstrImageFilenamesRGB[ni] << endl;
        return 1;
      } else {
        memcpy(ptrRGB, imRGB.data, sizeRGB);
        memcpy(ptrD, imD.data, sizeD);
        memcpy(ptrTFrame, &tFrame, sizeof(double));
        if (send(serverSocket, &buffer[0], buffer.size(), 0) < 0) {
          fprintf(stderr, "send() failed\n");
          return 1;
        }
      }
    }

    close(serverSocket);

    return 0;
}

void LoadImages(const string &strAssociationFilename, vector<string> &vstrImageFilenamesRGB,
                vector<string> &vstrImageFilenamesD, vector<double> &vTimestamps)
{
    ifstream fAssociation;
    fAssociation.open(strAssociationFilename.c_str());
    while(!fAssociation.eof())
    {
        string s;
        getline(fAssociation,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            double t;
            string sRGB, sD;
            ss >> t;
            vTimestamps.push_back(t);
            ss >> sRGB;
            vstrImageFilenamesRGB.push_back(sRGB);
            ss >> t;
            ss >> sD;
            vstrImageFilenamesD.push_back(sD);

        }
    }
}
