#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

class imageReader {
public:
    Mat loadImage(const string &filename) {
        Mat image = imread(filename);
        if (image.empty()) {
            cerr << "Error: Unable to load image from " << filename << endl;
            exit(EXIT_FAILURE);
        }
        imshow("Original Image", image);
        waitKey(0);
        return image;
    }
};
