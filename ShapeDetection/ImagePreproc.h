#ifndef IMAGEPREPROC_H
#define IMAGEPREPROC_H

class ImagePreproc
{
public:
	static void preprocess(const cv::Mat& src,
		                         cv::Mat& dst);

/************************************************************************/
/* 工具类设计，构造函数设为私有，不允许构造                                     */
/************************************************************************/
private:
	ImagePreproc(){}
};

#endif