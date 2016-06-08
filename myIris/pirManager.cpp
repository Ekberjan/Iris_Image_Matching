#include "stdafx.h"
#include "pirManager.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include "pirStringUtils.h"

namespace Proline
{


	// CONSTRUCTORS & DESTRUCTORS
	/////////////////////////////


	// Default constructor
	pirManager::pirManager()
	{
		// Associate lines of configuration file to the attributes
		mMapBool["Process segmentation"] = &mProcessSegmentation;
		mMapBool["Process normalization"] = &mProcessNormalization;
		mMapBool["Process encoding"] = &mProcessEncoding;
		mMapBool["Process matching"] = &mProcessMatching;
		mMapBool["Use the mask provided by osiris"] = &mUseMask;
		mMapString["List of images"] = &mFilenameListOfImages;
		mMapString["Load original images"] = &mInputDirOriginalImages;
		mMapString["Load parameters"] = &mInputDirParameters;
		mMapString["Load masks"] = &mInputDirMasks;
		mMapString["Load normalized images"] = &mInputDirNormalizedImages;
		mMapString["Load normalized masks"] = &mInputDirNormalizedMasks;
		mMapString["Load iris codes"] = &mInputDirIrisCodes;
		mMapString["Save segmented images"] = &mOutputDirSegmentedImages;
		mMapString["Save contours parameters"] = &mOutputDirParameters;
		mMapString["Save masks of iris"] = &mOutputDirMasks;
		mMapString["Save normalized images"] = &mOutputDirNormalizedImages;
		mMapString["Save normalized masks"] = &mOutputDirNormalizedMasks;
		mMapString["Save iris codes"] = &mOutputDirIrisCodes;
		mMapString["Save matching scores"] = &mOutputFileMatchingScores;
		mMapInt["Minimum diameter for pupil"] = &mMinPupilDiameter;
		mMapInt["Maximum diameter for pupil"] = &mMaxPupilDiameter;
		mMapInt["Minimum diameter for iris"] = &mMinIrisDiameter;
		mMapInt["Maximum diameter for iris"] = &mMaxIrisDiameter;
		mMapInt["Width of normalized image"] = &mWidthOfNormalizedIris;
		mMapInt["Height of normalized image"] = &mHeightOfNormalizedIris;
		mMapString["Gabor filters"] = &mFilenameGaborFilters;
		mMapString["Application points"] = &mFilenameApplicationPoints;
		mMapString["Suffix for segmented images"] = &mSuffixSegmentedImages;
		mMapString["Suffix for parameters"] = &mSuffixParameters;
		mMapString["Suffix for masks of iris"] = &mSuffixMasks;
		mMapString["Suffix for normalized images"] = &mSuffixNormalizedImages;
		mMapString["Suffix for normalized masks"] = &mSuffixNormalizedMasks;
		mMapString["Suffix for iris codes"] = &mSuffixIrisCodes;

		// Initialize all parameters
		initConfiguration();

	}  // end of default constructor

	pirManager::~pirManager()
	{
		// Release matrix for application points
		if (!mpApplicationPoints.empty())
		{
			mpApplicationPoints.release();
		}

		// Release matrix for Gabor filters
		for (int f = 0; f < mGaborFilters.size(); f++)
		{
			mGaborFilters[f].release();
		}
	} // end of function definition

	// OPERATORS
	////////////



	// Initialize all configuration parameters
	void pirManager::initConfiguration()
	{
		// Options of processing
		mProcessSegmentation = false;
		mProcessNormalization = false;
		mProcessEncoding = false;
		mProcessMatching = false;
		mUseMask = true;

		// Inputs
		mListOfImages.clear();
		mFilenameListOfImages = "";
		mInputDirOriginalImages = "";
		mInputDirMasks = "";
		mInputDirParameters = "";
		mInputDirNormalizedImages = "";
		mInputDirNormalizedMasks = "";
		mInputDirIrisCodes = "";

		// Outputs
		mOutputDirSegmentedImages = "";
		mOutputDirParameters = "";
		mOutputDirMasks = "";
		mOutputDirNormalizedImages = "";
		mOutputDirNormalizedMasks = "";
		mOutputDirIrisCodes = "";
		mOutputFileMatchingScores = "";

		// Parameters
		mMinPupilDiameter = 21;
		mMaxPupilDiameter = 91;
		mMinIrisDiameter = 99;
		mMaxIrisDiameter = 399;
		mWidthOfNormalizedIris = 512;
		mHeightOfNormalizedIris = 64;
		mFilenameGaborFilters = "./filters.txt";
		mFilenameApplicationPoints = "./points.txt";
		mGaborFilters.clear();
		mpApplicationPoints = 0;

		// Suffix for filenames
		mSuffixSegmentedImages = "_segm.bmp";
		mSuffixParameters = "_para.txt";
		mSuffixMasks = "_mask.bmp";
		mSuffixNormalizedImages = "_imno.bmp";
		mSuffixNormalizedMasks = "_mano.bmp";
		mSuffixIrisCodes = "_code.bmp";

	} // end of function definition

