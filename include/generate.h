#ifndef GENERATE_H_INCLUDED
#define GENERATE_H_INCLUDED

#include "mesh.h"
#include <utility>

Mesh invert(Mesh mesh)
{
    vector<Triangle> triangles;
    triangles.reserve(mesh->size());
    for(Triangle tri : *mesh)
    {
        swap(tri.p[0], tri.p[1]);
        swap(tri.c[0], tri.c[1]);
        swap(tri.t[0], tri.t[1]);
        triangles.push_back(tri);
    }
    return Mesh(new Mesh_t(mesh->texture(), triangles));
}

#endif // GENERATE_H_INCLUDED
