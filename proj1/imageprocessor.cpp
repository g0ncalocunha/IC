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

    Mat loadImage(const string &filename)
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
      
      Mat blueBGR, redBGR, greenBGR;
      cvtColor(channels[0], blueBGR, COLOR_GRAY2BGR);
      cvtColor(channels[1], redBGR, COLOR_GRAY2BGR);
      cvtColor(channels[2], greenBGR, COLOR_GRAY2BGR);

      imwrite("../imageprocessor_files/results/blueImage.png", blueBGR);
      imwrite("../imageprocessor_files/results/redImage.png", redBGR);
      imwrite("../imageprocessor_files/results/greenImage.png", greenBGR);
      waitKey(0);
      return channels;
    }

    Mat toGrayscale(const Mat image)
    {
      Mat grayMat;
      cvtColor(image, grayMat, COLOR_BGR2GRAY);
      imshow("Grey", grayMat);

      Mat grayMatBGR;
      cvtColor(grayMat, grayMatBGR, COLOR_GRAY2BGR);

      imwrite("../imageprocessor_files/results/grayMat.png", grayMatBGR);
      waitKey(0);
      return grayMat;
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
      Mat blurredImage13, blurredImage7;
      GaussianBlur(image, blurredImage13, Size(13, 13), 0);
      GaussianBlur(image, blurredImage7, Size(7, 7), 0);
      imshow("Blurred Image with 7x7 kernel", blurredImage7);
      imshow("Blurred Image with 13x13 kernel", blurredImage13);
      imwrite("../imageprocessor_files/results/blurredImage13.png", blurredImage13);
      imwrite("../imageprocessor_files/results/blurredImage7.png", blurredImage7);
      waitKey(0);
    }

    Mat imageDifference(const Mat &image1, const Mat &image2)
    {
      Mat diffImage;
      absdiff(image1, image2, diffImage);
      imshow("Difference between 2 images", diffImage);
      imwrite("../imageprocessor_files/results/diffImage.png", diffImage);
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
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <image_input_file>" << std::endl;
    return -1;
  }
  ImageProcessor processor;
  // Task 1
  Mat image = processor.loadImage(argv[1]);
  // Task 2
  vector<Mat> channels = processor.splitImage(image);
  Mat greyImage = processor.toGrayscale(image);
  // Task 4
  processor.gaussianBlur(image);
  Mat image2 = processor.loadImage("../imageprocessor_files/peppers.ppm");
  // Task 5
  processor.imageDifference(image,image2);
  processor.calculateMSE(image, image);
  processor.calculatePSNR(image, image2);
  // Task 6 - Quantize the grayscale image
  cout << "Quantization with 4 levels:  " << endl;
  Mat quantizedImage4 = processor.quantizeImage(greyImage, 4); // Bigger number = better quality, less MSE and more PSNR
  processor.calculatePSNR(greyImage, quantizedImage4);
  cout << "Quantization with 10 levels:  " << endl;
  Mat quantizedImage10 = processor.quantizeImage(greyImage, 10); // Bigger number = better quality, less MSE and more PSNR
  processor.calculatePSNR(greyImage, quantizedImage10);
  Mat quantizedImage4BGR, quantizedImage10BGR;
  cvtColor(quantizedImage4, quantizedImage4BGR, COLOR_GRAY2BGR);
  cvtColor(quantizedImage10, quantizedImage10BGR, COLOR_GRAY2BGR);
  imwrite("../imageprocessor_files/results/quantizedImage4.png", quantizedImage4BGR);
  imwrite("../imageprocessor_files/results/quantizedImage10.png", quantizedImage10BGR);
  // Task 3
  processor.calculateHistogram(greyImage);
  return 0;
}
