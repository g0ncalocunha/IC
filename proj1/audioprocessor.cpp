#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <clocale>
#include <cwctype>
#include <codecvt>
#include <locale>
#include <SFML/Audio.hpp>
#include <fftw3.h>
#include <SFML/Graphics.hpp>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

class AudioProcessor
{
private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    const sf::Int16 *samples;
    vector<sf::Int16> leftChannel;
    vector<sf::Int16> rightChannel;
    vector<sf::Int16> midChannel;
    vector<sf::Int16> sideChannel;
    vector<sf::Int16> targetChannel;
    vector<sf::Int16> quantizedSamples;
    unsigned int channelCount;
    const unsigned int WINDOW_WIDTH = 1920;
    const unsigned int WINDOW_HEIGHT = 800;
    const unsigned int LEFT_MARGIN = 100;
    const unsigned int RIGHT_MARGIN = 50;
    const unsigned int TOP_MARGIN = 50;
    const unsigned int BOTTOM_MARGIN = 100;

public:
    bool readFile(const string &file)
    {
        // open autf file
        ifstream infile(file);
        if (!infile.is_open())
        {
            cerr << "Error opening file" << endl;
            return false;
        }
        if (!buffer.loadFromFile(file))
        {
            cerr << "Error loading file" << endl;
            return false;
        }
        sound.setBuffer(buffer);
        return true;
    }

    void playAudio()
    {
        sound.play();
        while (sound.getStatus() == sf::Sound::Playing)
        {
            // wait for sound to finish
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // get information about the audio file
    void getAudioInfo()
    {
        cout << "File info: " << endl;
        cout << "Duration: " << buffer.getDuration().asSeconds() << " seconds" << endl;
        cout << "Sample rate: " << buffer.getSampleRate() << endl;
        cout << "Channel count: " << buffer.getChannelCount() << endl;
    }

    void plotAudioWaveform(const string &file, bool quantized = false)
    {
        const unsigned int GRAPH_HEIGHT = (WINDOW_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN) / 2;

        sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Audio Waveform", sf::Style::None);
        window.setVisible(false); // Hide the window

        sf::Font font;
        if (!font.loadFromFile("arial.ttf")) {
            cerr << "Error loading font!" << endl;
            return;
        }

        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;
        unsigned int sampleRate = buffer.getSampleRate();

        const sf::Int16* samplesToPlot = quantized ? quantizedSamples.data() : buffer.getSamples();

        float duration = static_cast<float>(samplesPerChannel) / sampleRate;
        float scaleX = (WINDOW_WIDTH - LEFT_MARGIN - RIGHT_MARGIN) / duration;
        float scaleY = GRAPH_HEIGHT / 2.0f;  

        vector<sf::VertexArray> waveforms(channelCount, sf::VertexArray(sf::LinesStrip, samplesPerChannel));
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            float channelOffset = TOP_MARGIN + GRAPH_HEIGHT * (channel + 0.5f);
            for (sf::Uint64 i = 0; i < samplesPerChannel; ++i) {
                float timeInSeconds = static_cast<float>(i) / sampleRate;
                float x = LEFT_MARGIN + timeInSeconds * scaleX;

                float sampleValue = static_cast<float>(samplesToPlot[i * channelCount + channel]) / 32768.0f;
                float y = channelOffset - sampleValue * scaleY;
                
                waveforms[channel][i].position = sf::Vector2f(x, y);

                waveforms[channel][i].color = quantized ? sf::Color::Red :sf::Color(50 * (channel + 1), 50 * (channel + 2), 250); 
            }
        }

        vector<sf::VertexArray> axes;
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            float yPosition = TOP_MARGIN + GRAPH_HEIGHT * (channel + 1);

            sf::VertexArray xAxis(sf::Lines, 2);
            xAxis[0].position = sf::Vector2f(LEFT_MARGIN, yPosition);
            xAxis[1].position = sf::Vector2f(WINDOW_WIDTH - RIGHT_MARGIN, yPosition);
            xAxis[0].color = xAxis[1].color = sf::Color::Black;
            axes.push_back(xAxis);

            sf::VertexArray yAxis(sf::Lines, 2);
            yAxis[0].position = sf::Vector2f(LEFT_MARGIN, yPosition - GRAPH_HEIGHT);
            yAxis[1].position = sf::Vector2f(LEFT_MARGIN, yPosition);
            yAxis[0].color = yAxis[1].color = sf::Color::Black;
            axes.push_back(yAxis);
        }

        sf::Text xAxisLabel("Time (s)", font, 20);
        xAxisLabel.setFillColor(sf::Color::Black);
        xAxisLabel.setPosition(WINDOW_WIDTH / 2.0f - 40, WINDOW_HEIGHT - (BOTTOM_MARGIN / 2.0f) + 10);

        sf::Text yAxisLabel("Amplitude", font, 20);
        yAxisLabel.setFillColor(sf::Color::Black);
        yAxisLabel.setPosition(LEFT_MARGIN - 80, WINDOW_HEIGHT / 2.0f);
        yAxisLabel.setRotation(-90);

        vector<sf::Text> timeTicks;
        vector<sf::VertexArray> timeLines;
        int majorTickInterval = 1;
        int minorTicksPerMajor = 4;

        for (int i = 0; i <= static_cast<int>(duration); ++i) {
            sf::Text tickLabel(to_string(i), font, 18);
            tickLabel.setFillColor(sf::Color::Black);
            tickLabel.setPosition(LEFT_MARGIN + i * scaleX, (WINDOW_HEIGHT - BOTTOM_MARGIN / 2.0f) - 20);
            timeTicks.push_back(tickLabel);

            sf::VertexArray majorLine(sf::Lines, 2);
            majorLine[0].position = sf::Vector2f(LEFT_MARGIN + i * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN);
            majorLine[1].position = sf::Vector2f(LEFT_MARGIN + i * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN + 10);
            majorLine[0].color = majorLine[1].color = sf::Color::Black;
            timeLines.push_back(majorLine);

            if (i < static_cast<int>(duration)) {
                for (int j = 1; j < minorTicksPerMajor; ++j) {
                    float minorX = LEFT_MARGIN + (i + static_cast<float>(j) / minorTicksPerMajor) * scaleX;
                    sf::VertexArray minorLine(sf::Lines, 2);
                    minorLine[0].position = sf::Vector2f(minorX, WINDOW_HEIGHT - BOTTOM_MARGIN);
                    minorLine[1].position = sf::Vector2f(minorX, WINDOW_HEIGHT - BOTTOM_MARGIN + 5);
                    minorLine[0].color = minorLine[1].color = sf::Color(200, 200, 200);
                    timeLines.push_back(minorLine);
                }
            }
        }

        vector<vector<sf::Text>> amplitudeTicks(channelCount);
        int tickCountY = 5;
        float tickPadding = 15.0f;
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            for (int i = 0; i <= tickCountY; ++i) {
                float amplitude = 1.0f - (2.0f * i / tickCountY);
                ostringstream amplitudeStream;
                amplitudeStream << fixed << setprecision(2) << amplitude;
                sf::Text tick(amplitudeStream.str(), font, 15);
                tick.setFillColor(sf::Color::Black);

                float yPos = TOP_MARGIN + GRAPH_HEIGHT * channel + i * (GRAPH_HEIGHT / tickCountY);
                if (i == 0) {
                    yPos += tickPadding;
                } else if (i == tickCountY) {
                    yPos -= tickPadding;
                }
                tick.setPosition(LEFT_MARGIN - 40, yPos);
                amplitudeTicks[channel].push_back(tick);
            }
        }

