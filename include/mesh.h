#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "vector.h"
#include "color.h"
#include "matrix.h"
#include <memory>
#include <iterator>
#include <ostream>
#include "image.h"
#include "texture_descriptor.h"

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
    friend ostream & operator <<(ostream & os, const TextureCoord & t)
    {
        return os << "<" << t.u << ", " << t.v << ">";
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
    operator Mesh() const;
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

class ImageNotSameException final : public runtime_error
{
public:
    explicit ImageNotSameException()
        : runtime_error("can't use more than one image per mesh")
    {
    }
};

class Mesh_t final
{
private:
    vector<float> points, colors, textureCoords;
    Image textureInternal;
    size_t length;
    static constexpr size_t floatsPerPoint = 3, pointsPerTriangle = 3,
                            floatsPerColor = 4, colorsPerTriangle = 3,
                            floatsPerTextureCoord = 2, textureCoordsPerTriangle = 3;
    friend class Renderer;
public:
    Mesh_t()
    {
        length = 0;
    }

    Mesh_t(Image texture, vector<Triangle> triangles = vector<Triangle>())
    {
        length = triangles.size();
        points.reserve(floatsPerPoint * pointsPerTriangle * length);
        colors.reserve(floatsPerColor * colorsPerTriangle * length);
        textureCoords.reserve(floatsPerTextureCoord * textureCoordsPerTriangle * length);
        textureInternal = texture;

        for(Triangle tri : triangles)
        {
            points.push_back(tri.p[0].x);
            points.push_back(tri.p[0].y);
            points.push_back(tri.p[0].z);
            points.push_back(tri.p[1].x);
            points.push_back(tri.p[1].y);
            points.push_back(tri.p[1].z);
            points.push_back(tri.p[2].x);
            points.push_back(tri.p[2].y);
            points.push_back(tri.p[2].z);
            colors.push_back(tri.c[0].r);
            colors.push_back(tri.c[0].g);
            colors.push_back(tri.c[0].b);
            colors.push_back(tri.c[0].a);
            colors.push_back(tri.c[1].r);
            colors.push_back(tri.c[1].g);
            colors.push_back(tri.c[1].b);
            colors.push_back(tri.c[1].a);
            colors.push_back(tri.c[2].r);
            colors.push_back(tri.c[2].g);
            colors.push_back(tri.c[2].b);
            colors.push_back(tri.c[2].a);
            textureCoords.push_back(tri.t[0].u);
            textureCoords.push_back(tri.t[0].v);
            textureCoords.push_back(tri.t[1].u);
            textureCoords.push_back(tri.t[1].v);
            textureCoords.push_back(tri.t[2].u);
            textureCoords.push_back(tri.t[2].v);
        }
    }

    Mesh_t(TextureDescriptor tex, vector<Triangle> triangles = vector<Triangle>())
    {
        length = triangles.size();
        points.reserve(floatsPerPoint * pointsPerTriangle * length);
        colors.reserve(floatsPerColor * colorsPerTriangle * length);
        textureCoords.reserve(floatsPerTextureCoord * textureCoordsPerTriangle * length);
        textureInternal = tex.image;

        for(Triangle tri : triangles)
        {
            points.push_back(tri.p[0].x);
            points.push_back(tri.p[0].y);
            points.push_back(tri.p[0].z);
            points.push_back(tri.p[1].x);
            points.push_back(tri.p[1].y);
            points.push_back(tri.p[1].z);
            points.push_back(tri.p[2].x);
            points.push_back(tri.p[2].y);
            points.push_back(tri.p[2].z);
            colors.push_back(tri.c[0].r);
            colors.push_back(tri.c[0].g);
            colors.push_back(tri.c[0].b);
            colors.push_back(tri.c[0].a);
            colors.push_back(tri.c[1].r);
            colors.push_back(tri.c[1].g);
            colors.push_back(tri.c[1].b);
            colors.push_back(tri.c[1].a);
            colors.push_back(tri.c[2].r);
            colors.push_back(tri.c[2].g);
            colors.push_back(tri.c[2].b);
            colors.push_back(tri.c[2].a);
            textureCoords.push_back(interpolate(tri.t[0].u, tex.minU, tex.maxU));
            textureCoords.push_back(interpolate(tri.t[0].v, tex.minV, tex.maxV));
            textureCoords.push_back(interpolate(tri.t[1].u, tex.minU, tex.maxU));
            textureCoords.push_back(interpolate(tri.t[1].v, tex.minV, tex.maxV));
            textureCoords.push_back(interpolate(tri.t[2].u, tex.minU, tex.maxU));
            textureCoords.push_back(interpolate(tri.t[2].v, tex.minV, tex.maxV));
        }
    }

    Mesh_t(const TransformedMesh &tm)
        : Mesh_t()
    {
        if(tm.mesh == nullptr)
        {
            return;
        }

        points = tm.mesh->points;
        colors = tm.mesh->colors;
        textureCoords = tm.mesh->textureCoords;
        textureInternal = tm.mesh->texture();
        length = tm.mesh->length;

        for(auto i = points.begin(); i != points.end(); i += floatsPerPoint)
        {
            Vector v;
            v.x = i[0];
            v.y = i[1];
            v.z = i[2];
            v = transform(tm.tform, v);
            i[0] = v.x;
            i[1] = v.y;
            i[2] = v.z;
        }

        for(auto i = colors.begin(); i != colors.end(); i += floatsPerColor)
        {
            Color c;
            c.r = i[0];
            c.g = i[1];
            c.b = i[2];
            c.a = i[3];
            c = scale(c, tm.factor);
            i[0] = c.r;
            i[1] = c.g;
            i[2] = c.b;
            i[3] = c.a;
        }
    }

    const Image &texture() const
    {
        return textureInternal;
    }

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
        friend const_iterator operator +(ssize_t i, const const_iterator &iter)
        {
            return iter.operator + (i);
        }
        const_iterator operator -(ssize_t i) const
        {
            return operator +(-i);
        }
        ssize_t operator -(const const_iterator &r) const
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
        bool operator >(const const_iterator &r) const
        {
            return pointIterator > r.pointIterator;
        }
        bool operator >=(const const_iterator &r) const
        {
            return pointIterator >= r.pointIterator;
        }
        bool operator <(const const_iterator &r) const
        {
            return pointIterator < r.pointIterator;
        }
        bool operator <=(const const_iterator &r) const
        {
            return pointIterator <= r.pointIterator;
        }
    };

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    size_t size() const
    {
        return length;
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

    void add(const Mesh_t &m)
    {
        if(texture())
        {
            if(m.texture() && m.texture() != texture())
            {
                throw new ImageNotSameException;
            }
        }
        else
        {
            textureInternal = m.texture();
        }

        length += m.length;
        points.insert(points.end(), m.points.begin(), m.points.end());
        colors.insert(colors.end(), m.colors.begin(), m.colors.end());
        textureCoords.insert(textureCoords.end(), m.textureCoords.begin(), m.textureCoords.end());
    }

    void add(Mesh m)
    {
        add(*m);
    }

    void add(TransformedMesh m)
    {
        Mesh_t m2(m);
        add(m2);
    }
};

inline TransformedMesh::operator Mesh() const
{
    return Mesh(new Mesh_t(*this));
}

class Renderer final
{
private:
    Renderer(const Renderer &) = delete;
    const Renderer operator =(const Renderer &) = delete;
public:
    Renderer()
    {
    }

    ~Renderer()
    {
    }

    Renderer &operator <<(const Mesh_t &m);

    Renderer &operator <<(Mesh m)
    {
        operator <<(*m);
        return *this;
    }

    Renderer &operator <<(TransformedMesh m)
    {
        Mesh_t m2(m);
        operator <<(m2);
        return *this;
    }
};

#endif // MESH_H_INCLUDED
