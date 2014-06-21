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
#include "block.h"
#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#error finish changing to new physics engine

#include "position.h"
#include <memory>
#include <array>

using namespace std;

struct Chunk;

const int ChunkSizeLog2 = 4;
const int ChunkSize = 1 << ChunkSizeLog2;
const int ChunkModSizeMask = ChunkSize - 1;
const int ChunkFloorSizeMask = ~ChunkModSizeMask;
const int ChunkHeightLog2 = 8;
const int ChunkHeight = 1 << ChunkHeightLog2;

struct ChunkPosition
{
    int x, z;
    Dimension d;
    ChunkPosition()
    {
    }
    ChunkPosition(int x, int z, Dimension d)
        : x(x), z(z), d(d)
    {
        assert((x & ChunkModSizeMask) == 0);
        assert((z & ChunkModSizeMask) == 0);
        assert((unsigned)d < (unsigned)Dimension::Last);
    }
    ChunkPosition(VectorF p, Dimension d)
        : x(ifloor(p.x) & ChunkFloorSizeMask), z(ifloor(p.z) & ChunkFloorSizeMask), d(d)
    {
        assert((unsigned)d < (unsigned)Dimension::Last);
    }
    explicit ChunkPosition(PositionF p)
        : x(ifloor(p.x) & ChunkFloorSizeMask), z(ifloor(p.z) & ChunkFloorSizeMask), d(p.d)
    {
        assert((unsigned)d < (unsigned)Dimension::Last);
    }
    ChunkPosition(VectorI p, Dimension d)
        : x(p.x & ChunkFloorSizeMask), z(p.z & ChunkFloorSizeMask), d(d)
    {
        assert((unsigned)d < (unsigned)Dimension::Last);
    }
    explicit ChunkPosition(PositionI p)
        : x(p.x & ChunkFloorSizeMask), z(p.z & ChunkFloorSizeMask), d(p.d)
    {
        assert((unsigned)d < (unsigned)Dimension::Last);
    }
    explicit ChunkPosition(const Chunk & c);
    ChunkPosition nx() const
    {
        return ChunkPosition(x - ChunkSize, z, d);
    }
    ChunkPosition px() const
    {
        return ChunkPosition(x + ChunkSize, z, d);
    }
    ChunkPosition nz() const
    {
        return ChunkPosition(x, z - ChunkSize, d);
    }
    ChunkPosition pz() const
    {
        return ChunkPosition(x, z + ChunkSize, d);
    }
    friend bool operator ==(const ChunkPosition & l, const ChunkPosition & r)
    {
        return l.x == r.x && l.z == r.z && l.d == r.d;
    }
    friend bool operator !=(const ChunkPosition & l, const ChunkPosition & r)
    {
        return !operator ==(l, r);
    }
    explicit operator PositionI() const
    {
        return PositionI(x, 0, z, d);
    }
};

namespace std
{
template <>
struct hash<ChunkPosition> final
{
    size_t operator()(const ChunkPosition & p) const
    {
        size_t retval = p.x;
        retval += retval << 16; // *= 65537
        retval += p.z;
        retval = (retval << 13) - retval; // *= 8191
        return retval + (size_t)p.d;
    }
};
}

struct Chunk
{
    const ChunkPosition pos;
    weak_ptr<Chunk> nx;
    weak_ptr<Chunk> px;
    weak_ptr<Chunk> nz;
    weak_ptr<Chunk> pz;
    array<array<array<BlockData, ChunkSize>, ChunkHeight>, ChunkSize> blocks;
    Chunk(ChunkPosition pos)
        : pos(pos)
    {
    }
};

inline ChunkPosition::ChunkPosition(const Chunk & c)
    : ChunkPosition(c.pos)
{
}

#endif // CHUNK_H_INCLUDED
