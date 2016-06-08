#include "stdafx.h"
#include <fstream>
#include <stdexcept>
#include "opencv\cv.h"
#include "pirEye.h"
#include "pirProcessings.h"

namespace Proline
{
	// CONSTRUCTOR & DESTRUCTOR
	///////////////////////////
	pirEye::pirEye()
	{
		mpOriginalImage = 0;
		mpSegmentedImage = 0;
		mpMask = 0;
		mpNormalizedImage = 0;
		mpNormalizedMask = 0;
		mpIrisCode = 0;
		mPupil.setCircle(0, 0, 0);
		mIris.setCircle(0, 0, 0);
	} // end of constructor

	pirEye::~pirEye()
	{
		mpOriginalImage.release();
		mpSegmentedImage.release();
		mpMask.release();
		mpNormalizedImage.release();
		mpNormalizedMask.release();
		mpIrisCode.release();
	} // end of destructor

	// Functions for loading images and parameters
	//////////////////////////////////////////////
	void pirEye::loadImage(const std::string & rFilename, cv::Mat & ppImage)
	{
		// :WARNING: ppImage is a pointer of pointer
		try
		{
			if (ppImage.cols <= 0 || ppImage.rows <= 0)
			{
				ppImage.release();
			} // end of if

			ppImage = cv::imread(rFilename, 0);

			if (!&ppImage)
			{
				std::cout << "Cannot load image : " << rFilename << std::endl;
			} // end of if-condition

		}
		catch (std::exception e)
		{
			std::cout << "Cannot load image. " << std::endl;
		} // end of try-catch

	} // end of function definition

	void pirEye::loadOriginalImage(const std::string & rFilename)
	{
		loadImage(rFilename, mpOriginalImage);
	} // end of function definition

	void pirEye::loadMask(const std::string & rFilename)
	{
		loadImage(rFilename, mpMask);
	} // end of function definition

	void pirEye::loadNormalizedImage(const std::string & rFilename)
	{
		loadImage(rFilename, mpNormalizedImage);
	} // end of function definition

	void pirEye::loadNormalizedMask(const std::string & rFilename)
	{
		loadImage(rFilename, mpNormalizedMask);
	} // end of function definition

	void pirEye::loadIrisCode(const std::string & rFilename)
	{
		loadImage(rFilename, mpIrisCode);
	} // end of function definition

	// Load parameters
	void pirEye::loadParameters(const std::string & rFilename)
	{
		// Open the file
		std::ifstream file(rFilename, std::ios::in);

		// If file is not opened
		if (!file.is_open())
		{
			throw std::runtime_error("Cannot load the parameters in " + rFilename);
		} // end of if
		try
		{
			int nbp = 0;
			int nbi = 0;
			file >> nbp;
			file >> nbi;
			mThetaCoarsePupil.resize(nbp, 0.0);
			mThetaCoarseIris.resize(nbi, 0.0);
			mCoarsePupilContour.resize(nbp, cv::Point(0, 0));
			mCoarseIrisContour.resize(nbi, cv::Point(0, 0));

			for (int i = 0; i < nbp; i++)
			{
				file >> mCoarsePupilContour[i].x;
				file >> mCoarsePupilContour[i].y;
				file >> mThetaCoarsePupil[i];
			} // end of i-loop

			for (int j = 0; j < nbi; j++)
			{
				file >> mCoarseIrisContour[j].x;
				file >> mCoarseIrisContour[j].y;
				file >> mThetaCoarseIris[j];
			} // end of j-loop
		} // end of try
		catch (std::exception & e)
		{
			std::cout << e.what() << std::endl;
			throw std::runtime_error("Error wile loading parameters from " + rFilename);
		} // end of catch

		// Close the file
		file.close();

	} // end of function definition

	// Functions for saving images and parameters
	/////////////////////////////////////////////
	void pirEye::saveImage(const std::string & rFilename, const cv::Mat & pImage)
	{
		// :TODO: no exception here, but 2 error messages
		// 1. pImage does NOT exist => "image was neither comptued nor loaded"
		// 2. cvSaveImage returns <=0 => "rFilename = invalid for saving"
		if (pImage.cols <= 0 || pImage.rows <= 0)
		{
			throw std::runtime_error("Cannot save image " + rFilename + " because this image is not built. ");
		}

		if (!cv::imwrite(rFilename, pImage))
		{
			std::cout << "Cannot save image as " << rFilename << std::endl;
		}

	} // end of function definition