	// Load the configuration from a textfile (ini)
	void pirManager::loadConfiguration(const std::string & rFilename)
	{
		// Open the file
		std::ifstream file(rFilename, std::ifstream::in);

		if (!file.good())
		{
			throw std::runtime_error("Cannot read configuration file " + rFilename);
		}

		// Some string functions
		pirStringUtils osu;

		// Loop on lines
		while (file.good() && !file.eof())
		{
			// Get the new line
			std::string line;
			std::getline(file, line);

			// Filter out comments
			if (!line.empty())
			{
				int pos = line.find('#');
				if (pos != std::string::npos)
				{
					line = line.substr(0, pos);
				}
			}

			// Split line into key and value
			if (!line.empty())
			{
				int pos = line.find("=");

				if (pos != std::string::npos)
				{
					// Trim key and value
					std::string key = osu.trim(line.substr(0, pos));
					std::string value = osu.trim(line.substr(pos + 1));

					if (!key.empty() && !value.empty())
					{
						// Option is type bool
						if (mMapBool.find(key) != mMapBool.end())
						{
							*mMapBool[key] = osu.fromString<bool>(value);
						}

						// Option is type int
						else if (mMapInt.find(key) != mMapInt.end())
						{
							*mMapInt[key] = osu.fromString<int>(value);
						}

						// Option is type string 
						else if (mMapString.find(key) != mMapString.end())
						{
							*mMapString[key] = osu.convertSlashes(value);
						}

						// Option is not stored in any mMap
						else
						{
							std::cout << "Unknown option in configuration file. " << line << std::endl;
						}

					}
				}
			}

		} // end of while-loop

		// Load the list containing all images
		loadListOfImages();

		// Load the datas for Gabor filters
		if (mProcessEncoding && mFilenameGaborFilters != "")
		{
			loadGaborFilters();
		}

		// Load the application points
		if (mProcessMatching && mFilenameApplicationPoints != "")
		{
			loadApplicationPoints();
		}

	} // end of function definition


