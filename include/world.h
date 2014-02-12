#include "block.h"
#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "chunk.h"
#include <unordered_map>

using namespace std;

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
    bool operator !() const
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
    friend class Chunk;
    friend class BlockIterator;
    World(const World &) = delete;
    const World & operator =(const World &) = delete;
private:
    list<shared_ptr<Chunk>> chunksList;
    unordered_map<ChunkPosition, shared_ptr<Chunk>> chunksMap;
    shared_ptr<Chunk> getChunk(ChunkPosition pos)
    {
        shared_ptr<Chunk> & c = chunksMap[pos];
        if(c != nullptr)
            return c;
        c = shared_ptr<Chunk>(new Chunk(pos));
        c->nx == chunksMap[pos.nx()];
        c->px == chunksMap[pos.px()];
        c->nz == chunksMap[pos.nz()];
        c->pz == chunksMap[pos.pz()];
        chunksList.push_back(c);
        return c;
    }
public:
    World()
    {
    }
    ~World()
    {
    }
};

#endif // WORLD_H_INCLUDED
