#ifndef COMPRESSED_STREAM_H_INCLUDED
#define COMPRESSED_STREAM_H_INCLUDED

#include "stream.h"

class InflateReader final : public Reader
{
private:
    shared_ptr<Reader> reader;
    void * state;
public:
    InflateReader(shared_ptr<Reader> reader);
    InflateReader(Reader & reader)
        : InflateReader(shared_ptr<Reader>(&reader, [](Reader *){}))
    {
    }
    virtual ~InflateReader();
    virtual uint8_t readByte() override;
};

#endif // COMPRESSED_STREAM_H_INCLUDED
