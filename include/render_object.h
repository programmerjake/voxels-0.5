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
    virtual void render(Mesh dest, RenderLayer rl, Dimension d, Client &client) = 0;
    virtual BoxRayCollision rayHits(Ray ray) = 0;
};

#if 0 //TODO(jacob#):finish
class RenderObjectEntityMesh final : public enable_shared_from_this<RenderObjectEntityMesh>
{
private:
    Mesh mesh;
    shared_ptr<RenderObjectWorld> world;
};

class RenderObjectEntity final : public RenderObject
{

};
#endif

class RenderObjectBlock;
class RenderObjectBlockMesh;

struct RenderObjectWorld final : public enable_shared_from_this<RenderObjectWorld>
{
    Client & client;
    UpdateList lightingUpdates;
    RenderObjectWorld(Client & client)
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
                for(Mesh & mesh : cachedMesh)
                    mesh = nullptr;
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
        shared_ptr<Chunk> & chunk = chunks[pos];
        if(chunk != nullptr)
            return chunk;
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
        if(nx != nullptr) nx->px = chunk;
        if(px != nullptr) px->nx = chunk;
        if(ny != nullptr) ny->py = chunk;
        if(py != nullptr) py->ny = chunk;
        if(nz != nullptr) nz->pz = chunk;
        if(pz != nullptr) pz->nz = chunk;
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
            : pos(pos), world(world), modPos(pos.x & Chunk::mod_size_mask, pos.y & Chunk::mod_size_mask, pos.z & Chunk::mod_size_mask)
        {
            assert(world != nullptr);
            PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
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
                    c->invalidateMesh();
            }
            if(modPos.x >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->px.lock();
                if(c != nullptr)
                    c->invalidateMesh();
            }
            if(modPos.y <= 0)
            {
                shared_ptr<Chunk> c = chunk->ny.lock();
                if(c != nullptr)
                    c->invalidateMesh();
            }
            if(modPos.y >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->py.lock();
                if(c != nullptr)
                    c->invalidateMesh();
            }
            if(modPos.z <= 0)
            {
                shared_ptr<Chunk> c = chunk->nz.lock();
                if(c != nullptr)
                    c->invalidateMesh();
            }
            if(modPos.z >= Chunk::size - 1)
            {
                shared_ptr<Chunk> c = chunk->pz.lock();
                if(c != nullptr)
                    c->invalidateMesh();
            }
        }
        void setMesh(shared_ptr<RenderObjectBlockMesh> v)
        {
            if(!chunk->blocksMesh[modPos.x][modPos.y][modPos.z])
                chunk->retrieved++;
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
                modPos.x--;
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
                modPos.y--;
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
                    chunk = world->getChunk(chunkPos);
                }
            }
            else
                modPos.z--;
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
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
                    PositionI chunkPos = PositionI(pos.x & Chunk::floor_size_mask, pos.y & Chunk::floor_size_mask, pos.z & Chunk::floor_size_mask, pos.d);
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
    //FIXME(#jacob): add entities
    //list<shared_ptr<RenderObjectEntity>> entities;
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
        shared_ptr<RenderObjectWorld> retval = client.getPtr<RenderObjectWorld>(worldId, Client::DataType::RenderObjectWorld);
        if(retval != nullptr)
        {
            return retval;
        }
        retval = shared_ptr<RenderObjectWorld>(new RenderObjectWorld(client));
        client.setPtr(retval, worldId, Client::DataType::RenderObjectWorld);
        return retval;
    }
    BlockIterator getFirstHitBlock(Ray ray, float maxT);
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
public:
    const bool nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked;
    const LightProperties lightProperties;
    const RenderLayer rl;
    void render(Mesh dest, RenderLayer rl, RenderObjectWorld::BlockIterator bi);
    void render(Mesh dest, RenderLayer rl, PositionI pos, Client &client)
    {
        if(rl != this->rl)
        {
            return;
        }
        LockedClient lock(client);
        shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
        RenderObjectWorld::BlockIterator bi(world, pos);
        render(dest, rl, bi);
    }
    RenderObjectBlockMesh(RenderObjectBlockClass blockClass, VectorF hitBoxMin, hitBoxMax, LightProperties lightProperties, Mesh center, Mesh nx, Mesh px, Mesh ny, Mesh py, Mesh nz, Mesh pz, bool nxBlocked, bool pxBlocked, bool nyBlocked, bool pyBlocked, bool nzBlocked, bool pzBlocked, RenderLayer rl)
        : blockClass(blockClass), center(center), nx(nx), px(px), ny(ny), py(py), nz(nz), pz(pz), hitBoxMin(hitBoxMin), hitBoxMax(hitBoxMax), nxBlocked(nxBlocked), pxBlocked(pxBlocked), nyBlocked(nyBlocked), pyBlocked(pyBlocked), nzBlocked(nzBlocked), pzBlocked(pzBlocked), lightProperties(lightProperties), rl(rl)
    {
    }
    static shared_ptr<RenderObjectBlockMesh> read(Reader &reader, Client &client);
    void write(Writer &writer, Client &client);
    BoxRayCollision rayHits(Ray ray, PositionI pos)
    {
        if(ray.position.d != pos.d || hitBoxMin == hitBoxMax)
            return BoxRayCollision();
        return rayHitBox(hitBoxMin + (VectorI)pos, hitBoxMax + (VectorI)pos, ray);
    }
};

