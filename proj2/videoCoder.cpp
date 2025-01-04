#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
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
        for(int c = 0; c < frame.channels(); c++) {
            result[c].reserve(frame.rows * frame.cols + 2);
            result[c].push_back(frame.rows);
            result[c].push_back(frame.cols);
            
            for(int i = 0; i < frame.rows; i++) {
                for(int j = 0; j < frame.cols; j++) {
                    int val = frame.at<Vec3b>(i, j)[c];
                    result[c].push_back((val * 64) / 255);
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
        
        if (rows <= 0 || cols <= 0) {
            throw runtime_error("Invalid dimensions in vector");
        }

        Mat result;
        if (vec.size() == 1) {
            result = Mat::zeros(rows, cols, CV_8UC1);
        } else if (vec.size() == 3) {
            result = Mat::zeros(rows, cols, CV_8UC3);
        } else {
            throw runtime_error("Unsupported number of channels");
        }

        for (size_t c = 0; c < vec.size(); c++) {
            if (vec[c].size() != static_cast<size_t>(rows * cols + 2)) {
                throw runtime_error("Inconsistent vector size");
            }

            int idx = 2;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    if (vec.size() == 1) {
                        result.at<uchar>(i, j) = saturate_cast<uchar>(vec[c][idx++] * 255 / 64);
                    } else {
                        result.at<Vec3b>(i, j)[c] = saturate_cast<uchar>(vec[c][idx++] * 255 / 64);
                    }
                }
            }
        }
        return result;
    }

    void writeFrame(ofstream& outFile, const vector<vector<int>>& frameData, const string& tempFile) {
        imageCoder.encodeWithGolomb(frameData, tempFile);
        
        ifstream temp(tempFile, ios::binary | ios::ate);
        size_t fileSize = temp.tellg();
        temp.seekg(0);

        outFile.write(reinterpret_cast<const char*>(&fileSize), sizeof(size_t));
        
        outFile << temp.rdbuf();
        temp.close();
    }

    vector<vector<int>> readFrame(ifstream& inFile, const string& tempFile) {
        size_t fileSize;
        inFile.read(reinterpret_cast<char*>(&fileSize), sizeof(size_t));
        
        vector<char> buffer(fileSize);
        inFile.read(buffer.data(), fileSize);
        
        ofstream temp(tempFile, ios::binary);
        temp.write(buffer.data(), fileSize);
        temp.close();
        
        return imageCoder.decodeWithGolomb(tempFile);
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

        ofstream outFile(outputPath, ios::binary);
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));

        Mat frame, yuv_frame, prevFrame;
        int frameCount = 0;
        string tempFile = "temp_frame.bin";

        while (cap.read(frame)) {
            if (frame.empty()) break;

            // Convert to YUV color space
            cvtColor(frame, yuv_frame, COLOR_BGR2YUV);

            bool isIFrame = (frameCount % iFrameInterval == 0);
            outFile.write(reinterpret_cast<const char*>(&isIFrame), sizeof(bool));

            if (isIFrame) {
                cout << "Encoding I-frame " << frameCount << "/" << header.totalFrames << endl;
                vector<vector<int>> frameVector = matToVector(yuv_frame);
                writeFrame(outFile, frameVector, tempFile);
                yuv_frame.copyTo(prevFrame);
            } else {
                cout << "Encoding P-frame " << frameCount << endl;
                for(int y = 0; y < yuv_frame.rows; y += blockSize) {
                    for(int x = 0; x < yuv_frame.cols; x += blockSize) {
                        Rect blockRect(x, y, 
                                     min(blockSize, yuv_frame.cols - x),
                                     min(blockSize, yuv_frame.rows - y));
                        
                        Mat currentBlock = yuv_frame(blockRect);
                        Point2i mv = estimateMotion(yuv_frame, prevFrame, x, y, blockSize, searchRange);
                        
                        Rect predRect(x + mv.x, y + mv.y, blockRect.width, blockRect.height);
                        
                        // Check if prediction block is within bounds
                        predRect.x = max(0, min(predRect.x, prevFrame.cols - predRect.width));
                        predRect.y = max(0, min(predRect.y, prevFrame.rows - predRect.height));
                        
                        Mat predictedBlock = prevFrame(predRect);
                        vector<vector<int>> residualVector = getResidualVector(currentBlock, predictedBlock);
                        
                        outFile.write(reinterpret_cast<const char*>(&mv), sizeof(Point2i));
                        writeFrame(outFile, residualVector, tempFile);
                    }
                }
            }
            
            yuv_frame.copyTo(prevFrame);
            frameCount++;
        }

        cap.release();
        outFile.close();
    }

    void decodeVideo(const string& inputPath, const string& outputPath) {
        ifstream inFile(inputPath, ios::binary);
        if (!inFile) {
            throw runtime_error("Cannot open input file: " + inputPath);
        }

        VideoHeader header;
        inFile.read(reinterpret_cast<char*>(&header), sizeof(header));

        VideoWriter writer(outputPath, VideoWriter::fourcc('a','v','c','1'), 
                         header.fps, Size(header.width, header.height));

        Mat frame, prevFrame;
        string tempFile = "temp_decode.bin";
        int frameCount = 0;

        while (inFile.peek() != EOF && frameCount < header.totalFrames) {
            bool isIFrame;
            inFile.read(reinterpret_cast<char*>(&isIFrame), sizeof(bool));

            if (isIFrame) {
                cout << "Decoding I-frame " << frameCount << "/" << header.totalFrames << endl;
                
                vector<vector<int>> frameVector = readFrame(inFile, tempFile);
                Mat yuv_frame = vectorToMat(frameVector);
                
                if (!yuv_frame.empty()) {
                    cvtColor(yuv_frame, frame, COLOR_YUV2BGR);
                    writer.write(frame);
                    yuv_frame.copyTo(prevFrame);
                } else {
                    throw runtime_error("Failed to decode I-frame");
                }
            } else {
                cout << "Decoding P-frame " << frameCount << endl;
                Mat currentFrame = Mat::zeros(header.height, header.width, CV_8UC3);
                
                for(int y = 0; y < header.height; y += header.blockSize) {
                    for(int x = 0; x < header.width; x += header.blockSize) {
                        Point2i mv;
                        inFile.read(reinterpret_cast<char*>(&mv), sizeof(Point2i));
                        
                        Rect currentRect(x, y,
                                       min(header.blockSize, header.width - x),
                                       min(header.blockSize, header.height - y));
                        
                        Rect predRect(x + mv.x, y + mv.y, currentRect.width, currentRect.height);
                        predRect.x = max(0, min(predRect.x, prevFrame.cols - predRect.width));
                        predRect.y = max(0, min(predRect.y, prevFrame.rows - predRect.height));
                        
                        vector<vector<int>> residualVector = readFrame(inFile, tempFile);
                        Mat residual = vectorToMat(residualVector);
                        
                        Mat predictedBlock = prevFrame(predRect);
                        Mat reconstructedBlock;
                        add(predictedBlock, residual, reconstructedBlock);
                        reconstructedBlock.copyTo(currentFrame(currentRect));
                    }
                }
                
                cvtColor(currentFrame, frame, COLOR_YUV2BGR);
                writer.write(frame);
                currentFrame.copyTo(prevFrame);
            }
            
            frameCount++;
        }

        writer.release();
        inFile.close();
    }
};

int main() {
    try {
        string inputPath = "videos/akiyo_cif.y4m";
        string encodedPath = "encoded.bin";
        string outputPath = "output.mp4";
        int iFrameInterval = 10;  // I-frame every 10 frames
        int blockSize = 16;       // 16x16 blocks
        int searchRange = 16;     // Search range of ±16 pixels

        VideoCoder coder;
        
        cout << "Encoding video..." << endl;
        cout << "I-frame interval: " << iFrameInterval << endl;
        cout << "Block size: " << blockSize << endl;
        cout << "Search range: " << searchRange << endl;
        
        coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);
        
        cout << "Decoding video..." << endl;
        coder.decodeVideo(encodedPath, outputPath);
        
        cout << "Complete! Output saved to: " << outputPath << endl;
        
    } catch(const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}