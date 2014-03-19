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
#ifndef BLOCK_FACE_H_INCLUDED
#define BLOCK_FACE_H_INCLUDED

#include <cstdint>

using namespace std;

enum class BlockFace : uint_fast32_t
{
    NX, PX, NY, PY, NZ, PZ
};

inline int dx(BlockFace bf)
{
    switch(bf)
    {
    case BlockFace::NX:
        return -1;
    case BlockFace::PX:
        return 1;
    default:
        return 0;
    }
}

inline int dy(BlockFace bf)
{
    switch(bf)
    {
    case BlockFace::NY:
        return -1;
    case BlockFace::PY:
        return 1;
    default:
        return 0;
    }
}

inline int dz(BlockFace bf)
{
    switch(bf)
    {
    case BlockFace::NZ:
        return -1;
    case BlockFace::PZ:
        return 1;
    default:
        return 0;
    }
}

#endif // BLOCK_FACE_H_INCLUDED
