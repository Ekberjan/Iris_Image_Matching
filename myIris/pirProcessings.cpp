#include "stdafx.h"
#include "opencv\cv.h"
#include "pirStringUtils.h"
#include "pirProcessings.h"
#include <iostream>

namespace Proline
{

	pirProcessings::pirProcessings()
	{
		// Do nothing
	}

	pirProcessings::~pirProcessings()
	{
		// Do nothing
	}

	void pirProcessings::segment(const cv::Mat & pSrc,
		cv::Mat & pMask,
		pirCircle & rPupil,
		pirCircle & rIris,
		std::vector<float> & rThetaCoarsePupil,
		std::vector<float> & rThetaCoarseIris,
		std::vector<cv::Point> & rCoarsePupilContour,
		std::vector<cv::Point> & rCoarseIrisContour,
		int minIrisDiameter,
		int minPupilDiameter,
		int maxIrisDiameter,
		int maxPupilDiameter)
	{
		// Check arguments 
		//////////////////

		// String functions
		pirStringUtils str;

		// Temporary int to check sizes of pupil and iris
		int check_size = 0;

		// Default value for maxIrisDiameter if user did not specify it
		if (maxIrisDiameter == 0)
		{
			maxIrisDiameter = std::min(pSrc.rows, pSrc.cols);
		}

		//change maxIrisDiameter if it is too big relative to image sizes
		else if (maxIrisDiameter > (check_size = std::floor((float)std::min(pSrc.rows, pSrc.cols))))
		{
			std::cout << "Warning in function pirProcessings::segment : maxIrisDiameter = " << maxIrisDiameter;
			std::cout << " is replaced by " << check_size;
			std::cout << " because image size is " << pSrc.rows << " x " << pSrc.cols << std::endl;
			maxIrisDiameter = check_size;
		}

		// Default value for maxPupilDiameter if user did not specify it
		if (maxPupilDiameter == 0)
		{
			maxPupilDiameter = PIR_MAX_RATIO_PUPIL_IRIS * maxIrisDiameter;
			std::cout << "maxPupilDiameter: " << maxPupilDiameter << "            maxIrisDiameter: " << maxIrisDiameter << std::endl;
		}

		// Change maxPupilDiameter if it is too big relative to maxIrisDiameter and PIR_MAX_RATIO_PUPIL_IRIS
		else if (maxPupilDiameter > (check_size = PIR_MAX_RATIO_PUPIL_IRIS * maxIrisDiameter))
		{
			std::cout << "Warning in function pirProcessings::segment : maxPupilDiameter = " << maxPupilDiameter;
			std::cout << " is replaced by " << check_size;
			std::cout << " because maxIrisDiameter = " << maxIrisDiameter;
			std::cout << " and ratio pupil/iris is generally lower than " << PIR_MAX_RATIO_PUPIL_IRIS << std::endl;
			maxPupilDiameter = check_size;
		}

		// Change minIrisDiameter if it is too small relative to PIR_SMALLEST_IRIS
		if (minIrisDiameter < (check_size = PIR_SMALLEST_IRIS))
		{
			std::cout << "Warning in function pirProcessings::segment : minIrisDiameter = " << minIrisDiameter;
			std::cout << " is replaced by " << check_size;
			std::cout << " which is the smallest size for detecting iris. " << std::endl;
			minIrisDiameter = check_size;
		}

		// Change minPupilDiameter if it is too small relative to minIrisDiameter and PIR_MIN_RATIO_PUPIL_IRIS
		if (minPupilDiameter < (check_size = minIrisDiameter * PIR_MIN_RATIO_PUPIL_IRIS))
		{
			std::cout << "Warning in function pirProcessings::segment : minPupilDiameter = " << minPupilDiameter;
			std::cout << " is replaced by " << check_size;
			std::cout << " because minIrisDiameter = " << minIrisDiameter;
			std::cout << " and ratio pupil/iris is generally upper than " << PIR_MIN_RATIO_PUPIL_IRIS << std::endl;
			minIrisDiameter = check_size;
		}

		// Check that minIrisDiameter < maxIrisDiameter
		if (minIrisDiameter > maxIrisDiameter)
		{
			throw std::invalid_argument("Error in function pirProcessings::segment : minIrisDiameter = " +
				str.toString(minIrisDiameter) +
				" should be lower than maxIrisDiameter = " + str.toString(maxIrisDiameter));
		}

		// Make size oods
		minIrisDiameter += (minIrisDiameter % 2) ? 0 : -1;
		maxIrisDiameter += (maxIrisDiameter % 2) ? 0 : +1;
		minPupilDiameter += (minPupilDiameter % 2) ? 0 : -1;
		maxPupilDiameter += (maxPupilDiameter % 2) ? 0 : +1;

		// Start processing
		///////////////////

		// Locate the pupil
		detectPupil(pSrc, rPupil, minPupilDiameter, maxPupilDiameter);

		// Fill the holes in an area surrounding pupil
		cv::Mat clone_src = pSrc.clone();
		cv::Rect mRoi = cv::Rect(rPupil.getCenter().x - 3.0 / 4.0 * maxIrisDiameter / 2.0,
			rPupil.getCenter().y - 3.0 / 4.0*maxIrisDiameter / 2.0,
			3.0 / 4.0*maxIrisDiameter,
			3.0 / 4.0*maxIrisDiameter);

		cv::Mat clone_src2 = clone_src(mRoi).clone();
		fillWhiteHoles(clone_src2, clone_src2);
		clone_src2.copyTo(clone_src(mRoi));

		// reset image roi, double check here
		//...
		//////////////////////////////////////
		float theta_step = 0;

		// Pupil Accurate Contour
		/////////////////////////


		// Will contain samples of angles, in radians
		std::vector<float> theta;
		theta.clear();
		theta_step = 360.0 / PIR_PI / rPupil.getRadius();
		for (float t = 0; t < 360; t += theta_step)
		{
			theta.push_back(t*PIR_PI / 180);
		}

		//cv::Mat pMask = cv::Mat(0, 0, pSrc.type());
		std::vector<cv::Point> pupil_accurate_contour = findContour(clone_src,
			rPupil.getCenter(),
			theta,
			rPupil.getRadius() - 20,
			rPupil.getRadius() + 20,
			pMask, false
			);

		// Circle the fitting on accurate contour
		rPupil.computeCircleFitting(pupil_accurate_contour);

		// Pupil Coarse Contour
		////////////////////////

		theta.clear();
		theta_step = 360.0 / PIR_PI / rPupil.getRadius() * 2;
		for (float t = 0; t < 360; t += theta_step)
		{
			if (t > 45 && t < 135) t += theta_step;
			theta.push_back(t*PIR_PI / 180);
		}

		std::vector<cv::Point> pupil_coarse_contour = findContour(clone_src,
			rPupil.getCenter(),
			theta,
			rPupil.getRadius() - 20,
			rPupil.getRadius() + 20,
			pMask, false);

		rThetaCoarsePupil = theta;
		rCoarsePupilContour = pupil_coarse_contour;

		// Circle fitting on coarse contour

		rPupil.computeCircleFitting(pupil_accurate_contour);

		// Mask of pupil
		////////////////

		cv::Mat mask_pupil = pSrc.clone();
		mask_pupil.setTo(0);
		drawContour(mask_pupil, pupil_accurate_contour, cv::Scalar(255), -1);

		// Iris coarse contour
		///////////////////////

		theta.clear();
		int min_radius = std::max<int>(rPupil.getRadius() / PIR_MAX_RATIO_PUPIL_IRIS, minIrisDiameter / 2);
		int max_radius = std::min<int>(rPupil.getRadius() / PIR_MIN_RATIO_PUPIL_IRIS, 3 * maxIrisDiameter / 4);

		if (min_radius >= max_radius)
		{
			std::runtime_error("Cannot detect iris pupil correctly. ");
		}

		theta_step = 360.0 / PIR_PI / min_radius;
		for (float t = 0; t < 360; t += theta_step)
		{
			if (t < 180 || (t > 225 && t < 315)) t += 2 * theta_step;
			theta.push_back(t*PIR_PI / 180);
		}

		try
		{
			std::vector<cv::Point> iris_coarse_contour = findContour(clone_src,
				rPupil.getCenter(),
				theta,
				min_radius,
				max_radius, pMask, false);

			rThetaCoarseIris = theta;
			rCoarseIrisContour = iris_coarse_contour;

			// Circle fitting on coarse contour
			rIris.computeCircleFitting(iris_coarse_contour);

			// Mask of iris
			////////////////

			cv::Mat mask_iris = mask_pupil.clone();
			mask_iris.setTo(0);
			drawContour(mask_iris, iris_coarse_contour, cv::Scalar(255), -1);

			// Iris Accurate Contour 
			/////////////////////////

			// For iris accurate contour, limit the search of contour inside a mask
			// mask = dilate(mask-iris) - dilate(mask_pupil)

			// Dilate mask of iris by a disk-shape element
			cv::Mat mask_iris2 = mask_iris.clone();
			cv::Mat struct_element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21), cv::Point(10, 10));
			cv::dilate(mask_iris2, mask_iris2, struct_element);

