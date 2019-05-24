#ifndef VIEWER2D_H
#define VIEWER2D_H

#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>

class Viewer2D {
public:
  Viewer2D(int, int, int, float, float, int);

  void project(float, float);

private:
  int fps;
  
  int imWidth;
  int imHeight;
  int padding;

  float projWidth;
  float projHeight;

  float xScale;
  float yScale;

  cv::Mat trail;
};

#endif