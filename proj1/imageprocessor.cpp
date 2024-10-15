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

    Scalar calculateMSE(const Mat &image1, const Mat &image2)
    {
      Scalar meanValue = mean((image1 - image2) ^ 2);
      cout << "MSE: " << meanValue << endl;
      return meanValue;
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
  processor.calculateMSE(image, image2);
  processor.calculateHistogram(greyImage);
  return 0;
}
