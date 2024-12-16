#include <SFML/Audio.hpp>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

class audioReader
{
private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
    const sf::Int16 *samples;
    int sampleCount;
    int channelCount;

public:
    bool readFile(const string &file)
    {
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

    const sf::Int16 *getSamples(const string &file)
    {
        samples = buffer.getSamples();
        return samples;
    }
    int getSampleCount(const string &file)
    {
        sampleCount = buffer.getSampleCount();
        return sampleCount;
    }
    int getChannelCount(const string &file)
    {
        channelCount = buffer.getChannelCount();
        return channelCount;
    }
    int getBitsPerSample(const string &file)
    {
        return buffer.getSampleRate();
    }
};