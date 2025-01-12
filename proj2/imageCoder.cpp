#include "imageCoder.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <filesystem> // For file size
#include <numeric>    // For residual mean calculation

int ImageCoder::mapToNonNegative(int n) {
    return (n >= 0) ? 2 * n : (-2 * n - 1);
}

int ImageCoder::mapToSigned(int n) {
    return (n % 2 == 0) ? (n / 2) : (-(n + 1) / 2);
}

vector<vector<int>> ImageCoder::calculateResiduals(const Mat& image, Predictors::Standards standard) {
    vector<vector<int>> residuals(image.channels());
    int rows = image.rows;
    int cols = image.cols;

    residuals[0].push_back(rows);
    residuals[0].push_back(cols);

    for (int c = 0; c < image.channels(); ++c) {
        residuals[c].reserve(rows * cols + 2);
        if (c > 0) {
            residuals[c].push_back(rows);
            residuals[c].push_back(cols);
        }

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                uchar actual = image.at<Vec3b>(i, j)[c];
                int left = (j > 0) ? image.at<Vec3b>(i, j - 1)[c] : 0;
                int above = (i > 0) ? image.at<Vec3b>(i - 1, j)[c] : 0;
                int diag = (i > 0 && j > 0) ? image.at<Vec3b>(i - 1, j - 1)[c] : 0;

                int predicted = predictors.predictors(standard, left, above, diag);
                int residual = static_cast<int>(actual) - predicted;
                residuals[c].push_back(mapToNonNegative(residual));
            }
        }
    }
    return residuals;
}

void ImageCoder::encodeWithGolomb(const vector<vector<int>>& residuals, const string& filename) {
    bitStream bs;
    bs.openFile(filename, ios::out | ios::binary);

    int numChannels = residuals.size();
    bs.writeBits(numChannels, 8);

    for (const auto& channelResiduals : residuals) {
        bs.writeBits(channelResiduals[0], 16);
        bs.writeBits(channelResiduals[1], 16);

        double mean = 0.0;
        for (size_t i = 2; i < channelResiduals.size(); ++i) {
            mean += channelResiduals[i];
        }
        mean /= (channelResiduals.size() - 2);

        m = max(1, (int)round(-1 / log2(mean / (mean + 1))));
        m = 1 << (int)ceil(log2(m));

        bs.writeBits(m, 16);

        golomb golombCoder(m);
        for (size_t i = 2; i < channelResiduals.size(); ++i) {
            golombCoder.encode(channelResiduals[i], bs);
        }
    }

    bs.flushBuffer();
}

vector<vector<int>> ImageCoder::decodeWithGolomb(const string& filename) {
    bitStream bs;
    bs.openFile(filename, ios::in | ios::binary);

    int numChannels = bs.readBits(8);
    vector<vector<int>> residuals(numChannels);

    for (int c = 0; c < numChannels; ++c) {
        int rows = bs.readBits(16);
        int cols = bs.readBits(16);

        m = bs.readBits(16);
        golomb golombCoder(m);

        vector<int> channelResiduals;
        channelResiduals.push_back(rows);
        channelResiduals.push_back(cols);
        channelResiduals.reserve(rows * cols + 2);

        for (int i = 0; i < rows * cols; ++i) {
            int encoded = golombCoder.decode(bs);
            channelResiduals.push_back(mapToSigned(encoded));
        }
        residuals[c] = move(channelResiduals);
    }

    return residuals;
}

Mat ImageCoder::reconstructImage(const vector<vector<int>>& residuals, Predictors::Standards standard) {
    int rows = residuals[0][0];
    int cols = residuals[0][1];
    int channels = residuals.size();

    Mat reconstructed = Mat::zeros(rows, cols, CV_8UC3);

    for (int c = 0; c < channels; ++c) {
        int idx = 2;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                int left = (j > 0) ? reconstructed.at<Vec3b>(i, j - 1)[c] : 0;
                int above = (i > 0) ? reconstructed.at<Vec3b>(i - 1, j)[c] : 0;
                int diag = (i > 0 && j > 0) ? reconstructed.at<Vec3b>(i - 1, j - 1)[c] : 0;

                int predicted = predictors.predictors(standard, left, above, diag);
                int pixel = predicted + residuals[c][idx++];
                pixel = ((pixel % 256) + 256) % 256;
                reconstructed.at<Vec3b>(i, j)[c] = static_cast<uchar>(pixel);
            }
        }
    }
    return reconstructed;
}

// int main() {
//     using namespace std::chrono;
    
//     string inputFile = "images/image.ppm";
//     string encodedFile = "encoded_residuals.bin";
    
//     Mat image = imread(inputFile, IMREAD_COLOR);
//     if (image.empty()) {
//         cerr << "Error: Could not load the image." << endl;
//         return -1;
//     }

//     ImageCoder coder;
//     Predictors::Standards selectedPredictor = Predictors::JPEG_PL;

//     // Encode
//     auto start = high_resolution_clock::now();
//     vector<vector<int>> residuals = coder.calculateResiduals(image, selectedPredictor);
//     coder.encodeWithGolomb(residuals, encodedFile);
//     auto encodeEnd = high_resolution_clock::now();

//     // Decode
//     auto decodeStart = high_resolution_clock::now();
//     vector<vector<int>> decodedResiduals = coder.decodeWithGolomb(encodedFile);
//     Mat reconstructed = coder.reconstructImage(decodedResiduals, selectedPredictor);
//     auto end = high_resolution_clock::now();

//     // Save reconstructed image
//     imwrite("reconstructed_image.ppm", reconstructed);

//     // Convert images to grayscale for comparison
//     Mat grayImage, grayReconstructed;
//     cvtColor(image, grayImage, COLOR_BGR2GRAY);
//     cvtColor(reconstructed, grayReconstructed, COLOR_BGR2GRAY);

//     // Verify lossless compression
//     Mat diff;
//     absdiff(grayImage, grayReconstructed, diff);
//     int totalDiff = countNonZero(diff);

//     if (totalDiff == 0) {
//         cout << "Compression is lossless!" << endl;
//     } else {
//         cout << "Warning: Compression is lossy. Total different pixels: " << totalDiff << endl;
//     }

//     // Performance metrics
//     auto encodeDuration = duration_cast<milliseconds>(encodeEnd - start).count();
//     auto decodeDuration = duration_cast<milliseconds>(end - decodeStart).count();
//     auto totalDuration = duration_cast<milliseconds>(end - start).count();

//     cout << "Encoding Time: " << encodeDuration << " ms" << endl;
//     cout << "Decoding Time: " << decodeDuration << " ms" << endl;
//     cout << "Total Time: " << totalDuration << " ms" << endl;

//     // Compression ratio
//     auto inputSize = filesystem::file_size(inputFile);
//     auto encodedSize = filesystem::file_size(encodedFile);
//     double compressionRatio = static_cast<double>(inputSize) / encodedSize;

//     cout << "Compression Ratio: " << compressionRatio << endl;

//     // Residual analysis
//     for (size_t c = 0; c < residuals.size(); ++c) {
//         auto &channel = residuals[c];
//         double meanResidual = accumulate(channel.begin() + 2, channel.end(), 0.0) / (channel.size() - 2);
//         cout << "Channel " << c << " Mean Residual: " << meanResidual << endl;
//     }

//     return 0;
// }