			// Dilate the mask of pupil by a horizontal line-shape element
			cv::Mat mask_pupil2 = mask_pupil.clone();
			cv::Mat struct_element2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21), cv::Point(10, 1));
			cv::dilate(mask_pupil2, mask_pupil2, struct_element2);

			cv::bitwise_xor(mask_iris2, mask_pupil2, mask_iris2);

			theta.clear();
			theta_step = 360.0 / PIR_PI / rIris.getRadius();
			for (float t = 0; t < 360; t += theta_step)
			{
				theta.push_back(t * PIR_PI / 180);
			}

			std::vector<cv::Point> iris_accurate_contour = findContour(clone_src,
				rPupil.getCenter(),
				theta,
				rIris.getRadius() - 50,
				rIris.getRadius() + 20,
				mask_iris2, true);

			// Release memory
			mask_pupil2.release();
			mask_iris2.release();

			// Mask of iris based on accurate contours
			//////////////////////////////////////////
			mask_iris.setTo(0);
			drawContour(mask_iris, iris_accurate_contour, cv::Scalar(255), -1);
			cv::bitwise_xor(mask_iris, mask_pupil, mask_iris);

			// Refine the mask by removing some noise
			/////////////////////////////////////////

			// Build a safe area = avoid occlusions
			cv::Mat safe_area = mask_iris.clone();
			cv::rectangle(safe_area, cv::Point(0, 0), cv::Point(safe_area.cols - 1, rPupil.getCenter().y), cv::Scalar(0), -1);
			cv::rectangle(safe_area, cv::Point(0, rPupil.getCenter().y + rPupil.getRadius()),
				cv::Point(safe_area.cols - 1, safe_area.rows - 1), cv::Scalar(0), -1);
			struct_element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(11, 11), cv::Point(5, 5));
			cv::erode(safe_area, safe_area, struct_element);

			// Compute the mean and the variance of iris texture inside safe area
			cv::Scalar iris_mean = cv::mean(pSrc, safe_area);
			cv::Mat variance = cv::Mat(pSrc.size(), CV_32FC1);
			//pSrc.copyTo(variance);
			pSrc.convertTo(variance, CV_32FC1);

			cv::subtract(variance, cv::Scalar(iris_mean), variance, safe_area);
			cv::multiply(variance, variance, variance);

			float iris_variance = std::sqrt(cv::mean(variance, safe_area)[0]);

			variance.release();
			safe_area.release();

			// Build mask of noise : |I-mean| > 2.35 * variance
			cv::Mat mask_noise = pSrc.clone();
			cv::absdiff(pSrc, cv::Scalar(iris_mean), mask_noise);
			cv::threshold(mask_noise, mask_noise, 2.35*iris_variance, 255, CV_THRESH_BINARY);
			cv::bitwise_and(mask_iris, mask_noise, mask_noise);

			// Fusion with accurate contours
			cv::Mat accurate_contours = mask_iris.clone();
			struct_element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(1, 1));
			cv::morphologyEx(accurate_contours, accurate_contours, cv::MORPH_GRADIENT, struct_element);
			reconstructMarkerByMask(accurate_contours, mask_noise, mask_noise);
			accurate_contours.release();
			cv::bitwise_xor(mask_iris, mask_noise, pMask);

			mask_noise.release();
			mask_pupil.release();
			mask_iris.release();

		}
		catch (std::exception e)
		{
			std::cout << e.what() << std::endl;
		}

	} // end of function segment

	void pirProcessings::normalize(const cv::Mat & pSrc, cv::Mat & pDst, const pirCircle & rPupil, const pirCircle & rIris)
	{
		// Local variables
		cv::Point point_pupil, point_iris;
		int x, y;
		float theta, radius;

		pDst.setTo(0);

		for (int j = 0; j < pDst.cols; j++)
		{
			theta = (float)j / pDst.cols * 2 * PIR_PI;

			// Coordinates relative to both centers : iris and pupil
			point_pupil = convertPolarToCartesian(rPupil.getCenter(), rPupil.getRadius(), theta);
			point_iris = convertPolarToCartesian(rIris.getCenter(), rIris.getRadius(), theta);

			// Loop on lines of normalized src
			for (int i = 0; i < pDst.rows; i++)
			{
				// The radial parameter
				radius = (float)i / pDst.rows;

				// Coordinates relative to both radii : iris and pupil
				x = (1 - radius) * point_pupil.x + radius * point_iris.x;
				y = (1 - radius) * point_pupil.y + radius * point_iris.y;

				// Do not exceed src size
				if (x >= 0 && x < pSrc.cols && y > 0 && y < pSrc.rows)
				{
					((uchar*)(pDst.data + i*pDst.step))[j] = ((uchar*)(pSrc.data + y*pSrc.step))[x];
				}
			} // end of i-loop
		} // end of j-loop

	} // end of function definition

	void pirProcessings::normalizeFromContour(const cv::Mat & pSrc,
		cv::Mat & pDst,
		const pirCircle & rPupil,
		const pirCircle & rIris,
		const std::vector<float> rThetaCoarsePupil,
		const std::vector<float> rThetaCoarseIris,
		const std::vector<cv::Point> & rPupilCoarseContour,
		const std::vector<cv::Point> & rIrisCoarseContour)
	{
		// Local variables
		cv::Point point_pupil, point_iris;
		int x, y;
		float theta, radius;

		// Set all pixels to zero
		pDst.setTo(0);

		// Loop on columns of normalized src
		for (int j = 0; j < pDst.cols; j++)
		{
			// One column corresponds to an angle theta
			theta = (float)j / pDst.cols * 2 * PIR_PI;

			// Interpolate pupil and iris radii from coarse contours
			point_pupil = interpolate(rPupilCoarseContour, rThetaCoarsePupil, theta);
			point_iris = interpolate(rIrisCoarseContour, rThetaCoarseIris, theta);

			// Loop on lines of normalized src
			for (int i = 0; i < pDst.rows; i++)
			{
				// The radial parameter
				radius = (float)i / pDst.rows;

				// Coordinates relative to both radii : iris and pupil
				x = (1 - radius) * point_pupil.x + radius * point_iris.x;
				y = (1 - radius) * point_pupil.y + radius * point_iris.y;

				// Do not exceed src size
				if (x >= 0 && x < pSrc.cols && y >= 0 && y <= pSrc.rows)
				{
					//((uchar*)(pDst.data + i*pDst.step))[j] = ((uchar*)(pSrc.data + y*pSrc.step))[x];
					pDst.at<uchar>(i, j) = pSrc.at<uchar>(y, x);
				}

			} // end of i-loop
		} // end of j-loop

	} // end of function definition

	cv::Point pirProcessings::interpolate(const std::vector<cv::Point> coarseContour,
		const std::vector<float> coarseTheta,
		const float theta)
	{
		float interpolation;
		int i1, i2;

		if (theta < coarseTheta[0])
		{
			i1 = coarseTheta.size() - 1;
			i2 = 0;
			interpolation = (theta - (coarseTheta[i1] - 2 * PIR_PI)) / (coarseTheta[i2] - (coarseTheta[i1] - 2 * PIR_PI));
		} // end of if

		else if (theta >= coarseTheta[coarseTheta.size() - 1])
		{
			i1 = coarseTheta.size() - 1;
			i2 = 0;
			interpolation = (theta - coarseTheta[i1]) / (coarseTheta[i2] + 2 * PIR_PI - coarseTheta[i1]);
		}
		else
		{
			int i = 0;
			while (coarseTheta[i + 1] <= theta) i++;
			i1 = i;
			i2 = i + 1;
			interpolation = (theta - coarseTheta[i1]) / (coarseTheta[i2] - coarseTheta[i1]);
		}

		float x = (1 - interpolation) * coarseContour[i1].x + interpolation * coarseContour[i2].x;
		float y = (1 - interpolation) * coarseContour[i1].y + interpolation * coarseContour[i2].y;

		return cv::Point(x, y);
	} // end of function definition

	void pirProcessings::encode(const cv::Mat & pSrc,
		cv::Mat & pDst,
		const std::vector<cv::Mat> & rFilters)
	{
		int max_width = 0;
		for (int f = 0; f < rFilters.size(); f++)
		{
			if (rFilters[f].cols > max_width)
			{
				max_width = rFilters[f].cols;
			}
		} // end of f-loop

		max_width = (max_width - 1) / 2;

		// Add wrapping borders on the left and right of image for convolution 
		cv::Mat & resized = addBorders(pSrc, max_width);

		// Temporary images to store the result of convolution
		cv::Mat img1 = cv::Mat(resized.size(), CV_32FC1);
		cv::Mat img2 = cv::Mat(resized.size(), CV_32FC1);

		// Loop on filters
		for (int f = 0; f < rFilters.size(); f++)
		{
			// Convolution 
			cv::filter2D(resized, img1, CV_32FC1, rFilters[f]);

			// Threshold : above or below 0
			cv::threshold(img1, img2, 0, 255, CV_THRESH_BINARY);

			// Form the iris code
			cv::Mat tmpImg2(img2(cv::Rect(max_width, 0, pSrc.cols, pSrc.rows)));
			cv::Mat dstImg2(pDst(cv::Rect(0, f*pSrc.rows, pSrc.cols, pSrc.rows)));
			//tmpImg2.copyTo(dstImg2);
			tmpImg2.convertTo(dstImg2, pDst.type());
		} // end of f-loop

		img1.release();
		img2.release();
		resized.release();

	} // end of function definition

	float pirProcessings::match(const cv::Mat & image1,
		const cv::Mat & image2,
		const cv::Mat & mask)
	{
		// Temporary matrix to store the XOR result
		cv::Mat result = cv::Mat(image1.size(), CV_8UC1);
		result.setTo(0);
		// Add borders on the image1 in order to shift it
		int shift = 10;
		cv::Mat shifted = addBorders(image1, shift);

		// The minimum score will be returned
		float score = 1;

		// Shift image1 and compare to image2 
		for (int s = -shift; s <= shift; s++)
		{
			cv::Mat tmpShifted(shifted(cv::Rect(shift + s, 0, image1.cols, image1.rows)));
			cv::bitwise_xor(tmpShifted, image2, result, mask);
			float mean = (cv::sum(result).val[0]) / (cv::sum(mask).val[0]);
			score = std::min(score, mean);

		} // end of s-loop

		// Free memory
		shifted.release();
		result.release();

		return score;
	} // end of function definition

	///////////////////////////////////
	// PRIVATE METHODS
	///////////////////////////////////


	// Convert polar coordinates into cartesian coordinates
	cv::Point pirProcessings::convertPolarToCartesian(const cv::Point & rCenter,
		int rRadius,
		float rTheta)
	{
		int x = rCenter.x + rRadius * cos(rTheta);
		int y = rCenter.y - rRadius * sin(rTheta);

		return cv::Point(x, y);
	}

	// Add left and right borders on an unwrapped image
	cv::Mat pirProcessings::addBorders(const cv::Mat & pImage, int width)
	{
		// Result image
		cv::Mat result = cv::Mat(pImage.rows, pImage.cols + 2 * width, pImage.type());

		// Copy the image in the center
		cv::copyMakeBorder(pImage, result, 0, 0, width, width, cv::BORDER_REPLICATE, cv::Scalar(0)); // double-check this part

		// Create the borders left and right assuming wrapping
		for (int i = 0; i < pImage.rows; i++)
		{
			for (int j = 0; j < width; j++)
			{
				result.at<uchar>(i, j) = pImage.at<uchar>(i, pImage.cols - width + j);
				result.at<uchar>(i, result.cols - width + j) = pImage.at<uchar>(i, j);
			} // end of j-loop
		} // end of i-loop

		return result;
	} // end of function

	// Detect and locate a pupil inside an eye image
	void pirProcessings::detectPupil(const cv::Mat & pSrc,
		pirCircle & rPupil,
		int minPupilDiameter,
		int maxPupilDiameter)
	{
		// Check arguments
		//////////////////

		// String functions
		pirStringUtils str;

		// Default value for maxPupilDiameter, if user did not specify it
		if (maxPupilDiameter == 0)
		{
			maxPupilDiameter = std::min(pSrc.rows, pSrc.cols) * PIR_MAX_RATIO_PUPIL_IRIS;
		}

		// Change maxPupilDiameter if it is too big relative to the image size and the ratio pupil/iris
		else if (maxPupilDiameter > std::min(pSrc.rows, pSrc.cols) * PIR_MAX_RATIO_PUPIL_IRIS)
		{
			int newmaxPupilDiameter = std::floor(std::min(pSrc.rows, pSrc.cols) * PIR_MAX_RATIO_PUPIL_IRIS);
			std::cout << "Warning in function pirProcessings::detectPupil() : maxPupilDiameter = " << maxPupilDiameter;
			std::cout << "is replaced by " << newmaxPupilDiameter;
			std::cout << " because image size is " << pSrc.cols << " x " << pSrc.rows;
			std::cout << " and ratio pupil/iris is generally lower than " << PIR_MAX_RATIO_PUPIL_IRIS << std::endl;
			maxPupilDiameter = newmaxPupilDiameter;
		} // end of else if

		// Change minPupilDiameter if it is too small relative to PIR_SMALLEST_PUPIL
		if (minPupilDiameter < PIR_SMALLEST_PUPIL)
		{
			std::cout << "Warning i function pirProcessings::detectPupil() : minPupilDiameter = " << minPupilDiameter;
			std::cout << " is replaced by " << PIR_SMALLEST_PUPIL;
			std::cout << " which is the smallest size for detecting pupil. " << std::endl;
			minPupilDiameter = PIR_SMALLEST_PUPIL;
		} // end of if

		// Check that minPupilDiameter < maxPupilDiameter
		if (minPupilDiameter >= maxPupilDiameter)
		{
			throw std::invalid_argument("Error in function pirProcessings::detectPupil() : minPupilDiameter = " +
				str.toString(minPupilDiameter) +
				" should be lower than maxPupilDiameter + " +
				str.toString(maxPupilDiameter));
		} // end of if

		// Start processing
		///////////////////

		// Resize image (downsample)
		float scale = (float)PIR_SMALLEST_PUPIL / minPupilDiameter;
		cv::Mat & resized = cv::Mat(pSrc.rows * scale, pSrc.cols *scale, pSrc.type());
		cv::resize(pSrc, resized, cv::Size(pSrc.cols * scale, pSrc.rows * scale));

		// Rescale sizes
		maxPupilDiameter = maxPupilDiameter * scale;
		minPupilDiameter = minPupilDiameter * scale;

		// Make sizes odd
		maxPupilDiameter += (maxPupilDiameter % 2) ? 0 : +1;
		minPupilDiameter += (minPupilDiameter % 2) ? 0 : -1;

		// Fill holes
		cv::Mat filled = cv::Mat(resized.size(), resized.type());
		fillWhiteHoles(resized, filled);

		// Gradients in horizontal direction
		cv::Mat gh = cv::Mat(filled.size(), CV_32FC1);
		cv::Sobel(filled, gh, CV_32FC1, 1, 0);

		// Gradients in vertical direction
		cv::Mat gv = cv::Mat(filled.size(), CV_32FC1);
		cv::Sobel(filled, gv, CV_32FC1, 0, 1);

		// Normalize gradients
		cv::Mat gh2 = cv::Mat(filled.size(), CV_32FC1);
		cv::multiply(gh, gh, gh2);

		cv::Mat gv2 = cv::Mat(filled.size(), CV_32FC1);
		cv::multiply(gv, gv, gv2);

		cv::Mat gn = cv::Mat(filled.size(), CV_32FC1);
		cv::add(gh2, gv2, gn);

		gn.convertTo(gn, CV_32FC1);
		cv::pow(gn, 0.5, gn);
		cv::divide(gh, gn, gh, 1, CV_32FC1);
		cv::divide(gv, gn, gv, 1, CV_32FC1);

		// Create the filters fh and fv
		int filter_size = maxPupilDiameter;

		filter_size += (filter_size % 2) ? 0 : -1;
		cv::Mat fh = cv::Mat(filter_size, filter_size, CV_32FC1);
		cv::Mat fv = cv::Mat(filter_size, filter_size, CV_32FC1);

		for (int i = 0; i < fh.rows; i++)
		{
			float x = i - (filter_size - 1) / 2;
			for (int j = 0; j < fh.cols; j++)
			{
				float y = j - (filter_size - 1) / 2;
				if (x != 0 || y != 0)
				{
					fh.at<float>(i, j) = y / std::sqrt(x*x + y*y);
					fv.at<float>(i, j) = x / std::sqrt(x*x + y*y);
				}
				else
				{
					fh.at<float>(i, j) = 0;
					fv.at<float>(i, j) = 0;
				} // end of if-else

			} // end of j-loop
		} // end of i-loop

		// Create the mask
		cv::Mat mask = cv::Mat(filter_size, filter_size, CV_8UC1);

		// Temporary matrix for masking the filter (later : tempfilter = filter * mask)
		cv::Mat temp_filter = cv::Mat(filter_size, filter_size, CV_32FC1);
		double old_max_val = 0;

		// Multi resolution of radius 
		for (int r = (PIR_SMALLEST_PUPIL - 1) / 2; r < (maxPupilDiameter - 1) / 2; r++)
		{
			// Centered ring with radius = r and width = 2
			mask.setTo(0);
			cv::circle(mask, cv::Point((filter_size - 1) / 2, (filter_size - 1) / 2), r, cv::Scalar(1), 2);

			// Fh * Gh
			temp_filter.setTo(0);
			fh.copyTo(temp_filter, mask);
			cv::filter2D(gh, gh2, CV_32FC1, temp_filter);

			// Fv * Gv
			temp_filter.setTo(0);
			fv.copyTo(temp_filter, mask);
			cv::filter2D(gv, gv2, -1, temp_filter);

			// Fv*Gh + Fv*Gv
			cv::add(gh2, gv2, gn);
			gn.convertTo(gn, CV_32FC1, 1.0 / cv::sum(mask).val[0]);

			// Sum in the disk-shaped neighbourhood
			mask.setTo(0);
			cv::circle(mask, cv::Point((filter_size - 1) / 2, (filter_size - 1) / 2), r, cv::Scalar(1), -1);
			cv::filter2D(filled, gh2, CV_32FC1, mask);

			gh2.convertTo(gh2, gn.type(), -1.0 / cv::sum(mask).val[0] / 255.0, 1);

			// Add the two features : contour + darkness
			cv::add(gn, gh2, gn);

			// Find the maximum in feature image
			double max_val;
			cv::Point max_loc;
			cv::minMaxLoc(gn, 0, &max_val, 0, &max_loc);

			if (max_val > old_max_val)
			{
				old_max_val = max_val;
				rPupil.setCircle(max_loc, r);
			} // end of if-condition

		} // end of r-loop

		// Rescale circle
		int x = ((float)(rPupil.getCenter().x * (pSrc.cols - 1))) / (filled.cols - 1) + (float)((1.0 / scale) - 1) / 2;
		int y = ((float)(rPupil.getCenter().y * (pSrc.rows - 1))) / (filled.rows - 1) + (float)((1.0 / scale) - 1) / 2;
		int r = rPupil.getRadius() / scale;
		rPupil.setCircle(x, y, r);

		// Release memory
		resized.release();
		filled.release();
		gh.release();
		gv.release();
		gh2.release();
		gv2.release();
		gn.release();
		fh.release();
		fv.release();
		mask.release();
		temp_filter.release();

	} // end of function definition

	// Morphological reconstruction
	void pirProcessings::reconstructMarkerByMask(const cv::Mat & pMarker, const cv::Mat & pMask, cv::Mat & pDst)
	{
		// Temporary image that will inform about marker evolution
		cv::Mat difference = pMask.clone();

		// :WARNING: if user calls f(x,y,y) instead of f(x,y,z), the mask MUST be cloned before processing
		cv::Mat mask = pMask.clone();

		// Copy the marker
		pMarker.copyTo(pDst);

		// Structuring element for morphological operation
		cv::Mat structuring_element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(1, 1));

		// Will stop when marker does not change anymore 
		while (cv::sum(difference).val[0])
		{
			// Remind marker before processing, in order to 
			// compare  with the marker after processing
			pDst.copyTo(difference);

			// Dilate the marker
			cv::dilate(pDst, pDst, structuring_element);

			// Keep the minimum between marker and mask
			cv::min(pDst, mask, pDst);

			// Evolution of the marker
			cv::absdiff(pDst, difference, difference);
		} // end of while

		mask.release();
		difference.release();
		structuring_element.release();

	} // end of function defnition

	// Fill the white holes surrounded by dark pixels, such as specular reflection inside pupil area
	void pirProcessings::fillWhiteHoles(const cv::Mat & pSrc, cv::Mat & pDst)
	{
		int width = pSrc.cols;
		int height = pSrc.rows;

		// Mask for reconstruction : pSrc + borders = 0
		cv::Mat mask = cv::Mat::zeros(height + 2, width + 2, pSrc.type());
		cv::Mat mask2 = mask(cv::Rect(1, 1, width, height)); // double-check this line
		//pSrc.copyTo(mask, mask2);
		pSrc.copyTo(mask2);

		// Marker for reconstruction : all = 0 + borders = 255
		cv::Mat marker = mask.clone();
		marker.setTo(0);
		cv::rectangle(marker, cv::Point(1, 1), cv::Point(width + 1, height + 1), cv::Scalar(255));

		// Temporary result of reconstruction
		cv::Mat result = mask.clone();

		// Morphological reconstruction
		reconstructMarkerByMask(marker, mask, result);

		// Remove borders
		cv::Mat result2 = result(cv::Rect(1, 1, width, height));
		result2.copyTo(pDst);

		marker.release();
		mask.release();
		result.release();
	} // end of function definition 

	// Rescale between 0 and 255, and show image
	void pirProcessings::showImage(const cv::Mat & pImage, int delay, const std::string & rWindowName)
	{
		cv::Mat show;
		if (pImage.channels() == 1)
		{
			// Rescale between 0 and 255 by computing : (X-min)/(max - min)
			double min_val, max_val;
			cv::minMaxLoc(pImage, &min_val, &max_val);

			cv::Mat scaled = pImage.clone();
			pImage.convertTo(scaled, pImage.type(), 255 / (max_val - min_val), -min_val / (max_val - min_val));

			// Convert into 8-bit
			show = cv::Mat(pImage.size(), CV_8UC1);
			scaled.convertTo(show, CV_8UC1);

			scaled.release();

		} // end of if-condition
		else
		{
			show = pImage.clone();
		} // end of else

		cv::imshow(rWindowName, show);
		cv::waitKey(delay);

		show.release();


	} // end of function definition

	// Unwrap a ring into a rectangular band
	cv::Mat pirProcessings::unwrapRing(const cv::Mat & pSrc,
		const cv::Point & rCenter, int minRadius, int maxRadius,
		const std::vector<float> & rTheta)
	{
		// Result image
		cv::Mat result = cv::Mat::zeros(cv::Size(rTheta.size(), maxRadius - minRadius + 1), pSrc.type());

		if (result.channels() != 1) cv::cvtColor(result, result, CV_RGB2GRAY);

		// Loop on lines of normalized image
		for (int j = 0; j < result.cols; j++)
		{
			for (int i = 0; i < result.rows; i++)
			{
				cv::Point point = convertPolarToCartesian(rCenter, minRadius + i, rTheta[j]);

				// Do not exceed image size
				if (point.x >= 0 && point.x < pSrc.cols && point.y >= 0 & point.y < pSrc.rows)
				{
					result.at<uchar>(i, j) = pSrc.at<uchar>(point.y, point.x); // double-check this line					
				} // end of if-condition

			} // end of i-loop
		} // end of j-loop

		//fillWhiteHoles(result, result);
		return result;
	} // end of function definition

	// Smooth the image by anisotropic smoothing (Gross & Brajovic,2003)
	void pirProcessings::processAnisotropicSmoothing(const cv::Mat & pSrc,
		cv::Mat & pDst, int iterations, float lambda)
	{
		// Temporary float images 
		cv::Mat tfs = cv::Mat(pSrc.size(), CV_32FC1);
		pSrc.convertTo(tfs, CV_32FC1);

		cv::Mat tfd = cv::Mat(pSrc.size(), CV_32FC1);
		pSrc.convertTo(tfd, CV_32FC1);

		// Make borders dark
		cv::rectangle(tfd, cv::Point(0, 0), cv::Point(tfd.cols - 1, tfd.rows - 1), cv::Scalar(0));

		// Weber coefficients
		float rhon, rhos, rhoe, rhow;

		// Store pixel values
		float tfsc, tfsn, tfss, tfse, tfsw, tfdn, tfds, tfde, tfdw;

		// Loop on iterations
		for (int k = 0; k < iterations; k++)
		{
			// Odd pixels
			for (int i = 1; i < tfs.rows - 1; i++)
			{
				for (int j = 2 - i % 2; j < tfs.cols - 1; j += 2)
				{
					// Get pixels in neighborhood of original image
					tfsc = tfs.at<float>(i, j);
					tfsn = tfs.at<float>(i - 1, j);
					tfss = tfs.at<float>(i + 1, j);
					tfse = tfs.at<float>(i, j - 1);
					tfsw = tfs.at<float>(i, j + 1);

					// Get pixels in neighbourhood of light image
					tfdn = tfd.at<float>(i - 1, j);
					tfds = tfd.at<float>(i + 1, j);
					tfde = tfd.at<float>(i, j - 1);
					tfdw = tfd.at<float>(i, j + 1);

					// Compute weber coefficients
					rhon = std::min(tfsn, tfsc) / std::max<float>(1.0, std::abs(tfsn - tfsc));
					rhos = std::min(tfss, tfsc) / std::max<float>(1.0, std::abs(tfss - tfsc));
					rhoe = std::min(tfse, tfsc) / std::max<float>(1.0, std::abs(tfsw - tfsc));
					rhow = std::min(tfsw, tfsc) / std::max<float>(1.0, std::abs(tfsw - tfsc));

					// Compute LightImage(i, j)
					tfd.at<float>(i, j) = ((tfsc + lambda * (rhon * tfdn + rhos * tfds + rhoe * tfde + rhow * tfdw)) /
						(1 + lambda * (rhon + rhos + rhoe + rhow)));
				} // end of j-loop
			} // end of i-loop

			tfd.copyTo(tfs);

			// Even pixels
			for (int i = 1; i < tfs.rows - 1; i++)
			{
				for (int j = 1 + i % 2; j < tfs.cols - 1; j += 2)
				{
					// Get pixels in neighborhood of original image
					tfsc = tfs.at<float>(i, j);
					tfsn = tfs.at<float>(i - 1, j);
					tfss = tfs.at<float>(i + 1, j);
					tfse = tfs.at<float>(i, j - 1);
					tfsw = tfs.at<float>(i, j + 1);

					// Get pixels in neighborhood of light image
					tfdn = tfd.at<float>(i - 1, j);
					tfds = tfd.at<float>(i + 1, j);
					tfde = tfd.at<float>(i, j - 1);
					tfdw = tfd.at<float>(i, j + 1);

					// Compute weber coefficients
					rhon = std::min(tfsn, tfsc) / std::max<float>(1.0, std::abs(tfsn - tfsc));
					rhos = std::min(tfss, tfsc) / std::max<float>(1.0, std::abs(tfss - tfsc));
					rhoe = std::min(tfse, tfsc) / std::max<float>(1.0, std::abs(tfse - tfsc));
					rhow = std::min(tfsw, tfsc) / std::max<float>(1.0, std::abs(tfsw - tfsc));

					// Compute LightImage(i, j)
					tfd.at<float>(i, j) = ((tfsc + lambda * (rhon * tfdn + rhos * tfds + rhoe * tfde + rhow * tfdw)) /
						(1 + lambda * (rhon + rhos + rhoe + rhow)));
				} // end of j-loop
			} // end of i-loop

			tfd.copyTo(tfs);
			//tfd.copyTo(pDst);
			tfd.convertTo(pDst, CV_8UC1);

		} // end of k-loop

		// Border of image
		for (int i = 0; i < tfd.rows; i++)
		{
			pDst.at<uchar>(i, 0) = pDst.at<uchar>(i, 1);
			pDst.at<uchar>(i, pDst.cols - 1) = pDst.at<uchar>(i, pDst.cols - 2);
		} // end of i-loop

		for (int j = 0; j < tfd.cols; j++)
		{
			pDst.at<uchar>(0, j) = pDst.at<uchar>(1, j);
			pDst.at<uchar>(pDst.rows - 1, j) = pDst.at<uchar>(pDst.rows - 2, j);
		} // end of j-loop

		tfs.release();
		tfd.release();

	} // end of function definition

	// Compute vertical gradients using Sobel operator
	void pirProcessings::computeVeticalGradients(const cv::Mat& pSrc, cv::Mat& pDst)
	{
		// Float values for Sobel
		cv::Mat& result_sobel = cv::Mat(pSrc.size(), CV_32FC1);

		// Sobel filter in vertical direction
		cv::Sobel(pSrc, result_sobel, CV_32FC1, 0, 1);

		// Remove negative edges, ie fro white (top) to black (bottom)
		cv::threshold(result_sobel, result_sobel, 0, 0, CV_THRESH_TOZERO);

		// Convert into 8-bit
		double min, max;
		cv::minMaxLoc(result_sobel, &min, &max);

		result_sobel.convertTo(pDst, pDst.type(), 255 / (max - min), -255 * min / (max - min));

		result_sobel.release();

	} // end of function definition

	// Run viterbi algorithm on gradient (or probability) image and find optimal path
	void pirProcessings::runViterbi(const cv::Mat & pSrc, std::vector<int> & rOptimalPath)
	{
		// Initialize the output
		rOptimalPath.clear();
		rOptimalPath.resize(pSrc.cols);

		// Initialize cost matrix to zero
		cv::Mat cost = cv::Mat::zeros(pSrc.size(), CV_32FC1);

		// Forward process : build the cost matrix
		for (int w = 0; w < pSrc.cols; w++)
		{
			for (int h = 0; h < pSrc.rows; h++)
			{
				// First column is same as source image
				if (w == 0)
				{
					cost.at<float>(h, w) = static_cast<float>(pSrc.at<uchar>(h, w));
				} // end of if
				else
				{
					// First line
					if (h == 0)
					{
						cost.at<float>(h, w) = std::max<float>(cost.at<float>(h, w - 1), cost.at<float>(h + 1, w - 1)) +
							static_cast<float>((pSrc.at<uchar>(h, w)));
					} // end of fist line if

					// last line
					else if (h == pSrc.rows - 1)
					{
						cost.at<float>(h, w) = std::max<float>(cost.at<float>(h, w - 1), cost.at<float>(h - 1, w - 1)) +
							static_cast<float>((pSrc.at<uchar>(h, w)));
					} // end of last line else if

					// Middle lines
					else
					{
						cost.at<float>(h, w) = std::max<float>(cost.at<float>(h, w - 1), std::max<float>(
							cost.at<float>(h + 1, w - 1), cost.at<float>(h - 1, w - 1))) +
							static_cast<float>((pSrc.at<uchar>(h, w)));
					} // end of else
				} // end of else
			} // end of h-loop
		} // end of w-loop

		// Get the maximum in last column of cost matrix
		cv::Mat subCost = cost(cv::Rect(cost.cols - 1, 0, 1, cost.rows)); // double-check this line

		cv::Point max_loc;
		cv::minMaxLoc(subCost, 0, 0, 0, &max_loc);
		int h = max_loc.y;
		int h0 = h;

		// Store the point in output vector
		rOptimalPath[rOptimalPath.size() - 1] = h0;

		float h1, h2, h3;

		// Backward process
		for (int w = rOptimalPath.size() - 2; w >= 0; w--)
		{
			// Constraint to close the contour
			if (h - h0 > w) h--;
			else if (h0 - h > w) h++;

			// When no need to constraint : use the cost matrix
			else
			{
				// h1 is the value above line h
				h1 = (h == 0) ? 0 : cost.at<float>(h - 1, w);

				// h2 is the value at line h
				h2 = cost.at<float>(h, w);

				// h3 is the value below line h
				h3 = (h == cost.rows - 1) ? 0 : cost.at<float>(h + 1, w);

				// h1 is the maximum => h decreases
				if (h1 > h2 && h1 > h3) h--;

				// h3 is the maximum => h increases
				else if (h3 > h2 & h3 > h2) h++;

			} // end of else

			// Store the point in output contour
			rOptimalPath[w] = h;

		} // end of w-loop

		cost.release();

	} // end of function definition

	// Find a contour in image using Viterbi algorithm and anisotropic smoothing
	std::vector<cv::Point> pirProcessings::findContour(const cv::Mat & pSrc,
		const cv::Point & rCenter,
		const std::vector<float> & rTheta,
		int minRadius, int maxRadius,
		const cv::Mat & pMask, bool hasMask)
	{

		// Output
		std::vector<cv::Point> contour;
		contour.resize(rTheta.size());

		// Unwrap the image
		cv::Mat unwrapped = unwrapRing(pSrc, rCenter, minRadius, maxRadius, rTheta);

		// Smooth image
		processAnisotropicSmoothing(unwrapped, unwrapped, 100, 1);

		// Extract the gradients
		computeVeticalGradients(unwrapped, unwrapped);

		// Take into account the mask
		if (hasMask)
		{
			cv::Mat mask_unwrapped = unwrapRing(pMask, rCenter, minRadius, maxRadius, rTheta);
			cv::Mat temp = unwrapped.clone();

			unwrapped.setTo(0);
			temp.copyTo(unwrapped, mask_unwrapped);

			temp.release();
			mask_unwrapped.release();

		} // end of if-condition

		// Find optimal path in unwrapped image
		std::vector<int> optimalPath;
		runViterbi(unwrapped, optimalPath);

		for (int i = 0; i < optimalPath.size(); i++)
		{
			contour[i] = convertPolarToCartesian(rCenter, minRadius + optimalPath[i], rTheta[i]);
		} // end of i-loop

		unwrapped.release();

		return contour;

	} // end of function defnition


	// Draw a contour (vector of cv::Point) on an image
	void pirProcessings::drawContour(cv::Mat& pImage, const std::vector<cv::Point>& rContour,
		const cv::Scalar & rColor, int thickness)
	{
		// Draw INSIDE the contour if  thickness is negative
		if (thickness < 0)
		{
			int sze = rContour.size();
			std::vector<cv::Point> points;
			for (int i = 0; i < rContour.size(); i++)
			{
				points.push_back(rContour[i]);
			} // end of i-loop

			cv::fillConvexPoly(pImage, &points[0], rContour.size(), rColor);
		} // end of if-condition

		// Else draw the contour
		else
		{
			// Draw the contour on binary mask
			cv::Mat mask = cv::Mat::zeros(pImage.size(), CV_8UC1);
			for (int i = 0; i < rContour.size(); i++)
			{
				// Do not exceed image sizes
				int x = std::min(std::max(0, rContour[i].x), pImage.cols);
				int y = std::min(std::max(0, rContour[i].y), pImage.rows);

				// Plot the point on image
				mask.at<uchar>(y, x) = 255;
			} // end of i-loop

			// Dilate mask if user specified thickness
			if (thickness > 1)
			{
				cv::Mat se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(1, 1));
				cv::dilate(mask, mask, se, cv::Point(-1, -1), thickness - 1);
				se.release();
			} // end of if

			// Color rgb
			pImage.setTo(rColor, mask);

			// Release memory
			mask.release();

		} // end of else condition


	} // end of function definition


} // end of namespace