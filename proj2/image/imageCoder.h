#ifndef IMAGECODER_H
#define IMAGECODER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "../util/predictors.cpp"
#include "../util/golomb.cpp"
#include "../util/bitStream.h"

using namespace std;
using namespace cv;

class ImageCoder {
private:
    Predictors predictors;
    int mapToNonNegative(int n);
    int mapToSigned(int n);

public:
    int m; 

    vector<vector<int>> calculateResiduals(const Mat& image, Predictors::Standards standard);

    void encodeWithGolomb(const vector<vector<int>>& residuals, const string& filename);

    vector<vector<int>> decodeWithGolomb(const string& filename);

    Mat reconstructImage(const vector<vector<int>>& residuals, Predictors::Standards standard);
};

#endif
