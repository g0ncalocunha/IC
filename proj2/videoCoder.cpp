#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "imageCoder.h"
#include "bitStream.h"

using namespace std;
using namespace cv;

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class VideoCoder {
private:
    ImageCoder imageCoder;
    
    std::vector<std::vector<int>> matToVector(const cv::Mat& frame) {
        std::vector<std::vector<int>> result(1);  // Single channel
        result[0].reserve(frame.rows * frame.cols + 2);
        result[0].push_back(frame.rows);
        result[0].push_back(frame.cols);
        
        for (int i = 0; i < frame.rows; ++i) {
            for (int j = 0; j < frame.cols; ++j) {
                int val = frame.at<uchar>(i, j);
                // Map values to ensure they're within valid range
                result[0].push_back(val > 0 ? val : 0);
            }
        }
        return result;
    }

    cv::Mat vectorToMat(const std::vector<std::vector<int>>& vec) {
        int rows = vec[0][0];
        int cols = vec[0][1];
        cv::Mat result = cv::Mat::zeros(rows, cols, CV_8UC1);
        
        int idx = 2;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                result.at<uchar>(i, j) = static_cast<uchar>(vec[0][idx++]);
            }
        }
        return result;
    }

    Point2i estimateMotion(const Mat& currentFrame, const Mat& referenceFrame, 
                              int blockX, int blockY, int blockSize, int searchRange) {
        int bestX = 0, bestY = 0;
        double minSAD = numeric_limits<double>::max();
        
        for (int dy = -searchRange; dy <= searchRange; dy++) {
            for (int dx = -searchRange; dx <= searchRange; dx++) {
                if (blockY + dy < 0 || blockX + dx < 0 || 
                    blockY + blockSize + dy > referenceFrame.rows ||
                    blockX + blockSize + dx > referenceFrame.cols) {
                    continue;
                }
                
                double SAD = 0;
                for (int y = 0; y < blockSize; y++) {
                    for (int x = 0; x < blockSize; x++) {
                        for (int c = 0; c < currentFrame.channels(); c++) {
                            int diff = currentFrame.at<Vec3b>(blockY + y, blockX + x)[c] - 
                                     referenceFrame.at<Vec3b>(blockY + y + dy, blockX + x + dx)[c];
                            SAD += abs(diff);
                        }
                    }
                }
                
                if (SAD < minSAD) {
                    minSAD = SAD;
                    bestX = dx;
                    bestY = dy;
                }
            }
        }
        
        return Point2i(bestX, bestY);
    }

