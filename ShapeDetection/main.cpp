#include "EllipseDetector.h"
#include "ImagePreproc.h"
#include "IShapeDetector.h"


int main(void) 
{
	cv::Mat img = cv::imread("image.bmp",0);
	cv::resize(img, img, cv::Size(img.size().width/4, img.size().height/4));
	IShapeDetector<cv::RotatedRect> *detector = new EllipseDetector();
	std::vector<cv::RotatedRect> ellipses = detector->excute(img);
	delete detector;
	for (unsigned int i = 0; i < ellipses.size(); ++i) {
		cv::ellipse(
			img, ellipses[i].center, 
			ellipses[i].size*0.5f, 
			ellipses[i].angle, 0, 360, 
			cv::Scalar(0,255,255), 1, CV_AA
		);
	}
	cv::imshow("image",img);
	cv::waitKey();
	return 0;
}