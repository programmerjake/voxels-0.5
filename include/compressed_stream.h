#ifndef COMPRESSED_STREAM_H_INCLUDED
#define COMPRESSED_STREAM_H_INCLUDED

#include <deque>
#include "stream.h"
#include <iostream>

class LZ77FormatException final : public IOException
{
public:
    LZ77FormatException()
        : IOException("LZ77 format error")
    {
    }
};

struct LZ77CodeType final
{
    static constexpr int lengthBits = 6, offsetBits = 16 - lengthBits;
    static constexpr size_t maxLength = (1 << lengthBits) - 1, maxOffset = (1 << offsetBits) - 1;
    size_t length;
    size_t offset;
    uint8_t nextByte;
    LZ77CodeType(size_t length, size_t offset, uint8_t nextByte)
        : length(length), offset(offset), nextByte(nextByte)
    {
    }
    LZ77CodeType() // initialize with EOF
        : length(0), offset(1), nextByte(0)
    {
    }
    LZ77CodeType(uint8_t nextByte)
        : length(0), offset(0), nextByte(nextByte)
    {
    }
    bool hasNextByte()
    {
        return length != 0 || offset == 0;
    }
    bool eof()
    {
        return length == 0 && offset != 0;
    }
    static LZ77CodeType read(Reader &reader)
    {
        LZ77CodeType retval;
        retval.nextByte = reader.readByte();

        try
        {
            uint16_t v = reader.readU16();
            retval.length = v >> offsetBits;
            retval.offset = v & maxOffset;
        }
        catch(EOFException &e)
        {
            throw LZ77FormatException();
        }

        //cout << "Read code : \'" << retval.nextByte << "\'\nlength : " << retval.length << "\noffset : " << retval.offset << endl;

        return retval;
    }
    void write(Writer &writer)
    {
        writer.writeByte(nextByte);
        uint16_t v = (offset & maxOffset) | (length << offsetBits);
        writer.writeU16(v);
    }
};

class ExpandReader final : public Reader
{
private:
    shared_ptr<Reader> reader;
    static constexpr size_t bufferSize = LZ77CodeType::maxOffset + 1;
    deque<uint8_t> buffer;
    LZ77CodeType currentCode;
public:
    ExpandReader(shared_ptr<Reader> reader)
        : reader(reader)
    {
    }
    ExpandReader(Reader &reader)
        : ExpandReader(shared_ptr<Reader>(&reader, [](Reader *) {}))
    {
    }
    virtual ~ExpandReader()
    {
    }
    virtual uint8_t readByte() override
    {
        while(currentCode.eof())
        {
            currentCode = LZ77CodeType::read(*reader);
        }

        uint8_t retval;

        if(currentCode.length == 0)
        {
            retval = currentCode.nextByte;
            currentCode = LZ77CodeType();
        }
        else
        {
            if(currentCode.offset >= buffer.size())
            {
                throw LZ77FormatException();
            }

            retval = buffer.cbegin()[currentCode.offset];

            if(--currentCode.length == 0)
            {
                currentCode = LZ77CodeType(currentCode.nextByte);
            }
        }

        buffer.push_front(retval);

        if(buffer.size() > bufferSize)
        {
            buffer.pop_back();
        }

        return retval;
    }
};

class CompressWriter final : public Writer
{
private:
    static constexpr int uint8_max = (1 << 8) - 1;
    static constexpr size_t bufferSize = LZ77CodeType::maxOffset + 1;
    struct Match
    {
        size_t location, length;
        Match(size_t location, size_t length)
            : location(location), length(length)
        {
        }
        Match()
            : Match(0, 0)
        {
        }
    };

    shared_ptr<Writer> writer;
    deque<uint_fast8_t> currentInput;
    deque<uint_fast8_t> buffer;

    void addByte(uint_fast8_t v)
    {
        buffer.push_front(v);
        if(buffer.size() > bufferSize)
            buffer.pop_back();
    }

    Match getBiggestMatch()
    {
        Match retval;
        size_t curPosition = 0;
        for(auto i = buffer.cbegin(); i != buffer.cend(); i++, curPosition++)
        {
            size_t curLength = 0;
            auto ii = currentInput.cbegin();
            for(size_t j = 0; j <= curPosition && ii != currentInput.cend(); j++, curLength++)
            {
                if(i[-j] != *ii++)
                {
                    break;
                }
            }
            if(curLength >= LZ77CodeType::maxLength)
                return Match(curPosition, LZ77CodeType::maxLength);
            if(curLength > retval.length)
            {
                retval.location = curPosition;
                retval.length = curLength;
            }
        }
        return retval;
    }

    void writeCode()
    {
        if(currentInput.empty())
            return;
        if(currentInput.size() == 1)
        {
            addByte(currentInput.front());
            LZ77CodeType(currentInput.front()).write(*writer);
            currentInput.pop_front();
            return;
        }
        Match m = getBiggestMatch();
        if(m.length <= 1)
        {
            addByte(currentInput.front());
            LZ77CodeType(currentInput.front()).write(*writer);
            currentInput.pop_front();
            return;
        }
        m.length--;
        for(size_t i = 0; i < m.length; i++)
        {
            addByte(currentInput.front());
            currentInput.pop_front();
        }
        LZ77CodeType code(m.length, m.location, currentInput.front());
        code.write(*writer);
        addByte(currentInput.front());
        currentInput.pop_front();
    }

public:
    CompressWriter(shared_ptr<Writer> writer)
        : writer(writer)
    {
    }
    CompressWriter(Writer &writer)
        : CompressWriter(shared_ptr<Writer>(&writer, [](Writer *) {}))
    {
    }
    virtual ~CompressWriter()
    {
        flush();
    }
    virtual void flush() override
    {
        while(!currentInput.empty())
            writeCode();
    }
    virtual void writeByte(uint8_t v) override
    {
        currentInput.push_back(v);
        if(currentInput.size() < bufferSize)
            return;
        writeCode();
    }
};

#endif // COMPRESSED_STREAM_H_INCLUDED