	void pirEye::saveSegmentedImage(const std::string & rFilename)
	{
		saveImage(rFilename, mpSegmentedImage);
	}

	void pirEye::saveMask(const std::string & rFilename)
	{
		saveImage(rFilename, mpMask);
	}

	void pirEye::saveNormalizedImage(const std::string & rFilename)
	{
		saveImage(rFilename, mpNormalizedImage);
	}

	void pirEye::saveNormalizedMask(const std::string & rFilename)
	{
		saveImage(rFilename, mpNormalizedMask);
	}

	void pirEye::saveIrisCode(const std::string & rFilename)
	{
		saveImage(rFilename, mpIrisCode);
	}

	void pirEye::saveParameters(const std::string & rFilename)
	{
		// Open the file
		std::ofstream file(rFilename, std::ios::in);

		// If file isnot opened
		if (!file.is_open())
		{
			throw std::runtime_error("Cannot save the parameters in " + rFilename);
		}

		try
		{
			file << mCoarsePupilContour.size() << std::endl;
			file << mCoarseIrisContour.size() << std::endl;
			for (int i = 0; i < (mCoarsePupilContour.size()); i++)
			{
				file << mCoarsePupilContour[i].x << " ";
				file << mCoarsePupilContour[i].y << " ";
				file << mThetaCoarsePupil[i] << " ";
			}

			file << std::endl;
			for (int j = 0; j < (mCoarseIrisContour.size()); j++)
			{
				file << mCoarseIrisContour[j].x << " ";
				file << mCoarseIrisContour[j].y << " ";
				file << mThetaCoarseIris[j] << " ";
			}
		}
		catch (std::exception e)
		{
			std::cout << e.what() << std::endl;
			throw std::runtime_error("Error while saving parameters in " + rFilename);
		}

		// Close the file
		file.close();
	} // end of function defnition

	// Functions for processings
	////////////////////////////

	void pirEye::initMask()
	{
		if (mpMask.rows > 0 || mpMask.cols > 0)
		{
			mpMask.release();
		}

		if (mpOriginalImage.rows <= 0 || mpOriginalImage.cols <= 0)
		{
			throw std::runtime_error("Cannot initialize the mask because original image is not loaded. ");
		}

		mpMask = cv::Mat(mpOriginalImage.size(), CV_8UC1);
		mpMask.setTo(255);

	} // end of function definition

	void pirEye::segment(int minIrisDiameter, int minPupilDiameter, int maxIrisDiameter, int maxPupilDiameter)
	{
		if (mpOriginalImage.empty())
		{
			throw std::runtime_error("Cannot segment image because origianl image is not loaded. ");
		}

		// Initialize mask and segmented image
		mpMask = cv::Mat::zeros(mpOriginalImage.size(), CV_8UC1);
		mpSegmentedImage = cv::Mat(mpOriginalImage.size(), CV_8UC3);
		
		cv::cvtColor(mpOriginalImage, mpSegmentedImage, CV_GRAY2BGR);

		// Processing functions
		pirProcessings op;

		// Segment the eye
		op.segment(mpOriginalImage, mpMask, mPupil, mIris, mThetaCoarsePupil, mThetaCoarseIris, mCoarsePupilContour, mCoarseIrisContour, minIrisDiameter, minPupilDiameter, maxIrisDiameter, maxPupilDiameter);

		// Draw on segmented image
		cv::Mat tmp = mpMask.clone();
		tmp.setTo(0);
		cv::circle(tmp, mIris.getCenter(), mIris.getRadius(), cv::Scalar(255), -1);
		cv::circle(tmp, mPupil.getCenter(), mPupil.getRadius(), cv::Scalar(0), -1);
		cv::subtract(tmp, mpMask, tmp);
		mpSegmentedImage.setTo(cv::Scalar(0, 0, 255), tmp);
		tmp.release();
		cv::circle(mpSegmentedImage, mPupil.getCenter(), mPupil.getRadius(), cv::Scalar(0, 255, 0));
		cv::circle(mpSegmentedImage, mIris.getCenter(), mIris.getRadius(), cv::Scalar(0, 255, 0));
	} // end of function definition

