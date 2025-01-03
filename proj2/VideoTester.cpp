#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <iomanip>
#include "CompressionVerifier.cpp"

using namespace std;
using namespace cv;

int main() {
    string originalVideoPath = "videos/videos/akiyo_cif.y4m";
    string reconstructedVideoPath = "decoded_output.avi";

    CompressionVerifier verifier;

    VideoCapture originalVideo(originalVideoPath);
    VideoCapture reconstructedVideo(reconstructedVideoPath);

    if (!originalVideo.isOpened()) {
        cerr << "Error: Could not open the original video." << endl;
        return -1;
    }
    if (!reconstructedVideo.isOpened()) {
        cerr << "Error: Could not open the reconstructed video." << endl;
        return -1;
    }

    Mat originalFrame, reconstructedFrame;
    int frameIndex = 0;
    bool lossless = true;

    while (true) {
        originalVideo >> originalFrame;
        reconstructedVideo >> reconstructedFrame;

        if (originalFrame.empty() || reconstructedFrame.empty()) {
            break; 
        }

        CompressionVerifier::VerificationResults frameResults = 
            verifier.verifyCompression(originalFrame, reconstructedFrame, "");

        cout << "Frame " << frameIndex << ": ";
        if (frameResults.isLossless) {
            cout << "Lossless" << endl;
        } else {
            lossless = false;
            cout << "Lossy" << endl;
            cout << "  Total Pixels: " << frameResults.totalPixels << endl;
            cout << "  Mismatched Pixels: " << frameResults.mismatchedPixels
                 << " (" << fixed << setprecision(2) 
                 << (100.0 * frameResults.mismatchedPixels / frameResults.totalPixels) << "%)" << endl;
            cout << "  PSNR: " << frameResults.psnr << " dB" << endl;
        }

        frameIndex++;
    }

    originalVideo.release();
    reconstructedVideo.release();

    if (lossless) {
        cout << "The video compression was lossless for all frames." << endl;
    } else {
        cout << "Warning: Some frames were lossy during compression." << endl;
    }

    return 0;
}
