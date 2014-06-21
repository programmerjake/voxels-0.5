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
#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <unordered_map>
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
#include "stream.h"

#ifndef PHYSICS_OBJECT_H_INCLUDED
struct PhysicsWorld;
#endif // PHYSICS_OBJECT_H_INCLUDED

using namespace std;

class Client final
{
public:
    enum class DataType
    {
        Image, // Image::ImageData
        RenderObjectBlockMesh, // RenderObjectBlockMesh
        RenderObjectEntityMesh, // RenderObjectEntityMesh
        RenderObjectEntity, // RenderObjectEntity
        RenderObjectEntitySet, // set<RenderObjectEntity>
        RenderObjectWorld, // RenderObjectWorld
        ServerFlag, // flag
        UpdateList, // UpdateList
        VectorF, // VectorF
        PositionF, // PositionF
        Script, // Script
        Double, // double
        Float, // float
        Player, // EntityData
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
    unordered_map<shared_ptr<void>, IdType> idMap[(int)DataType::Last];
    unordered_map<IdType, shared_ptr<void>> ptrMap[(int)DataType::Last];
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
    recursive_mutex &getLock()
    {
        return theLock;
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
    void removeId(IdType id, DataType dataType)
    {
        assert(id != NullId);
        lock();
        auto ptr = ptrMap[(int)dataType][id];

        if(ptr != nullptr)
        {
            idMap[(int)dataType].erase(ptr);
        }

        ptrMap[(int)dataType].erase(id);
        unlock();
    }
    template<typename T>
    void removePtr(shared_ptr<T> ptr, DataType dataType)
    {
        assert(ptr != nullptr);
        lock();
        auto id = idMap[(int)dataType][ptr];

        if(id != NullId)
        {
            ptrMap[(int)dataType].erase(id);
        }

        idMap[(int)dataType].erase(ptr);
        unlock();
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
    template <typename T>
    static shared_ptr<T> defaultInternalReader(Reader &reader, Client &client)
    {
        return T::readInternal(reader, client);
    }
    template <typename T>
    inline shared_ptr<T> readObjectNonNull(Reader &reader, DataType dataType,
                                           shared_ptr<T> (*internalReader)(Reader &, Client &) = &defaultInternalReader<T>)
    {
        IdType id = readIdNonNull(reader);
        shared_ptr<T> retval = getPtr<T>(id, dataType);

        if(retval != nullptr)
        {
            return retval;
        }

        retval = internalReader(reader, *this);
        assert(retval != nullptr);
        setPtr(retval, dataType);
        return retval;
    }
    template <typename T>
    inline shared_ptr<T> readObject(Reader &reader, DataType dataType,
                                    shared_ptr<T> (*internalReader)(Reader &, Client &) = &defaultInternalReader<T>)
    {
        IdType id = readId(reader);

        if(id == NullId)
        {
            return nullptr;
        }

        shared_ptr<T> retval = getPtr<T>(id, dataType);

        if(retval != nullptr)
        {
            return retval;
        }

        retval = internalReader(reader, *this);
        assert(retval != nullptr);
        setPtr(retval, id, dataType);
        return retval;
    }
    template <typename T>
    static void defaultInternalWriter(Writer &writer, Client &client, shared_ptr<T> object)
    {
        object->writeInternal(writer, client);
    }
    template <typename T>
    inline void writeObject(Writer &writer, shared_ptr<T> object, DataType dataType,
                            void (*internalWriter)(Writer &, Client &, shared_ptr<T>))
    {
        if(object == nullptr)
        {
            writeId(writer, NullId);
            return;
        }

        IdType id = getId(object, dataType);

        if(id != NullId)
        {
            writeId(writer, id);
            return;
        }

        id = makeId(object, dataType);
        writeId(writer, id);
        internalWriter(writer, *this, object);
    }

    template <typename T>
    inline void writeObject(Writer &writer, shared_ptr<T> object, DataType dataType)
    {
        writeObject(writer, object, dataType, &defaultInternalWriter);
    }

    template <typename T>
    static shared_ptr<T> defaultMakeObject()
    {
        return make_shared<T>();
    }
    template <typename T, size_t n>
    inline T &getPropertyReference(DataType dataType,
                                   shared_ptr<T>(*makeObject)() = &defaultMakeObject<T>)
    {
        static IdType id = NullId;
        static mutex idLock;

        if(id == NullId)
        {
            lock_guard<mutex> lockIt(idLock);

            if(id == NullId)
            {
                id = getNewId();
            }
        }

        lock_guard<recursive_mutex> lockIt(getLock());
        shared_ptr<T> retval = getPtr<T>(id, dataType);

        if(retval == nullptr)
        {
            setPtr(retval = makeObject(), id, dataType);
        }

        return *retval;
    }
    template <typename T, size_t n>
    inline shared_ptr<T> getPropertyPtr(DataType dataType,
                                   shared_ptr<T>(*makeObject)() = &defaultMakeObject<T>)
    {
        static IdType id = NullId;
        static mutex idLock;

        if(id == NullId)
        {
            lock_guard<mutex> lockIt(idLock);

            if(id == NullId)
            {
                id = getNewId();
            }
        }

        lock_guard<recursive_mutex> lockIt(getLock());
        shared_ptr<T> retval = getPtr<T>(id, dataType);

        if(retval == nullptr)
        {
            setPtr(retval = makeObject(), id, dataType);
        }

        return retval;
    }
    template <typename T>
    vector<shared_ptr<T>> getAllPtrs(DataType dataType)
    {
        lock_guard<recursive_mutex> lockIt(getLock());
        vector<shared_ptr<T>> retval;
        retval.reserve(ptrMap[(int)dataType].size());
        for(auto p : ptrMap[(int)dataType])
        {
            retval.push_back(static_pointer_cast<T>(get<1>(p)));
        }
        return retval;
    }
    shared_ptr<PhysicsWorld> physicsWorld;
};

class LockedClient
{
    LockedClient(const LockedClient &) = delete;
    const LockedClient &operator =(const LockedClient &) = delete;
private:
    Client &client;
public:
    explicit LockedClient(Client &client)
        : client(client)
    {
        client.lock();
    }
    ~LockedClient()
    {
        client.unlock();
    }
};

void clientProcess(StreamRW &streamRW);

#include "physics.h"

#endif // CLIENT_H_INCLUDED
