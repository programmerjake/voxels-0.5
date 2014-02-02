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
#ifndef STREAM_H
#define STREAM_H

#include <cstdint>
#include <stdexcept>
#include <cwchar>
#include <string>
#include <cstdio>
#include <cstring>
#include "util.h"

using namespace std;

class IOException : public runtime_error
{
public:
    explicit IOException(const string & msg)
        : runtime_error(msg)
    {
    }
    explicit IOException(exception * e, bool deleteIt = true)
        : runtime_error((dynamic_cast<IOException *>(e) == nullptr) ? string("IO Error : ") + e->what() : string(e->what()))
    {
        if(deleteIt)
            delete e;
    }
    explicit IOException(exception & e, bool deleteIt = false)
        : IOException(&e, deleteIt)
    {
    }
};

class EOFException final : public IOException
{
public:
    explicit EOFException()
        : IOException("IO Error : reached end of file")
    {
    }
};

class UTFDataFormatException final : public IOException
{
public:
    explicit UTFDataFormatException()
        : IOException("IO Error : invalid UTF data")
    {
    }
};

class Reader
{
public:
    Reader()
    {
    }
    Reader(const Reader &) = delete;
    const Reader & operator =(const Reader &) = delete;
    virtual ~Reader()
    {
    }
    virtual uint8_t readByte() = 0;
    uint8_t readU8()
    {
        return readByte();
    }
    int8_t readS8()
    {
        return readByte();
    }
    uint16_t readU16()
    {
        uint16_t v = readU8();
        return (v << 8) | readU8();
    }
    int16_t readS16()
    {
        return readU16();
    }
    uint32_t readU32()
    {
        uint32_t v = readU16();
        return (v << 16) | readU16();
    }
    int32_t readS32()
    {
        return readU32();
    }
    uint64_t readU64()
    {
        uint64_t v = readU32();
        return (v << 32) | readU32();
    }
    int64_t readS64()
    {
        return readU64();
    }
    float readF32()
    {
        static_assert(sizeof(float) == sizeof(uint32_t), "float is not 32 bits");
        union
        {
            uint32_t ival;
            float fval;
        };
        ival = readU32();
        return fval;
    }
    double readF64()
    {
        static_assert(sizeof(double) == sizeof(uint64_t), "double is not 64 bits");
        union
        {
            uint64_t ival;
            double fval;
        };
        ival = readU64();
        return fval;
    }
    bool readBool()
    {
        return readU8() != 0;
    }
    wstring readString()
    {
        wstring retval = L"";
        for(;;)
        {
            uint32_t b1 = readU8();
            if(b1 == 0)
            {
                return retval;
            }
            else if((b1 & 0x80) == 0)
            {
                retval += (wchar_t)b1;
            }
            else if((b1 & 0xE0) == 0xC0)
            {
                uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t v = b2 & 0x3F;
                v |= (b1 & 0x1F) << 6;
                retval += (wchar_t)v;
            }
            else if((b1 & 0xF0) == 0xE0)
            {
                uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t b3 = readU8();
                if((b3 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t v = b3 & 0x3F;
                v |= (b2 & 0x3F) << 6;
                v |= (b1 & 0xF) << 12;
                retval += (wchar_t)v;
            }
            else if((b1 & 0xF8) == 0xF0)
            {
                uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t b3 = readU8();
                if((b3 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t b4 = readU8();
                if((b4 & 0xC0) != 0x80)
                    throw new UTFDataFormatException();
                uint32_t v = b4 & 0x3F;
                v |= (b3 & 0x3F) << 6;
                v |= (b2 & 0x3F) << 12;
                v |= (b1 & 0x7) << 18;
                if(v >= 0x10FFFF)
                    throw new UTFDataFormatException();
                retval += (wchar_t)v;
            }
            else
                throw new UTFDataFormatException();
        }
    }
};

class Writer
{
public:
    Writer()
    {
    }
    Writer(const Writer &) = delete;
    const Writer & operator =(const Writer &) = delete;
    virtual ~Writer()
    {
    }
    virtual void writeByte(uint8_t) = 0;
    void writeU8(uint8_t v)
    {
        writeByte(v);
    }
    void writeS8(int8_t v)
    {
        writeByte(v);
    }
    void writeU16(uint16_t v)
    {
        writeU8((uint8_t)(v >> 8));
        writeU8((uint8_t)(v & 0xFF));
    }
    void writeS16(int16_t v)
    {
        writeU16(v);
    }
    void writeU32(uint32_t v)
    {
        writeU16((uint16_t)(v >> 16));
        writeU16((uint16_t)(v & 0xFFFF));
    }
    void writeS32(int32_t v)
    {
        writeU32(v);
    }
    void writeU64(uint64_t v)
    {
        writeU64((uint64_t)(v >> 32));
        writeU64((uint64_t)(v & 0xFFFFFFFFU));
    }
    void writeS64(int64_t v)
    {
        writeU64(v);
    }
    void writeF32(float v)
    {
        static_assert(sizeof(float) == sizeof(uint32_t), "float is not 32 bits");
        union
        {
            uint32_t ival;
            float fval;
        };
        fval = v;
        writeU32(ival);
    }
    void writeF64(double v)
    {
        static_assert(sizeof(double) == sizeof(uint64_t), "double is not 64 bits");
        union
        {
            uint64_t ival;
            double fval;
        };
        fval = v;
        writeU64(ival);
    }
    void writeBool(bool v)
    {
        writeU8(v ? 1 : 0);
    }
    void writeString(wstring v)
    {
        for(size_t i = 0; i < v.length(); i++)
        {
            uint32_t ch = v[i];
            if(ch != 0 && ch < 0x80)
            {
                writeU8(ch);
            }
            else if(ch < 0x800)
            {
                writeU8(0xC0 | ((ch >> 6) & 0x1F));
                writeU8(0x80 | ((ch) & 0x3F));
            }
            else if(ch < 0x1000)
            {
                writeU8(0xE0 | ((ch >> 12) & 0xF));
                writeU8(0x80 | ((ch >> 6) & 0x3F));
                writeU8(0x80 | ((ch) & 0x3F));
            }
            else
            {
                writeU8(0xF0 | ((ch >> 18) & 0x7));
                writeU8(0x80 | ((ch >> 12) & 0x3F));
                writeU8(0x80 | ((ch >> 6) & 0x3F));
                writeU8(0x80 | ((ch) & 0x3F));
            }
        }
        writeU8(0);
    }
};

class FileReader final : public Reader
{
private:
    FILE * f;
public:
    FileReader(wstring fileName)
    {
        string str = wcsrtombs(fileName);
        f = fopen(str.c_str(), "rb");
        if(f == nullptr)
            throw new IOException(string("IO Error : ") + strerror(errno));
    }
    virtual ~FileReader()
    {
        fclose(f);
    }
    virtual uint8_t readByte() override
    {
        int ch = fgetc(f);
        if(ch == EOF)
        {
            if(ferror(f))
                throw new IOException("IO Error : can't read from file");
            throw new EOFException;
        }
        return ch;
    }
};

class FileWriter final : public Writer
{
private:
    FILE * f;
public:
    FileWriter(wstring fileName)
    {
        string str = wcsrtombs(fileName);
        f = fopen(str.c_str(), "wb");
        if(f == nullptr)
            throw new IOException(string("IO Error : ") + strerror(errno));
    }
    virtual ~FileWriter()
    {
        fclose(f);
    }
    virtual void writeByte(uint8_t v) override
    {
        if(fputc(v, f) == EOF)
            throw new IOException("IO Error : can't write to file");
    }
};

#endif // STREAM_H