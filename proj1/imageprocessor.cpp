#include <iostream>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

class ImageProcessor
{
  private:
    wifstream fin;

  public:

    bool showImage(const string &filename)
    {
      Mat image = imread(filename);
      imshow("Original image",image);
      waitKey(0);

      return true;
    }
    // bool splitImage(const Mat image)
    // {
      
    // }
};

int main(int argc, char const *argv[])
{
  ImageProcessor processor;
  processor.showImage("imageprocessor_files/demons.jpg");
  return 0;
}
