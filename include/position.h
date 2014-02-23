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
    friend bool operator ==(const VectorI & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend bool operator !=(const VectorI & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend bool operator ==(const PositionI & a, const VectorI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend bool operator !=(const PositionI & a, const VectorI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend PositionI operator +(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a + b, a.d);
    }
    friend PositionI operator +(VectorI a, PositionI b)
    {
        return PositionI(a + (VectorI)b, b.d);
    }
    friend PositionI operator -(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a - b, a.d);
    }
    friend PositionI operator -(VectorI a, PositionI b)
    {
        return PositionI(a - (VectorI)b, b.d);
    }
    friend PositionI operator *(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a * b, a.d);
    }
    friend PositionI operator *(VectorI a, PositionI b)
    {
        return PositionI(a * (VectorI)b, b.d);
    }
    friend PositionI operator *(PositionI a, int b)
    {
        return PositionI((VectorI)a * b, a.d);
    }
    friend PositionI operator *(int a, PositionI b)
    {
        return PositionI(a * (VectorI)b, b.d);
    }
    PositionI operator -() const
    {
        return PositionI(-(VectorI)*this, d);
    }
    const PositionI & operator +=(VectorI rt)
    {
        *(VectorI *)this += rt;
        return *this;
    }
    const PositionI & operator -=(VectorI rt)
    {
        *(VectorI *)this -= rt;
        return *this;
    }
    const PositionI & operator *=(VectorI rt)
    {
        *(VectorI *)this *= rt;
        return *this;
    }
    const PositionI & operator *=(int rt)
    {
        *(VectorI *)this *= rt;
        return *this;
    }
};

namespace std
{
template <>
struct hash<PositionI>
{
    size_t operator ()(const PositionI & v) const
    {
        return (size_t)v.x + (size_t)v.y * 97 + (size_t)v.z * 8191 + (size_t)v.d * 65537;
    }
};
}

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
    friend bool operator ==(const VectorF & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend bool operator !=(const VectorF & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend bool operator ==(const PositionF & a, const VectorF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend bool operator !=(const PositionF & a, const VectorF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend PositionF operator +(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a + b, a.d);
    }
    friend PositionF operator +(VectorF a, PositionF b)
    {
        return PositionF(a + (VectorF)b, b.d);
    }
    friend PositionF operator -(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a - b, a.d);
    }
    friend PositionF operator -(VectorF a, PositionF b)
    {
        return PositionF(a - (VectorF)b, b.d);
    }
    friend PositionF operator *(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a * b, a.d);
    }
    friend PositionF operator *(VectorF a, PositionF b)
    {
        return PositionF(a * (VectorF)b, b.d);
    }
    friend PositionF operator *(PositionF a, float b)
    {
        return PositionF((VectorF)a * b, a.d);
    }
    friend PositionF operator *(float a, PositionF b)
    {
        return PositionF(a * (VectorF)b, b.d);
    }
    friend PositionF operator /(PositionF a, float b)
    {
        return PositionF((VectorF)a / b, a.d);
    }
    PositionF operator -() const
    {
        return PositionF(-(VectorF)*this, d);
    }
    const PositionF & operator +=(VectorF rt)
    {
        *(VectorF *)this += rt;
        return *this;
    }
    const PositionF & operator -=(VectorF rt)
    {
        *(VectorF *)this -= rt;
        return *this;
    }
    const PositionF & operator *=(VectorF rt)
    {
        *(VectorF *)this *= rt;
        return *this;
    }
    const PositionF & operator /=(VectorF rt)
    {
        *(VectorF *)this /= rt;
        return *this;
    }
    const PositionF & operator *=(float rt)
    {
        *(VectorF *)this *= rt;
        return *this;
    }
    const PositionF & operator /=(float rt)
    {
        *(VectorF *)this /= rt;
        return *this;
    }
};

#endif // POSITION_H_INCLUDED
