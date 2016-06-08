#ifndef MAKEDLL
#define DLLEXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

#ifndef PIRMANAGER_H
#define PIRMANAGER_H

#include <iostream>
#include <vector>
#include "opencv\highgui.h"
#include "pirEye.h"

namespace Proline
{
	/** Overall manager.
	* This class manages all the files, configuration, saving
	* and loading options. It uses pirEye to execute technical processings.
	* @see pirEye
	*/

	class DLLEXPORT pirManager
	{

	public:
		/** Default constructor.
		* Associate lines of configuration file to the attributes of the class.\n
		* Initialize all parameters to default values.
		* @see initConfiguration()
		*/
		pirManager();

		/** Default destructor.
		* Release matrix containing the application points.\n
		* Release the bank of Gabor Filters.
		*/
		~pirManager();

		/** Load configuration from a text file.
		* @param rFilename The relative path to the filename (relative to the executable)
		* @return void
		* @see showConfiguration()
		*/
		void loadConfiguration(const std::string & rFilename = "configuration.ini");

		/** Show configuration in prompt command.
		* @see initConfiguration()
		* @see loadConfiguration()
		*/
		void showConfiguration();

		/** Run pirris according to the configuration.
		* Build the eyes and process them as requested by the configuration file.
		* @see processOneEye()
		*/
		void run(cv::Mat & genImage, cv::Mat & testImage, std::string genName, std::string testName);

		/** Get the score after matching */
		double getScore();

	private:

		// Commands
		bool mProcessSegmentation;
		bool mProcessNormalization;
		bool mProcessEncoding;
		bool mProcessMatching;
		bool mUseMask;

		// Inputs
		std::string mFilenameListOfImages;
		std::vector<std::string> mListOfImages;
		std::string mInputDirOriginalImages;
		std::string mInputDirMasks;
		std::string mInputDirParameters;
		std::string mInputDirNormalizedImages;
		std::string mInputDirNormalizedMasks;
		std::string mInputDirIrisCodes;

		// Outputs
		std::string mOutputDirSegmentedImages;
		std::string mOutputDirParameters;
		std::string mOutputDirMasks;
		std::string mOutputDirNormalizedImages;
		std::string mOutputDirNormalizedMasks;
		std::string mOutputDirIrisCodes;
		std::string mOutputFileMatchingScores;

		// Parameters
		int mMinPupilDiameter;
		int mMaxPupilDiameter;
		int mMinIrisDiameter;
		int mMaxIrisDiameter;
		int mWidthOfNormalizedIris;
		int mHeightOfNormalizedIris;

		double score;

		std::string mFilenameGaborFilters;
		std::vector<cv::Mat> mGaborFilters;
		std::string mFilenameApplicationPoints;
		cv::Mat mpApplicationPoints;

		// Suffix for filenames
		std::string mSuffixSegmentedImages;
		std::string mSuffixParameters;
		std::string mSuffixMasks;
		std::string mSuffixNormalizedImages;
		std::string mSuffixNormalizedMasks;
		std::string mSuffixIrisCodes;

		// Maps to associate a string (conf file) to a variable (not the value of the variable !)
		std::map<std::string, bool*> mMapBool;
		std::map<std::string, int*> mMapInt;
		std::map<std::string, std::string*> mMapString;

		// Private methods
		//////////////////


		/** Initialize all configuration options to default values.
		* Default values are :
		* - For all directory/textfile paths : ""
		* - Minimum and maximum diameter for the pupil : 21 - 91 pixels
		* - Minimum and maximum diameter for the iris : 99 - 399 pixels
		* - Size of normalized iris : 512 x 64
		* - Gabor filter bank is empty
		* - Application points matrix is blank
		* - All commands of processing are set to false => nothing is going to be executed
		* - Suffix for filenames are ""_segm.bmp", "_para.txt", "_mask.bmp", "_imno.bmp",
		* "_mano.bmp", and "_code.bmp" respectively for segmented image, parameters, mask,
		* normalized image, normalized mask, iris code
		* @see loadConfiguration()
		* @see showConfiguration()
		*/
		void initConfiguration();

		/** Load the list of images.
		* The list of images is a textfile containing the name of all
		* images to be loaded/processed/compared. Each blank or endline
		* is considered as a separator between two different images.
		* For matching lists, it may be more readable to present the list
		* on two columns of names. For other process (segmentation, normalization,
		* encoding), it is more readable to present only one column.
		*/
		void loadListOfImages();

		/** Load the Gabor filters.
		* The coefficient of Gabor filters are stored in a textfile
		* according to a specific structure (see documentation of pirris).
		* This function reads the textfile and store the filters into a vector of matrix.
		*/
		void loadGaborFilters();

		/** Load the application points.
		* The application points are stored in a textfile
		* according to a specific structure (see documentation of pirris).
		* This function reads the textfile and store the application points into a binary matrix
		* in which the on-pixels will be considered during the matching.
		*/
		void loadApplicationPoints();

		/** Load, segment, normalize, encode, and save according to user configuration.
		* @param rName The eye name (used to name the loading/saving files)
		* @param rEye The eye to be processed
		* @return void
		* @see pirEye
		*/
		void processOneEye(cv::Mat & inputImage, std::string short_name, pirEye & rEye);

	}; // end of class

} // end of namespace


#endif