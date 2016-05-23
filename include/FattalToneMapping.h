#include <vector>
#include <opencv2/opencv.hpp>

class FattalToneMapping
{
public:

	bool static applyToneMapping(cv::Mat& pLogLuma, cv::Mat& divG);

	void static setMappingSettings(double pBheta, double pAlphaMultiplier);

private:

	static double mBheta;
	static double mAlphaMultiplier;

	void static buildGaussianPy(cv::Mat& pImage, std::vector<cv::Mat>& pPyramid);

	void static calculateDivergence(cv::Mat& Gx, cv::Mat& Gy, cv::Mat& divG);

	void static calculateGradientPy(cv::Mat& pImage,int pLevel, cv::Mat& pGradX, cv::Mat& pGradY);

	void static calculateAttenuatedGradient(cv::Mat& pImage, cv::Mat& attenuation, cv::Mat& pGradX, cv::Mat& pGradY);

	void static calculateScaling(cv::Mat& pGradMag, cv::Mat& pScaling);

	void static calculateAttenuations(std::vector<cv::Mat> pScalings, cv::Mat& Attenuation);


};
