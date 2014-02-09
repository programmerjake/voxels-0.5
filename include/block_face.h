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
