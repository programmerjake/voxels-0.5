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
        : IOException("IO Error : " + e->what())
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
            reader->readBytes(testMagicString, sizeof(testMagicString))
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
};

#endif // GAME_LOAD_STREAM_H_INCLUDED
