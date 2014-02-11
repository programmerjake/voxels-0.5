#ifndef GAME_LOAD_STREAM_H_INCLUDED
#define GAME_LOAD_STREAM_H_INCLUDED

#include "stream.h"
#include "game_version.h"
#include <stdexcept>

class InvalidFileFormatException  : public IOException
{
public:
    explicit InvalidFileFormatException(string msg)
        : IOException(msg)
    {
    }
    explicit InvalidFileFormatException(IOException * e)
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
                    throw new InvalidFileFormatException("Invalid Magic String");
            fileVersionInternal = reader->readU32();
            if(fileVersionInternal > GameVersion::FILE_VERSION)
                throw new VersionTooNewException;
        }
        catch(InvalidFileFormatException * e)
        {
            throw e;
        }
        catch(IOException * e)
        {
            throw InvalidFileFormatException(e);
        }
    }
    virtual uint8_t readByte() override
    {
        return reader->readByte();
    }
    BlockDescriptorPtr readBlockDescriptor()
    {
    #error finish
    }
};

#endif // GAME_LOAD_STREAM_H_INCLUDED
