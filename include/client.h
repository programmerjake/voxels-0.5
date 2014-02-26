#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <map>
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
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
        ServerFlag,
        Last
    };
    typedef uint_fast64_t IdType;
    static constexpr IdType NullId = 0;
    static void writeId(Writer &writer, IdType id)
    {
        writer.writeU64(id);
    }
    static IdType readId(Reader &reader)
    {
        IdType retval = reader.readU64();
        DUMP_V(Client::readId, retval);
        return retval;
    }
    static IdType readIdNonNull(Reader &reader)
    {
        IdType retval = reader.readLimitedU64(1, ~(uint64_t)0);
        DUMP_V(Client::readIdNonNull, retval);
        return retval;
    }
    static IdType getNewId()
    {
        return nextId++;
    }
private:
    static atomic_uint_fast64_t nextId;
    map<shared_ptr<void>, IdType> idMap[(int)DataType::Last];
    map<IdType, shared_ptr<void>> ptrMap[(int)DataType::Last];
    recursive_mutex theLock;
public:
    Client()
    {
    }
    void lock()
    {
        theLock.lock();
    }
    void unlock()
    {
        theLock.unlock();
    }
    template <typename T>
    IdType getId(shared_ptr<T> ptr, DataType dataType)
    {
        assert(ptr != nullptr);
        lock();
        IdType retval = idMap[(int)dataType][ptr];
        unlock();
        return retval;
    }
    template <typename T>
    IdType makeId(shared_ptr<T> ptr, DataType dataType)
    {
        assert(ptr != nullptr);
        lock();
        IdType retval = idMap[(int)dataType][ptr] = getNewId();
        ptrMap[(int)dataType][retval] = ptr;
        unlock();
        return retval;
    }
    template <typename T>
    shared_ptr<T> getPtr(IdType id, DataType dataType)
    {
        assert(id != NullId);
        lock();
        shared_ptr<T> retval = static_pointer_cast<T>(ptrMap[(int)dataType][id]);
        unlock();
        return retval;
    }
    template <typename T>
    void setPtr(shared_ptr<T> ptr, IdType id, DataType dataType)
    {
        assert(ptr != nullptr && id != NullId);
        lock();
        idMap[(int)dataType][ptr] = id;
        ptrMap[(int)dataType][id] = ptr;
        unlock();
    }
};

class LockedClient
{
    LockedClient(const LockedClient &) = delete;
    const LockedClient & operator =(const LockedClient &) = delete;
private:
    Client & client;
public:
    explicit LockedClient(Client & client)
        : client(client)
    {
        client.lock();
    }
    ~LockedClient()
    {
        client.unlock();
    }
};

void clientProcess(StreamRW & streamRW);

#endif // CLIENT_H_INCLUDED
