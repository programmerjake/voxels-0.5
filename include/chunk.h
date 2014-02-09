#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "position.h"
#include <memory>

using namespace std;

struct Chunk;

const int ChunkSizeLog2 = 4;
const int ChunkSize = 1 << ChunkSizeLog2;
const int ChunkModSizeMask = ChunkSize - 1;
const int ChunkFloorSizeMask = ~ChunkModSizeMask;
const int ChunkHeightLog2 = 8;
const int ChunkHeight = 1 << ChunkHeightLog2;

struct ChunkPosition : public PositionI
{
    ChunkPosition()
    {
    }
    ChunkPosition(int x, int y, int z, Dimension d)
        : PositionI(x, y, z, d)
    {
    }
    ChunkPosition(VectorF p, Dimension d)
        : PositionI(p, d)
    {
    }
    ChunkPosition(VectorI p, Dimension d)
        : PositionI(p, d)
    {
    }
    explicit ChunkPosition(const Chunk & c);
    ChunkPosition nx() const
    {
        return ChunkPosition(x - ChunkSize, y, z, d);
    }
    ChunkPosition px() const
    {
        return ChunkPosition(x + ChunkSize, y, z, d);
    }
    ChunkPosition nz() const
    {
        return ChunkPosition(x, y, z - ChunkSize, d);
    }
    ChunkPosition pz() const
    {
        return ChunkPosition(x, y, z + ChunkSize, d);
    }
};

struct Chunk
{
    ChunkPosition pos;
    weak_ptr<Chunk> nx;
    weak_ptr<Chunk> px;
    weak_ptr<Chunk> nz;
    weak_ptr<Chunk> pz;
};

inline ChunkPosition::ChunkPosition(const Chunk & c)
    : ChunkPosition(c.pos)
{
}

#endif // CHUNK_H_INCLUDED