	// Show the configuration of pirIris in prompt command
	void pirManager::showConfiguration()
	{
		std::cout << "=============" << std::endl;
		std::cout << "Configuration" << std::endl;
		std::cout << "=============" << std::endl;

		std::cout << std::endl;

		std::cout << "- Process : ";
		if (mProcessSegmentation)
		{
			std::cout << "| segmentation |";
		}
		if (mProcessNormalization)
		{
			std::cout << "| normalization |";
		}
		if (mProcessEncoding)
		{
			std::cout << "| encoding |";
		}
		if (mProcessMatching)
		{
			std::cout << "| matching |";
		}
		if (!mUseMask)
		{
			std::cout << " do not use osiris masks";
		}
		std::cout << std::endl;

		std::cout << "- List of images " << mFilenameListOfImages << " contains " << mListOfImages.size() << " images" << std::endl;

		std::cout << std::endl;

		if (mInputDirOriginalImages != "")
		{
			std::cout << "- Original images will be loaded from : " << mInputDirOriginalImages << std::endl;
		}
		if (mInputDirMasks != "")
		{
			std::cout << "- Masks will be loaded from : " << mInputDirMasks << std::endl;
		}
		if (mInputDirParameters != "")
		{
			std::cout << "- Parameters will be loaded from : " << mInputDirParameters << std::endl;
		}
		if (mInputDirNormalizedImages != "")
		{
			std::cout << "- Normalized images will be loaded from : " << mInputDirNormalizedImages << std::endl;
		}
		if (mInputDirNormalizedMasks != "")
		{
			std::cout << "- Normalized masks will be loaded from : " << mInputDirNormalizedMasks << std::endl;
		}
		if (mInputDirIrisCodes != "")
		{
			std::cout << "- Iris codes will be loaded from : " << mInputDirIrisCodes << std::endl;
		}

		std::cout << std::endl;

		if (mProcessSegmentation && mOutputDirSegmentedImages != "")
		{
			std::cout << "- Segmented images will be saved as : " << mOutputDirSegmentedImages << "XXX" << mSuffixSegmentedImages << std::endl;
		}
		if (mProcessSegmentation && mOutputDirParameters != "")
		{
			std::cout << "- Parameters will be saved as : " << mOutputDirParameters << "XXX" << mSuffixParameters << std::endl;
		}
		if (mProcessSegmentation && mOutputDirMasks != "")
		{
			std::cout << "- Masks will be saved as : " << mOutputDirMasks << "XXX" << mSuffixMasks << std::endl;
		}
		if (mProcessNormalization && mOutputDirNormalizedImages != "")
		{
			std::cout << "- Normalized images will be saved as : " << mOutputDirNormalizedImages << "XXX" << mSuffixNormalizedImages << std::endl;
		}
		if (mProcessNormalization && mOutputDirNormalizedMasks != "")
		{
			std::cout << "- Normalized masks will be saved as : " << mOutputDirNormalizedMasks << "XXX" << mSuffixNormalizedMasks << std::endl;
		}
		if (mProcessEncoding && mOutputDirIrisCodes != "")
		{
			std::cout << "- Iris codes will be saved as : " << mOutputDirIrisCodes << "XXX" << mSuffixIrisCodes << std::endl;
		}
		if (mProcessMatching && mOutputFileMatchingScores != "")
		{
			std::cout << "- Matching scores will be saved in : " << mOutputFileMatchingScores << std::endl;
		}

		std::cout << std::endl;

		if (mProcessSegmentation)
		{
			std::cout << "- Pupil diameter ranges from " << mMinPupilDiameter << " to " << mMaxPupilDiameter << std::endl;
			std::cout << "- Iris diameter ranges from " << mMinIrisDiameter << " to " << mMaxIrisDiameter << std::endl;
		}

		if (mProcessNormalization || mProcessMatching || mProcessEncoding)
		{
			std::cout << "- Size of normalized iris is " << mWidthOfNormalizedIris << " x " << mHeightOfNormalizedIris << std::endl;
		}

		std::cout << std::endl;

		if (mProcessEncoding && mGaborFilters.size())
		{
			std::cout << "- " << mGaborFilters.size() << " Gabor filters : ";
			for (int f = 0; f < mGaborFilters.size(); f++)
				std::cout << mGaborFilters[f].rows << "x" << mGaborFilters[f].cols << " ";
			std::cout << std::endl;
		}

		if (mProcessMatching && !mpApplicationPoints.empty())
		{
			double max_val;
			cv::minMaxLoc(mpApplicationPoints, 0, &max_val);
			std::cout << "- " << cv::sum(mpApplicationPoints).val[0] / max_val << " application points" << std::endl;
		}
	} // end of function definition

	// Load the Gabor filters (matrix coefficients) from a textfile
	void pirManager::loadGaborFilters()
	{
		// Open text file containing the filters
		std::ifstream file(mFilenameGaborFilters.c_str(), std::ios::in);
		if (!file)
		{
			throw std::runtime_error("Cannot load Gabor filters in file " + mFilenameGaborFilters);
		}

		// Get the number of filters
		int n_filters;
		file >> n_filters;
		mGaborFilters.resize(n_filters);

		// Size of the filter
		int rows, cols;

		// Loop on each filter
		for (int f = 0; f < n_filters; f++)
		{
			// Get the size of the filter
			file >> rows;
			file >> cols;

			// Temporary filter. Will be destroyed at the end of loop
			mGaborFilters[f] = cv::Mat(rows, cols, CV_32FC1);

			// Set the value at coordinates r, c
			for (int r = 0; r < rows; r++)
			{
				for (int c = 0; c < cols; c++)
				{
					file >> mGaborFilters[f].at<float>(r, c);
				} // end of c-loop
			} // end of r-loop

		} // end of f-loop

		// Close the file
		file.close();

	} // end of function definition

