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
    
    // Helper function to map residuals to non-negative integers
    int mapToNonNegative(int n) {
        return (n >= 0) ? 2 * n : -2 * n - 1;
    }
    
    // Helper function to map back from non-negative to original integers
    int mapToSigned(int n) {
        return (n % 2 == 0) ? n / 2 : -(n + 1) / 2;
    }

public:
    int m;

    vector<vector<int>> calculateResiduals(const Mat& image, Predictors::Standards standard) {
        vector<vector<int>> residuals(image.channels());
        int rows = image.rows;
        int cols = image.cols;

        // Pre-allocate vectors to avoid reallocation
        for (int c = 0; c < image.channels(); ++c) {
            residuals[c].reserve(rows * cols);
        }

        for (int c = 0; c < image.channels(); ++c) {
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    int actual = image.at<Vec3b>(i, j)[c];
                    
                    // Handle border cases
                    int left = (j > 0) ? image.at<Vec3b>(i, j-1)[c] : 0;
                    int above = (i > 0) ? image.at<Vec3b>(i-1, j)[c] : 0;
                    int diag = (i > 0 && j > 0) ? image.at<Vec3b>(i-1, j-1)[c] : 0;

                    int predicted = predictors.predictors(standard, left, above, diag);
                    int residual = actual - predicted;
                    
                    // Map residual to non-negative integer for Golomb coding
                    residuals[c].push_back(mapToNonNegative(residual));
                }
            }
        }
        return residuals;
    }

    void encodeWithGolomb(const vector<vector<int>>& residuals, const string& filename) {
        bitStream bs;
        bs.openFile(filename, ios::out | ios::binary);

        int numChannels = residuals.size();
        bs.writeBits(numChannels, 8);  // Number of channels

        for (const auto& channelResiduals : residuals) {
            double mean = 0.0;
            for (int res : channelResiduals) {
                mean += res;
            }
            mean /= channelResiduals.size();
            
            // Calculate optimal m value
            m = max(1, (int)round(-1/log2(mean/(mean+1))));
            m = 1 << (int)ceil(log2(m));  // Round to power of 2
            
            bs.writeBits(m, 16);
            
            // Write residuals
            golomb golombCoder(m);
            for (int res : channelResiduals) {
                golombCoder.encode(res, bs);
            }
        }
        
        bs.flushBuffer();
    }

    vector<vector<int>> decodeWithGolomb(const string& filename, int rows, int cols) {
        bitStream bs;
        bs.openFile(filename, ios::in | ios::binary);

        int numChannels = bs.readBits(8);
        vector<vector<int>> residuals(numChannels);

        for (int c = 0; c < numChannels; ++c) {
            // Read m parameter
            m = bs.readBits(16);
            golomb golombCoder(m);
            
            vector<int> channelResiduals;
            channelResiduals.reserve(rows * cols);
            
            // Decode residuals
            for (int i = 0; i < rows * cols; ++i) {
                int encoded = golombCoder.decode(bs);
                // Map back to signed integer
                channelResiduals.push_back(mapToSigned(encoded));
            }
            residuals[c] = move(channelResiduals);
        }

        return residuals;
    }

    Mat reconstructImage(const vector<vector<int>>& residuals, Size imageSize, int type, Predictors::Standards standard) {
        Mat reconstructed = Mat::zeros(imageSize, type);
        int rows = reconstructed.rows;
        int cols = reconstructed.cols;

        for (int c = 0; c < residuals.size(); ++c) {
            int idx = 0;
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    int left = (j > 0) ? reconstructed.at<Vec3b>(i, j-1)[c] : 0;
                    int above = (i > 0) ? reconstructed.at<Vec3b>(i-1, j)[c] : 0;
                    int diag = (i > 0 && j > 0) ? reconstructed.at<Vec3b>(i-1, j-1)[c] : 0;

                    int predicted = predictors.predictors(standard, left, above, diag);
                    int pixel = predicted + residuals[c][idx++];
                    
                    // Ensure pixel values stay within valid range
                    reconstructed.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(pixel);
                }
            }
        }
        return reconstructed;
    }
};

int main() {
    string inputFile = "images/image.ppm";
    string encodedFile = "encoded_residuals.bin";
    
    // Read input image
    Mat image = imread(inputFile, IMREAD_COLOR);
    if (image.empty()) {
        cerr << "Error: Could not load the image." << endl;
        return -1;
    }

    // Convert to YUV color space
    Mat imageYUV;
    cvtColor(image, imageYUV, COLOR_BGR2YUV);

    ImageCoder coder;
    Predictors::Standards selectedPredictor = Predictors::JPEG_PL;

    // Encode
    vector<vector<int>> residuals = coder.calculateResiduals(imageYUV, selectedPredictor);
    coder.encodeWithGolomb(residuals, encodedFile);

    // Decode
    vector<vector<int>> decodedResiduals = coder.decodeWithGolomb(encodedFile, imageYUV.rows, imageYUV.cols);
    Mat reconstructedYUV = coder.reconstructImage(decodedResiduals, imageYUV.size(), imageYUV.type(), selectedPredictor);

    // Convert back to RGB
    Mat reconstructedRGB;
    cvtColor(reconstructedYUV, reconstructedRGB, COLOR_YUV2BGR);

    // Save the reconstructed image
    imwrite("reconstructed_image.ppm", reconstructedRGB);

    // Display results
    Mat comparison;
    hconcat(image, reconstructedRGB, comparison);
    imshow("Original (Left) vs Reconstructed (Right)", comparison);
    waitKey(0);

    return 0;
}
