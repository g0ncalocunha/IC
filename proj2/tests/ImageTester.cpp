#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "bitStream.h"

#include "CompressionVerifier.cpp"

using namespace std;
using namespace cv;

int main() {
    string originalImagePath = "images/image.ppm";
    string reconstructedImagePath = "reconstructed_image.ppm";
    string encodedFilePath = "encoded_residuals.bin";

    Mat originalImage = imread(originalImagePath, IMREAD_COLOR);
    if (originalImage.empty()) {
        cerr << "Error: Could not load the original image." << endl;
        return -1;
    }

    Mat reconstructedImage = imread(reconstructedImagePath, IMREAD_COLOR);
    if (reconstructedImage.empty()) {
        cerr << "Error: Could not load the reconstructed image." << endl;
        return -1;
    }

    CompressionVerifier verifier;

    CompressionVerifier::VerificationResults results = 
        verifier.verifyCompression(originalImage, reconstructedImage, encodedFilePath);

    verifier.printResults(results);

    return 0;
}