	// Load the application points (build a binary matrix) from a textfile
	void pirManager::loadApplicationPoints()
	{
		// Open text file containing the filters
		std::ifstream file(mFilenameApplicationPoints.c_str(), std::ios::in);
		if (!file)
		{
			throw std::runtime_error("Cannot load the application points in " + mFilenameApplicationPoints);
		}

		// Get the number of points
		int n_points = 0;
		file >> n_points;

		// Allocate memory for the matrix containing the points
		mpApplicationPoints = cv::Mat::zeros(mHeightOfNormalizedIris, mWidthOfNormalizedIris, CV_8UC1);

		// Initialize all pixels to "off"
		mpApplicationPoints.setTo(0);

		// Local variables
		int i, j;

		// Loop on each point
		for (int p = 0; p < n_points; p++)
		{
			// Get the coordinates
			file >> i; file >> j;

			// Set pixel to "on"
			if (i < 0 || i > mpApplicationPoints.rows - 1 || j < 0 || j > mpApplicationPoints.cols - 1)
			{
				std::cout << "Point (" << i << "," << j << ") ";
				std::cout << "exceeds size of normalized image : ";
				std::cout << mpApplicationPoints.rows << "x" << mpApplicationPoints.cols;
				std::cout << " while loading application points. " << std::endl;
			}
			else
			{
				mpApplicationPoints.at<uchar>(i, j) = static_cast<uchar>(255);
			}

		} // end of p-loop

		// Close the file
		file.close();


	} // end of function definition


	// Load the application points from a textfile
	void pirManager::loadListOfImages()
	{
		// Open the file
		std::ifstream file(mFilenameListOfImages.c_str(), std::ios::in);

		// If file is not opened
		if (!file)
		{
			std::cout << "mFilenameListOfImages: " << mFilenameListOfImages << std::endl;
			throw std::runtime_error("Cannot load the list of images in " + mFilenameListOfImages);
		}

		// Fill in the list
		std::copy(std::istream_iterator<std::string>(file), std::istream_iterator<std::string>(), std::back_inserter(mListOfImages));

		// Close the file
		file.close();

	} // end of function definition


