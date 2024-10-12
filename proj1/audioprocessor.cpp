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
#include <SFML/Graphics.hpp>
#include <thread>
#include <chrono>
#include <cmath>

using namespace std;

class AudioProcessor
{
private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    const sf::Int16* samples;
    vector<sf::Int16> leftChannel;
    vector<sf::Int16> rightChannel;
    vector<sf::Int16> midChannel;
    vector<sf::Int16> sideChannel;
    vector<sf::Int16> targetChannel;
    vector<sf::Int16> quantizedSamples;
    unsigned int channelCount;

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

    //get information about the audio file
    void getAudioInfo()
    {
        cout << "File info: " << endl;
        cout << "Duration: " << buffer.getDuration().asSeconds() << " seconds" << endl;
        cout << "Sample rate: " << buffer.getSampleRate() << endl;
        cout << "Channel count: " << buffer.getChannelCount() << endl;
    }

    void plotAudioWaveform()
    {
        samples = buffer.getSamples();
        sf::Uint64 sampleCount = buffer.getSampleCount();
        unsigned int channelCount = buffer.getChannelCount();
        sf::Uint64 samplesPerChannel = sampleCount / channelCount;
        
        sf::RenderWindow window(sf::VideoMode(1920, 600), "Audio's Waveform");

        vector<sf::VertexArray> waveforms(channelCount, sf::VertexArray(sf::LinesStrip, samplesPerChannel));
        float scaleX = static_cast<float>(window.getSize().x) / samplesPerChannel;
        float scaleY = window.getSize().y / (2.0f * channelCount);

        for (unsigned int channel = 0; channel < channelCount; ++channel) {
            for (sf::Uint64 i = 0; i < samplesPerChannel; ++i) {
                float x = i * scaleX;
                float y = (samples[i * channelCount + channel] / 32768.0f) * scaleY;
                waveforms[channel][i].position = sf::Vector2f(x, (channel + 0.5f) * (window.getSize().y / channelCount) + y);
                waveforms[channel][i].color = sf::Color(50 * (channel + 1), 50 * (channel + 2), 250); // Different color for each channel
            }
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            window.clear(sf::Color::Transparent);
            for (const auto& waveform : waveforms) {
                window.draw(waveform);
            }
            window.display();
        }
    }

    void getLeftRightChannels() 
    {
        channelCount = buffer.getChannelCount();
        sf::Uint64 sampleCount = buffer.getSampleCount();

        if (channelCount == 2) {
            for (sf::Uint64 i = 0; i < sampleCount; i += 2) {
                leftChannel.push_back(samples[i]);     
                rightChannel.push_back(samples[i + 1]);
            }
        } else {
            cerr << "Audio is not stereo." << endl;
        }
    }

    void getMIDchannel()
    {
        for (size_t i = 0; i < leftChannel.size(); ++i) {
            int mid = (leftChannel[i] + rightChannel[i]) / 2;
            int side = (leftChannel[i] - rightChannel[i]) / 2;
            midChannel.push_back(mid);
            sideChannel.push_back(side);
        }
    }

    void plotHistogram(bool quantized = false)
    {
        if(quantized){
            targetChannel=quantizedSamples;
        }
        else{
            if (channelCount > 1) {
                getLeftRightChannels();
                getMIDchannel();
                targetChannel = midChannel; 
            } else {
                cerr << "Error: Channel count is not stereo." << endl;
                return;
            }
        }
        vector<int> histogram(256, 0);
        int binSize = 256;  // Handle 16-bit sample values

        sf::Color color = sf::Color::Red;
        int windowWidth = 600;
        int windowHeight = 600;
        int binWidth = 3;
        int maxHeight = 400;

        sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), quantized ? "Audio Quantization Histogram" : "Audio Histogram");

        for (auto sample : targetChannel) {
            int binIndex = (sample + 32768) / binSize;  // Handle signed 16-bit samples
            if (binIndex >= 0 && binIndex < 256) {
                histogram[binIndex]++;
            }
        }

        int maxBinValue = *max_element(histogram.begin(), histogram.end());

        int totalBinsWidth = histogram.size() * binWidth;
        int startX = (windowWidth - totalBinsWidth) / 2;  // Center horizontally
        int startY = windowHeight - 50;  
        
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }

            window.clear(sf::Color::Black);

            // Draw histogram bars
            for (size_t i = 0; i < histogram.size(); ++i) {
                float binHeight = static_cast<float>(histogram[i]) / maxBinValue * maxHeight;

                sf::RectangleShape bar(sf::Vector2f(binWidth - 1, -binHeight)); // -binHeight to flip vertically
                bar.setPosition(startX + i * binWidth, startY);  
                bar.setFillColor(color);
                window.draw(bar);
            }

            window.display();
        }

    }


    void quantization(int n)
    {
        int sampleCount = buffer.getSampleCount();  
        int max_value = *max_element(samples, samples + sampleCount);
        int min_value = *min_element(samples, samples + sampleCount);

        int numLevels = 1 << n;  
        float step_size = static_cast<float>(max_value - min_value) / (numLevels - 1);

        quantizedSamples.resize(sampleCount); 

        for (size_t i = 0; i < sampleCount; ++i) {
            float normalizedSample = (samples[i] - min_value) / step_size;
            quantizedSamples[i] = static_cast<sf::Int16>(round(normalizedSample)) * step_size + min_value;
        }
    }
};

int main(int argc, char *argv[])
{
    AudioProcessor p;
      string filename;
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

    p.playAudio();
    p.getAudioInfo();
    p.plotAudioWaveform();
    p.plotHistogram();
    p.quantization(8);
    p.plotHistogram(true);
    return 0;
}