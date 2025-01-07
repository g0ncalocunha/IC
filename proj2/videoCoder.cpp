#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include "imageCoder.h"
#include "bitStream.h"

using namespace std;
using namespace cv;

struct VideoHeader {
    int width;
    int height;
    int blockSize;
    int iFrameInterval;
    int searchRange;
    double fps;
    int totalFrames;
};

class VideoCoder {
private:
    ImageCoder imageCoder;
    
    int mapValue(int value) {
        value = max(-255, min(255, value));
        return (value + 255) * 64 / 510;
    }

    Point2i estimateMotion(const Mat& currentFrame, const Mat& referenceFrame, 
                          int blockX, int blockY, int blockSize, int searchRange) {
        Point2i bestMotion(0, 0);
        double minSAD = numeric_limits<double>::max();
        
        Rect currentBlock(blockX, blockY, 
                         min(blockSize, currentFrame.cols - blockX),
                         min(blockSize, currentFrame.rows - blockY));
        
        for(int dy = -searchRange; dy <= searchRange; dy++) {
            for(int dx = -searchRange; dx <= searchRange; dx++) {
                if(blockY + dy < 0 || blockX + dx < 0 || 
                   blockY + blockSize + dy > referenceFrame.rows ||
                   blockX + blockSize + dx > referenceFrame.cols) {
                    continue;
                }
                
                Rect searchBlock(blockX + dx, blockY + dy, 
                               currentBlock.width, currentBlock.height);
                Mat curr = currentFrame(currentBlock);
                Mat ref = referenceFrame(searchBlock);
                
                Mat diff;
                absdiff(curr, ref, diff);
                double SAD = sum(diff)[0];
                
                if(SAD < minSAD) {
                    minSAD = SAD;
                    bestMotion = Point2i(dx, dy);
                }
            }
        }
        return bestMotion;
    }

    vector<vector<int>> getResidualVector(const Mat& currentBlock, const Mat& predictedBlock) {
        Mat residual;
        absdiff(currentBlock, predictedBlock, residual);
        
        vector<vector<int>> result(residual.channels());
        for(int c = 0; c < residual.channels(); c++) {
            result[c].reserve(residual.rows * residual.cols + 2);
            result[c].push_back(residual.rows);
            result[c].push_back(residual.cols);
            
            for(int i = 0; i < residual.rows; i++) {
                for(int j = 0; j < residual.cols; j++) {
                    int val = currentBlock.at<Vec3b>(i, j)[c] - 
                             predictedBlock.at<Vec3b>(i, j)[c];
                    result[c].push_back(mapValue(val));
                }
            }
        }
        return result;
    }
    
    vector<vector<int>> matToVector(const Mat& frame) {
        vector<vector<int>> result(frame.channels());
        for (int c = 0; c < frame.channels(); c++) {
            result[c].reserve(frame.rows * frame.cols + 2);
            result[c].push_back(frame.rows);
            result[c].push_back(frame.cols);

            for (int i = 0; i < frame.rows; i++) {
                for (int j = 0; j < frame.cols; j++) {
                    int val = frame.at<Vec3b>(i, j)[c];
                    result[c].push_back(val);
                }
            }
        }
        return result;
    }

    Mat vectorToMat(const vector<vector<int>>& vec) {
        if (vec.empty() || vec[0].size() < 2) {
            throw runtime_error("Invalid vector format");
        }

        int rows = vec[0][0];
        int cols = vec[0][1];

        Mat result(rows, cols, CV_8UC3);

        for (size_t c = 0; c < vec.size(); c++) {
            int idx = 2;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    result.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(vec[c][idx++]);
                }
            }
        }
        return result;
    }

    void writeFrame(ofstream& outFile, const vector<vector<int>>& frameData) {
        for (const auto& channel : frameData) {
            size_t size = channel.size();
            outFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
            outFile.write(reinterpret_cast<const char*>(channel.data()), size * sizeof(int));
        }
    }

    vector<vector<int>> readFrame(ifstream& inFile) {
        vector<vector<int>> frameData(3);
        for (auto& channel : frameData) {
            size_t size;
            inFile.read(reinterpret_cast<char*>(&size), sizeof(size_t));
            channel.resize(size);
            inFile.read(reinterpret_cast<char*>(channel.data()), size * sizeof(int));
        }
        return frameData;
    }