	// Load, segment, normalize, encode, and save according to user configuration
	void pirManager::processOneEye(cv::Mat & inputImage, std::string short_name, pirEye & rEye)
	{
		// Strings handle
		pirStringUtils psu;

		// Get eye name
		//std::string short_name = psu.extractFileName(rFilename);

		// Load original image only if segmentation or normalization is requested
		if (mProcessSegmentation || mProcessNormalization)
		{
			if (mInputDirOriginalImages != "")
			{
				//rEye.loadOriginalImage(mInputDirOriginalImages + rFilename);
				//rEye.loadOriginalImage(rFilename);
				rEye.mpOriginalImage = inputImage;
			}
			else
			{
				throw std::runtime_error("Cannot segment/normalize without loading original image");
			}
		}


		/////////////////////////////////////////////////////////////////
		// SEGMENTATION : process, load
		/////////////////////////////////////////////////////////////////

		// Segmentation step
		if (mProcessSegmentation)
		{
			rEye.segment(mMinIrisDiameter, mMinPupilDiameter, mMaxIrisDiameter, mMaxPupilDiameter);

			// Save segmented image
			if (mOutputDirSegmentedImages != "")
			{
				rEye.saveSegmentedImage(mOutputDirSegmentedImages + short_name + mSuffixSegmentedImages);
			}


			// If user don't want to use the mask provided by pirIris
			if (!mUseMask)
			{
				rEye.initMask();
			}
		}

		// Load parameters
		if (mInputDirParameters != "")
		{
			rEye.loadParameters(mInputDirParameters + short_name + mSuffixParameters);
		}

		// Load mask
		if (mInputDirMasks != "")
		{
			rEye.loadMask(mInputDirMasks + short_name + mSuffixMasks);
		}



		/////////////////////////////////////////////////////////////////
		// NORMALIZATION : process, load
		/////////////////////////////////////////////////////////////////

		// Normalization step
		if (mProcessNormalization)
		{
			rEye.normalize(mWidthOfNormalizedIris, mHeightOfNormalizedIris);
		}

		// Load normalized image
		if (mInputDirNormalizedImages != "")
		{
			rEye.loadNormalizedImage(mInputDirNormalizedImages + short_name + mSuffixNormalizedImages);
		}

		// Load normalized mask
		if (mInputDirNormalizedMasks != "")
		{
			rEye.loadNormalizedMask(mInputDirNormalizedMasks + short_name + mSuffixNormalizedMasks);
		}

		/////////////////////////////////////////////////////////////////
		// ENCODING : process, load
		/////////////////////////////////////////////////////////////////

		// Encoding step
		if (mProcessEncoding)
		{
			rEye.encode(mGaborFilters);
		}

		// Load iris code
		if (mInputDirIrisCodes != "")
		{
			rEye.loadIrisCode(mInputDirIrisCodes + short_name + mSuffixIrisCodes);
		}

		/////////////////////////////////////////////////////////////////
		// SAVE
		/////////////////////////////////////////////////////////////////

		// Save parameters
		if (mOutputDirParameters != "")
		{
			if (!mProcessSegmentation && (mInputDirParameters == ""))
			{
				std::cout << "Cannot save parameters because they are neither computed nor loaded. " << std::endl;
			}
			else
			{
				rEye.saveParameters(mOutputDirParameters + short_name + mSuffixParameters);
			}
		}

		// Save mask
		if (mOutputDirNormalizedImages != "")
		{
			if (!mProcessNormalization && (mInputDirOriginalImages == ""))
			{
				std::cout << "Cannot save normalized images because they are neither computed nor loaded" << std::endl;
			}
			else
			{
				rEye.saveNormalizedImage(mOutputDirNormalizedImages + short_name + mSuffixNormalizedImages);
			}
		}

		// Save normalized mask
		if (mOutputDirNormalizedMasks != "")
		{
			if (!mProcessNormalization && (mInputDirNormalizedMasks == ""))
			{
				std::cout << "Cannot save normalized masks because they are neither computed nor loaded. " << std::endl;
			}
			else
			{
				rEye.saveNormalizedMask(mOutputDirNormalizedMasks + short_name + mSuffixNormalizedMasks);
			}
		}

		// Save iris code
		if (mOutputDirIrisCodes != "")
		{
			if (!mProcessEncoding && (mInputDirIrisCodes == ""))
			{
				std::cout << "Cannot save iris codes because they are neither computed nor loaded. " << std::endl;
			}
			else
			{
				rEye.saveIrisCode(mOutputDirIrisCodes + short_name + mSuffixIrisCodes);
			}
		}

	} // end of function definition


	// Run pirIris
	void pirManager::run(cv::Mat & genImage, cv::Mat & testImage, std::string genName, std::string testName)
	{
		std::cout << std::endl;
		std::cout << "================" << std::endl;
		std::cout << "Start processing ... " << std::endl;
		std::cout << "================" << std::endl;
		std::cout << std::endl;

		// If matching is requested, create a file
		std::ofstream result_matching;
		if (mProcessMatching && mOutputFileMatchingScores != "")
		{
			try
			{
				result_matching.open(mOutputFileMatchingScores.c_str(), std::ios::out);
			}
			catch (std::exception & e)
			{
				std::cout << e.what() << std::endl;
				throw std::runtime_error("Cannot create the filter for matching scores : " + mOutputFileMatchingScores);
			}
		}



		try
		{
			// Process the eye
			pirEye eye;

			processOneEye(genImage, genName, eye);

			// Process a second eye if matching is requested

			{
				pirEye eye2;


				processOneEye(testImage, testName, eye2);

				// Match the two iris codes
				score = eye.match(eye2, mpApplicationPoints);

			} // end of m-loop					
		}
		catch (std::exception & e)
		{
			std::cout << e.what() << std::endl;
			//break;
		}

		// If matching is requested, close the file
		if (result_matching)
		{
			result_matching.close();
		}

		std::cout << std::endl;
		std::cout << "====================" << std::endl;
		std::cout << "End processing. " << std::endl;
		std::cout << "====================" << std::endl;
		std::cout << std::endl;

	} // end of function definition

	double pirManager::getScore()
	{
		return 1.0 - score;
	}


} // end of namespace