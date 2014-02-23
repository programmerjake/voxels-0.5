#ifndef RENDER_OBJECT_H_INCLUDED
#define RENDER_OBJECT_H_INCLUDED

#include "mesh.h"
#include "position.h"
#include "render_layer.h"
#include "stream.h"
#include "client.h"
#include "block_face.h"
#include <atomic>
#include <unordered_map>

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
    void write(Writer &writer, Client &client)
    {
        writer.writeU8((uint8_t)type());
        writeInternal(writer, client);
    }
    static shared_ptr<RenderObject> read(Reader &reader, Client &client);
    virtual bool operator ==(const RenderObject &rt) const = 0;
    virtual void render(Mesh dest, RenderLayer rl, Dimension d, Client &client) = 0;
};

class RenderObjectBlock;

struct RenderObjectWorld final
{
    unordered_map<PositionI, shared_ptr<RenderObjectBlock>> blocks;
    static shared_ptr<RenderObjectWorld> getWorld(Client &client)
    {
        static Client::IdType worldId = Client::NullId;
        LockedClient lock(client);
        if(worldId == Client::NullId)
        {
            shared_ptr<RenderObjectWorld> retval = shared_ptr<RenderObjectWorld>(new RenderObjectWorld);
            worldId = client.makeId(retval, Client::DataType::RenderObjectWorld);
            return retval;
        }
        shared_ptr<RenderObjectWorld> retval = client.getPtr<RenderObjectWorld>(worldId, Client::DataType::RenderObjectWorld);
        if(retval != nullptr)
        {
            return retval;
        }
        retval = shared_ptr<RenderObjectWorld>(new RenderObjectWorld);
        client.setPtr(retval, worldId, Client::DataType::RenderObjectWorld);
        return retval;
    }
};

class RenderObjectBlockMesh final : public enable_shared_from_this<RenderObjectBlockMesh>
{
private:
    Mesh center, nx, px, ny, py, nz, pz;
    shared_ptr<RenderObjectWorld> world;
public:
    const bool nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked;
    const RenderLayer rl;
    void render(Mesh dest, RenderLayer rl, PositionI pos, Client &client);
    RenderObjectBlockMesh(Mesh center, Mesh nx, Mesh px, Mesh ny, Mesh py, Mesh nz, Mesh pz, bool nxBlocked, bool pxBlocked, bool nyBlocked, bool pyBlocked, bool nzBlocked, bool pzBlocked, RenderLayer rl)
        : center(center), nx(nx), px(px), ny(ny), py(py), nz(nz), pz(pz), nxBlocked(nxBlocked), pxBlocked(pxBlocked), nyBlocked(nyBlocked), pyBlocked(pyBlocked), nzBlocked(nzBlocked), pzBlocked(pzBlocked), rl(rl)
    {
    }
    static shared_ptr<RenderObjectBlockMesh> read(Reader &reader, Client &client)
    {
        Client::IdType id = Client::readIdNonNull(reader);
        shared_ptr<RenderObjectBlockMesh> retval = client.getPtr<RenderObjectBlockMesh>(id, Client::DataType::RenderObjectBlockMesh);
        if(retval != nullptr)
        {
            return retval;
        }
        Mesh center, nx, px, ny, py, nz, pz;
        center = readMesh(reader, client);
        nx = readMesh(reader, client);
        px = readMesh(reader, client);
        ny = readMesh(reader, client);
        py = readMesh(reader, client);
        nz = readMesh(reader, client);
        pz = readMesh(reader, client);
        RenderLayer rl = readRenderLayer(reader);
        uint8_t blockedMask = reader.readU8();
        bool nxBlocked = blockedMask & (1 << (int)BlockFace::NX);
        bool pxBlocked = blockedMask & (1 << (int)BlockFace::PX);
        bool nyBlocked = blockedMask & (1 << (int)BlockFace::NY);
        bool pyBlocked = blockedMask & (1 << (int)BlockFace::PY);
        bool nzBlocked = blockedMask & (1 << (int)BlockFace::NZ);
        bool pzBlocked = blockedMask & (1 << (int)BlockFace::PZ);
        retval = shared_ptr<RenderObjectBlockMesh>(new RenderObjectBlockMesh(center, nx, px, ny, py, nz, pz, nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked, rl));
        client.setPtr(retval, id, Client::DataType::RenderObjectBlockMesh);
        return retval;
    }
    void write(Writer &writer, Client &client)
    {
        Client::IdType id = client.getId(shared_from_this(), Client::DataType::RenderObjectBlockMesh);
        if(id != Client::NullId)
        {
            Client::writeId(writer, id);
            return;
        }
        id = client.makeId(shared_from_this(), Client::DataType::RenderObjectBlockMesh);
        Client::writeId(writer, id);
        writeMesh(center, writer, client);
        writeMesh(nx, writer, client);
        writeMesh(px, writer, client);
        writeMesh(ny, writer, client);
        writeMesh(py, writer, client);
        writeMesh(nz, writer, client);
        writeMesh(pz, writer, client);
        writeRenderLayer(rl, writer);
        uint8_t blockedMask = 0;
        if(nxBlocked)
            blockedMask |= 1 << (int)BlockFace::NX;
        if(pxBlocked)
            blockedMask |= 1 << (int)BlockFace::PX;
        if(nyBlocked)
            blockedMask |= 1 << (int)BlockFace::NY;
        if(pyBlocked)
            blockedMask |= 1 << (int)BlockFace::PY;
        if(nzBlocked)
            blockedMask |= 1 << (int)BlockFace::NZ;
        if(pzBlocked)
            blockedMask |= 1 << (int)BlockFace::PZ;
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
    static shared_ptr<RenderObjectBlock> read(Reader &reader, Client &client)
    {
        shared_ptr<RenderObjectBlockMesh> block = RenderObjectBlockMesh::read(reader, client);
        PositionI pos;
        pos.x = reader.readS32();
        pos.y = reader.readS32();
        pos.z = reader.readS32();
        pos.d = reader.readDimension();
        auto retval = shared_ptr<RenderObjectBlock>(new RenderObjectBlock(block, pos));
        shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
        world->blocks[pos] = retval;
        return retval;
    }
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
        shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
        world->blocks[pos] = shared_from_this();
    }
};