public:
    void encodeVideo(const string& inputPath, const string& outputPath, 
                 int iFrameInterval, int blockSize, int searchRange) {
        VideoCapture cap(inputPath);
        if (!cap.isOpened()) {
            throw runtime_error("Cannot open input video: " + inputPath);
        }

        VideoHeader header = {
            static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH)),
            static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT)),
            blockSize,
            iFrameInterval,
            searchRange,
            cap.get(CAP_PROP_FPS),
            static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT))
        };

        bitStream bs;
        bs.openFile(outputPath, std::ios::out | std::ios::binary);

        bs.writeBits(header.width, 16);
        bs.writeBits(header.height, 16);
        bs.writeBits(static_cast<int>(header.fps), 16);
        bs.writeBits(header.totalFrames, 32);

        Mat frame, prevFrame;
        int frameCount = 0;

        while (cap.read(frame)) {
            if (frame.empty()) break;

            bool isIFrame = (frameCount % iFrameInterval == 0);
            bs.writeBit(isIFrame);

            if (isIFrame) {
                // Write I-frame
                for (int i = 0; i < frame.rows; i++) {
                    for (int j = 0; j < frame.cols; j++) {
                        Vec3b pixel = frame.at<Vec3b>(i, j);
                        bs.writeBits(pixel[0], 8);
                        bs.writeBits(pixel[1], 8);
                        bs.writeBits(pixel[2], 8);
                    }
                }
                frame.copyTo(prevFrame);
            } else {
                // Write P-frame
                for (int i = 0; i < frame.rows; i++) {
                    for (int j = 0; j < frame.cols; j++) {
                        Vec3b currPixel = frame.at<Vec3b>(i, j);
                        Vec3b prevPixel = prevFrame.at<Vec3b>(i, j);

                        for (int c = 0; c < 3; c++) {
                            int diff = currPixel[c] - prevPixel[c];
                            if (abs(diff) < 16) {
                                bs.writeBit(0);
                                bs.writeBits(abs(diff), 4);
                                bs.writeBit(diff < 0);
                            } else {
                                bs.writeBit(1);
                                bs.writeBits(currPixel[c], 8);
                            }
                        }
                    }
                }
                frame.copyTo(prevFrame);
            }

            frameCount++;
        }

        cap.release();
    }


    void decodeVideo(const string& inputPath, const string& outputPath) {
        bitStream bs;
        bs.openFile(inputPath, std::ios::in | std::ios::binary);

        int width = bs.readBits(16);
        int height = bs.readBits(16);
        int fps = bs.readBits(16);
        int totalFrames = bs.readBits(32);

        VideoWriter writer(outputPath, VideoWriter::fourcc('a', 'v', 'c', '1'),
                        fps, Size(width, height));

        Mat frame(height, width, CV_8UC3);
        Mat prevFrame;
        int frameCount = 0;

        while (frameCount < totalFrames) {
            bool isIFrame = bs.readBit();

            if (isIFrame) {
                // Decode I-frame
                for (int i = 0; i < height; i++) {
                    for (int j = 0; j < width; j++) {
                        Vec3b& pixel = frame.at<Vec3b>(i, j);
                        pixel[0] = bs.readBits(8);
                        pixel[1] = bs.readBits(8);
                        pixel[2] = bs.readBits(8);
                    }
                }
                frame.copyTo(prevFrame);
            } else {
                // Decode P-frame
                for (int i = 0; i < height; i++) {
                    for (int j = 0; j < width; j++) {
                        Vec3b& pixel = frame.at<Vec3b>(i, j);
                        Vec3b prevPixel = prevFrame.at<Vec3b>(i, j);

                        for (int c = 0; c < 3; c++) {
                            if (bs.readBit() == 0) {
                                int diff = bs.readBits(4);
                                if (bs.readBit()) diff = -diff;
                                pixel[c] = prevPixel[c] + diff;
                            } else {
                                pixel[c] = bs.readBits(8);
                            }
                        }
                    }
                }
                frame.copyTo(prevFrame);
            }

            writer.write(frame);
            frameCount++;
        }

        writer.release();
    }

};

int main() {
    try {
        string inputPath = "videos/akiyo_cif.y4m";
        string encodedPath = "encoded.bin";
        string outputPath = "output.mp4";
        int iFrameInterval = 10;
        int blockSize = 16;
        int searchRange = 16;

        VideoCoder coder;

        cout << "Encoding video..." << endl;
        coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);

        cout << "Decoding video..." << endl;
        coder.decodeVideo(encodedPath, outputPath);

        cout << "Verifying if compression is lossless..." << endl;

        // Open original and reconstructed videos for comparison
        VideoCapture originalVideo(inputPath);
        VideoCapture reconstructedVideo(outputPath);

        if (!originalVideo.isOpened() || !reconstructedVideo.isOpened()) {
            throw runtime_error("Failed to open video files for verification.");
        }

        Mat originalFrame, reconstructedFrame;
        int totalFrames = 0;
        int mismatchedFrames = 0;

        while (true) {
            originalVideo >> originalFrame;
            reconstructedVideo >> reconstructedFrame;

            if (originalFrame.empty() || reconstructedFrame.empty()) {
                break; // End of video
            }

            totalFrames++;

            // Check if the frames are identical
            if (originalFrame.size() != reconstructedFrame.size() ||
                originalFrame.type() != reconstructedFrame.type()) {
                cerr << "Frame size or type mismatch at frame " << totalFrames << endl;
                mismatchedFrames++;
                continue;
            }

            Mat diff;
            absdiff(originalFrame, reconstructedFrame, diff);

            // Convert difference to grayscale
            Mat grayDiff;
            cvtColor(diff, grayDiff, COLOR_BGR2GRAY);

            if (countNonZero(grayDiff) > 0) {
                cout << "Mismatch detected in frame " << totalFrames << endl;
                mismatchedFrames++;
            }
        }


        cout << "\n=== Compression Verification ===\n";
        cout << "Total frames: " << totalFrames << endl;
        cout << "Mismatched frames: " << mismatchedFrames << endl;

        if (mismatchedFrames == 0) {
            cout << "Compression is lossless." << endl;
        } else {
            cout << "Compression is not lossless. " << mismatchedFrames 
                 << " frames have mismatches." << endl;
        }

        cout << "================================\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
