#ifndef ISHAPEDETECTOR_H
#define ISHAPEDETECTOR_H

#include "ConfigCV.h"
#include "ImagePreproc.h"

template<typename T>
class IShapeDetector 
{
public:
	virtual std::vector<T> excute(cv::Mat src) = 0;
};

#endif