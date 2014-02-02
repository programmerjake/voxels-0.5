#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "vector.h"
#include "color.h"
#include "matrix.h"
#include <memory>
#include <iterator>

class Mesh_t;

typedef shared_ptr<Mesh_t> Mesh;

struct TextureCoord
{
    float u, v;
    TextureCoord(float u, float v)
        : u(u), v(v)
    {
    }
    TextureCoord()
        : TextureCoord(0, 0)
    {
    }
};

struct Triangle
{
    Vector p[3];
    Color c[3];
    TextureCoord t[3];
    Triangle()
    {
    }
    Triangle(Vector p1, Color c1, TextureCoord t1, Vector p2, Color c2, TextureCoord t2, Vector p3, Color c3, TextureCoord t3)
    {
        p[0] = p1;
        c[0] = c1;
        t[0] = t1;
        p[1] = p2;
        c[1] = c2;
        t[1] = t2;
        p[2] = p3;
        c[2] = c3;
        t[2] = t3;
    }
    Vector normal() const
    {
        return normalize(cross(p[1] - p[0], p[2] - p[0]));
    }
};

inline Triangle transform(const Matrix &m, Triangle t)
{
    for(Vector &p : t.p)
    {
        p = transform(m, p);
    }
    return t;
}

struct TransformedMesh
{
    Mesh mesh;
    Matrix tform;
    Color factor;
    TransformedMesh()
        : mesh(nullptr), tform(Matrix::identity()), factor(1, 1, 1, 1)
    {
    }
    TransformedMesh(Mesh mesh, Matrix tform, Color factor = Color(1, 1, 1, 1))
        : mesh(mesh), tform(tform), factor(factor)
    {
    }
};

inline TransformedMesh transform(const Matrix &m, Mesh mesh)
{
    return TransformedMesh(mesh, m);
}

inline TransformedMesh transform(const Matrix &m, TransformedMesh mesh)
{
    return TransformedMesh(mesh.mesh, mesh.tform.concat(m), mesh.factor);
}

inline TransformedMesh scaleColors(Color factor, Mesh mesh)
{
    return TransformedMesh(mesh, Matrix::identity(), factor);
}

inline TransformedMesh scaleColors(Color factor, TransformedMesh mesh)
{
    return TransformedMesh(mesh.mesh, mesh.tform, scale(mesh.factor, factor));
}

class Mesh_t final
{
private:
    vector<float> points, colors, textureCoords;
    Image textureInternal;
    static constexpr size_t floatsPerPoint = 3, pointsPerTriangle = 3,
                            floatsPerColor = 4, colorsPerTriangle = 3,
                            floatsPerTextureCoord = 2, textureCoordsPerTriangle = 3;
    friend class Renderer;
public:
    friend class const_iterator;
    class const_iterator final : public iterator<iterator_traits<vector<float>::iterator>::value_type, const Triangle, ssize_t>
    {
        friend class Mesh_t;
    private:
        typedef vector<float>::const_iterator subIterator;
        mutable Triangle tri;
        subIterator pointIterator, colorIterator, textureCoordIterator;
        const_iterator(subIterator pointIterator, subIterator colorIterator, subIterator textureCoordIterator)
            : pointIterator(pointIterator), colorIterator(colorIterator), textureCoordIterator(textureCoordIterator)
        {
        }
    public:
        const_iterator()
        {
        }
        bool operator ==(const const_iterator &rt) const
        {
            return pointIterator == rt.pointIterator;
        }
        bool operator !=(const const_iterator &rt) const
        {
            return pointIterator != rt.pointIterator;
        }
        const Triangle &operator *() const
        {
            subIterator p = pointIterator, c = colorIterator, t = textureCoordIterator;
            tri.p[0] = Vector(p[0], p[1], p[2]);
            tri.p[1] = Vector(p[3], p[4], p[5]);
            tri.p[2] = Vector(p[6], p[7], p[8]);
            tri.c[0] = Color(c[0], c[1], c[2], c[3]);
            tri.c[1] = Color(c[4], c[5], c[6], c[7]);
            tri.c[2] = Color(c[8], c[9], c[10], c[11]);
            tri.t[0] = TextureCoord(t[0], t[1]);
            tri.t[1] = TextureCoord(t[2], t[3]);
            tri.t[2] = TextureCoord(t[4], t[5]);
            return tri;
        }
        const Triangle &operator[](ssize_t index) const
        {
            return operator +(index).operator * ();
        }
        const Triangle *operator ->() const
        {
            return &operator *();
        }
        const_iterator operator +(ssize_t i) const
        {
            return const_iterator(pointIterator + i * floatsPerPoint * pointsPerTriangle,
                                  colorIterator + i * floatsPerColor * colorsPerTriangle,
                                  textureCoordIterator + i * floatsPerTextureCoord * textureCoordsPerTriangle);
        }
        friend const_iterator operator +(ssize_t i, const const_iterator & iter)
        {
            return iter.operator +(i);
        }
        const_iterator operator -(ssize_t i) const
        {
            return operator +(-i);
        }
        ssize_t operator -(const const_iterator & r) const
        {
            return (textureCoordIterator - r.textureCoordIterator) / (floatsPerTextureCoord * textureCoordsPerTriangle);
        }
        const const_iterator &operator +=(ssize_t i)
        {
            return *this = operator +(i);
        }
        const const_iterator &operator -=(ssize_t i)
        {
            return *this = operator -(i);
        }
        const const_iterator &operator ++()
        {
            return operator +=(1);
        }
        const const_iterator &operator --()
        {
            return operator -=(1);
        }
        const_iterator operator ++(int)
        {
            const_iterator retval = *this;
            operator ++();
            return retval;
        }
        const_iterator operator --(int)
        {
            const_iterator retval = *this;
            operator --();
            return retval;
        }
        bool operator >(const const_iterator & r) const
        {
            return pointIterator > r.pointIterator;
        }
        bool operator >=(const const_iterator & r) const
        {
            return pointIterator >= r.pointIterator;
        }
        bool operator <(const const_iterator & r) const
        {
            return pointIterator < r.pointIterator;
        }
        bool operator <=(const const_iterator & r) const
        {
            return pointIterator <= r.pointIterator;
        }
    };

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    size_t length() const
    {
        return points.length() / (floatsPerPoint * pointsPerTriangle);
    }

    const_iterator begin() const
    {
        return const_iterator(points.begin(), colors.begin(), textureCoords.begin());
    }

    const_iterator end() const
    {
        return const_iterator(points.end(), colors.end(), textureCoords.end());
    }

    const_iterator cbegin() const
    {
        return const_iterator(points.begin(), colors.begin(), textureCoords.begin());
    }

    const_iterator cend() const
    {
        return const_iterator(points.end(), colors.end(), textureCoords.end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }
};

#endif // MESH_H_INCLUDED
