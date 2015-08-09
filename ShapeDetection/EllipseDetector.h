#ifndef ELLIPSEDETECTOR_H
#define ELLIPSEDETECTOR_H

#include "ConfigCV.h"
#include "ImagePreproc.h"
#include "IShapeDetector.h"

class EllipseDetector : public IShapeDetector<cv::RotatedRect>
{
private:
	cv::Mat prepocess(cv::Mat src);
	cv::Mat getEdge(cv::Mat prepocessed_src);
	std::vector<cv::RotatedRect> getRawEllipses(cv::Mat edge);
	std::vector<cv::RotatedRect> tuneRawEliipses(std::vector<cv::RotatedRect> raw_ellipses, cv::Mat src);
	/*--------tool method--------*/
	float diff(float x, float y){ return std::abs(x-y); }
	std::vector<cv::RotatedRect> sort(std::vector<cv::RotatedRect> src);
public:
	EllipseDetector(){}
	~EllipseDetector(){}
	virtual std::vector<cv::RotatedRect> excute(cv::Mat src);
};


#endif