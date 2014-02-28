#include <zlib.h>
#include "compressed_stream.h"

namespace
{
inline z_streamp getZStream(void * v)
{
    return (z_streamp)v;
}
}

InflateReader::InflateReader(shared_ptr<Reader> reader)
    : reader(reader)
{
    state = (void *)new z_stream;
    getZStream(state)->zalloc = Z_NULL;
    getZStream(state)->zfree = Z_NULL;
    getZStream(state)->opaque = Z_NULL;
    int retval = deflateInit(getZStream(state), Z_DEFAULT_COMPRESSION);
    if(retval != Z_OK)
    {
        delete getZStream(state);
        switch(retval)
        {
        case Z_VERSION_ERROR:
            throw runtime_error("zlib version doesn't match compiled version");
        default:
            throw runtime_error("unknown zlib error");
        }
    }

}
