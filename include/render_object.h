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
#ifndef RENDER_OBJECT_H_INCLUDED
#define RENDER_OBJECT_H_INCLUDED

#include "mesh.h"
#include "position.h"
#include "render_layer.h"
#include "stream.h"
#include "client.h"
#include "block_face.h"
#include "light.h"
#include "ray_casting.h"
#include "script.h"
#include "physics.h"
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <tuple>
#include <utility>

using namespace std;

class RenderObject : public enable_shared_from_this<RenderObject>
{
private:
    RenderObject(const RenderObject &) = delete;
    const RenderObject &operator =(const RenderObject &) = delete;
    static uint64_t makeId()
    {
        static atomic_uint_fast64_t nextId(0);
        return (uint64_t)(nextId++);
    }
protected:
    virtual void writeInternal(Writer &writer, Client &client) = 0;
public:
    enum class Type : uint_fast8_t
    {
        Entity,
        Block,
        Last
    };
    const uint64_t id;
    virtual Type type() const = 0;
protected:
    RenderObject()
        : id(makeId())
    {
    }
public:
    virtual ~RenderObject()
    {
    }
    void write(Writer &writer, Client &client);
    static shared_ptr<RenderObject> read(Reader &reader, Client &client);
    virtual bool operator ==(const RenderObject &rt) const = 0;
    virtual void render(Mesh dest, RenderLayer rl, Dimension d, Client &client, unsigned naturalLight) = 0;
    virtual BoxRayCollision rayHits(Ray ray) = 0;
    virtual bool isPlayer() const
    {
        return false;
    }
};

class RenderObjectWorld;

class RenderObjectEntityMesh final : public enable_shared_from_this<RenderObjectEntityMesh>
{
private:
    struct Part
    {
        Mesh mesh;
        shared_ptr<Script> script;
        Part()
        {
        }
        Part(Mesh mesh, shared_ptr<Script> script)
            : mesh(mesh), script(script)
        {
        }
    };
    vector<Part> parts;
    VectorF hitBoxMin, hitBoxMax;
    shared_ptr<PhysicsObjectConstructor> physicsConstructor;
public:
    const bool isPlayer;
    RenderObjectEntityMesh(VectorF hitBoxMin, VectorF hitBoxMax, shared_ptr<PhysicsObjectConstructor> physicsConstructor, bool isPlayer = false)
        : hitBoxMin(hitBoxMin), hitBoxMax(hitBoxMax), physicsConstructor(physicsConstructor), isPlayer(isPlayer)
    {
    }
    void addPart(Mesh mesh, shared_ptr<Script> script)
    {
        assert(mesh && script);
        parts.push_back(Part(mesh, script));
    }
    void render(Mesh dest, VectorF position, VectorF velocity, float age, unsigned naturalLight, shared_ptr<RenderObjectWorld> world, shared_ptr<Scripting::DataObject> io);
private:
    friend class Client;
    static shared_ptr<RenderObjectEntityMesh> readInternal(Reader &reader, Client &client)
    {
        VectorF hitBoxMin, hitBoxMax;
        hitBoxMin.x = reader.readLimitedF32(-1, 1);
        hitBoxMin.y = reader.readLimitedF32(-1, 1);
        hitBoxMin.z = reader.readLimitedF32(-1, 1);
        hitBoxMax.x = reader.readLimitedF32(-1, 1);
        hitBoxMax.y = reader.readLimitedF32(-1, 1);
        hitBoxMax.z = reader.readLimitedF32(-1, 1);
        uint32_t partCount = reader.readU32();
        bool isPlayer = reader.readBool();
        shared_ptr<PhysicsObjectConstructor> physicsConstructor = PhysicsObjectConstructor::read(reader);
        auto retval = make_shared<RenderObjectEntityMesh>(hitBoxMin, hitBoxMax, physicsConstructor, isPlayer);

        for(uint32_t i = 0; i < partCount; i++)
        {
            Mesh mesh = readMesh(reader, client);
            shared_ptr<Script> script = Script::read(reader);
            retval->parts.push_back(Part(mesh, script));
        }

        return retval;
    }
    void writeInternal(Writer &writer, Client &client)
    {
        writer.writeF32(hitBoxMin.x);
        writer.writeF32(hitBoxMin.y);
        writer.writeF32(hitBoxMin.z);
        writer.writeF32(hitBoxMax.x);
        writer.writeF32(hitBoxMax.y);
        writer.writeF32(hitBoxMax.z);
        writer.writeU32(parts.size());
        writer.writeBool(isPlayer);
        physicsConstructor->write(writer);

        for(auto part : parts)
        {
            writeMesh(part.mesh, writer, client);
            part.script->write(writer);
        }
    }
public:
    static shared_ptr<RenderObjectEntityMesh> readOrNull(Reader &reader, Client &client)
    {
        return client.readObject<RenderObjectEntityMesh>(reader, Client::DataType::RenderObjectEntityMesh);
    }
    void write(Writer &writer, Client &client)
    {
        client.writeObject(writer, shared_from_this(), Client::DataType::RenderObjectEntityMesh);
    }
    BoxRayCollision rayHits(Ray ray, PositionF pos)
    {
        if(ray.position.d != pos.d || hitBoxMin.x >= hitBoxMax.x || hitBoxMin.y >= hitBoxMax.y
                || hitBoxMin.z >= hitBoxMax.z)
        {
            return BoxRayCollision();
        }

        return rayHitBox(hitBoxMin + (VectorF)pos, hitBoxMax + (VectorF)pos, ray);
    }
    shared_ptr<PhysicsObject> constructPhysicsObject(PositionF position, VectorF velocity, shared_ptr<PhysicsWorld> physicsWorld)
    {
        return physicsConstructor->make(position, velocity, physicsWorld);
    }
};

