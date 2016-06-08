#ifndef MAKEDLL
#define DLLEXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

#ifndef PIREYE_H
#define PIREYE_H

#include <iostream>
#include "pirCircle.h"

namespace Proline
{
	/** Eye handler.
	* Allows to process one eye, and to load/save
	* elements relative to this eye such as masks ...
	* This class is the link between the manager
	* and the pocessing functions.
	* @see pirProcessings
	* @see pirManager
	*/

	class DLLEXPORT pirEye
	{
	public:

		/** Default constructor.
		* Initialize all pointers of image to 0. \n
		* Initialize pupil and iris circles to (0, 0, 0).
		*/
		pirEye();

		/** Default destructor
		* Release all images (free memory).
		*/
		~pirEye();

		/** Load the original image corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see loadImage()
		*/
		void loadOriginalImage(const std::string & rFileName);

		/** Load the binary mask corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see loadImage()
		*/
		void loadMask(const std::string & rFilename);

		/** Load the normalized image corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see loadImage()
		*/
		void loadNormalizedImage(const std::string & rFilename);

		/** Load the normalized mask corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see loadImage()
		*/
		void loadNormalizedMask(const std::string & rFilename);

		/** Load the iris code (stored as an image) corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see loadImage()
		*/
		void loadIrisCode(const std::string & rFielname);

		/** Load the contour parameters corresponding to the eye.
		* @param rFilename Complete path of the textfile
		* @return void
		*/
		void loadParameters(const std::string & rFilename);

		/** Save the segmented color image corresponding to the eye.
		* @param rFilename Complete path of the image
		* @param return void
		* @see saveImage()
		*/
		void saveSegmentedImage(const std::string & rFilename);

		/** Svae the binary mask corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see saveImage()
		*/
		void saveMask(const std::string & rFilename);

		/** Save the normalized image corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see saveImage()
		*/
		void saveNormalizedImage(const std::string & rFilename);

		/** Save the normalized mask corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see saveImage()
		*/
		void saveNormalizedMask(const std::string & rFilename);

		/** Save the iris code (stored as an image) corresponding to the eye.
		* @param rFilename Complete path of the image
		* @return void
		* @see saveImage()
		*/
		void saveIrisCode(const std::string & rFilename);

		/** Save the contour parameters corresponding to the eye.
		* @param rFilename Complete path of the textfile
		* @return void
		* @see saveImage()
		*/
		void saveParameters(const std::string & rFilename);

		/** Initialize the mask.
		* Create the mask and set all pixels to 255.
		* This function is used when user did not provide the mask
		* or did not ask to compute the mask and yet mask is required to go on.
		* @return void
		*/
		void initMask();

		/** Segment the eye.
		* Initialize the mask if they have not been created yet.\n
		* Call the function pirProcessings::segment().\n
		* Draw the results of segmentation on the private
		* attribute mpSegmentedImage (color image).
		* @param minIrisDiameter The minimum diameter for segmenting the iris
		* @param minPupilDiameter The minimum diameter for segmenting the pupil
		* @param maxIrisDiameter The maximum diameter for segmenting the iris
		* @param maxPupilDiameter The maximum diameter for segmenting the pupil
		* @return void
		* @see pirProcessings::segment()
		*/
		void segment(int minIrisDiameter, int minPupilDiameter, int maxIrisDiameter, int maxPupilDiameter);


		/** Normalize image and mask.
		* If the mask is not already initialized, the function does intialize it to 255.
		* Use the Daugman's rubber-sheet method.
		* @param widthOfNormalizedIris Width of normalized image
		* @param heightOfNormalizedIris Height of normalized image
		* @return void
		* @see pirProcessings::normalize()
		*/
		void normalize(int widthOfNormalizedIris, int heightOfNormalizedIris);

		/** Encode the normalized image.
		* Use a bank of Gabor filters.
		* @param rGaborFilters The gabor filters used to extract iris texture
		* @return void
		* @see pirProcessings::encode()
		*/
		void encode(const std::vector<cv::Mat> & rGaborFilters);

		/** Match two eyes (hamming distance between iris codes).
		* Normalized masks are used.\n
		* If normalized mask is not already initialized,
		* the function does intialize it to 255, that is why rEye is not "const"
		* @param rEye The other eye to match
		* @param pApplicationPoints A binary image indicating which pixels
		* will be considered for the matching. This image is the same size as a normalized iris.
		* @return The hamming distance between the two eyes.
		* @see pirProcessings::match()
		*/
		float match(pirEye & rEye, const cv::Mat & pApplicationPoints);

		/** The original image corresponding to the eye (input only). */
		cv::Mat mpOriginalImage;


	private:



		/** The segmented image (color) corresponding to the eye (output only). */
		cv::Mat mpSegmentedImage;

		/** The mask corresponding to the eye (input and/or output). */
		cv::Mat mpMask;

		/** The normalized image corresponding to the eye (input and/or output). */
		cv::Mat mpNormalizedImage;

		/** The normalized mask corresponding to the eye (input and/or output). */
		cv::Mat mpNormalizedMask;

		/** The iris code (stored as an image) corresponding to the eye (input and/or output). */
		cv::Mat mpIrisCode;

		/** The pupil circle corresponding to the eye (input and/or output). */
		pirCircle mPupil;

		/** The iris circle corresponding to the eye (input and/or output). */
		pirCircle mIris;

		/** The pupil coarse contour corresponding to the eye. */
		std::vector<cv::Point> mCoarsePupilContour;

		/** The irirs coarse contour corresponding to the eye. */
		std::vector<cv::Point> mCoarseIrisContour;

		/** The theta sampling for pupil coarse contour. */
		std::vector<float> mThetaCoarsePupil;

		/** The theta sampling for iris coarse contours. */
		std::vector<float> mThetaCoarseIris;

		/** Generic function to load the image-like attributes of the eye.
		* @param rFilename The complete path of the image
		* @param pImage A pointer of pointer on the image
		* @return void
		*/
		void loadImage(const std::string & rFilename, cv::Mat & ppImage);

		/** Generic function to save the image-like attributes of the eye.
		* @param rFilename The complete path of the image
		* @param ppImage A pointer on the image
		* @return void
		*/
		void saveImage(const std::string & rFilename, const cv::Mat & pImage);

	}; // end of class


} // end of namespace



#endif