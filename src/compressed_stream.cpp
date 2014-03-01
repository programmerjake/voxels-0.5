#include "compressed_stream.h"
#include "util.h"
#include <iostream>
#include <cstdlib>
#include <thread>

#if 0 // use demo code
namespace
{
class DumpWriter final : public Writer
{
public:
    virtual void writeByte(uint8_t v) override
    {
        cout << hex << (unsigned)(v >> 4) << (unsigned)(v & 0xF) << endl << dec;
    }
};

void dumpRead(shared_ptr<Reader> preader)
{
    ExpandReader reader(preader);
    try
    {
        while(true)
            cout << (char)reader.readByte() << endl;
    }
    catch(EOFException & e)
    {
    }
}

initializer init1([]()
{
    cout << "test compression :\n";
    thread readerThread;
    {
        StreamPipe pipe;
        readerThread = thread(dumpRead, pipe.preader());
        CompressWriter w(pipe.pwriter());
        for(const char * str = "abcdefghij012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123abcdefg";*str;str++)
        {
            w.writeByte(*str);
        }
        w.flush();
    }
    readerThread.join();
    exit(0);
});
}
#endif // use demo code

