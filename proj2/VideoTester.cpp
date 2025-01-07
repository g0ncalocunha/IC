#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include "bitStream.h" 
#include "CompressionVerifier.cpp"

using namespace std;
using namespace cv;

class VideoTester {
public:
    struct VideoVerificationResults {
        bool isLossless;
        double averagePSNR;
        double compressionRatio;
        int totalFrames;
        int losslessFrames;
        int totalPixels;
        int mismatchedPixels;
    };

    VideoVerificationResults verifyVideoCompression(const string& originalPath, 
                                                     const string& reconstructedPath, 
                                                     const string& encodedFile) {
        VideoCapture originalVideo(originalPath);
        VideoCapture reconstructedVideo(reconstructedPath);

        if (!originalVideo.isOpened() || !reconstructedVideo.isOpened()) {
            throw runtime_error("Cannot open video files for verification.");
        }

        CompressionVerifier verifier;
        VideoVerificationResults videoResults = {};
        videoResults.isLossless = true;

        Mat originalFrame, reconstructedFrame;
        int frameCount = 0;
        double totalPSNR = 0.0;

        while (true) {
            originalVideo >> originalFrame;
            reconstructedVideo >> reconstructedFrame;

            if (originalFrame.empty() || reconstructedFrame.empty()) {
                break;
            }

            // Verify dimensions match
            if (originalFrame.size() != reconstructedFrame.size()) {
                throw runtime_error("Frame size mismatch: Original vs. Reconstructed");
            }

            auto frameResults = verifier.verifyCompression(originalFrame, reconstructedFrame, encodedFile);

            if (!frameResults.isLossless) {
                videoResults.isLossless = false;
                videoResults.mismatchedPixels += frameResults.mismatchedPixels;
            } else {
                videoResults.losslessFrames++;
            }

            totalPSNR += frameResults.psnr;
            frameCount++;
        }

        // Finalize results
        videoResults.totalFrames = frameCount;
        videoResults.averagePSNR = frameCount > 0 ? totalPSNR / frameCount : 0.0;

        // Calculate total pixels
        if (frameCount > 0) {
            videoResults.totalPixels = originalFrame.rows * originalFrame.cols * originalFrame.channels() * frameCount;
        } else {
            videoResults.totalPixels = 0;
        }

        // Calculate compression ratio
        ifstream encodedStream(encodedFile, ios::binary | ios::ate);
        double compressedSize = encodedStream.tellg();
        encodedStream.close();

        if (compressedSize > 0) {
            videoResults.compressionRatio = static_cast<double>(videoResults.totalPixels) / compressedSize;
        } else {
            videoResults.compressionRatio = 0; // Avoid division by zero
        }

        return videoResults;
    }

    void printVideoResults(const VideoVerificationResults& results) {
        cout << "\n=== Video Compression Verification Results ===\n";
        cout << "Total frames: " << results.totalFrames << "\n";
        cout << "Lossless frames: " << results.losslessFrames << " ("
             << (100.0 * results.losslessFrames / results.totalFrames) << "%)\n";
        cout << "Total pixels: " << results.totalPixels << "\n";
        cout << "Mismatched pixels: " << results.mismatchedPixels << "\n";
        cout << "Average PSNR: " << fixed << setprecision(2) << results.averagePSNR << " dB\n";
        cout << "Compression ratio: " << fixed << setprecision(2) << results.compressionRatio << ":1\n";
        cout << "Space saving: " << fixed << setprecision(2) 
             << (1 - 1 / results.compressionRatio) * 100 << "%\n";
        cout << "===============================================\n";
    }
};
int main() {
    try {
        string originalPath = "videos/akiyo_cif.y4m";
        string reconstructedPath = "output.mp4";
        string encodedFile = "encoded.bin";

        VideoTester videoVerifier;
        auto results = videoVerifier.verifyVideoCompression(originalPath, reconstructedPath, encodedFile);
        videoVerifier.printVideoResults(results);

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}


