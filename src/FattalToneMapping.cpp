#include "FattalToneMapping.h"
#include <iostream>

double FattalToneMapping::mAlphaMultiplier = 0.1;
double FattalToneMapping::mBheta = 0.83;

bool FattalToneMapping::applyToneMapping(cv::Mat& pLogLuma, cv::Mat& divG)
{
	std::vector<cv::Mat> pyramid;
	buildGaussianPy(pLogLuma, pyramid);

	cv::Mat gradX, gradY, gradMag;
	cv::Mat AttenuatedGradX, AttenuatedGradY;
	cv::Mat scaling;
	std::vector<cv::Mat> scalingVector;
	for (int i = 0; i < pyramid.size(); i++)
	{
		calculateGradientPy(pyramid[i], i, gradX, gradY);
		cv::magnitude(gradX, gradY, gradMag);
		calculateScaling(gradMag, scaling);
		scalingVector.push_back(scaling);
	}

	cv::Mat attenuation;
	calculateAttenuations(scalingVector, attenuation);

	cv::imshow("Attenuation", attenuation);
	cv::waitKey(0);

	calculateAttenuatedGradient(pLogLuma, attenuation, AttenuatedGradX, AttenuatedGradY);
	calculateDivergence(AttenuatedGradX, AttenuatedGradY, divG);

	return true;
}

void FattalToneMapping::buildGaussianPy(cv::Mat& pImage, std::vector<cv::Mat>& pPyramid)
{
	cv::Mat downImage = pImage;
	while (downImage.cols > 32 && downImage.rows > 32)
	{
		pPyramid.push_back(downImage);
		cv::pyrDown(downImage, downImage, cv::Size(downImage.cols / 2, downImage.rows / 2));
	}
}

void FattalToneMapping::calculateDivergence(cv::Mat& Gx, cv::Mat& Gy, cv::Mat& divG)
{
	divG = cv::Mat::zeros(Gx.rows, Gx.cols, CV_32FC1);
	for (int i = 0; i < Gx.rows; i++)
		for (int j = 0; j < Gx.cols; j++)
		{
			divG.at<float>(i, j) = Gx.at<float>(i, j) + Gy.at<float>(i, j);
			if (j > 0) { divG.at<float>(i, j) -= Gx.at<float>(i, j - 1); }
			if (i > 0) { divG.at<float>(i, j) -= Gy.at<float>(i - 1, j); }

			if (j == 0) { divG.at<float>(i, j) += Gx.at<float>(i, j); }
			if (i == 0) { divG.at<float>(i, j) += Gy.at<float>(i, j); }
		}
}

void FattalToneMapping::calculateGradientPy(cv::Mat& pImage, int level, cv::Mat& pGradX, cv::Mat& pGradY) 
{
	pGradX = cv::Mat::zeros(pImage.rows, pImage.cols, CV_32FC1);
	pGradY = cv::Mat::zeros(pImage.rows, pImage.cols, CV_32FC1);
	for (int i = 0; i < pImage.rows; i++)
		for (int j = 0; j < pImage.cols; j++)
		{
			if ( i == 0 ) { 
				pGradY.at<float>(i, j) = (pImage.at<float>(i + 1, j) - pImage.at<float>(i, j)) / pow(2, level + 1);
			}
			else if ( i == pImage.rows - 1) {
				pGradY.at<float>(i, j) = (pImage.at<float>(i, j) - pImage.at<float>(i - 1, j)) / pow(2, level + 1);
			}
			else {
				pGradY.at<float>(i, j) = (pImage.at<float>(i + 1, j) - pImage.at<float>(i - 1, j)) / pow(2, level + 1);
			}
			if ( j == 0 ) {
				pGradX.at<float>(i, j) = (pImage.at<float>(i, j + 1) - pImage.at<float>(i, j)) / pow(2, level + 1);
			}
			else if ( j == pImage.cols - 1) {
				pGradX.at<float>(i, j) = (pImage.at<float>(i, j) - pImage.at<float>(i, j - 1)) / pow(2, level + 1);
			}
			else {
				pGradX.at<float>(i, j) = (pImage.at<float>(i, j + 1) - pImage.at<float>(i, j - 1)) / pow(2, level + 1);
			}
		}
}

void FattalToneMapping::setMappingSettings(double pBheta, double pAlphaMultiplier)
{
	mBheta = pBheta;
	mAlphaMultiplier = pAlphaMultiplier;
}

void FattalToneMapping::calculateAttenuatedGradient(cv::Mat& pImage, cv::Mat& phi, cv::Mat& pGradX, cv::Mat& pGradY)
{
	pGradX = cv::Mat::zeros(pImage.rows, pImage.cols, CV_32FC1);
	pGradY = cv::Mat::zeros(pImage.rows, pImage.cols, CV_32FC1);
	for (int i = 0; i < pImage.rows; i++)
		for (int j = 0; j < pImage.cols; j++)
		{
			if (j + 1 >= pImage.cols) {
				pGradX.at<float>(i, j) =  ( pImage.at<float>(i, pImage.cols-2) - pImage.at<float>(i, j) ) * 0.5 * ( phi.at<float>(i, pImage.cols - 2) + phi.at<float>(i, j) );
			}
			else
			{
				pGradX.at<float>(i, j) = ( pImage.at<float>(i, j + 1) - pImage.at<float>(i, j) ) * 0.5 * (phi.at<float>(i, j + 1) + phi.at<float>(i, j));
			}
			if (i + 1 >= pImage.rows) { 
				pGradY.at<float>(i, j) = ( pImage.at<float>(pImage.rows-2, j) - pImage.at<float>(i, j) ) * 0.5 * (phi.at<float>(pImage.rows - 2, j) + phi.at<float>(i, j));
			}
			else
			{
				pGradY.at<float>(i, j) = ( pImage.at<float>(i + 1, j) - pImage.at<float>(i, j) ) * 0.5 * (phi.at<float>(i + 1, j) + phi.at<float>(i, j));
			}
		}
}

void FattalToneMapping::calculateScaling(cv::Mat& pGradMag, cv::Mat& pScaling) 
{
	cv::Mat temp;
	double alpha = mAlphaMultiplier * cv::mean(pGradMag)[0];

	cv::pow((pGradMag / alpha),mBheta,temp);
	cv::multiply(alpha / pGradMag, temp, pScaling);
}

void FattalToneMapping::calculateAttenuations(std::vector<cv::Mat> pScalings, cv::Mat& Attenuation) 
{
	cv::Mat temp;
	for (int i = pScalings.size() - 2; i >= 0; i--)
	{
		cv::resize(pScalings[i + 1], temp, cv::Size(pScalings[i].cols, pScalings[i].rows));
		cv::multiply(pScalings[i], temp, pScalings[i]);
	}
	Attenuation = pScalings[0];
}