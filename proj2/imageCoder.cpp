#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include "predictors.cpp"
#include "golomb.cpp"
#include "bitStream.h"

using namespace std;
using namespace cv;

class ImageCoder {
private:
    Predictors predictors; 

public:

    vector<vector<int>> calculateResiduals(const Mat& image, Predictors::Standards standard) {
        vector<vector<int>> residuals(image.channels());

        int rows = image.rows;
        int cols = image.cols;

        for (int c = 0; c < image.channels(); ++c) {
            vector<int> channelResiduals;
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    int actual = image.at<Vec3b>(i, j)[c];
                    int left = (j > 0) ? image.at<Vec3b>(i, j - 1)[c] : 0;
                    int above = (i > 0) ? image.at<Vec3b>(i - 1, j)[c] : 0;
                    int diag = (i > 0 && j > 0) ? image.at<Vec3b>(i - 1, j - 1)[c] : 0;

                    int predicted = predictors.predictors(standard, left, above, diag);
                    channelResiduals.push_back(actual - predicted);
                }
            }
            residuals[c] = channelResiduals;
        }
        return residuals;
    }

    void encodeWithGolomb(const vector<vector<int>>& residuals, int m, const string& filename) {
        golomb golombCoder(m);

        bitStream bs;
        bs.openFile(filename, ios::out | ios::binary);

        for (const auto& channelResiduals : residuals) {
            for (const int& res : channelResiduals) {
                golombCoder.encode(res, bs);
            }
        }

        bs.flushBuffer();
        cout << "Residuals encoded and saved to: " << filename << endl;
    }


    vector<vector<int>> decodeWithGolomb(const string& filename, int m, int channels, int rows, int cols) {
        golomb golombCoder(m);
        vector<vector<int>> residuals(channels);

        bitStream bs;
        bs.openFile(filename, ios::in | ios::binary);

        for (int c = 0; c < channels; ++c) {
            vector<int> channelResiduals;
            int totalPixels = rows * cols;
            for (int i = 0; i < totalPixels; ++i) {
                int res = golombCoder.decode(bs);
                channelResiduals.push_back(res);
            }
            residuals[c] = channelResiduals;
        }

        return residuals;
    }

    Mat reconstructImage(const vector<vector<int>>& residuals, const Mat& originalImage, Predictors::Standards standard) {
        Mat reconstructed = Mat::zeros(originalImage.size(), originalImage.type());

        int rows = reconstructed.rows;
        int cols = reconstructed.cols;

        for (int c = 0; c < originalImage.channels(); ++c) {
            int index = 0;
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    int left = (j > 0) ? reconstructed.at<Vec3b>(i, j - 1)[c] : 0;
                    int above = (i > 0) ? reconstructed.at<Vec3b>(i - 1, j)[c] : 0;
                    int diag = (i > 0 && j > 0) ? reconstructed.at<Vec3b>(i - 1, j - 1)[c] : 0;

                    int predicted = predictors.predictors(standard, left, above, diag);
                    int actual = predicted + residuals[c][index++];
                    reconstructed.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(actual);
                }
            }
        }
        return reconstructed;
    }

};


int main() {
    string inputFile = "image.ppm";
    string encodedFile = "encoded_residuals.bin";

    Mat image = imread(inputFile, IMREAD_COLOR);
    if (image.empty()) {
        cerr << "Error: Could not load the image." << endl;
        return -1;
    }

    ImageCoder coder;

    Predictors::Standards selectedPredictor = Predictors::JPEG_PL;

    vector<vector<int>> residuals = coder.calculateResiduals(image, selectedPredictor);

    int m = 4; 
    coder.encodeWithGolomb(residuals, m, encodedFile);

    vector<vector<int>> decodedResiduals = coder.decodeWithGolomb(encodedFile, m, image.channels(), image.rows, image.cols);

    Mat reconstructed = coder.reconstructImage(decodedResiduals, image, selectedPredictor);

    if (image.size() != reconstructed.size()) {
        resize(reconstructed, reconstructed, image.size());
    }

    Mat combined;
    hconcat(image, reconstructed, combined);

    imshow("Original (Left) vs Reconstructed (Right)", combined);
    waitKey(0);


    if (countNonZero(image != reconstructed) == 0) { //ensure lossless codec
        cout << "Lossless compression verified: Original and reconstructed images are identical." << endl;
    } else {
        cout << "Error: Reconstructed image differs from the original." << endl;
    }

    return 0;
}