struct RenderObjectEntity final : public RenderObject
{
    virtual Type type() const override
    {
        return Type::Entity;
    }
private:
    shared_ptr<RenderObjectEntityMesh> meshInternal;
public:
    const shared_ptr<RenderObjectEntityMesh> mesh() const
    {
        return meshInternal;
    }
    bool good() const
    {
        return mesh() != nullptr;
    }
    void clear()
    {
        meshInternal = nullptr;
    }
    shared_ptr<PhysicsObject> physicsObject;
    float age, updateAge;
    RenderObjectEntity()
        : meshInternal(nullptr), scriptIOObject(make_shared<Scripting::DataObject>())
    {
    }
    RenderObjectEntity(shared_ptr<RenderObjectEntityMesh> mesh, PositionF position, VectorF velocity, shared_ptr<PhysicsWorld> physicsWorld,
                       float age, shared_ptr<Scripting::DataObject> scriptIOObject = make_shared<Scripting::DataObject>())
        : meshInternal(mesh), physicsObject(mesh->constructPhysicsObject(position, velocity, physicsWorld)), age(age), scriptIOObject(scriptIOObject)
    {
    }
    const shared_ptr<Scripting::DataObject> scriptIOObject;
    friend class RenderObject;
protected:
    static shared_ptr<RenderObjectEntity> read(Reader &reader, Client &client, shared_ptr<PhysicsWorld> physicsWorld)
    {
        Client::IdType id = Client::readIdNonNull(reader);
        shared_ptr<RenderObjectEntity> retval = client.getPtr<RenderObjectEntity>(id, Client::DataType::RenderObjectEntity);

        if(retval == nullptr)
        {
            retval = make_shared<RenderObjectEntity>();
            client.setPtr(retval, id, Client::DataType::RenderObjectEntity);
        }

        retval->meshInternal = RenderObjectEntityMesh::readOrNull(reader, client);

        if(retval->mesh() == nullptr)
        {
            client.removeId(id, Client::DataType::RenderObjectEntity);
            return retval;
        }

        retval->age = reader.readLimitedF32(0, 1e10);
        retval->updateAge = 0;
        PositionF position;
        position.x = reader.readFiniteF32();
        position.y = reader.readFiniteF32();
        position.z = reader.readFiniteF32();
        position.d = reader.readDimension();
        VectorF velocity;
        velocity.x = reader.readFiniteF32();
        velocity.y = reader.readFiniteF32();
        velocity.z = reader.readFiniteF32();
        if(retval->physicsObject == nullptr)
        {
            retval->physicsObject = retval->mesh()->constructPhysicsObject(position, velocity, physicsWorld);
        }
        else
        {
            retval->physicsObject->setCurrentState(position, velocity);
        }
        shared_ptr<Scripting::Data> newData = Scripting::Data::read(reader);
        if(newData->type() != Scripting::Data::Type::Object)
            throw IOException("RenderObjectEntity : script variables object is not an object");
        auto newObj = dynamic_pointer_cast<Scripting::DataObject>(newData);
        assert(newObj);
        for(auto v : newObj->value)
        {
            retval->scriptIOObject->value[get<0>(v)] = get<1>(v);
        }
        return retval;
    }
    virtual void writeInternal(Writer &writer, Client &client) override
    {
        Client::IdType id = client.getId(shared_from_this(), Client::DataType::RenderObjectEntity);

        if(id == Client::NullId)
        {
            id = client.makeId(shared_from_this(), Client::DataType::RenderObjectEntity);
        }

        Client::writeId(writer, id);

        if(mesh() != nullptr)
        {
            mesh()->write(writer, client);
        }
        else
        {
            Client::writeId(writer, Client::NullId);
            client.removeId(id, Client::DataType::RenderObjectEntity);
            return;
        }

        writer.writeF32(age);
        PositionF position = physicsObject->getPosition();
        writer.writeF32(position.x);
        writer.writeF32(position.y);
        writer.writeF32(position.z);
        writer.writeDimension(position.d);
        VectorF velocity = physicsObject->getVelocity();
        writer.writeF32(velocity.x);
        writer.writeF32(velocity.y);
        writer.writeF32(velocity.z);
        scriptIOObject->write(writer);
    }
public:
    virtual void render(Mesh dest, RenderLayer rl, Dimension d, Client & client, unsigned) override;
    void move(float deltaTime, shared_ptr<RenderObjectWorld> world);
    bool isOnGround(shared_ptr<RenderObjectWorld> world);
    virtual bool operator ==(const RenderObject &rt) const override
    {
        return this == &rt;
    }
    virtual BoxRayCollision rayHits(Ray ray) override
    {
        if(!good())
        {
            return BoxRayCollision();
        }

        return mesh()->rayHits(ray, physicsObject->getPosition());
    }
    virtual bool isPlayer() const override
    {
        if(!good())
            return false;
        return mesh()->isPlayer;
    }
};

