#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include "dimension.h"
#include "vector.h"
#include "util.h"

struct PositionI : public VectorI
{
    Dimension d;
    PositionI()
    {
    }
    PositionI(int x, int y, int z, Dimension d)
        : VectorI(x, y, z), d(d)
    {
    }
    PositionI(VectorF p, Dimension d)
        : VectorI(p), d(d)
    {
    }
    PositionI(VectorI p, Dimension d)
        : VectorI(p), d(d)
    {
    }
    friend bool operator ==(const PositionI & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend bool operator !=(const PositionI & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
};

struct PositionF : public VectorF
{
    Dimension d;
    PositionF()
    {
    }
    explicit PositionF(PositionI p)
        : VectorF(p.x, p.y, p.z), d(p.d)
    {
    }
    PositionF(float x, float y, float z, Dimension d)
        : VectorF(x, y, z), d(d)
    {
    }
    PositionF(VectorF p, Dimension d)
        : VectorF(p.x, p.y, p.z), d(d)
    {
    }
    PositionF(VectorI p, Dimension d)
        : VectorF(p.x, p.y, p.z), d(d)
    {
    }
    explicit operator VectorF() const
    {
        return VectorF(x, y, z);
    }
    explicit operator PositionI() const
    {
        return PositionI(VectorI(x, y, z), d);
    }
    friend bool operator ==(const PositionF & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend bool operator !=(const PositionF & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend bool operator ==(const PositionI & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend bool operator !=(const PositionI & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend bool operator ==(const PositionF & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend bool operator !=(const PositionF & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
};

#endif // POSITION_H_INCLUDED
