#include "EllipseDetector.h"
#include "IShapeDetector.h"
std::vector<cv::RotatedRect> EllipseDetector::excute(cv::Mat src) 
{
	/*
	 * 图像的预处理
	 */
	cv::Mat prepocessed_src = this->prepocess(src);
	/*
	 * 提取图像的边缘
	 */
	cv::Mat edge = this->getEdge(prepocessed_src);
	/*
	 * 粗提取椭圆
	 */
	std::vector<cv::RotatedRect> raw_ellipses = this->getRawEllipses(edge);
	/*
	 * 对粗提取的椭圆进行校正
	 */
	std::vector<cv::RotatedRect> final_ellipses = this->tuneRawEliipses(raw_ellipses, src);
	/*
	 * 返回结果
	 */
	return final_ellipses;
}

cv::Mat EllipseDetector::prepocess(cv::Mat src) 
{
	cv::Mat img_smooth;
	//double kernel_length = 25.0;
	//cv::bilateralFilter(src, img_smooth, 5,kernel_length*2,kernel_length/2);
	cv::GaussianBlur(src, img_smooth, cv::Size(5,5), 0);
	return img_smooth;
}

cv::Mat EllipseDetector::getEdge(cv::Mat prepocessed_src) 
{
	/*-----------------比较重要的参数---------------*/
	double grad_thresh = 35;
	/*-------------------------------------------*/
	// 计算图像各个方向的梯度
	cv::Mat grad_x, grad_y, grad;
	cv::Sobel(prepocessed_src, grad_x, prepocessed_src.depth(), 1, 0);
	cv::Sobel(prepocessed_src, grad_y, prepocessed_src.depth(), 0, 1);
	// 剔除一些噪声梯度点，数学形态学做闭运算
	grad_x = (grad_x > grad_thresh) * 255;
	grad_y = (grad_y > grad_thresh) * 255;
	cv::Mat se(5,5,CV_8U,cv::Scalar(1));
	grad   = cv::abs(grad_x) + cv::abs(grad_y);
	morphologyEx(grad,grad,cv::MORPH_CLOSE,se);
	// 利用大津法计算阈值，利用canny提取边界
	cv::Mat edge;
	double high_threshold = cv::threshold(prepocessed_src,cv::Mat(), 0, 255, cv::THRESH_OTSU);
	cv::Canny(prepocessed_src, edge, 0.5 * high_threshold, high_threshold, 7);
	edge = edge.mul(grad);
	//cv::imshow("grad", grad);
	//cv::imshow("edge", edge);
	return edge;
}

