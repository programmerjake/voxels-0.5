#ifndef RENDER_OBJECT_H_INCLUDED
#define RENDER_OBJECT_H_INCLUDED

#include "mesh.h"
#include "position.h"
#include "render_layer.h"
#include "stream.h"
#include "client.h"
#include "block_face.h"
#include <atomic>
#include <map>

using namespace std;

class RenderObject
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
    enum class Type : uint8_t
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
        writer->writeU8((uint8_t)type());
        writeInternal(writer);
    }
    static shared_ptr<RenderObject> read(Reader &reader, Client &client);
    virtual bool operator ==(const RenderObject &rt) const = 0;
};

class RenderObjectBlockMesh final : public enable_shared_from_this<RenderObjectBlockMesh>
{
private:
    Mesh center, nx, px, ny, py, nz, pz;
public:
    const RenderLayer rl;
    const bool nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked;
    void render(Mesh dest, RenderLayer rl, VectorI pos);
    RenderObjectBlockMesh(Mesh center, Mesh nx, Mesh px, Mesh ny, Mesh py, Mesh nz, Mesh pz, bool nxBlocked, bool pxBlocked, bool nyBlocked, bool pyBlocked, bool nzBlocked, bool pzBlocked, RenderLayer rl)
        : center(center), nx(nx), px(px), ny(ny), py(py), nz(nz), pz(pz), nxBlocked(nxBlocked), pxBlocked(pxBlocked), nyBlocked(nyBlocked), pyBlocked(pyBlocked), nzBlocked(nzBlocked), pzBlocked(pzBlocked), rl(rl)
    {
    }
    static shared_ptr<RenderObjectBlockMesh> read(Reader &reader, Client &client)
    {
        Client::IdType id = Client::readIdNonNull(reader);
        shared_ptr<RenderObjectBlockMesh> retval = client.getPtr(id, Client::DataType::RenderObjectBlockMesh);
        if(retval != nullptr)
            return retval;
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
#error finish
};

inline void RenderObjectBlockMesh::render(Mesh dest, RenderLayer rl, VectorI pos)
{
    if(rl != this->rl)
    {
        return;
    }
    #error finish
}

#endif // RENDER_OBJECT_H_INCLUDED