	void pirEye::normalize(int rWidthOfNormalizedIris, int rHeightOfNormalizedIris)
	{
		// Processing functions
		pirProcessings op;

		// For the image
		if (mpOriginalImage.empty())
		{
			throw std::runtime_error("Cannot normalize image because original image is not loaded. ");
		}

		mpNormalizedImage = cv::Mat(rHeightOfNormalizedIris, rWidthOfNormalizedIris, CV_8UC1);

		if (mThetaCoarsePupil.empty() || mThetaCoarseIris.empty())
		{
			std::cout << mThetaCoarsePupil.size() << "           " << mThetaCoarseIris.size() << std::endl;
			throw std::runtime_error("Cannot normalize image because contours are not correctly computed/loaded");
		}

		op.normalizeFromContour(mpOriginalImage, mpNormalizedImage, mPupil, mIris, mThetaCoarsePupil, mThetaCoarseIris, mCoarsePupilContour, mCoarseIrisContour);

		// For the mask
		if (mpMask.empty())
		{
			initMask();
		}

		mpNormalizedMask = cv::Mat(rHeightOfNormalizedIris, rWidthOfNormalizedIris, CV_8UC1);
		op.normalizeFromContour(mpMask, mpNormalizedMask, mPupil, mIris, mThetaCoarsePupil, mThetaCoarseIris, mCoarsePupilContour, mCoarseIrisContour);

	} // end of function definition

	void pirEye::encode(const std::vector<cv::Mat> & rGaborFilters)
	{
		if (mpNormalizedImage.empty())
		{
			throw std::runtime_error("Cannot encode because normalized image is not loaded. ");
		}

		// Create the image to store the iris code
		cv::Size size = mpNormalizedImage.size();
		mpIrisCode = cv::Mat(size.height * rGaborFilters.size(), size.width, CV_8UC1);

		// Encode 
		pirProcessings op;
		op.encode(mpNormalizedImage, mpIrisCode, rGaborFilters);

	} // end of function definition

	float pirEye::match(pirEye & rEye, const cv::Mat & pApplicationPoints)
	{
		// Check that both iris codes are built
		if (mpIrisCode.empty())
		{
			throw std::runtime_error("Cannot match because iris code 1 is not built (nor computed neither loaded)");
		}

		if (rEye.mpIrisCode.empty())
		{
			throw std::runtime_error("Cannot match because iris code 2 not built (nor cmoputed neither loaded)");
		}

		// Initialize the normalized masks
		// :TODO: must inform the user of this step, for example if user provides masks for all images
		// but one is missing for only one image. However, message must not be spammed if the user
		// did not provide any mask ! So it must be found a way to inform user but without spamming
		if (mpNormalizedMask.empty())
		{
			mpNormalizedMask = cv::Mat(pApplicationPoints.size(), CV_8UC1);
			mpNormalizedMask.setTo(255);
		}

		if (rEye.mpNormalizedMask.empty())
		{
			rEye.mpNormalizedMask = cv::Mat(pApplicationPoints.size(), CV_8UC1);
			rEye.mpNormalizedMask.setTo(255);
		}

		// Build the total mask = mask1 * mask2 * points
		cv::Mat temp = cv::Mat::zeros(pApplicationPoints.size(), mpIrisCode.type());
		if (temp.channels() != 1)
		{
			cv::cvtColor(temp, temp, CV_BGR2GRAY);
		}

		temp.setTo(0);

		cv::bitwise_and(mpNormalizedMask, rEye.mpNormalizedMask, temp, pApplicationPoints);

		// Copy the mask f times, where f correspond to the number of codes (= number of filters)
		int n_codes = mpIrisCode.rows / pApplicationPoints.rows;
		cv::Mat total_mask = cv::Mat(mpIrisCode.size(), CV_8UC1);

		for (int n = 0; n < n_codes; n++)
		{
			cv::Mat total_mask2(total_mask(cv::Rect(0, n*pApplicationPoints.rows, pApplicationPoints.cols, pApplicationPoints.rows)));
			temp.copyTo(total_mask2); // double-check this part
		}

		// Match 
		pirProcessings op;

		cv::Mat mat1;
		cv::subtract(mpIrisCode, rEye.mpIrisCode, mat1);
		float score = op.match(mpIrisCode, rEye.mpIrisCode, total_mask);

		// Free memory 
		temp.release();
		total_mask.release();
		rEye.mpIrisCode.release();
		mat1.release();
		mpNormalizedMask.release();
		mpNormalizedImage.release();
		//mpIrisCode.release();
		mpMask.release();
		mpOriginalImage.release();
		mpSegmentedImage.release();
		mPupil.~pirCircle();
		mIris.~pirCircle();

		return score;

	} // end of function definition

} // end of namespace