class RenderObjectBlock final : public RenderObject
{
private:
    shared_ptr<RenderObjectBlockMesh> block;
public:
    const PositionI pos;
    RenderObjectBlock(shared_ptr<RenderObjectBlockMesh> block, PositionI pos)
        : block(block), pos(pos)
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
    }
public:
    static shared_ptr<RenderObjectBlock> read(Reader &reader, Client &client);
    void render(Mesh dest, RenderLayer rl, Dimension d, Client &client) override
    {
        if(d == pos.d)
            block->render(dest, rl, pos, client);
    }
    virtual bool operator ==(const RenderObject &rt) const override
    {
        if(rt.type() != type())
            return false;
        const RenderObjectBlock & b = (const RenderObjectBlock &)rt;
        if(block == b.block && pos == b.pos)
            return true;
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
    }
    virtual BoxRayCollision rayHits(Ray ray) override
    {
        if(ray.position.d != pos.d)
            return BoxRayCollision();
        return block->rayHits(ray, pos);
    }
};

inline void RenderObjectBlockMesh::render(Mesh dest, RenderLayer rl, RenderObjectWorld::BlockIterator bi)
{
    //TODO(#jacob): add lighting
    if(rl != this->rl)
    {
        return;
    }
    Matrix tform = Matrix::translate(bi.getPosition());
    shared_ptr<RenderObjectBlockMesh> nxMesh = bi.getNX().getMesh();
    shared_ptr<RenderObjectBlockMesh> pxMesh = bi.getPX().getMesh();
    shared_ptr<RenderObjectBlockMesh> nyMesh = bi.getNY().getMesh();
    shared_ptr<RenderObjectBlockMesh> pyMesh = bi.getPY().getMesh();
    shared_ptr<RenderObjectBlockMesh> nzMesh = bi.getNZ().getMesh();
    shared_ptr<RenderObjectBlockMesh> pzMesh = bi.getPZ().getMesh();
    if(nxMesh != nullptr && !nxMesh->pxBlocked && nxMesh->blockClass != blockClass)
        dest->add(transform(tform, nx));
    if(pxMesh != nullptr && !pxMesh->nxBlocked && pxMesh->blockClass != blockClass)
        dest->add(transform(tform, px));
    if(nyMesh != nullptr && !nyMesh->pyBlocked && nyMesh->blockClass != blockClass)
        dest->add(transform(tform, ny));
    if(pyMesh != nullptr && !pyMesh->nyBlocked && pyMesh->blockClass != blockClass)
        dest->add(transform(tform, py));
    if(nzMesh != nullptr && !nzMesh->pzBlocked && nzMesh->blockClass != blockClass)
        dest->add(transform(tform, nz));
    if(pzMesh != nullptr && !pzMesh->nzBlocked && pzMesh->blockClass != blockClass)
        dest->add(transform(tform, pz));
    dest->add(transform(tform, center));
}

inline shared_ptr<PositionI> RenderObjectWorld::getFirstHitBlock(Ray ray, float maxT)
{
    PositionI retval = (PositionI)ray.position;
    BlockIterator bi(shared_from_this(), retval);

}

#endif // RENDER_OBJECT_H_INCLUDED
