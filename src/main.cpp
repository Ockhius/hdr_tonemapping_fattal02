#include <iostream>
#include <opencv2/opencv.hpp>
#include "hdrloader.h"
#include "FattalToneMapping.h"
#include <boost/multi_array.hpp>
#include "laplace.h"
#include <fstream>

void copyMatObject2Array(cv::Mat& divG, boost::multi_array<double, 2>& F)
{
	for (int i = 0; i < divG.rows; i++)
		for (int j = 0; j < divG.cols; j++)
		{
			F[i][j] = divG.at<float>(i, j);
		}
}

void copyArray2MatObject(boost::multi_array<double, 2>& U, cv::Mat& I)
{
	for (int i = 0; i < I.rows; i++)
		for (int j = 0; j < I.cols; j++)
		{
			I.at<float>(i, j) = U[i][j];
		}
}

int main(int argc, char* argv[])
{
	char *InputFilePath, *OutputFilePath;
	double alphaMultiplier = 0.18, bheta = 0.87;
	double s = 0.55;
	// Input Patameters: 
	if (argc < 3)
	{
		std::cout << "Usage: ./app HDRInputPath LDROutputPath [AlphaMultiplier] [Bheta] [S]" << std::endl;
		getchar();
		return 0;
	}
	else if (argc < 4)
	{
		InputFilePath = argv[1];
		OutputFilePath = argv[2];
	}
	else if (argc < 5)
	{
		InputFilePath = argv[1];
		OutputFilePath = argv[2];
		alphaMultiplier = atof(argv[3]);
	}
	else if (argc < 6)
	{
		InputFilePath = argv[1];
		OutputFilePath = argv[2];
		alphaMultiplier = atof(argv[3]);
		bheta = atof(argv[4]);
	}
	else
	{
		InputFilePath = argv[1];
		OutputFilePath = argv[2];
		alphaMultiplier = atof(argv[3]);
		bheta = atof(argv[4]);
		s = atof(argv[5]);
	}


	HDRLoaderResult result;
	bool ret = HDRLoader::load(InputFilePath, result);

	if (!ret)
	{
		return -1;
 	}

	int width = result.width;
	int height = result.height;

	cv::Mat hdrImage = cv::Mat(height, width, CV_32FC3);
	memcpy(hdrImage.data, result.cols, 3 * width*height*sizeof(float));

	double min, max;

	cv::minMaxIdx(hdrImage, &min, &max);
	cv::Mat linhdr = cv::Mat(height, width,CV_8UC3);

	hdrImage.convertTo(hdrImage, CV_32FC3, 1 / max);
	cv::Mat hdrXYZ = cv::Mat(height, width, CV_32FC3);
	cv::cvtColor(hdrImage, hdrXYZ, CV_RGB2XYZ);

	std::vector<cv::Mat> channels;
	cv::split(hdrXYZ, channels);

	cv::minMaxIdx(channels[1], &min, &max); 
	channels[1].convertTo(linhdr, CV_8UC3, 255 / max);
	cv::imshow("Inputluminance", linhdr);
	cv::waitKey(0);

	cv::Mat logLuma;
	cv::log(channels[1], logLuma);

	cv::Mat divG;
	FattalToneMapping::setMappingSettings(bheta, alphaMultiplier);
	FattalToneMapping::applyToneMapping(logLuma, divG);
	
	/************************************
	*	Solve Poisson Equation here to find I(x,y) from G(x,y)
	*	FFT Poisson solver
	*************************************/
	pde::fftw_threads(4);

	double h1 = 1.0, h2 = 1.0, a1 = 1.0, a2 = 1.0;
	pde::types::boundary bdtype = pde::types::Neumann;
	double bdvalue = 0.0;
	double trunc;

	boost::multi_array<double, 2> F(boost::extents[divG.rows][divG.cols]);
	boost::multi_array<double, 2> U;

	copyMatObject2Array(divG, F);

	if (bdtype == pde::types::Neumann) 
	{
		bdvalue = pde::neumann_compat(F, a1, a2, h1, h2);
	}
	trunc = pde::poisolve(U, F, a1, a2, h1, h2, bdvalue, bdtype, false);
	pde::fftw_clean();

	/************************************
	*   Create HDR2LDR output
	*	exp(I,outlogLuma)
	*	cout = (cin / lin)^s * lour
	*************************************/

	cv::Mat I(height, width, CV_32FC1);
	cv::Mat outLuma,outLuma255;

	copyArray2MatObject(U, I);
	cv::exp(I, outLuma);

	cv::minMaxIdx(outLuma, &min, &max);
	outLuma.convertTo(outLuma, CV_32FC1, 1 / max);
	outLuma.convertTo(outLuma255, CV_8UC1, 255);
	std::string outPath = std::string(OutputFilePath) + std::string("LDRLuminance.png");
	cv::imwrite(outPath, outLuma255);
	cv::imshow("outluminance", outLuma255);
	cv::waitKey(0);

	cv::Mat ldrImage;
	cv::Mat ldrImageR, ldrImageG, ldrImageB;
	cv::Mat temp(height, width, CV_32FC1);
	std::vector<cv::Mat> colorchannels;
	std::vector<cv::Mat> outcolorchannels;
	cv::split(hdrImage, colorchannels);

	cv::divide(colorchannels[2], channels[1],temp);
	cv::pow(temp, s, temp);
	cv::multiply(temp, outLuma, ldrImageB);
	outcolorchannels.push_back(ldrImageB);

	cv::divide(colorchannels[1], channels[1], temp);
	cv::pow(temp, s, temp);
	cv::multiply(temp, outLuma, ldrImageG);
	outcolorchannels.push_back(ldrImageG);

	cv::divide(colorchannels[0], channels[1], temp);
	cv::pow(temp, s, temp);
	cv::multiply(temp, outLuma, ldrImageR);
	outcolorchannels.push_back(ldrImageR);

	cv::merge(outcolorchannels, ldrImage);

	outPath = std::string(OutputFilePath) + "LDRImage.png";
	cv::minMaxIdx(ldrImage, &min, &max);
	ldrImage.convertTo(ldrImage, CV_8UC1, 255/max);
	cv::imwrite(outPath, ldrImage);
	cv::imshow("result", ldrImage);
	cv::waitKey(0);
	return 0;
}