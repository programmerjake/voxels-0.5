#include "mesh.h"
#include "platform.h"
#include <iostream>

Renderer & Renderer::operator <<(const Mesh_t & m)
{
    m.texture().bind();
    glVertexPointer(3, GL_FLOAT, 0, (const void *)m.points.data());
    glTexCoordPointer(2, GL_FLOAT, 0, (const void *)m.textureCoords.data());
    glColorPointer(4, GL_FLOAT, 0, (const void *)m.colors.data());
    glDrawArrays(GL_TRIANGLES, 0, (GLint)m.size() * 3);
    return *this;
}