class RenderObjectBlock;
class RenderObjectBlockMesh;

struct RenderObjectWorld final : public enable_shared_from_this<RenderObjectWorld>
{
    Client &client;
    UpdateList lightingUpdates;
    unordered_set<shared_ptr<RenderObjectEntity>> entities;
    void handleReadEntity(shared_ptr<RenderObjectEntity> e)
    {
        assert(e != nullptr);

        if(e->good())
        {
            entities.insert(e);
        }
        else
        {
            entities.erase(e);
        }
    }
    RenderObjectWorld(Client &client)
        : client(client)
    {
    }
    void addLightingUpdate(PositionI pos)
    {
        lightingUpdates.add(pos);
    }
    struct Chunk final
    {
        static constexpr int log2_size = 3;
        static constexpr int size = 1 << log2_size;
        static constexpr int mod_size_mask = size - 1;
        static constexpr int floor_size_mask = ~mod_size_mask;
        const PositionI pos;
        array<array<array<shared_ptr<RenderObjectBlockMesh>, size>, size>, size> blocksMesh;
        array<array<array<Lighting, size>, size>, size> blocksLighting;
        weak_ptr<Chunk> nx, px, ny, py, nz, pz;
        Mesh cachedMesh[(int)RenderLayer::Last]; /// must be cleared whenever the chunk is modified
        bool cachedMeshValid = false;
        void invalidateMesh()
        {
            if(cachedMeshValid)
            {
                for(Mesh &mesh : cachedMesh)
                {
                    mesh = nullptr;
                }
            }

            cachedMeshValid = false;
        }
        int retrieved = 0;
        Chunk(PositionI pos)
            : pos(pos)
        {
        }
    };
    unordered_map<PositionI, shared_ptr<Chunk>> chunks;
    shared_ptr<Chunk> getChunk(PositionI pos) /// must have the client locked
    {
        shared_ptr<Chunk> &chunk = chunks[pos];

        if(chunk != nullptr)
        {
            return chunk;
        }

        chunk = make_shared<Chunk>(pos);
        shared_ptr<Chunk> nx = chunks[pos - VectorI(Chunk::size, 0, 0)];
        shared_ptr<Chunk> px = chunks[pos + VectorI(Chunk::size, 0, 0)];
        shared_ptr<Chunk> ny = chunks[pos - VectorI(0, Chunk::size, 0)];
        shared_ptr<Chunk> py = chunks[pos + VectorI(0, Chunk::size, 0)];
        shared_ptr<Chunk> nz = chunks[pos - VectorI(0, 0, Chunk::size)];
        shared_ptr<Chunk> pz = chunks[pos + VectorI(0, 0, Chunk::size)];
        chunk->nx = nx;
        chunk->px = px;
        chunk->ny = ny;
        chunk->py = py;
        chunk->nz = nz;
        chunk->pz = pz;

        if(nx != nullptr)
        {
            nx->px = chunk;
        }

        if(px != nullptr)
        {
            px->nx = chunk;
        }

        if(ny != nullptr)
        {
            ny->py = chunk;
        }

        if(py != nullptr)
        {
            py->ny = chunk;
        }

        if(nz != nullptr)
        {
            nz->pz = chunk;
        }

        if(pz != nullptr)
        {
            pz->nz = chunk;
        }

        return chunk;
    }
    class BlockIterator final /// Client must be locked when any member function is called
    {
    private:
        PositionI pos;
        shared_ptr<RenderObjectWorld> world;
        VectorI modPos;
        shared_ptr<Chunk> chunk;
    public:
        PositionI getPosition() const
        {
            return pos;
        }
        shared_ptr<Chunk> getChunk() const
        {
            return chunk;
        }
        BlockIterator(shared_ptr<RenderObjectWorld> world, PositionI pos)
            : pos(pos), world(world), modPos(pos.x & Chunk::mod_size_mask, pos.y & Chunk::mod_size_mask,
                                             pos.z & Chunk::mod_size_mask)
        {
            assert(world != nullptr);
            PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                           pos.z & Chunk::floor_size_mask, pos.d);
            chunk = world->getChunk(chunkPos);
        }
        shared_ptr<RenderObjectBlockMesh> getMesh()
        {
            return chunk->blocksMesh[modPos.x][modPos.y][modPos.z];
        }
        Lighting getLighting()
        {
            return chunk->blocksLighting[modPos.x][modPos.y][modPos.z];
        }
        void invalidate()
        {
            chunk->invalidateMesh();

            if(modPos.x <= 0)
            {
                shared_ptr<Chunk> c = chunk->nx.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }

            if(modPos.x >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->px.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }

            if(modPos.y <= 0)
            {
                shared_ptr<Chunk> c = chunk->ny.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }

            if(modPos.y >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->py.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }

            if(modPos.z <= 0)
            {
                shared_ptr<Chunk> c = chunk->nz.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }

            if(modPos.z >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->pz.lock();

                if(c != nullptr)
                {
                    c->invalidateMesh();
                }
            }
        }
        void setMesh(shared_ptr<RenderObjectBlockMesh> v)
        {
            if(!chunk->blocksMesh[modPos.x][modPos.y][modPos.z])
            {
                chunk->retrieved++;
            }

            chunk->blocksMesh[modPos.x][modPos.y][modPos.z] = v;
            invalidate();
            world->addLightingUpdate(pos);
        }
        void setLighting(Lighting v)
        {
            chunk->blocksLighting[modPos.x][modPos.y][modPos.z] = v;
            invalidate();
        }
        void moveNX()
        {
            pos.x--;

            if(modPos.x <= 0)
            {
                modPos.x = Chunk::size - 1;
                chunk = chunk->nx.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
            {
                modPos.x--;
            }
        }
        void moveNY()
        {
            pos.y--;

            if(modPos.y <= 0)
            {
                modPos.y = Chunk::size - 1;
                chunk = chunk->ny.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
            {
                modPos.y--;
            }
        }
        void moveNZ()
        {
            pos.z--;

            if(modPos.z <= 0)
            {
                modPos.z = Chunk::size - 1;
                chunk = chunk->nz.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
            {
                modPos.z--;
            }
        }
        void movePX()
        {
            pos.x++;

            if(++modPos.x >= Chunk::size)
            {
                modPos.x = 0;
                chunk = chunk->px.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
        }
        void movePY()
        {
            pos.y++;

            if(++modPos.y >= Chunk::size)
            {
                modPos.y = 0;
                chunk = chunk->py.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
        }
        void movePZ()
        {
            pos.z++;

            if(++modPos.z >= Chunk::size)
            {
                modPos.z = 0;
                chunk = chunk->pz.lock();

                if(chunk == nullptr)
                {
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask,
                                                   pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
        }
        BlockIterator getNX() const
        {
            BlockIterator retval = *this;
            retval.moveNX();
            return retval;
        }
        BlockIterator getPX() const
        {
            BlockIterator retval = *this;
            retval.movePX();
            return retval;
        }
        BlockIterator getNY() const
        {
            BlockIterator retval = *this;
            retval.moveNY();
            return retval;
        }
        BlockIterator getPY() const
        {
            BlockIterator retval = *this;
            retval.movePY();
            return retval;
        }
        BlockIterator getNZ() const
        {
            BlockIterator retval = *this;
            retval.moveNZ();
            return retval;
        }
        BlockIterator getPZ() const
        {
            BlockIterator retval = *this;
            retval.movePZ();
            return retval;
        }
    };
    BlockIterator get(PositionI pos)
    {
        return BlockIterator(shared_from_this(), pos);
    }
    static shared_ptr<RenderObjectWorld> getWorld(Client &client)
    {
        static Client::IdType worldId = Client::NullId;
        LockedClient lock(client);

        if(worldId == Client::NullId)
        {
            shared_ptr<RenderObjectWorld> retval = shared_ptr<RenderObjectWorld>(new RenderObjectWorld(client));
            worldId = client.makeId(retval, Client::DataType::RenderObjectWorld);
            return retval;
        }

        shared_ptr<RenderObjectWorld> retval = client.getPtr<RenderObjectWorld>(worldId,
                                               Client::DataType::RenderObjectWorld);

        if(retval != nullptr)
        {
            return retval;
        }

        retval = shared_ptr<RenderObjectWorld>(new RenderObjectWorld(client));
        client.setPtr(retval, worldId, Client::DataType::RenderObjectWorld);
        return retval;
    }
    shared_ptr<PositionI> getFirstHitBlock(Ray ray, float maxT);
    static void calcLight(float & lNXNYNZ, float & lNXNYPZ, float & lNXPYNZ, float & lNXPYPZ, float & lPXNYNZ, float & lPXNYPZ, float & lPXPYNZ, float & lPXPYPZ, BlockIterator bi, unsigned naturalLight);
};

typedef uint16_t RenderObjectBlockClass;

inline RenderObjectBlockClass readRenderObjectBlockClass(Reader &reader)
{
    return reader.readU16();
}

inline void writeRenderObjectBlockClass(Writer &writer, RenderObjectBlockClass v)
{
    writer.writeU16(v);
}

inline RenderObjectBlockClass getRenderObjectBlockClass()
{
    static atomic_uint_fast16_t next(0);
    return next++;
}

class RenderObjectBlockMesh final : public enable_shared_from_this<RenderObjectBlockMesh>
{
public:
    const RenderObjectBlockClass blockClass;
private:
    Mesh center, nx, px, ny, py, nz, pz;
    shared_ptr<RenderObjectWorld> world;
    VectorF hitBoxMin, hitBoxMax;
    shared_ptr<PhysicsObjectConstructor> physicsConstructor;
public:
    const bool nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked;
    const LightProperties lightProperties;
    const RenderLayer rl;
    void render(Mesh dest, RenderLayer rl, RenderObjectWorld::BlockIterator bi, unsigned naturalLight);
    void render(Mesh dest, RenderLayer rl, PositionI pos, Client &client, unsigned naturalLight)
    {
        if(rl != this->rl)
        {
            return;
        }

        LockedClient lock(client);
        shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
        RenderObjectWorld::BlockIterator bi(world, pos);
        render(dest, rl, bi, naturalLight);
    }
    RenderObjectBlockMesh(RenderObjectBlockClass blockClass, VectorF hitBoxMin, VectorF hitBoxMax,
                          LightProperties lightProperties, Mesh center, Mesh nx, Mesh px, Mesh ny, Mesh py, Mesh nz, Mesh pz,
                          bool nxBlocked, bool pxBlocked, bool nyBlocked, bool pyBlocked, bool nzBlocked, bool pzBlocked,
                          RenderLayer rl, shared_ptr<PhysicsObjectConstructor> physicsConstructor)
        : blockClass(blockClass), center(center), nx(nx), px(px), ny(ny), py(py), nz(nz), pz(pz),
          hitBoxMin(hitBoxMin), hitBoxMax(hitBoxMax), physicsConstructor(physicsConstructor), nxBlocked(nxBlocked), pxBlocked(pxBlocked),
          nyBlocked(nyBlocked), pyBlocked(pyBlocked), nzBlocked(nzBlocked), pzBlocked(pzBlocked),
          lightProperties(lightProperties), rl(rl)
    {
    }
    static shared_ptr<RenderObjectBlockMesh> read(Reader &reader, Client &client);
    void write(Writer &writer, Client &client);
    BoxRayCollision rayHits(Ray ray, PositionI pos)
    {
        if(ray.position.d != pos.d || hitBoxMin.x >= hitBoxMax.x || hitBoxMin.y >= hitBoxMax.y
                || hitBoxMin.z >= hitBoxMax.z)
        {
            return BoxRayCollision();
        }

        return rayHitBox(hitBoxMin + (VectorI)pos, hitBoxMax + (VectorI)pos, ray);
    }
    shared_ptr<PhysicsObject> constructPhysicsObject(PositionI position, shared_ptr<PhysicsWorld> physicsWorld)
    {
        return physicsConstructor->make((PositionF)position, VectorF(0), physicsWorld);
    }
};

class RenderObjectBlock final : public RenderObject
{
private:
    shared_ptr<RenderObjectBlockMesh> block;
public:
    const PositionI pos;
    Lighting lighting;
    RenderObjectBlock(shared_ptr<RenderObjectBlockMesh> block, PositionI pos, Lighting lighting)
        : block(block), pos(pos), lighting(lighting)
    {
    }
    virtual Type type() const override
    {
        return Type::Block;
    }
protected:
    virtual void writeInternal(Writer &writer, Client &client) override
    {
        block->write(writer, client);
        writer.writeS32(pos.x);
        writer.writeS32(pos.y);
        writer.writeS32(pos.z);
        writer.writeDimension(pos.d);
        lighting.write(writer);
    }
public:
    static shared_ptr<RenderObjectBlock> read(Reader &reader, Client &client);
    void render(Mesh dest, RenderLayer rl, Dimension d, Client &client, unsigned naturalLight) override
    {
        if(d == pos.d)
        {
            block->render(dest, rl, pos, client, naturalLight);
        }
    }
    virtual bool operator ==(const RenderObject &rt) const override
    {
        if(rt.type() != type())
        {
            return false;
        }

        const RenderObjectBlock &b = (const RenderObjectBlock &)rt;

        if(block == b.block && pos == b.pos)
        {
            return true;
        }

        return false;
    }
    shared_ptr<RenderObjectBlockMesh> getBlockMesh()
    {
        return block;
    }
    void addToClient(Client &client)
    {
        LockedClient lock(client);
        shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
        RenderObjectWorld::BlockIterator bi(world, pos);
        bi.setMesh(block);
        bi.setLighting(lighting);
    }
    virtual BoxRayCollision rayHits(Ray ray) override
    {
        if(ray.position.d != pos.d)
        {
            return BoxRayCollision();
        }

        return block->rayHits(ray, pos);
    }
    shared_ptr<PhysicsObject> constructPhysicsObject(shared_ptr<PhysicsWorld> physicsWorld)
    {
        return block->constructPhysicsObject(pos, VectorF(0), physicsWorld);
    }
};

inline void RenderObjectBlockMesh::render(Mesh dest2, RenderLayer rl,
        RenderObjectWorld::BlockIterator bi, unsigned naturalLight)
{
    //TODO(#jacob): add lighting
    if(rl != this->rl)
    {
        return;
    }

    Mesh dest = Mesh(new Mesh_t);

    Matrix tform = Matrix::translate(bi.getPosition());
    RenderObjectWorld::BlockIterator bNX = bi.getNX();
    RenderObjectWorld::BlockIterator bPX = bi.getPX();
    RenderObjectWorld::BlockIterator bNY = bi.getNY();
    RenderObjectWorld::BlockIterator bPY = bi.getPY();
    RenderObjectWorld::BlockIterator bNZ = bi.getNZ();
    RenderObjectWorld::BlockIterator bPZ = bi.getPZ();
    shared_ptr<RenderObjectBlockMesh> nxMesh = bNX.getMesh();
    shared_ptr<RenderObjectBlockMesh> pxMesh = bPX.getMesh();
    shared_ptr<RenderObjectBlockMesh> nyMesh = bNY.getMesh();
    shared_ptr<RenderObjectBlockMesh> pyMesh = bPY.getMesh();
    shared_ptr<RenderObjectBlockMesh> nzMesh = bNZ.getMesh();
    shared_ptr<RenderObjectBlockMesh> pzMesh = bPZ.getMesh();
    float centerLight = bi.getLighting().calcLight(naturalLight);
    float nxLight = 0;
    float pxLight = 0;
    float nyLight = 0;
    float pyLight = 0;
    float nzLight = 0;
    float pzLight = 0;

    if(nxMesh != nullptr)
    {
        nxLight = max<float>(bNX.getLighting().calcLight(naturalLight), centerLight);

        if(!nxMesh->pxBlocked && nxMesh->blockClass != blockClass)
        {
            nxLight /= Lighting::MAX_INTENSITY;
            nxLight = interpolate(nxLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(nxLight), transform(tform, nx)));
        }
    }

    if(pxMesh != nullptr)
    {
        pxLight = max<float>(bPX.getLighting().calcLight(naturalLight), centerLight);

        if(!pxMesh->nxBlocked && pxMesh->blockClass != blockClass)
        {
            pxLight /= Lighting::MAX_INTENSITY;
            pxLight = interpolate(pxLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(pxLight), transform(tform, px)));
        }
    }

    if(nyMesh != nullptr)
    {
        nyLight = max<float>(bNY.getLighting().calcLight(naturalLight), centerLight);

        if(!nyMesh->pyBlocked && nyMesh->blockClass != blockClass)
        {
            nyLight /= Lighting::MAX_INTENSITY;
            nyLight = interpolate(nyLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(nyLight), transform(tform, ny)));
        }
    }

    if(pyMesh != nullptr)
    {
        pyLight = max<float>(bPY.getLighting().calcLight(naturalLight), centerLight);

        if(!pyMesh->nyBlocked && pyMesh->blockClass != blockClass)
        {
            pyLight /= Lighting::MAX_INTENSITY;
            pyLight = interpolate(pyLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(pyLight), transform(tform, py)));
        }
    }

    if(nzMesh != nullptr)
    {
        nzLight = max<float>(bNZ.getLighting().calcLight(naturalLight), centerLight);

        if(!nzMesh->pzBlocked && nzMesh->blockClass != blockClass)
        {
            nzLight /= Lighting::MAX_INTENSITY;
            nzLight = interpolate(nzLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(nzLight), transform(tform, nz)));
        }
    }

    if(pzMesh != nullptr)
    {
        pzLight = max<float>(bPZ.getLighting().calcLight(naturalLight), centerLight);

        if(!pzMesh->nzBlocked && pzMesh->blockClass != blockClass)
        {
            pzLight /= Lighting::MAX_INTENSITY;
            pzLight = interpolate(pzLight, getZeroBrightnessLevel(bi.getPosition().d), 1.0f);
            dest->add(scaleColors(Color(pzLight), transform(tform, pz)));
        }
    }

    float lNXNYNZ = max(nxLight, max(nyLight, nzLight));
    float lNXNYPZ = max(nxLight, max(nyLight, pzLight));
    float lNXPYNZ = max(nxLight, max(pyLight, nzLight));
    float lNXPYPZ = max(nxLight, max(pyLight, pzLight));
    float lPXNYNZ = max(pxLight, max(nyLight, nzLight));
    float lPXNYPZ = max(pxLight, max(nyLight, pzLight));
    float lPXPYNZ = max(pxLight, max(pyLight, nzLight));
    float lPXPYPZ = max(pxLight, max(pyLight, pzLight));
    dest->add(transform(tform, interpolateColors(center, Color(lNXNYNZ), Color(lNXNYPZ), Color(lNXPYNZ), Color(lNXPYPZ), Color(lPXNYNZ), Color(lPXNYPZ), Color(lPXPYNZ), Color(lPXPYPZ))));
    dest2->add(lightColors(dest, VectorF(0, 1, 0), 0.7f, 0.3f));
}

inline shared_ptr<PositionI> RenderObjectWorld::getFirstHitBlock(Ray ray, float maxT)
{
    PositionI retval = (PositionI)ray.position;
    BlockIterator bi(shared_from_this(), retval);

    if(bi.getMesh() != nullptr)
    {
        BoxRayCollision brc = bi.getMesh()->rayHits(ray, retval);

        if(brc.good() && brc.t <= maxT)
        {
            return make_shared<PositionI>(retval);
        }
    }
    else
    {
        return nullptr;
    }

    bool useX = (abs(ray.direction.x) >= eps);
    bool useY = (abs(ray.direction.y) >= eps);
    bool useZ = (abs(ray.direction.z) >= eps);
    assert(useX || useY || useZ);
    VectorF invDir = VectorF(0);
    VectorF next, step;
    PositionF currentPos = ray.position;
    VectorI dest, delta;

    if(useX)
    {
        invDir.x = 1 / ray.direction.x;
        step.x = abs(invDir.x);
        int target;

        if(ray.direction.x < 0)
        {
            target = iceil(currentPos.x) - 1;
            delta.x = -1;
        }
        else
        {
            delta.x = 1;
            target = ifloor(currentPos.x) + 1;
        }

        dest.x = target;

        if(ray.direction.x < 0)
        {
            dest.x--;
        }

        next.x = (target - ray.position.x) * invDir.x;
    }

    if(useY)
    {
        invDir.y = 1 / ray.direction.y;
        step.y = fabs(invDir.y);
        int target;

        if(ray.direction.y < 0)
        {
            target = iceil(currentPos.y) - 1;
            delta.y = -1;
        }
        else
        {
            delta.y = 1;
            target = ifloor(currentPos.y) + 1;
        }

        dest.y = target;

        if(ray.direction.y < 0)
        {
            dest.y--;
        }

        next.y = (target - ray.position.y) * invDir.y;
    }

    if(useZ)
    {
        invDir.z = 1 / ray.direction.z;
        step.z = fabs(invDir.z);
        int target;

        if(ray.direction.z < 0)
        {
            target = iceil(currentPos.z) - 1;
            delta.z = -1;
        }
        else
        {
            delta.z = 1;
            target = ifloor(currentPos.z) + 1;
        }

        dest.z = target;

        if(ray.direction.z < 0)
        {
            dest.z--;
        }

        next.z = (target - ray.position.z) * invDir.z;
    }

    while(true)
    {
        float t;

        if(useX && (!useY || next.x < next.y) && (!useZ || next.x < next.z))
        {
            t = next.x;
            next.x += step.x;

            if(delta.x > 0)
            {
                bi.movePX();
            }
            else
            {
                bi.moveNX();
            }

            dest.x += delta.x;
        }
        else if(useY && (!useZ || next.y < next.z))
        {
            t = next.y;
            next.y += step.y;

            if(delta.y > 0)
            {
                bi.movePY();
            }
            else
            {
                bi.moveNY();
            }

            dest.y += delta.y;
        }
        else // if(useZ) // useZ must be true
        {
            t = next.z;
            next.z += step.z;

            if(delta.z > 0)
            {
                bi.movePZ();
            }
            else
            {
                bi.moveNZ();
            }

            dest.z += delta.z;
        }

        if(t > maxT)
        {
            return nullptr;
        }

        if(bi.getMesh() != nullptr)
        {
            BoxRayCollision brc = bi.getMesh()->rayHits(ray, retval);

            if(brc.good() && brc.t <= maxT)
            {
                return make_shared<PositionI>(bi.getPosition());
            }
        }
        else
        {
            return nullptr;
        }
    }
}

inline void RenderObjectEntityMesh::render(Mesh dest, VectorF position, VectorF velocity, float age, unsigned, shared_ptr<RenderObjectWorld>, shared_ptr<Scripting::DataObject> io)
{
    for(Part part : parts)
    {
        runEntityPartScript(dest, part.mesh, part.script, position, velocity, age, io);
    }
}

inline void RenderObjectWorld::calcLight(float & lNXNYNZ, float & lNXNYPZ, float & lNXPYNZ, float & lNXPYPZ, float & lPXNYNZ, float & lPXNYPZ, float & lPXPYNZ, float & lPXPYPZ, RenderObjectWorld::BlockIterator bi, unsigned naturalLight)
{
    RenderObjectWorld::BlockIterator bNX = bi.getNX();
    RenderObjectWorld::BlockIterator bPX = bi.getPX();
    RenderObjectWorld::BlockIterator bNY = bi.getNY();
    RenderObjectWorld::BlockIterator bPY = bi.getPY();
    RenderObjectWorld::BlockIterator bNZ = bi.getNZ();
    RenderObjectWorld::BlockIterator bPZ = bi.getPZ();
    shared_ptr<RenderObjectBlockMesh> nxMesh = bNX.getMesh();
    shared_ptr<RenderObjectBlockMesh> pxMesh = bPX.getMesh();
    shared_ptr<RenderObjectBlockMesh> nyMesh = bNY.getMesh();
    shared_ptr<RenderObjectBlockMesh> pyMesh = bPY.getMesh();
    shared_ptr<RenderObjectBlockMesh> nzMesh = bNZ.getMesh();
    shared_ptr<RenderObjectBlockMesh> pzMesh = bPZ.getMesh();
    float centerLight = bi.getLighting().calcLight(naturalLight);
    float nxLight = 0;
    float pxLight = 0;
    float nyLight = 0;
    float pyLight = 0;
    float nzLight = 0;
    float pzLight = 0;

    if(nxMesh != nullptr)
    {
        nxLight = max<float>(bNX.getLighting().calcLight(naturalLight), centerLight);
    }

    if(pxMesh != nullptr)
    {
        pxLight = max<float>(bPX.getLighting().calcLight(naturalLight), centerLight);
    }

    if(nyMesh != nullptr)
    {
        nyLight = max<float>(bNY.getLighting().calcLight(naturalLight), centerLight);
    }

    if(pyMesh != nullptr)
    {
        pyLight = max<float>(bPY.getLighting().calcLight(naturalLight), centerLight);
    }

    if(nzMesh != nullptr)
    {
        nzLight = max<float>(bNZ.getLighting().calcLight(naturalLight), centerLight);
    }

    if(pzMesh != nullptr)
    {
        pzLight = max<float>(bPZ.getLighting().calcLight(naturalLight), centerLight);
    }

    lNXNYNZ = max(nxLight, max(nyLight, nzLight));
    lNXNYPZ = max(nxLight, max(nyLight, pzLight));
    lNXPYNZ = max(nxLight, max(pyLight, nzLight));
    lNXPYPZ = max(nxLight, max(pyLight, pzLight));
    lPXNYNZ = max(pxLight, max(nyLight, nzLight));
    lPXNYPZ = max(pxLight, max(nyLight, pzLight));
    lPXPYNZ = max(pxLight, max(pyLight, nzLight));
    lPXPYPZ = max(pxLight, max(pyLight, pzLight));
}

inline void RenderObjectEntity::render(Mesh dest, RenderLayer rl, Dimension d, Client & client, unsigned naturalLight)
{
    if(!good() || rl != RenderLayer::Opaque)
    {
        return;
    }
    PositionF position = physicsObject->getPosition());
    if(d != position.d)
    {
        return;
    }

    shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);

    Mesh temp(new Mesh_t);
    VectorF velocity = physicsObject->getVelocity();

    mesh()->render(temp, (VectorF)position, velocity, age, naturalLight, world, scriptIOObject);
    vector<Triangle> triangles;
    triangles.reserve(temp->size());
    const int arraySize = 2;
    PositionI lOrigin = (PositionI)(position + VectorF(0.5f)) - VectorI(arraySize / 2);
    array<array<array<float, arraySize>, arraySize>, arraySize> lNXNYNZ, lNXNYPZ, lNXPYNZ, lNXPYPZ, lPXNYNZ, lPXNYPZ, lPXPYNZ, lPXPYPZ;

    for(int dx = 0; dx < arraySize; dx++)
    {
        for(int dy = 0; dy < arraySize; dy++)
        {
            for(int dz = 0; dz < arraySize; dz++)
            {
                RenderObjectWorld::calcLight(lNXNYNZ[dx][dy][dz], lNXNYPZ[dx][dy][dz], lNXPYNZ[dx][dy][dz], lNXPYPZ[dx][dy][dz], lPXNYNZ[dx][dy][dz], lPXNYPZ[dx][dy][dz], lPXPYNZ[dx][dy][dz], lPXPYPZ[dx][dy][dz], RenderObjectWorld::BlockIterator(world, lOrigin + VectorI(dx, dy, dz)), naturalLight);
            }
        }
    }
    for(Triangle t : *temp)
    {
        for(size_t i = 0; i < 3; i++)
        {
            VectorF p = t.p[i];
            float nxnynz;
            float nxnypz;
            float nxpynz;
            float nxpypz;
            float pxnynz;
            float pxnypz;
            float pxpynz;
            float pxpypz;
            VectorI ipos = (VectorI)p;
            PositionI idpos = PositionI(ipos - (VectorI)lOrigin, lOrigin.d);
            if(idpos.x < 0 || idpos.x >= arraySize || idpos.y < 0 || idpos.y >= arraySize || idpos.z < 0 || idpos.z >= arraySize)
            {
                RenderObjectWorld::calcLight(nxnynz, nxnypz, nxpynz, nxpypz, pxnynz, pxnypz, pxpynz, pxpypz, RenderObjectWorld::BlockIterator(world, PositionI(ipos, lOrigin.d)), naturalLight);
            }
            else
            {
                nxnynz = lNXNYNZ[idpos.x][idpos.y][idpos.z];
                nxnypz = lNXNYPZ[idpos.x][idpos.y][idpos.z];
                nxpynz = lNXPYNZ[idpos.x][idpos.y][idpos.z];
                nxpypz = lNXPYPZ[idpos.x][idpos.y][idpos.z];
                pxnynz = lPXNYNZ[idpos.x][idpos.y][idpos.z];
                pxnypz = lPXNYPZ[idpos.x][idpos.y][idpos.z];
                pxpynz = lPXPYNZ[idpos.x][idpos.y][idpos.z];
                pxpypz = lPXPYPZ[idpos.x][idpos.y][idpos.z];
            }
            p -= (VectorF)ipos;
            float c = interpolate(p.x, interpolate(p.y, interpolate(p.z, nxnynz, nxnypz), interpolate(p.z, nxpynz, nxpypz)), interpolate(p.y, interpolate(p.z, pxnynz, pxnypz), interpolate(p.z, pxpynz, pxpypz)));
            t.c[i] = scale(t.c[i], c);
        }
        triangles.push_back(t);
    }
    dest->add(lightColors(Mesh(new Mesh_t(temp->texture(), triangles)), VectorF(0, 1, 0), 0.7f, 0.3f));
}

#endif // RENDER_OBJECT_H_INCLUDED

