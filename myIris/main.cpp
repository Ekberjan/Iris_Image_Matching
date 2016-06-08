// myIris.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Iris\pirManager.h"

int _tmain(int argc, _TCHAR* argv[])
{
	std::string path = "/Images/";
	cv::Mat genImage = cv::imread(path + "/1.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat testImage = cv::imread(path + "/2.jpg", CV_LOAD_IMAGE_GRAYSCALE);

	try
	{
		Proline::pirManager pir;
		pir.loadConfiguration("D:/Workspace/C++/Iris/myIris/x64/Release/configuration.ini");		
		pir.run(genImage, testImage, "1.jpg", "2.jpg");
		std::cout << "matching score: " << pir.getScore() << std::endl;

	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
	

	return 0;
}

