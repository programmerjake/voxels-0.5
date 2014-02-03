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
#include <iostream>
#include "platform.h"
#include "generate.h"
#include "texture_atlas.h"

using namespace std;

int main()
{
    Triangle t1;
    t1.p[0] = Vector(-1, -1, 0);
    t1.p[1] = Vector(1, -1, 0);
    t1.p[2] = Vector(-1, 1, 0);
    t1.c[0] = Color(1);
    t1.c[1] = Color(1);
    t1.c[2] = Color(1);
    t1.t[0] = TextureCoord(0, 0);
    t1.t[1] = TextureCoord(1, 0);
    t1.t[2] = TextureCoord(0, 1);
    Triangle t2;
    t2.p[0] = Vector(1, 1, 0);
    t2.p[1] = Vector(-1, 1, 0);
    t2.p[2] = Vector(1, -1, 0);
    t2.c[0] = Color(1);
    t2.c[1] = Color(1);
    t2.c[2] = Color(1);
    t2.t[0] = TextureCoord(1, 1);
    t2.t[1] = TextureCoord(0, 1);
    t2.t[2] = TextureCoord(1, 0);
    Mesh mesh = Mesh(new Mesh_t(TextureAtlas::OakWood.td(), vector<Triangle>{t1, t2}));
    mesh->add(invert(mesh));
    Renderer r;
    while(true)
    {
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        r << transform(Matrix::rotateY(Display::timer() * M_PI / 2 * 0).concat(Matrix::translate(0, 0, -1)), mesh);
        Display::flip();
        Display::handleEvents(nullptr);
    }
}
