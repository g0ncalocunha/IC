#include <iostream>
#include <string>
#include "video/videoCoder.h"
#include "image/imageCoder.h"
#include "audio/audioCoding.h"

using namespace std;

void encodeImage(string inputPath) {
    // string inputPath = "../proj2/input/images/sample.jpg";
    string encodedPath = "../proj2/output/encoded_image.bin";
    string outputPath = "../proj2/output/decoded_image.jpg";

    ImageCoder coder;
    cout << "Encoding image..." << endl;
    coder.encodeImage(inputPath, encodedPath);
    cout << "Decoding image..." << endl;
    coder.decodeImage(encodedPath, outputPath);
    cout << "Image encoding and decoding completed." << endl;
}

void encodeAudio(string inputPath) {
    // string inputPath = "../proj2/input/audio/sample.wav";
    string encodedPath = "../proj2/output/encoded_audio.bin";
    string outputPath = "../proj2/output/decoded_audio.wav";

    AudioCodec codec;
    cout << "Encoding audio..." << endl;
    codec.encode(inputPath, encodedPath);
    cout << "Decoding audio..." << endl;
    codec.decode(encodedPath, outputPath);
    cout << "Audio encoding and decoding completed." << endl;
}

void encodeVideo(string inputPath) {
    string inputPath = "../proj2/input/videos/sample.mp4";
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
    string inputPath;
    cout << "Select encoding type:" << endl;
    cout << "1. Image Encoding" << endl;
    cout << "2. Audio Encoding" << endl;
    cout << "3. Video Encoding" << endl;
    cout << "Enter your choice: ";
    cin >> choice;

    switch (choice) {
        case 1:
            cout << "Enter the path to image" << endl;
            cin >> inputPath;
            inputPath = "../proj2" + inputPath;
            encodeImage(inputPath);
            break;
        case 2:
            cout << "Enter the path to audio" << endl;
            cin >> inputPath;
            inputPath = "../proj2" + inputPath;
            encodeAudio(inputPath);
            break;
        case 3:
            cout << "Enter the path to video" << endl;
            cin >> inputPath;
            inputPath = "../proj2" + inputPath;
            encodeVideo(inputPath);
            break;
        default:
            cout << "Invalid choice." << endl;
            continue;
    }
  }

    return 0;
}