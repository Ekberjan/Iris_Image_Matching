#ifndef MAKEDLL
#define DLLEXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

#ifndef PIRCIRCLE_H
#define PIRCIRCLE_H

#include <iostream>
#include "opencv\highgui.h"

namespace Proline
{
	/** Circle handler
	* used by the Daugman's rubber sheet method
	# see pirProcessing::normalize()
	*/
	class DLLEXPORT pirCircle
	{
	public:
		/* Default constructor */
		pirCircle();

		/* Default destructor */
		~pirCircle();

		/** Overloaded contructor.
		* @param rCenter Initialization of the center
		* @param rRadius Initialization of the radius
		*/
		pirCircle(const cv::Point & rCenter, int rRadius);

		/** Compute circle fitting by least-squares emthod.
		* This function is called by pirProcessings::segment()
		* Reference: http://www.dtcenter.org/met/users/docs/write_ups/circle_fit.pdf
		* @param rPoints A contour in cartesian coordinates
		* @return void
		* @see segment()
		*/
		void computeCircleFitting(const std::vector<cv::Point> & rPoints);

		/** Draw a circle on an image.
		* @param pImage The image on which circle is to be drawn
		* @param rColor Color of the circle
		* @param thickness Circle thickness. Set to -1 to draw the dist inside the circle
		* @return void
		*/

		void drawCircle(cv::Mat & pImage, const cv::Scalar & rColor = cv::Scalar(255), int thickness = 1);

		/** Get the circle center.
		* @return The circle center
		*/
		cv::Point getCenter() const;

		/** Get the circle radius.
		* @return The circle center
		*/
		int getRadius() const;

		/** Set the circle center.
		* @param rRadius The circle radius
		* @return void
		*/
		void setCenter(const cv::Point & rCenter);


		/** Set the circle radius.
		* @param rRadius The circle radius
		* @return void
		*/
		void setRadius(int rRadius);

		/** Set the circle center and radius.
		* @param rCenter The circle center
		* @param rRadius The circle radius
		* @return void
		*/
		void setCircle(const cv::Point & rCenter, int rRadius);

		/** Set the circle center and radius.
		* @param rCenterX The x-coordinate of circle center
		* @param rCenterY The y-coordinate of circle center
		* @param rRadius The circle radius
		* @return void
		*/
		void setCircle(int rCenterX, int rCenterY, int rRadius);


	private:


		/* The circle center */
		cv::Point mCenter;

		/* The circle radius */
		int mRadius;

	}; // end of class definition
} // end of namespace


#endif