        vector<sf::Text> amplitudeLabels(channelCount);
        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            sf::Text label("Amplitude", font, 18);
            label.setFillColor(sf::Color::Black);
            float yPos = TOP_MARGIN + GRAPH_HEIGHT * (channel + 0.5f);
            label.setPosition(LEFT_MARGIN - 80, yPos);
            label.setRotation(-90);
            amplitudeLabels[channel] = label;
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) window.close();
            }

            window.clear(sf::Color::White);
            for (const auto& axis : axes) window.draw(axis);
            for (const auto& waveform : waveforms) window.draw(waveform);
            for (const auto& line : timeLines) window.draw(line);
            for (const auto& tick : timeTicks) window.draw(tick);
            for (const auto& channelTicks : amplitudeTicks) {
                for (const auto& tick : channelTicks) window.draw(tick);
            }
            window.draw(xAxisLabel);
            window.draw(yAxisLabel);
            window.display();

            // Capture the window content and save it as an image
            sf::Texture texture;
            texture.create(WINDOW_WIDTH, WINDOW_HEIGHT);
            texture.update(window);
            sf::Image screenshot = texture.copyToImage();
            string outputFolder = "audioprocessor_files/plots/";
            string outputFilename = quantized ? "quantized_waveform.png" : "original_waveform.png";
            string fullPath = outputFolder + file.substr(file.find_last_of("/") + 1, file.find_last_of(".") - file.find_last_of("/") - 1) + "/" + outputFilename;

            // Create the directory if it doesn't exist
            string command = "mkdir -p " + fullPath.substr(0, fullPath.find_last_of("/"));
            system(command.c_str());

            screenshot.saveToFile(fullPath);

            window.close(); // Close the window after saving the image
        }
    }


    void getLeftRightMidSideChannels(const string& title)
    {
        samples = buffer.getSamples();
        sf::Uint64 sampleCount = buffer.getSampleCount();
        
        if (channelCount == 2)
        {
            leftChannel.reserve(sampleCount / 2);
            rightChannel.reserve(sampleCount / 2);
            
            for (sf::Uint64 i = 0; i < sampleCount; i += 2)
            {
                leftChannel.push_back(samples[i]);
                if (i + 1 < sampleCount) 
                {
                    rightChannel.push_back(samples[i + 1]);
                }
            }
            if (title.find("Mid") != string::npos || title.find("Side") != string::npos)
            {
                for (size_t i = 0; i < leftChannel.size(); ++i)
                {
                    int mid = (leftChannel[i] + rightChannel[i]) / 2;
                    int side = (leftChannel[i] - rightChannel[i]) / 2;
                    midChannel.push_back(mid);
                    sideChannel.push_back(side);
                }
            }
        }
    }


    void plotHistogram(const string &file, string title)
    {
        channelCount = buffer.getChannelCount();

        if (channelCount < 2)
        {
            cerr << "Error: Channel count is not stereo." << endl;
            return;
        }

        if (title == "Right Channel")
        {
            getLeftRightMidSideChannels(title);
            targetChannel = rightChannel;
        }
        else if (title == "Left Channel")
        {
            getLeftRightMidSideChannels(title);
            targetChannel = leftChannel;
        }
        else if (title == "Mid Channel")
        {
            getLeftRightMidSideChannels(title);
            targetChannel = midChannel;
        }
        else
        {
            getLeftRightMidSideChannels(title);
            targetChannel = sideChannel; 
        }

        if (targetChannel.empty())
        {
            cerr << "Error: targetChannel is empty." << endl;
            return;
        }

        vector<int> histogram(256, 0);  // Ajusta o histograma para 256 bins
        int binSize = 256;  // 256 bins para lidar com valores de 16 bits

        sf::Color color = sf::Color::Magenta;

        int windowWidth = 950;
        int windowHeight = 650;
        int binWidth = windowWidth / histogram.size();
        int maxHeight = 400;

        sf::Font font;
        if (!font.loadFromFile("arial.ttf"))
        {
            cerr << "Error loading font." << endl;
        }

        sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), title + " Histogram");
        window.setVisible(false); // Hide the window

        for (auto sample : targetChannel)
        {
            int binIndex = (sample + 32768) / binSize;
            if (binIndex >= 0 && binIndex < static_cast<int>(histogram.size()))
            {
                histogram[binIndex]++;
            }
            else
            {
                cerr << "ERROR: Out of bounds access in histogram" << endl;
            }
        }

        int maxBinValue = *max_element(histogram.begin(), histogram.end());

        int totalBinsWidth = histogram.size() * binWidth;
        int startX = (windowWidth - totalBinsWidth) / 2;  // Centrando histograma
        int startY = windowHeight - 50; 

        sf::Text xLabel("Amplitude", font, 14);
        xLabel.setFillColor(sf::Color::Black);
        xLabel.setPosition(windowWidth / 2, windowHeight - 35);  

        sf::Text yLabel("Frequency", font, 14);
        yLabel.setFillColor(sf::Color::Black);
        yLabel.setPosition(20, windowHeight / 2 - 20);  
        yLabel.setRotation(-90);

        sf::Vertex xAxis[] =
        {
            sf::Vertex(sf::Vector2f(startX, startY - 20)),
            sf::Vertex(sf::Vector2f(startX + totalBinsWidth, startY - 20))  
        };

        sf::Vertex yAxis[] =
        {
            sf::Vertex(sf::Vector2f(startX, startY)),
            sf::Vertex(sf::Vector2f(startX, startY - windowHeight))  
        };

        xAxis[0].color = xAxis[1].color = sf::Color::Black;
        yAxis[0].color = yAxis[1].color = sf::Color::Black;

        vector<sf::Text> yTickLabels;
        vector<sf::Vertex> yTicks;
        int numYTicks = 5;
        for (int i = 0; i <= numYTicks; ++i)
        {
            float yPos = startY - (i * maxHeight / numYTicks) - 20;  

            yTicks.push_back(sf::Vertex(sf::Vector2f(startX - 5, yPos), sf::Color::Black));
            yTicks.push_back(sf::Vertex(sf::Vector2f(startX + 5, yPos), sf::Color::Black));

            sf::Text label(to_string(i * maxBinValue / numYTicks), font, 12);
            label.setFillColor(sf::Color::Black);
            label.setPosition(startX - 40, yPos - 10);
            yTickLabels.push_back(label);
        }

        vector<sf::Text> xTickLabels;
        vector<sf::Vertex> xTicks;
        int numXTicks = 10;
        float xTickInterval = static_cast<float>(histogram.size()) / numXTicks;

        for (int i = 0; i <= numXTicks; ++i)
        {
            float xPos = startX + i * (totalBinsWidth / numXTicks);
            
            xTicks.push_back(sf::Vertex(sf::Vector2f(xPos, startY - 25), sf::Color::Black));  
            xTicks.push_back(sf::Vertex(sf::Vector2f(xPos, startY - 15), sf::Color::Black));  

            int labelValue = static_cast<int>(i * (256 / numXTicks));
            sf::Text label(to_string(labelValue), font, 12);
            label.setFillColor(sf::Color::Black);
            label.setPosition(xPos - 10, startY + 10 - 20); 
            xTickLabels.push_back(label);
        }

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                }
            }

            window.clear(sf::Color::White);

            // Desenhar eixos
            window.draw(xAxis, 2, sf::Lines);
            window.draw(yAxis, 2, sf::Lines);

            // Desenhar ticks do eixo Y e seus rótulos
            for (size_t i = 0; i < yTicks.size(); i += 2)
            {
                window.draw(&yTicks[i], 2, sf::Lines);
            }
            for (const auto& label : yTickLabels)
            {
                window.draw(label);
            }

            // Desenhar ticks do eixo X e seus rótulos
            for (size_t i = 0; i < xTicks.size(); i += 2)
            {
                window.draw(&xTicks[i], 2, sf::Lines);
            }
            for (const auto& label : xTickLabels)
            {
                window.draw(label);
            }

            // Desenhar as barras 
            for (size_t i = 0; i < histogram.size(); ++i)
            {
                float binHeight = static_cast<float>(histogram[i]) / maxBinValue * maxHeight;
                sf::RectangleShape bar(sf::Vector2f(binWidth - 1, -binHeight));
                bar.setPosition(startX + i * binWidth, startY -20);
                bar.setFillColor(color);
                window.draw(bar);
            }

            // Desenhar rótulos dos eixos
            window.draw(xLabel);
            window.draw(yLabel);

            window.display();

            // Capture the window content and save it as an image
            sf::Texture texture;
            texture.create(windowWidth, windowHeight);
            texture.update(window);
            sf::Image screenshot = texture.copyToImage();
            string outputFolder = "audioprocessor_files/plots/";
            string outputFilename = title + "_histogram.png";
            string fullPath = outputFolder + file.substr(file.find_last_of("/") + 1, file.find_last_of(".") - file.find_last_of("/") - 1) + "/" + outputFilename;

            // Create the directory if it doesn't exist
            string command = "mkdir -p " + fullPath.substr(0, fullPath.find_last_of("/"));
            system(command.c_str());

            screenshot.saveToFile(fullPath);

            window.close(); // Close the window after saving the image
        }
    }

    void quantization(int n) 
    {
        const sf::Int16* samples = buffer.getSamples();
        int sampleCount = buffer.getSampleCount();
        if (sampleCount <= 0) {
            cerr << "Error: No samples to quantize!" << endl;
            return;
        }
        
        int max_value = *max_element(samples, samples + sampleCount);
        int min_value = *min_element(samples, samples + sampleCount);
        
        if (max_value == min_value) {
            cerr << "Error: All samples have the same value!" << endl;
            quantizedSamples.resize(sampleCount, static_cast<sf::Int16>(min_value)); 
            return;
        }

        int numLevels = 1 << n;
        float step_size = static_cast<float>(max_value - min_value) / (numLevels - 1);
        
        quantizedSamples.resize(sampleCount);

        // Quantize the samples
        for (size_t i = 0; i < sampleCount; ++i) {
            // Normalize the sample to [0, numLevels-1], then round and map back to the original range
            float normalizedSample = static_cast<float>(samples[i] - min_value) / (max_value - min_value) * (numLevels - 1);
            quantizedSamples[i] = static_cast<sf::Int16>(round(normalizedSample) * step_size + min_value);
        }
    }

    void plotBothWaveforms(const string &file) 
    {
        const unsigned int WINDOW_WIDTH = 1850;
        const unsigned int WINDOW_HEIGHT = 800;
        const unsigned int LEFT_MARGIN = 100;
        const unsigned int RIGHT_MARGIN = 50;
        const unsigned int TOP_MARGIN = 50;
        const unsigned int BOTTOM_MARGIN = 80;
        const unsigned int CENTER_MARGIN = 100;
        const unsigned int GRAPH_WIDTH = (WINDOW_WIDTH - LEFT_MARGIN - RIGHT_MARGIN - CENTER_MARGIN) / 2;
        const unsigned int GRAPH_HEIGHT = WINDOW_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN;
        const float TICK_LENGTH = 5.0f;

        // Constants for 16-bit audio
        const sf::Int16 MAX_AMPLITUDE = 32767;
        const sf::Int16 MIN_AMPLITUDE = -32768;

        sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Original vs Quantized Audio Waveforms");
        window.setVisible(false); // Hide the window

        sf::Font font;
        if (!font.loadFromFile("arial.ttf")) {
            cerr << "Error loading font!" << endl;
            return;
        }

        // Get audio buffer properties
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;
        const sf::Int16* samples = buffer.getSamples();

        // Scale factors
        float scaleX = static_cast<float>(GRAPH_WIDTH) / samplesPerChannel;
        float scaleY = (GRAPH_HEIGHT / 2.0f) / MAX_AMPLITUDE;

        // Lambda function to draw the waveform
        auto drawWaveform = [&](const sf::Int16* samples, sf::Color color, float xOffset, bool quantized, int bitDepth = 16) {
            sf::VertexArray waveform(sf::LineStrip, samplesPerChannel);
            
            int levels = 1 << bitDepth;  // Number of quantization levels
            float stepSize = static_cast<float>(MAX_AMPLITUDE) / (levels / 2);  // Step size for uniform quantization

            for (sf::Uint64 i = 0; i < samplesPerChannel; ++i) {
                float x = xOffset + i * scaleX;
                float amplitude = samples[i * channelCount];

                if (quantized) {
                    // Quantize the sample to the desired bit depth
                    amplitude = round(amplitude / stepSize) * stepSize;
                }

                // Scale and position the sample in the graph
                float y = TOP_MARGIN + GRAPH_HEIGHT / 2 - amplitude * scaleY;
                waveform[i] = sf::Vertex(sf::Vector2f(x, y), color);
            }
            return waveform;
        };

        auto drawAxis = [&](float xOffset) {
            sf::VertexArray axis(sf::Lines, 4);
            axis[0] = sf::Vertex(sf::Vector2f(xOffset, TOP_MARGIN), sf::Color::Black);
            axis[1] = sf::Vertex(sf::Vector2f(xOffset, WINDOW_HEIGHT - BOTTOM_MARGIN), sf::Color::Black);
            axis[2] = sf::Vertex(sf::Vector2f(xOffset, WINDOW_HEIGHT - BOTTOM_MARGIN), sf::Color::Black);
            axis[3] = sf::Vertex(sf::Vector2f(xOffset + GRAPH_WIDTH, WINDOW_HEIGHT - BOTTOM_MARGIN), sf::Color::Black);
            return axis;
        };

        auto drawTicks = [&](float xOffset) -> sf::VertexArray {
            const int NUM_Y_TICKS = 7;
            const int NUM_X_TICKS = 11;
            int totalVertices = (NUM_Y_TICKS + NUM_X_TICKS) * 2;
            
            sf::VertexArray ticks(sf::Lines, totalVertices);
            int currentVertex = 0;

            for (int i = -3; i <= 3; ++i) {
                float amplitude = i * (MAX_AMPLITUDE / 4);
                float y = WINDOW_HEIGHT / 2 + TOP_MARGIN - amplitude * scaleY;
                ticks[currentVertex] = sf::Vertex(sf::Vector2f(xOffset, y), sf::Color::Black);
                ticks[currentVertex + 1] = sf::Vertex(sf::Vector2f(xOffset - TICK_LENGTH, y), sf::Color::Black);
                currentVertex += 2;
            }

            int tickInterval = samplesPerChannel / 10;
            for (int i = 0; i <= 10; ++i) {
                float x = xOffset + (i * tickInterval) * scaleX;
                ticks[currentVertex] = sf::Vertex(sf::Vector2f(x, WINDOW_HEIGHT - BOTTOM_MARGIN), sf::Color::Black);
                ticks[currentVertex + 1] = sf::Vertex(sf::Vector2f(x, WINDOW_HEIGHT - BOTTOM_MARGIN + TICK_LENGTH), sf::Color::Black);
                currentVertex += 2;
            }

            return ticks;
        };

        sf::VertexArray originalWaveform = drawWaveform(samples, sf::Color::Blue, LEFT_MARGIN, false);
        sf::VertexArray quantizedWaveform = drawWaveform(samples, sf::Color::Red, LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN, true, 4);  // Quantized to 4 bits

        sf::VertexArray originalAxis = drawAxis(LEFT_MARGIN);
        sf::VertexArray quantizedAxis = drawAxis(LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN);
        sf::VertexArray originalTicks = drawTicks(LEFT_MARGIN);
        sf::VertexArray quantizedTicks = drawTicks(LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN);

        auto createText = [&](const string& content, unsigned int size, sf::Color color, float x, float y, float rotation = 0) {
            sf::Text text(content, font, size);
            text.setFillColor(color);
            text.setPosition(x, y);
            text.setRotation(rotation);
            return text;
        };

        sf::Text originalLabel = createText("Original", 24, sf::Color::Blue, LEFT_MARGIN, TOP_MARGIN / 2);
        sf::Text quantizedLabel = createText("Quantized", 24, sf::Color::Red, LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN, TOP_MARGIN / 2);
        sf::Text xAxisLabel = createText("Sample", 20, sf::Color::Black, WINDOW_WIDTH / 2 - 40, WINDOW_HEIGHT - BOTTOM_MARGIN / 2);
        sf::Text yAxisLabel = createText("Amplitude", 20, sf::Color::Black, LEFT_MARGIN / 4 - 20, WINDOW_HEIGHT / 2 + 20, -90);

        vector<sf::Text> sampleTicks;
        int tickInterval = samplesPerChannel / 10;
        for (int i = 0; i <= 10; ++i) {
            int sampleNum = i * tickInterval;
            sampleTicks.push_back(createText(to_string(sampleNum), 16, sf::Color::Black, LEFT_MARGIN + sampleNum * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN + 10));
            sampleTicks.push_back(createText(to_string(sampleNum), 16, sf::Color::Black, LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN + sampleNum * scaleX, WINDOW_HEIGHT - BOTTOM_MARGIN + 10));
        }

        vector<sf::Text> amplitudeTicks;
        for (int i = -4; i <= 4; ++i) {
            int amplitude = i * (MAX_AMPLITUDE / 4);
            amplitudeTicks.push_back(createText(to_string(amplitude), 16, sf::Color::Black, LEFT_MARGIN - 60, WINDOW_HEIGHT / 2 - amplitude * scaleY - 45));
            amplitudeTicks.push_back(createText(to_string(amplitude), 16, sf::Color::Black, LEFT_MARGIN + GRAPH_WIDTH + CENTER_MARGIN - 60, WINDOW_HEIGHT / 2 - amplitude * scaleY - 45));
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            window.clear(sf::Color::White);

            window.draw(originalWaveform);
            window.draw(quantizedWaveform);
            window.draw(originalAxis);
            window.draw(quantizedAxis);
            window.draw(originalTicks);
            window.draw(quantizedTicks);
            window.draw(originalLabel);
            window.draw(quantizedLabel);
            window.draw(xAxisLabel);
            window.draw(yAxisLabel);

            for (const auto& tick : sampleTicks)
                window.draw(tick);
            for (const auto& tick : amplitudeTicks)
                window.draw(tick);

            window.display();

            // Capture the window content and save it as an image
            sf::Texture texture;
            texture.create(WINDOW_WIDTH, WINDOW_HEIGHT);
            texture.update(window);
            sf::Image screenshot = texture.copyToImage();
            string outputFolder = "audioprocessor_files/plots/";
            string outputFilename = "original_vs_quantized_waveforms.png";
            string fullPath = outputFolder + file.substr(file.find_last_of("/") + 1, file.find_last_of(".") - file.find_last_of("/") - 1) + "/" + outputFilename;

            // Create the directory if it doesn't exist
            string command = "mkdir -p " + fullPath.substr(0, fullPath.find_last_of("/"));
            system(command.c_str());

            screenshot.saveToFile(fullPath);

            window.close(); // Close the window after saving the image
        }
    }


    void compareAudios()
    {
        //compare quantized audio with original audio
        sf::Uint64 sampleCount = buffer.getSampleCount();
        
        //Mean Squared Error
        double mse = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            mse += pow(samples[i] - quantizedSamples[i], 2);
        }
        mse /= sampleCount;
        cout << "Mean Squared Error: " << mse << endl;
        //Signal to Noise Ratio
        double snr = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            snr += pow(samples[i], 2);
        }
        snr /= mse;
        cout << "Signal to Noise Ratio: " << snr << endl;

    }

    void frequencyAnalyser(const string &file)
    {
        // Fourier Transform
        samples = buffer.getSamples();
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;

        // Use FFTW library for Fourier Transform
        vector<double> real(samplesPerChannel);
        vector<double> imag(samplesPerChannel, 0.0);

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            real[i] = samples[i * channelCount] / 32768.0; // Normalize sample
        }

        // Perform FFT
        fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * samplesPerChannel);
        fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * samplesPerChannel);
        fftw_plan p = fftw_plan_dft_1d(samplesPerChannel, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            in[i][0] = real[i];
            in[i][1] = imag[i];
        }

        fftw_execute(p);

        // Calculate magnitude
        vector<double> magnitude(samplesPerChannel);
        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        }

        fftw_destroy_plan(p);
        fftw_free(in);
        fftw_free(out);

        sf::RenderWindow window(sf::VideoMode(1200, 600), "Frequency Spectrum");
        window.setVisible(false); // Hide the window

        float leftPadding = 100.0f;  
        float rightPadding = 50.0f;
        float topPadding = 50.0f;
        float bottomPadding = 50.0f;

        sf::VertexArray spectrum(sf::LinesStrip, samplesPerChannel);
        float scaleX = (window.getSize().x - (leftPadding + rightPadding)) / samplesPerChannel;
        float scaleY = (window.getSize().y - (topPadding + bottomPadding)) / *max_element(magnitude.begin(), magnitude.end()) * 0.8;

        for (sf::Uint64 i = 0; i < samplesPerChannel; ++i)
        {
            float x = i * scaleX;
            float y = magnitude[i] * scaleY;
            spectrum[i].position = sf::Vector2f(x + leftPadding + 2, window.getSize().y - bottomPadding - y);
            spectrum[i].color = sf::Color::Green;
        }

        sf::Font font;
        if (!font.loadFromFile("arial.ttf"))
        {
            cerr << "Error loading font!" << endl;
            return;
        }

        int labelSize = 14;
        int tickLabelSize = 12;

        sf::Text xAxisLabel("Frequency (Hz)", font, labelSize);
        xAxisLabel.setFillColor(sf::Color::Black);
        xAxisLabel.setPosition(window.getSize().x / 2 - 50, window.getSize().y - bottomPadding + 25);

        sf::Text yAxisLabel("Magnitude", font, labelSize);
        yAxisLabel.setFillColor(sf::Color::Black);
        yAxisLabel.setPosition(15, window.getSize().y / 2);
        yAxisLabel.setRotation(-90);

        sf::RectangleShape xAxis(sf::Vector2f(window.getSize().x - (leftPadding + rightPadding), 2));
        xAxis.setFillColor(sf::Color::Black);
        xAxis.setPosition(leftPadding, window.getSize().y - bottomPadding);

        sf::RectangleShape yAxis(sf::Vector2f(2, window.getSize().y - (topPadding + bottomPadding)));
        yAxis.setFillColor(sf::Color::Black);
        yAxis.setPosition(leftPadding, topPadding);

        int xTicks = 10;
        int yTicks = 8;

        vector<sf::Text> xTickLabels;
        vector<sf::RectangleShape> xTickMarks;
        vector<sf::Text> yTickLabels;
        vector<sf::RectangleShape> yTickMarks;

        for (int i = 0; i <= xTicks; ++i)
        {
            float x = leftPadding + i * (window.getSize().x - (leftPadding + rightPadding)) / xTicks;
            
            sf::RectangleShape tick(sf::Vector2f(2, 6));
            tick.setFillColor(sf::Color::Black);
            tick.setPosition(x, window.getSize().y - bottomPadding);
            xTickMarks.push_back(tick);

            sf::Text tickLabel;
            tickLabel.setFont(font);
            tickLabel.setCharacterSize(tickLabelSize);
            tickLabel.setFillColor(sf::Color::Black);

            int frequencyValue = static_cast<int>((i * (44100 / 2)) / xTicks);
            string labelText = to_string(frequencyValue);
            tickLabel.setString(labelText);
            
            float textWidth = labelText.length() * (tickLabelSize * 0.6f);
            tickLabel.setPosition(x - textWidth / 2, window.getSize().y - bottomPadding + 8);
            xTickLabels.push_back(tickLabel);
        }

        float maxMagnitude = *max_element(magnitude.begin(), magnitude.end());
        for (int i = 0; i <= yTicks; ++i)
        {
            float y = window.getSize().y - bottomPadding - i * (window.getSize().y - (topPadding + bottomPadding)) / yTicks;
            
            sf::RectangleShape tick(sf::Vector2f(6, 2));
            tick.setFillColor(sf::Color::Black);
            tick.setPosition(leftPadding - 6, y);
            yTickMarks.push_back(tick);

            sf::Text tickLabel;
            tickLabel.setFont(font);
            tickLabel.setCharacterSize(tickLabelSize);
            tickLabel.setFillColor(sf::Color::Black);

            float magnitudeValue = (maxMagnitude * i) / yTicks;
            
            // Format number with 2 decimal places
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << magnitudeValue;
            string labelText = stream.str();
            
            tickLabel.setString(labelText);
            
            float textWidth = labelText.length() * (tickLabelSize * 0.6f);
            tickLabel.setPosition(leftPadding - textWidth - 10, y - tickLabelSize / 2);
            yTickLabels.push_back(tickLabel);
        }

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            window.clear(sf::Color::White);
            
            window.draw(xAxis);
            window.draw(yAxis);
            
            window.draw(spectrum);
            
            for (const auto& tick : xTickMarks)
                window.draw(tick);
            for (const auto& label : xTickLabels)
                window.draw(label);
            for (const auto& tick : yTickMarks)
                window.draw(tick);
            for (const auto& label : yTickLabels)
                window.draw(label);
                
            window.draw(xAxisLabel);
            window.draw(yAxisLabel);

            window.display();

            // Capture the window content and save it as an image
            sf::Texture texture;
            texture.create(window.getSize().x, window.getSize().y);
            texture.update(window);
            sf::Image screenshot = texture.copyToImage();
            string outputFolder = "audioprocessor_files/plots/";
            string outputFilename = "frequency_spectrum.png";
            string fullPath = outputFolder + file.substr(file.find_last_of("/") + 1, file.find_last_of(".") - file.find_last_of("/") - 1) + "/" + outputFilename;

            // Create the directory if it doesn't exist
            string command = "mkdir -p " + fullPath.substr(0, fullPath.find_last_of("/"));
            system(command.c_str());

            screenshot.saveToFile(fullPath);

            window.close(); // Close the window after saving the image
        }
    }


    void noiseAdder(const string &filename){
        //add noise to audio
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;

        // Add white noise
        vector<sf::Int16> noisySamples(sampleCount);
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            noisySamples[i] = samples[i] + (rand() % 32768 - 16384); // Add random noise
        }

        // Save noisy audio to a new file
        sf::SoundBuffer noisyBuffer;
        noisyBuffer.loadFromSamples(&noisySamples[0], sampleCount, channelCount, buffer.getSampleRate());
        //save to directory audioprocessor_files
        noisyBuffer.saveToFile(filename.substr(0, filename.find_last_of('.')) + "Noisy.wav");
        cout << "Noisy audio saved to " << filename.substr(0, filename.find_last_of('.')) + "Noisy.wav" << endl;
        // Load noisy audio from file
        sf::SoundBuffer noisyBuffer2;

        //MSE and SNR
        double mse = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            mse += pow(samples[i] - noisySamples[i], 2);
        }
        mse /= sampleCount;
        cout << "Mean Squared Error: " << mse << endl;
        //Signal to Noise Ratio
        double snr = 0;
        for (sf::Uint64 i = 0; i < sampleCount; ++i)
        {
            snr += pow(samples[i], 2);
        }
        snr /= mse;
        cout << "Signal to Noise Ratio: " << snr << endl;
        
    }
};

int main(int argc, char *argv[])
{
    AudioProcessor p;
    string filename;
    string filename2;

    if (argc != 2)
    {
        filename = "audioprocessor_files/sample04.wav";
    }
    else
    {
        filename = argv[1];
    }

    if (!p.readFile(filename))
    {
        cerr << "Failed to load audio file." << endl;
        return -1;
    }

    // p.playAudio();
    p.getAudioInfo();
    p.plotAudioWaveform(filename);
    p.plotHistogram(filename, "Right Channel");
    p.plotHistogram(filename, "Left Channel");
    p.plotHistogram(filename, "Mid Channel");
    p.plotHistogram(filename, "Side Channel");
    p.quantization(16);
    p.plotBothWaveforms(filename);
    p.compareAudios();
    p.frequencyAnalyser(filename);
    p.noiseAdder(filename);

    return 0;
}