public:
    void encodeVideo(const std::string& inputPath, const std::string& outputPath, 
                 int iFrameInterval, int blockSize, int searchRange) {
        cv::VideoCapture cap(inputPath);
        if (!cap.isOpened()) {
            throw std::runtime_error("Cannot open input video: " + inputPath);
        }

        // Convert frames to grayscale to match ImageCoder expectations
        cv::Mat color_frame, gray_frame, prevFrame;
        int frameCount = 0;
        std::string tempFile = "temp_frame.bin";
        
        int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        std::ofstream outFile(outputPath, std::ios::binary);
        
        outFile.write(reinterpret_cast<char*>(&width), sizeof(width));
        outFile.write(reinterpret_cast<char*>(&height), sizeof(height));
        outFile.write(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));

        while (true) {
            cap >> color_frame;
            if (color_frame.empty()) break;
            
            cv::cvtColor(color_frame, gray_frame, cv::COLOR_BGR2GRAY);
            bool isIFrame = (frameCount % iFrameInterval == 0);
            outFile.write(reinterpret_cast<const char*>(&isIFrame), sizeof(bool));
            
            if (isIFrame) {
                // Handle Intra-frame (I-frame)
                vector<vector<int>> frameVector = matToVector(gray_frame);
                cout << "Encoding I-frame" << endl;
                imageCoder.encodeWithGolomb(frameVector, tempFile);
                
                std::ifstream temp(tempFile, std::ios::binary);
                outFile << temp.rdbuf();
                temp.close();
            } else {
                // Handle Inter-frame (P-frame)
                cv::Mat residuals = cv::Mat::zeros(height, width, CV_8UC1);
                std::vector<cv::Point2i> motionVectors;
                
                // Calculate residuals based on motion vectors and reference frame
                for (int y = 0; y < height; y += blockSize) {
                    for (int x = 0; x < width; x += blockSize) {
                        // Estimate motion vector for current block
                        cv::Point2i mv = estimateMotion(gray_frame, prevFrame, x, y, blockSize, searchRange);
                        motionVectors.push_back(mv);
                        
                        for (int by = 0; by < blockSize && y + by < height; by++) {
                            for (int bx = 0; bx < blockSize && x + bx < width; bx++) {
                                int refX = x + bx + mv.x;
                                int refY = y + by + mv.y;
                                if (refX >= 0 && refY >= 0 && refX < width && refY < height) {
                                    residuals.at<uchar>(y + by, x + bx) = 
                                        gray_frame.at<uchar>(y + by, x + bx) - 
                                        prevFrame.at<uchar>(refY, refX);
                                }
                            }
                        }
                    }
                }

                // Debugging residuals for P-frames
                cout << "P-frame residuals:" << endl;
                for (int y = 0; y < height; y += blockSize) {
                    for (int x = 0; x < width; x += blockSize) {
                        int residual = residuals.at<uchar>(y, x);
                        cout << residual << " ";
                        if (abs(residual) > 255) {
                            cout << "Large residual detected at (" << y << ", " << x << ")" << endl;
                        }
                    }
                }
                cout << endl;

                // Write motion vectors to output file
                for (const auto& mv : motionVectors) {
                    outFile.write(reinterpret_cast<const char*>(&mv.x), sizeof(mv.x));
                    outFile.write(reinterpret_cast<const char*>(&mv.y), sizeof(mv.y));
                }
                
                // Convert residuals to vector and encode using Golomb coding
                vector<vector<int>> residualVector = matToVector(residuals);
                cout << "Encoding P-frame residuals" << endl;
                imageCoder.encodeWithGolomb(residualVector, tempFile);
                
                std::ifstream temp(tempFile, std::ios::binary);
                outFile << temp.rdbuf();
                temp.close();
            }
            
            // Update previous frame for next iteration
            gray_frame.copyTo(prevFrame);
            frameCount++;
        }
        
        std::remove(tempFile.c_str());
        cap.release();
        outFile.close();
    }


    void decodeVideo(const string& inputPath, const string& outputPath) {
        ifstream inFile(inputPath, ios::binary);
        if (!inFile) {
            throw runtime_error("Cannot open input file");
        }
        
        int width, height, blockSize;
        inFile.read(reinterpret_cast<char*>(&width), sizeof(width));
        inFile.read(reinterpret_cast<char*>(&height), sizeof(height));
        inFile.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
        
        VideoWriter writer(outputPath, VideoWriter::fourcc('X', '2', '6', '4'), 
                             30, Size(width, height));
        
        Mat prevFrame;
        string tempFile = "temp_decode.bin";
        
        while (inFile.peek() != EOF) {
            bool isIFrame;
            inFile.read(reinterpret_cast<char*>(&isIFrame), sizeof(bool));
            
            if (isIFrame) {
                // Create temporary file for frame data
                ofstream temp(tempFile, ios::binary);
                temp << inFile.rdbuf();
                temp.close();
                
                vector<vector<int>> frameVector = imageCoder.decodeWithGolomb(tempFile);
                Mat frame = vectorToMat(frameVector);
                writer.write(frame);
                frame.copyTo(prevFrame);
            } else {
                Mat currentFrame = Mat::zeros(height, width, CV_8UC3);
                int numBlocksX = (width + blockSize - 1) / blockSize;
                int numBlocksY = (height + blockSize - 1) / blockSize;
                
                vector<Point2i> motionVectors;
                for (int i = 0; i < numBlocksX * numBlocksY; i++) {
                    Point2i mv;
                    inFile.read(reinterpret_cast<char*>(&mv.x), sizeof(mv.x));
                    inFile.read(reinterpret_cast<char*>(&mv.y), sizeof(mv.y));
                    motionVectors.push_back(mv);
                }
                
                // Create temporary file for residual data
                ofstream temp(tempFile, ios::binary);
                temp << inFile.rdbuf();
                temp.close();
                
                vector<vector<int>> residualVector = imageCoder.decodeWithGolomb(tempFile);
                Mat residuals = vectorToMat(residualVector);
                
                int blockIndex = 0;
                for (int y = 0; y < height; y += blockSize) {
                    for (int x = 0; x < width; x += blockSize) {
                        Point2i mv = motionVectors[blockIndex++];
                        
                        for (int by = 0; by < blockSize && y + by < height; by++) {
                            for (int bx = 0; bx < blockSize && x + bx < width; bx++) {
                                for (int c = 0; c < 3; c++) {
                                    int refX = x + bx + mv.x;
                                    int refY = y + by + mv.y;
                                    
                                    if (refX >= 0 && refY >= 0 && refX < width && refY < height) {
                                        currentFrame.at<Vec3b>(y + by, x + bx)[c] = 
                                            prevFrame.at<Vec3b>(refY, refX)[c] + 
                                            residuals.at<Vec3b>(y + by, x + bx)[c];
                                    }
                                }
                            }
                        }
                    }
                }
                
                writer.write(currentFrame);
                currentFrame.copyTo(prevFrame);
            }
        }
        
        remove(tempFile.c_str());
        writer.release();
        inFile.close();
    }
};

// int main() {
//     string inputVideo;
//     string encodedFile;
//     string outputVideo;
//     int intraFrameInterval;
//     int blockSize;
//     int searchRange;

//     cout << "Enter input video path: ";
//     cin >> inputVideo;

//     cout << "Enter encoded file path: ";
//     cin >> encodedFile;

//     cout << "Enter output video path: ";
//     cin >> outputVideo;

//     cout << "Enter intra-frame interval (e.g., 10): ";
//     cin >> intraFrameInterval;

//     cout << "Enter block size for motion estimation (e.g., 16): ";
//     cin >> blockSize;

//     cout << "Enter search range for motion estimation (e.g., 4): ";
//     cin >> searchRange;

//     VideoCoder videoCoder;

//     cout << "Encoding video..." << endl;
//     videoCoder.encodeVideo(inputVideo, encodedFile, intraFrameInterval, blockSize, searchRange);

//     cout << "Decoding video..." << endl;
//     videoCoder.decodeVideo(encodedFile, outputVideo);

//     cout << "Encoding and decoding complete. Output saved to: " << outputVideo << endl;

//     return 0;
// }
int main() {
    string inputPath = "videos/video.mp4";
    string encodedPath = "encoded.bin";
    string outputPath = "decoded.mp4";
    int iFrameInterval = 10;
    int blockSize = 16;
    int searchRange = 4;

    cout << "Using default values:\n"
         << "I-frame interval: " << iFrameInterval << "\n"
         << "Block size: " << blockSize << "\n"
         << "Search range: " << searchRange << "\n";
    
    cout << "Starting encoding...\n";
    try {
        VideoCoder coder;
        coder.encodeVideo(inputPath, encodedPath, iFrameInterval, blockSize, searchRange);
        coder.decodeVideo(encodedPath, outputPath);
        cout << "Completed. Output saved to: " << outputPath << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