inline void RenderObjectBlockMesh::render(Mesh dest, RenderLayer rl, PositionI pos, Client &client)
{
    if(rl != this->rl)
    {
        return;
    }
    shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
    shared_ptr<RenderObjectBlock> nxBlock = world->blocks[pos + VectorI(-1, 0, 0)];
    shared_ptr<RenderObjectBlock> pxBlock = world->blocks[pos + VectorI(1, 0, 0)];
    shared_ptr<RenderObjectBlock> nyBlock = world->blocks[pos + VectorI(0, -1, 0)];
    shared_ptr<RenderObjectBlock> pyBlock = world->blocks[pos + VectorI(0, 1, 0)];
    shared_ptr<RenderObjectBlock> nzBlock = world->blocks[pos + VectorI(0, 0, -1)];
    shared_ptr<RenderObjectBlock> pzBlock = world->blocks[pos + VectorI(0, 0, 1)];
    Matrix tform = Matrix::translate(pos);
    if(nxBlock != nullptr && !nxBlock->getBlockMesh()->pxBlocked)
        dest->add(transform(tform, nx));
    if(pxBlock != nullptr && !pxBlock->getBlockMesh()->nxBlocked)
        dest->add(transform(tform, px));
    if(nyBlock != nullptr && !nyBlock->getBlockMesh()->pyBlocked)
        dest->add(transform(tform, ny));
    if(pyBlock != nullptr && !pyBlock->getBlockMesh()->nyBlocked)
        dest->add(transform(tform, py));
    if(nzBlock != nullptr && !nzBlock->getBlockMesh()->pzBlocked)
        dest->add(transform(tform, nz));
    if(pzBlock != nullptr && !pzBlock->getBlockMesh()->nzBlocked)
        dest->add(transform(tform, pz));
    dest->add(transform(tform, center));
}

inline shared_ptr<RenderObject> RenderObject::read(Reader &reader, Client &client)
{
    Type type = (Type)reader.readLimitedU8(0, (uint8_t)Type::Last);
    switch(type)
    {
    case Type::Block:
        return RenderObjectBlock::read(reader, client);
    default:
        throw new InvalidDataValueException("read RenderObject type not implemented");
    }
}

#endif // RENDER_OBJECT_H_INCLUDED
