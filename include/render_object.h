#ifndef RENDER_OBJECT_H_INCLUDED
#define RENDER_OBJECT_H_INCLUDED

#include "mesh.h"
#include "position.h"
#include "render_layer.h"
#include <stream.h>
#include <atomic>

class RenderObject
{
private:
    RenderObject(const RenderObject &) = delete;
    const RenderObject & operator =(const RenderObject &) = delete;
    static uint64_t makeId()
    {
        static atomic_uint_fast64_t nextId(0);
        return (uint64_t)(nextId++);
    }
protected:
    virtual void writeInternal(Writer & writer) = 0;
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
    void write(Writer & writer)
    {
        writer->writeU8((uint8_t)type());
        writeInternal(writer);
    }
    static shared_ptr<RenderObject> read(Reader & reader);
    virtual bool operator ==(const RenderObject & rt) const = 0;
};

class RenderObjectBlockMesh final
{
private:
    uint32_t id;
    Mesh center, nx, px, ny, py, nz, pz;
    RenderLayer rl;


public:
    const bool nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked;
    void render(Mesh dest, RenderLayer rl, VectorI pos)
    {
#error finish
    }
};

class RenderObjectBlock final : public RenderObject
{
private:
    PositionI pos;
#error finish
};

#endif // RENDER_OBJECT_H_INCLUDED
