#ifndef GAME_LOAD_STREAM_H_INCLUDED
#define GAME_LOAD_STREAM_H_INCLUDED

#include "stream.h"
#include "game_version.h"
#include <stdexcept>
#include <map>
#include <cmath>
#include <vector>

using namespace std;

class InvalidFileFormatException  : public IOException
{
public:
    explicit InvalidFileFormatException(string msg)
        : IOException(msg)
    {
    }
    explicit InvalidFileFormatException(IOException *e)
        : IOException(string("IO Error : ") + e->what())
    {
        delete e;
    }
};

class VersionTooNewException final : public InvalidFileFormatException
{
public:
    explicit VersionTooNewException()
        : InvalidFileFormatException("file version too new")
    {
    }
};

class InvalidDataValueException final : public InvalidFileFormatException
{
public:
    explicit InvalidDataValueException(string msg)
        : InvalidFileFormatException(msg)
    {
    }
};

class GameLoadStream final : public Reader
{
private:
    shared_ptr<Reader> reader;
    uint32_t fileVersionInternal;
    vector<BlockDescriptorPtr> blocks;
    template <typename T>
    static T limitAfterRead(T v, T min, T max)
    {
        if(v < min || v > max)
        {
            throw new InvalidDataValueException("read value out of range");
        }
        return v;
    }
public:
    static_assert(sizeof(uint8_t) == 1, "uint8_t is not a byte");
    static constexpr uint8_t MAGIC_STRING[8] = {'V', 'o', 'x', 'e', 'l', 's', ' ', ' '};
    const uint32_t fileVersion() const
    {
        return fileVersionInternal;
    }
    GameLoadStream(shared_ptr<Reader> reader)
        : reader(reader)
    {
        assert(reader);
        try
        {
            uint8_t testMagicString[8];
            reader->readBytes(testMagicString, sizeof(testMagicString));
            for(int i = 0; i < sizeof(MAGIC_STRING); i++)
                if(testMagicString[i] != MAGIC_STRING[i])
                {
                    throw new InvalidFileFormatException("Invalid Magic String");
                }
            fileVersionInternal = reader->readU32();
            if(fileVersionInternal > GameVersion::FILE_VERSION)
            {
                throw new VersionTooNewException;
            }
        }
        catch(InvalidFileFormatException *e)
        {
            throw e;
        }
        catch(IOException *e)
        {
            throw InvalidFileFormatException(e);
        }
    }
    virtual uint8_t readByte() override
    {
        return reader->readByte();
    }
    uint8_t readLimitedU8(uint8_t min, uint8_t max)
    {
        return limitAfterRead(readU8(), min, max);
    }
    int8_t readLimitedS8(int8_t min, int8_t max)
    {
        return limitAfterRead(readS8(), min, max);
    }
    uint16_t readLimitedU16(uint16_t min, uint16_t max)
    {
        return limitAfterRead(readU16(), min, max);
    }
    int16_t readLimitedS16(int16_t min, int16_t max)
    {
        return limitAfterRead(readS16(), min, max);
    }
    uint32_t readLimitedU32(uint32_t min, uint32_t max)
    {
        return limitAfterRead(readU32(), min, max);
    }
    int32_t readLimitedS32(int32_t min, int32_t max)
    {
        return limitAfterRead(readS32(), min, max);
    }
    uint64_t readLimitedU64(uint64_t min, uint64_t max)
    {
        return limitAfterRead(readU64(), min, max);
    }
    int64_t readLimitedS64(int64_t min, int64_t max)
    {
        return limitAfterRead(readS64(), min, max);
    }
    float readFiniteF32()
    {
        float retval = readF32();
        if(!isfinite(retval))
        {
            throw new InvalidDataValueException("read value is not finite");
        }
        return retval;
    }
    double readFiniteF64()
    {
        double retval = readF64();
        if(!isfinite(retval))
        {
            throw new InvalidDataValueException("read value is not finite");
        }
        return retval;
    }
    float readLimitedF32(float min, float max)
    {
        return limitAfterRead(readFiniteF32(), min, max);
    }
    double readLimitedF64(double min, double max)
    {
        return limitAfterRead(readFiniteF64(), min, max);
    }
    BlockDescriptorPtr readBlockDescriptor();
};

class GameStoreStream final : public Writer
{
private:
    shared_ptr<Writer> writer;
    uint32_t nextBlockIndex;
    map<wstring, uint32_t> blocks;
public:
    GameStoreStream(shared_ptr<Writer> writer)
        : writer(writer), nextBlockIndex(0)
    {
        assert(writer);
        writer->writeBytes(GameLoadStream::MAGIC_STRING, sizeof(GameLoadStream::MAGIC_STRING));
        writer->writeU32(GameVersion::FILE_VERSION);
    }
    virtual void writeByte(uint8_t v) override
    {
        return writer->writeByte(v);
    }
    void writeBlockDescriptor(BlockDescriptorPtr bd);
};

#endif // GAME_LOAD_STREAM_H_INCLUDED
