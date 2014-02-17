#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <map>
#include <cstdint>
#include <atomic>
#include <memory>
#include "stream.h"

using namespace std;

class Client final
{
public:
    enum class DataType : uint8_t
    {
        Image,
        RenderObjectBlockMesh,
        RenderObjectWorld,
        Last
    };
    typedef uint_fast64_t IdType;
    static constexpr IdType NullId = 0;
    static void writeId(Writer &writer, IdType id)
    {
        writer->writeU64(id);
    }
    static IdType readId(Reader &reader)
    {
        return reader->readU64();
    }
    static IdType readIdNonNull(Reader &reader)
    {
        return reader->readlimitedU64(1, ~(uint64_t)0);
    }
private:
    static atomic_uint_fast64_t nextId;
    map<shared_ptr<void>, IdType> idMap[DataType::Last];
    map<IdType, shared_ptr<void> ptrMap[DataType::Last];
public:
    Client()
    {
    }
    template <typename T>
    IdType getId(shared_ptr<T> ptr, DataType dataType)
    {
        assert(ptr != nullptr);
        return idMap[dataType][ptr];
    }
    template <typename T>
    IdType makeId(shared_ptr<T> ptr, DataType dataType)
    {
        assert(ptr != nullptr);
        IdType retval = idMap[dataType][ptr] = nextId++;
        ptrMap[dataType][retval] = ptr;
        return retval;
    }
    template <typename T>
    shared_ptr<T> getPtr(IdType id, DataType dataType)
    {
        assert(id != NullId);
        return ptrMap[dataType][id];
    }
    template <typename T>
    void setPtr(shared_ptr<T> ptr, IdType id, DataType dataType)
    {
        assert(ptr != nullptr && id != NullId);
        idMap[dataType][ptr] = id;
        ptrMap[dataType][id] = ptr;
    }
};

#endif // CLIENT_H_INCLUDED
