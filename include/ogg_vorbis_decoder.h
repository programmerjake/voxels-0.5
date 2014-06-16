#ifndef OGG_VORBIS_DECODER_H_INCLUDED
#define OGG_VORBIS_DECODER_H_INCLUDED

#include "stream.h"
#include "audio.h"
#include <vorbis/vorbisfile.h>
#include <cerrno>
#include <endian.h>

class OggVorbisDecoder final : public AudioDecoder
{
private:
    OggVorbis_File ovf;
    shared_ptr<Reader> reader;
    uint64_t samples;
    unsigned channels;
    uint64_t currentSample;
    unsigned sampleRate;
    static size_t read_fn(void * dataPtr_in, size_t blockSize, size_t numBlocks, void * dataSource)
    {
        OggVorbisDecoder & decoder = *(OggVorbisDecoder *)dataSource;
        size_t readCount = 0;
        try
        {
            uint8_t * dataPtr = (uint8_t *)dataPtr_in;
            for(size_t i = 0; i < numBlocks; i++, readCount++)
            {
                decoder.reader->readBytes(dataPtr, blockSize);
                dataPtr += blockSize;
            }
        }
        catch(EOFException & e)
        {
            return readCount;
        }
        catch(IOException & e)
        {
            errno = EIO;
            return readCount;
        }
        return readCount;
    }
public:
    OggVorbisDecoder(shared_ptr<Reader> reader)
        : reader(reader)
    {
        ov_callbacks callbacks;
        callbacks.read_func = &read_fn;
        callbacks.seek_func = nullptr;
        callbacks.close_func = nullptr;
        callbacks.tell_func = nullptr;
        int openRetval = ov_open_callbacks((void *)this, &ovf, NULL, 0, callbacks);
        switch(openRetval)
        {
        case 0:
            break;
        default:
            throw IOException("invalid ogg vorbis audio");
        }
        vorbis_info *info = ov_info(&ovf, -1);
        channels = info->channels;
        sampleRate = info->rate;
        samples = ov_pcm_total(&ovf, -1);
        currentSample = 0;
        if(samples == 0 || channels == 0 || sampleRate == 0)
        {
            ov_clear(&ovf);
            throw IOException("invalid ogg vorbis audio");
        }
    }
    virtual ~OggVorbisDecoder()
    {
        ov_clear(&ovf);
    }
    virtual unsigned samplesPerSecond() override
    {
        return sampleRate;
    }
    virtual uint64_t numSamples() override
    {
        return samples;
    }
    virtual bool eof() override
    {
        return currentSample >= sampleCount;
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual uint64_t decodeAudioBlock(int16_t * data, uint64_t readCount) override // returns number of samples decoded
    {
        int currentSection;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        return ov_read(&ovf, (char *)data, channels * readCount * sizeof(int16_t), 0, sizeof(int16_t), 1, &currentSection) / channels / sizeof(int16_t);
#elif __BYTE_ORDER == __BIG_ENDIAN
        return ov_read(&ovf, (char *)data, channels * readCount * sizeof(int16_t), 1, sizeof(int16_t), 1, &currentSection) / channels / sizeof(int16_t);
#else
#error invalid endian value
#endif
    }
};

#endif // OGG_VORBIS_DECODER_H_INCLUDED
