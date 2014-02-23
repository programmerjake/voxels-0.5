#include "block.h"
#ifndef GAME_LOAD_STREAM_H_INCLUDED
#define GAME_LOAD_STREAM_H_INCLUDED

#include "stream.h"
#include "game_version.h"
#include <stdexcept>
#include <map>
#include <cmath>
#include <vector>
#include <memory>

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

class GameLoadStream final : public Reader
{
private:
    shared_ptr<Reader> reader;
    uint32_t fileVersionInternal;
    vector<BlockDescriptorPtr> blocks;
public:
    static_assert(sizeof(uint8_t) == 1, "uint8_t is not a byte");
    static constexpr uint8_t MAGIC_STRING[8] = {'V', 'o', 'x', 'e', 'l', 's', ' ', ' '};
    uint32_t fileVersion() const
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
            for(size_t i = 0; i < sizeof(MAGIC_STRING); i++)
            {
                if(testMagicString[i] != MAGIC_STRING[i])
                {
                    throw new InvalidFileFormatException("Invalid Magic String");
                }
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
    BlockDescriptorPtr readBlockDescriptor();
};

class GameStoreStream final : public Writer
{
private:
    shared_ptr<Writer> writer;
    uint32_t nextBlockIndex;
    map<const wstring, uint32_t> blocks;
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
        writer->writeByte(v);
    }
    virtual void flush() override
    {
        writer->flush();
    }
    void writeBlockDescriptor(BlockDescriptorPtr bd);
};

#endif // GAME_LOAD_STREAM_H_INCLUDED
