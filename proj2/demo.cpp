#include <iostream>
#include <string>
#include "video/videoCoder.cpp"
#include "image/imageCoder.h"
#include "audio/audioCoding.cpp"

using namespace std;

int encodeImage(string inputFile) {
    // string inputPath = "../proj2/input/images/sample.jpg";
    string encodedFile = "../proj2/output/encoded_image.bin";
    string outputPath = "../proj2/output/decoded_image.jpg";
    Mat image = imread(inputFile, IMREAD_COLOR);
    if (image.empty()) {
        cerr << "Error: Could not load the image." << endl;
        return -1;
    }

    ImageCoder coder;
    Predictors::Standards selectedPredictor = Predictors::JPEG_PL;

    // Encode
    vector<vector<int>> residuals = coder.calculateResiduals(image, selectedPredictor);
    coder.encodeWithGolomb(residuals, encodedFile);

    // Decode
    vector<vector<int>> decodedResiduals = coder.decodeWithGolomb(encodedFile);
    Mat reconstructed = coder.reconstructImage(decodedResiduals, selectedPredictor);

    // Save reconstructed image
    imwrite("../proj2/output/decoded_image.ppm", reconstructed);
    
    // Verify lossless compression
    Mat diff;
    absdiff(image, reconstructed, diff);
    
    // Check each channel separately
    vector<Mat> diffChannels;
    split(diff, diffChannels);
    
    int totalDiff = 0;
    for (int i = 0; i < 3; i++) {
        int channelDiff = countNonZero(diffChannels[i]);
        totalDiff += channelDiff;
        if (channelDiff > 0) {
            cout << "Channel " << i << " has " << channelDiff << " different pixels" << endl;
        }
    }
    
    if (totalDiff == 0) {
        cout << "Compression is lossless!" << endl;
    } else {
        cout << "Warning: Compression is lossy. Total different pixels: " << totalDiff << endl;
    }

    // Display comparison
    Mat comparison;
    hconcat(image, reconstructed, comparison);
    imshow("Original (Left) vs Reconstructed (Right)", comparison);
    waitKey(0);
    return 0;
}

void encodeAudio(string inputPath, bool lossy, string bitrate) {
    // string inputPath = "../proj2/input/audio/sample.wav";
    string encodedPath = "../proj2/output/encoded_audio.bin";
    string outputPath = "../proj2/output/decoded_audio.wav";

    bool isLossy = false;
    double targetBitrate = 0;
    bool isAdaptive = false;

    if (lossy) {
        isLossy = true;
        targetBitrate = stod(bitrate);
    } else {
        isAdaptive = true;
    }

    AudioCodec codec(4, isAdaptive, isLossy, targetBitrate);

    cout << "Encoding..." << endl;
    codec.encode(inputPath, encodedPath);

    cout << "Decoding..." << endl;
    codec.decode(encodedPath, outputPath);

}

void encodeVideo(string inputPath) {
    // string inputPath = "../proj2/input/videos/sample.mp4";
    string encodedPath = "../proj2/output/encoded_video.bin";
    string outputPath = "../proj2/output/decoded_video.mp4";
    int iFrameInterval = 10;
    int blockSize = 16;
    int searchRange = 16;

    VideoCoder coder;
    cout << "Encoding video..." << endl;
    EncodingMetrics metrics = coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);

    cout << "Encoding completed." << endl;
    cout << "PSNR: " << metrics.psnr << " dB" << endl;
    cout << "MSE: " << metrics.mse << endl;
    cout << "Compression Ratio: " << metrics.compressionRatio << endl;
    cout << "Encoding Time: " << metrics.encodingTime << " seconds" << endl;

    cout << "Decoding video..." << endl;
    auto decodeStartTime = chrono::high_resolution_clock::now();
    coder.decodeVideo(encodedPath, outputPath);
    auto decodeEndTime = chrono::high_resolution_clock::now();
    double decodeTime = chrono::duration_cast<chrono::milliseconds>(decodeEndTime - decodeStartTime).count() / 1000.0;

    cout << "Decoding completed." << endl;       
    cout << "Decoding Time: " << decodeTime << " seconds" << endl;
}

int main() {
  while (true)
  {  
    int choice;
    string inputPath, fileName, bitrate;
    cout << "Select encoding type:" << endl;
    cout << "1. Image Encoding" << endl;
    cout << "2. Audio Encoding" << endl;
    cout << "3. Video Encoding" << endl;
    cout << "Enter your choice: ";
    cin >> choice;

    switch (choice) {
        case 1:
            cout << "Enter the image file name (has to be in /inputs/images)" << endl;
            cin >> fileName;
            inputPath = "../proj2/input/images/" + fileName;
            encodeImage(inputPath);
            break;

        case 2:
            cout << "Enter the audio file name (has to be in /inputs/audios)" << endl;
            cin >> fileName;
            inputPath = "../proj2/input/audios/" + fileName;
            char lossyChoice;
            bool lossy;
            bitrate = "0.0";
            cout << "Do you want lossy encoding? (y/n): ";
            cin >> lossyChoice;

            if (lossyChoice == 'y' || lossyChoice == 'Y')   {
                lossy = true;
                cout << "What's the target bitrate: ";
                cin >> bitrate;
            }
            encodeAudio(inputPath,lossy,bitrate);
            break;

        case 3:
            cout << "Enter the video file name (has to be in /inputs/videos)" << endl;
            cin >> fileName;
            inputPath = "../proj2/input/videos/" + fileName;
            encodeVideo(inputPath);
            break;
        default:
            cout << "Invalid choice." << endl;
            continue;
    }
  }

    return 0;
}