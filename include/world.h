#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "chunk.h"

class World;

const int WorldHeight = ChunkHeight;

class BlockIterator final
{
    friend class World;
private:
    shared_ptr<World> worldInternal;
public:
    shared_ptr<World> world() const
    {
        return worldInternal;
    }
    operator bool() const
    {
        return world() != nullptr;
    }
    operator !() const
    {
        return world() == nullptr;
    }
    BlockIterator()
    {
    }
private:
    PositionI pos;
public:
    explicit operator PositionI() const
    {
        return pos;
    }
private:
    ssize_t chunkIndex;
    shared_ptr<Chunk> chunk;
    BlockIterator(shared_ptr<World> w, PositionI pos);
};

class World final
{
#error finish
};

#endif // WORLD_H_INCLUDED
