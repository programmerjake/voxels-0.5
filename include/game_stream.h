/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "block.h"
#include "entity.h"
#ifndef GAME_STREAM_H_INCLUDED
#define GAME_STREAM_H_INCLUDED

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
    explicit InvalidFileFormatException(IOException &e)
        : IOException(string("IO Error : ") + e.what())
    {
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
    vector<EntityDescriptorPtr> entities;
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
                    throw InvalidFileFormatException("Invalid Magic String");
                }
            }
            fileVersionInternal = reader->readU32();
            if(fileVersionInternal > GameVersion::FILE_VERSION)
            {
                throw VersionTooNewException();
            }
        }
        catch(InvalidFileFormatException &e)
        {
            throw e;
        }
        catch(IOException &e)
        {
            throw InvalidFileFormatException(e);
        }
    }
    virtual uint8_t readByte() override
    {
        return reader->readByte();
    }
    BlockDescriptorPtr readBlockDescriptor();
    EntityDescriptorPtr readEntityDescriptor();
};

class GameStoreStream final : public Writer
{
private:
    shared_ptr<Writer> writer;
    uint32_t nextBlockIndex;
    map<const wstring, uint32_t> blocks;
    uint32_t nextEntityIndex;
    map<const wstring, uint32_t> entities;
public:
    GameStoreStream(shared_ptr<Writer> writer)
        : writer(writer), nextBlockIndex(0), nextEntityIndex(0)
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
    void writeEntityDescriptor(EntityDescriptorPtr ed);
};

#endif // GAME_STREAM_H_INCLUDED
