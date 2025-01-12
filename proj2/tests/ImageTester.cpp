#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "../header/bitStream.h"

#include "CompressionVerifier.cpp"

using namespace std;
using namespace cv;

size_t getFileSize(const string& filename) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << ". File might not exist or is inaccessible." << endl;
        return 0;
    }
    size_t size = file.tellg();
    if (size == -1) {
        cerr << "Error: Could not retrieve size for file " << filename << endl;
        return 0;
    }
    return size;
}

string formatFileSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }
    
    stringstream ss;
    ss << fixed << setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

int main() {
    string originalImagePath = "../images/image.ppm";
    string reconstructedImagePath = "../reconstructed_image.ppm";
    string encodedFilePath = "../encoded_residuals.bin";

    size_t originalSize = getFileSize(originalImagePath);
    size_t encodedSize = getFileSize(encodedFilePath);

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

    cout << "\n=======File Size Analysis:==============" << endl;
    cout << "Original Image: " << formatFileSize(originalSize) 
         << " (" << originalSize << " bytes)" << endl;
    cout << "Encoded File:   " << formatFileSize(encodedSize) 
         << " (" << encodedSize << " bytes)" << endl;
    cout << "========================================" << endl;

    return 0;
}