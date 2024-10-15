#include <iostream>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <matplotlibcpp.h>


namespace plt = matplotlibcpp;
using namespace cv;
using namespace std;

class ImageProcessor
{
  public:

    Mat showImage(const string &filename)
    {
      Mat image = imread(filename);
      imshow("Original image",image);
      waitKey(0);

      return image;
    }

    vector<Mat> splitImage(const Mat image)
    {
      vector<Mat> channels;
      split(image, channels);
      imshow("Blue",channels[0]);
      imshow("Red",channels[1]);
      imshow("Green",channels[2]);
      waitKey(0);
      return channels;
    }

    Mat toGrayscale(const Mat image)
    {
      Mat greyMat;
      cvtColor(image, greyMat, COLOR_BGR2GRAY);
      imshow("Grey", greyMat);
      waitKey(0);
      return greyMat;
    }

    void calculateHistogram(const Mat &image)
    {
      // Calculate histogram
      Mat hist;
      int histSize = 256; // Number of bins
      float range[] = {0, 256}; // Range of pixel values
      const float *histRange = {range};

      calcHist(&image, 1, 0, Mat(), hist, 1, &histSize, &histRange);

      // Convert histogram to vector for matplotlib
      vector<float> histVec(histSize);
      for (int i = 0; i < histSize; i++)
      {
        histVec[i] = hist.at<float>(i);
      }

      // Plot histogram using matplotlib
      plt::figure_size(800, 600);
      plt::bar(histVec);
      plt::title("Pixel Intensity");
      plt::show();
    }

    void gaussianBlur(const Mat &image)
    {
      Mat blurredImage3, blurredImage7;
      GaussianBlur(image, blurredImage3, Size(3, 3), 0);
      GaussianBlur(image, blurredImage7, Size(7, 7), 0);
      imshow("Blurred Image with 3x3 kernel", blurredImage3);
      imshow("Blurred Image with 7x7 kernel", blurredImage7);
      waitKey(0);
    }

    Mat imageDifference(const Mat &image1, const Mat &image2)
    {
      Mat diffImage;
      absdiff(image1, image2, diffImage);
      imshow("Difference between 2 images", diffImage);
      waitKey(0);
      return diffImage;
    }

    double calculateMSE(const Mat &image1, const Mat &image2)
    {
      Mat s1;
      absdiff(image1, image2, s1);       // |image1 - image2|
      s1.convertTo(s1, CV_32F);  // convert to float
      s1 = s1.mul(s1);           // |image1 - image2|^2

      Scalar s = sum(s1);        // sum elements per channel

      double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

      double mse = sse / (double)(image1.channels() * image1.total());
      cout << "MSE: " << mse << endl;
      return mse;
    }

    double calculatePSNR(const Mat &image1, const Mat &image2)
    {
      double mse = calculateMSE(image1, image2);
      if (mse == 0) 
      {
        cout << "MSE is zero: there is no noise in the signal";
        return 100;
      }
      double max_pixel = 255.0;
      double psnr = 20 * log10(max_pixel / sqrt(mse));
      cout << "PSNR: " << psnr << endl;
      return psnr;
    }

    Mat quantizeImage(const Mat &image, int levels)
    {
      Mat quantizedImage;
      float scale = 255.0 / (levels - 1);
      image.convertTo(quantizedImage, CV_32F); // Convert to float for processing
      quantizedImage = quantizedImage / scale;
      quantizedImage.convertTo(quantizedImage, CV_32F, 1.0, 0.5); // Add 0.5 for rounding
      quantizedImage.convertTo(quantizedImage, CV_8U); // Convert back to 8-bit
      quantizedImage = quantizedImage * scale;
      imshow("Quantized Image", quantizedImage);
      waitKey(0);
      return quantizedImage;
    }

};

int main(int argc, char const *argv[])
{
  ImageProcessor processor;
  Mat image = processor.showImage("imageprocessor_files/baboon.ppm");
  vector<Mat> channels = processor.splitImage(image);
  Mat greyImage = processor.toGrayscale(image);
  processor.gaussianBlur(image);
  Mat image2 = processor.showImage("imageprocessor_files/peppers.ppm");
  processor.imageDifference(image,image2);
  processor.calculateMSE(image, image);
  processor.calculatePSNR(image, image2);

  // Quantize the grayscale image to 4 levels
  Mat quantizedImage = processor.quantizeImage(greyImage, 10); // Bigger number = better quality, less MSE and more PSNR
  processor.calculatePSNR(greyImage, quantizedImage);
  processor.calculateHistogram(greyImage);
  return 0;
}