std::vector<cv::RotatedRect> EllipseDetector::getRawEllipses(cv::Mat edge)
{
	/*-----------------比较重要的参数---------------*/
	unsigned int min_contour_size = 200;
	double center_threshold = 5.0;
	double distance_threshold = 10.0;
	/*-------------------------------------------*/
	std::vector<std::vector<cv::Point> > contours;                          
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(edge, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
	std::vector<cv::RotatedRect> boxes;
	cv::Point2f mean_center(0.0,0.0);
	for (unsigned int i=0; i < contours.size(); ++i)
	{
		if (contours[i].size() > min_contour_size) {
			cv::Mat pointsf;
			cv::Mat(contours[i]).convertTo(pointsf, CV_32F);
			cv::RotatedRect box = fitEllipse(pointsf);
			if( MAX(box.size.width, box.size.height) > MIN(box.size.width, box.size.height)*30 )
				continue;
			mean_center += box.center;
			boxes.push_back(box);
		} 
	}
	mean_center.x /= (float) boxes.size();
	mean_center.y /= (float) boxes.size();
	std::vector<cv::RotatedRect> raw_ellipses;
	/*
	 ** 对中心做一个聚类,实际上要做一个k-means，不过这边偷偷懒
	 */
	for (unsigned int i = 0; i < boxes.size(); ++i) 
	{
		float x_diff = this->diff(
			boxes[i].center.x,
			mean_center.x
		);
		float y_diff = this->diff(
			boxes[i].center.y,
			mean_center.y
		);
		if ( x_diff < center_threshold && y_diff < center_threshold) 
			raw_ellipses.push_back(boxes[i]);
	}
	/*
	 * 根据轴的最大值先进行一个排序
	 */
	raw_ellipses = this->sort(raw_ellipses);
	/*
	 * 剔除太相近的圆，取最大的那个
	 */
	std::vector<cv::RotatedRect> final_raw_ellipse;
	final_raw_ellipse.push_back(raw_ellipses[0]);
	for (unsigned int i = 0; i < raw_ellipses.size(); ++i)
	{
		float height_diff = this->diff(
			final_raw_ellipse[final_raw_ellipse.size()-1].size.height,
			raw_ellipses[i].size.height
		);
		float width_diff = this->diff(
			final_raw_ellipse[final_raw_ellipse.size()-1].size.width,
			raw_ellipses[i].size.width
		);
		if ( height_diff > distance_threshold && width_diff  > distance_threshold)
			final_raw_ellipse.push_back(raw_ellipses[i]);
	}
	return final_raw_ellipse;
}


std::vector<cv::RotatedRect> EllipseDetector::tuneRawEliipses(std::vector<cv::RotatedRect> raw_ellipses, cv::Mat src)
{
	/*-----------------比较重要的参数---------------*/
	int LENGTH = 1;
	int WINSIZE = 10;
	/*-------------------------------------------*/
	std::vector<cv::RotatedRect> tune_ellipses;
	int DEGREE = 360;
	cv::Mat grad_x, grad_y, grad;
	cv::Sobel(src, grad_x, src.depth(), 1, 0);
	cv::Sobel(src, grad_y, src.depth(), 0, 1);
	grad   = cv::abs(grad_x) + cv::abs(grad_y);
	for (unsigned int i = 0; i < raw_ellipses.size(); ++i)
	{
		std::vector<cv::Point> tuneContour;
		for (int j = 0; j < DEGREE; ++j)
		{
			float angle = raw_ellipses[i].angle;
			double A  = raw_ellipses[i].size.height * 0.5;
			double B  = raw_ellipses[i].size.width  * 0.5;
			float x0 = raw_ellipses[i].center.x;
			float y0 = raw_ellipses[i].center.y;
			float cosTHETA = std::cos(j - angle);
			float sinTHETA = std::sin(j - angle);
			double x = x0 + A * cosTHETA;
			double y = y0 + B * sinTHETA;
			unsigned int x_index = (unsigned int) x;
			unsigned int y_index = (unsigned int) y;
			// 沿着方向对椭圆进行校正
			std::vector<double> Variants;
			cv::Mat mean, var;
			for (int l = -LENGTH; l <= LENGTH; ++l) 
			{
				int x_moved = (int)(x + l * cosTHETA);
				int y_moved = (int)(y + l * sinTHETA);
				if ( x_moved - WINSIZE >= 0 &&
					 y_moved - WINSIZE >= 0 &&
					 x_moved + WINSIZE < src.size().height &&
					 y_moved + WINSIZE < src.size().width)
				{
					
					cv::meanStdDev(
						grad.rowRange(int(x_moved)-WINSIZE, int(x_moved)+WINSIZE).
						     colRange(int(y_moved)-WINSIZE, int(y_moved)+WINSIZE), 
						mean, var
					);
					Variants.push_back(var.at<double>(0));
				}
			}
			
			if (Variants.size() != 0) 
			{
				int index = this->findMaxIndex(Variants);
				double tune_x = x + (index - LENGTH) * cosTHETA;
				double tune_y = y + (index - LENGTH) * sinTHETA;
				tuneContour.push_back(cv::Point2f((float)tune_x, (float)tune_y));
			}
		}
		/*---------重新拟合椭圆--------*/
		cv::Mat pointsf;
		cv::Mat(tuneContour).convertTo(pointsf, CV_32F);
		cv::RotatedRect box = fitEllipse(pointsf);
		tune_ellipses.push_back(box);
	}
	return tune_ellipses;
}


std::vector<cv::RotatedRect> EllipseDetector::sort(std::vector<cv::RotatedRect> src)
{
	for (unsigned int i = 0; i < src.size(); ++i)
	{
		for (unsigned int j = i+1; j < src.size(); ++j) 
		{
			if( MAX(src[i].size.width, src[j].size.height) >
				MAX(src[i].size.width, src[j].size.height) )
			{
				cv::RotatedRect tmp = src[i];
				src[i] = src[j];
				src[j] = tmp;
			}
		}
	}
	return src;
}

int EllipseDetector::findMaxIndex(std::vector<double> src)
{
	int index = 0;
	for (unsigned int i = 0; i < src.size() - 1; ++i)
	{
		if (src[i] > src[i+1])
		{
			index = i;
			src[i+1] = src[i];
		}
	}
	return index;
}
