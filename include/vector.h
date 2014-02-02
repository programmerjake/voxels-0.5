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
#ifndef VECTOR_H
#define VECTOR_H

#include "util.h"
#include <cmath>
#include <stdexcept>
#include <random>

using namespace std;

class Vector
{
public:
    float x, y, z;

    Vector(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }

    Vector(float v = 0)
        : x(v), y(v), z(v)
    {
    }

    const Vector operator -() const
    {
        return Vector(-x, -y, -z);
    }

    const Vector & operator +() const
    {
        return *this;
    }

    const Vector operator +(const Vector & r) const
    {
        return Vector(x + r.x, y + r.y, z + r.z);
    }

    const Vector operator -(const Vector & r) const
    {
        return Vector(x - r.x, y - r.y, z - r.z);
    }

    const Vector operator *(const Vector & r) const
    {
        return Vector(x * r.x, y * r.y, z * r.z);
    }

    const Vector operator /(const Vector & r) const
    {
        return Vector(x / r.x, y / r.y, z / r.z);
    }

    const Vector operator *(float r) const
    {
        return Vector(x * r, y * r, z * r);
    }

    friend Vector operator *(float a, const Vector & b)
    {
        return Vector(a * b.x, a * b.y, a * b.z);
    }

    const Vector operator /(float r) const
    {
        return Vector(x / r, y / r, z / r);
    }

    bool operator ==(const Vector & r) const
    {
        return x == r.x && y == r.y && z == r.z;
    }

    bool operator !=(const Vector & r) const
    {
        return x != r.x || y != r.y || z != r.z;
    }

    const Vector & operator +=(const Vector & r)
    {
        x += r.x;
        y += r.y;
        z += r.z;
        return *this;
    }
    const Vector & operator -=(const Vector & r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;
        return *this;
    }
    const Vector & operator *=(const Vector & r)
    {
        x *= r.x;
        y *= r.y;
        z *= r.z;
        return *this;
    }
    const Vector & operator /=(const Vector & r)
    {
        x /= r.x;
        y /= r.y;
        z /= r.z;
        return *this;
    }
    const Vector & operator *=(float r)
    {
        x *= r;
        y *= r;
        z *= r;
        return *this;
    }
    const Vector & operator /=(float r)
    {
        x /= r;
        y /= r;
        z /= r;
        return *this;
    }

    friend float dot(const Vector & a, const Vector & b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    friend float absSquared(const Vector & v)
    {
        return dot(v, v);
    }

    friend float abs(const Vector & v)
    {
        return sqrt(absSquared(v));
    }

    friend const Vector normalizeNoThrow(const Vector & v)
    {
        float r = abs(v);
        if(r == 0)
        {
            r = 1;
        }
        return v / r;
    }

    friend const Vector normalize(const Vector v)
    {
        float r = abs(v);
        if(v == 0)
        {
            throw new domain_error("can't normalize <0, 0, 0>");
        }
        return v / r;
    }

    const Vector static normalize(float x, float y, float z)
    {
        Vector v(x, y, z);
        float r = abs(v);
        if(v == 0)
        {
            throw new domain_error("can't normalize <0, 0, 0>");
        }
        return v / r;
    }

    float phi() const
    {
        float r = abs(*this);
        if(r == 0)
        {
            return 0;
        }
        float v = y / r;
        v = limit(v, -1.0f, 1.0f);
        return asin(v);
    }

    float theta() const
    {
        return atan2(x, z);
    }

    float rSpherical() const
    {
        return abs(*this);
    }

    friend Vector cross(const Vector & a, const Vector & b)
    {
        return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    template<typename RE = default_random_engine>
    static Vector random(RE & re = defaultRandom)
    {
        Vector retval;
        do
        {
            retval.x = uniform_real_distribution<float>(-1, 1)(re);
            retval.y = uniform_real_distribution<float>(-1, 1)(re);
            retval.z = uniform_real_distribution<float>(-1, 1)(re);
        }
        while(absSquared(retval) > 1 || absSquared(retval) < eps);
        return retval;
    }
private:
};

#endif // VECTOR_H
