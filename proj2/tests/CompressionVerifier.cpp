#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include "../util/bitStream.h"

using namespace std;
using namespace cv;

class CompressionVerifier {
public:
    struct VerificationResults {
        bool isLossless;
        int totalPixels;
        int mismatchedPixels;
        double psnr;
        vector<int> channelDifferences;
        double compressionRatio;
    };

    VerificationResults verifyCompression(const Mat& original, const Mat& reconstructed, const string& encodedFile) {
        VerificationResults results;
        results.isLossless = true;
        results.totalPixels = original.rows * original.cols * original.channels();
        results.mismatchedPixels = 0;
        results.channelDifferences.resize(original.channels(), 0);
        
        // Calculate MSE for PSNR
        double mse = 0.0;
        
        for (int i = 0; i < original.rows; i++) {
            for (int j = 0; j < original.cols; j++) {
                for (int c = 0; c < original.channels(); c++) {
                    int originalVal = original.at<Vec3b>(i, j)[c];
                    int reconstructedVal = reconstructed.at<Vec3b>(i, j)[c];
                    
                    if (originalVal != reconstructedVal) {
                        results.isLossless = false;
                        results.mismatchedPixels++;
                        results.channelDifferences[c]++;
                        
                        double diff = originalVal - reconstructedVal;
                        mse += diff * diff;
                    }
                }
            }
        }
        
        // Calculate PSNR
        mse /= results.totalPixels;
        results.psnr = (mse > 0) ? 20 * log10(255.0 / sqrt(mse)) : INFINITY;
        
        // Calculate compression ratio
        ifstream encodedStream(encodedFile, ios::binary | ios::ate);
        double compressedSize = encodedStream.tellg();
        double originalSize = results.totalPixels;
        results.compressionRatio = originalSize / compressedSize;
        
        return results;
    }

    void printResults(const VerificationResults& results) {
        cout << "\n=== Compression Verification Results ===\n";
        cout << "Lossless: " << (results.isLossless ? "Yes" : "No") << "\n";
        cout << fixed << setprecision(2);
        
        if (!results.isLossless) {
            cout << "Total pixels: " << results.totalPixels << "\n";
            cout << "Mismatched pixels: " << results.mismatchedPixels << " ("
                 << (100.0 * results.mismatchedPixels / results.totalPixels) << "%)\n";
            cout << "PSNR: " << results.psnr << " dB\n";
            
            cout << "Channel differences:\n";
            for (size_t i = 0; i < results.channelDifferences.size(); i++) {
                cout << "Channel " << i << ": " << results.channelDifferences[i] << " pixels\n";
            }
        }
        
        cout << "Compression ratio: " << results.compressionRatio << ":1\n";
        cout << "Space saving: " << ((1 - 1/results.compressionRatio) * 100) << "%\n";
        cout << "========================================\n";
    }
};