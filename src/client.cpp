#include "client.h"
#include <thread>

atomic_uint_fast64_t Client::nextId(1);

namespace
{
void clientProcessH(Reader & reader, Writer & writer, Client & client)
{

}
}

void clientProcess(Reader & reader, Writer & writer)
{
    uint64_t readCount = reader.readU64();
    for(uint64_t i = 0; i < readCount; i++)
}
