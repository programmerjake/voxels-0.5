#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include <cwchar>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include "stream.h"

using namespace std;

class AudioLoadError final : public runtime_error
{
public:
    explicit AudioLoadError(const string &arg)
        : runtime_error(arg)
    {
    }
};

class AudioDecoder
{
    AudioDecoder(const AudioDecoder &) = delete;
    const AudioDecoder & operator =(const AudioDecoder &) = delete;
public:
    AudioDecoder()
    {
    }
    virtual ~AudioDecoder()
    {
    }
    virtual unsigned samplesPerSecond() = 0;
    virtual uint64_t numSamples() = 0;
    virtual bool eof() = 0;
    virtual unsigned channelCount() = 0;
    double lengthInSeconds()
    {
        return (double)numSamples() / samplesPerSecond();
    }
    virtual uint64_t decodeAudioBlock(int16_t * data, uint64_t samplesCount) = 0; // returns number of samples decoded
};

class MemoryAudioDecoder final : public AudioDecoder
{
    vector<int16_t> samples;
    unsigned sampleRate;
    size_t currentLocation;
    unsigned channels;
public:
    MemoryAudioDecoder(const vector<int16_t> &data, unsigned sampleRate, unsigned channelCount)
        : samples(data), sampleRate(sampleRate), currentLocation(0), channels(channelCount)
    {
        assert(sampleRate > 0);
        assert(channels > 0);
        assert(samples.size() >= channels);
        assert(samples.size() % channels == 0);
    }
    virtual unsigned samplesPerSecond() override
    {
        return sampleRate;
    }
    virtual uint64_t numSamples() override
    {
        return samples.size() / channels;
    }
    virtual bool eof() override
    {
        if(currentLocation >= samples.size())
            return true;
        return false;
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual uint64_t decodeAudioBlock(int16_t * data, uint64_t samplesCount) override // returns number of samples decoded
    {
        uint64_t retval = 0;
        for(uint64_t i = 0; i < samplesCount && !eof(); i++)
        {
            for(unsigned j = 0; j < channels; j++)
                *data++ = samples[currentLocation++];
            retval++;
        }
        return retval;
    }
};

struct AudioData;
struct PlayingAudioData;

class PlayingAudio final
{
    shared_ptr<PlayingAudioData> data;
    PlayingAudio(shared_ptr<PlayingAudioData> data)
        : data(data)
    {
    }
public:
    bool isPlaying();
    double currentTime();
    void stop();
    float volume();
    void volume(float v);
    double duration();
};

class Audio final
{
    shared_ptr<AudioData> data;
public:
    Audio()
        : data(nullptr)
    {
    }
    Audio(nullptr_t)
        : data(nullptr)
    {
    }
    explicit Audio(wstring resourceName, bool isStreaming = false);
    explicit Audio(const vector<int16_t> &data, unsigned sampleRate, unsigned channelCount);
    shared_ptr<PlayingAudio> play(float volume = 1, bool looped = false);
    inline shared_ptr<PlayingAudio> play(bool looped = false)
    {
        return play(1, looped);
    }
};

#endif // AUDIO_H_INCLUDED
