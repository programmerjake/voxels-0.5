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
#include <sstream>
#include "platform.h"
#include "generate.h"
#include "texture_atlas.h"
#include "text.h"
#include "game_version.h"

using namespace std;

int main()
{
    Mesh mesh = Mesh(new Mesh_t());
    int size = 5;
    for(int dx = -size; dx <= size; dx++)
    {
        for(int dy = -size; dy <= size; dy++)
        {
            for(int dz = -size; dz <= size; dz++)
            {
                mesh->add(transform(Matrix::translate(dx - 0.5, dy - 0.5, dz - 0.5),
                                    Generate::unitBox(dx == -size ? TextureAtlas::OakWood.td() : TextureDescriptor(),
                                                      dx == size ? TextureAtlas::OakWood.td() : TextureDescriptor(),
                                                      dy == -size ? TextureAtlas::WoodEnd.td() : TextureDescriptor(),
                                                      dy == size ? TextureAtlas::WoodEnd.td() : TextureDescriptor(),
                                                      dz == -size ? TextureAtlas::OakWood.td() : TextureDescriptor(),
                                                      dz == size ? TextureAtlas::OakWood.td() : TextureDescriptor())));
            }
        }
    }
    Renderer r;
    while(true)
    {
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        r << transform(Matrix::rotateY(M_PI / 40 * Display::timer()).concat(Matrix::rotateX(Display::timer() / 10)).concat(Matrix::translate(0, 0, -20)), mesh);
        Display::initOverlay();
        wstringstream s;
        s << L"Voxels " << GameVersion::VERSION;
        if(GameVersion::DEBUG)
            s << L" Debug";
        s << "\nFPS : " << Display::averageFPS() << endl;
        r << transform(Matrix::translate(-40 * Display::scaleX(), 40 * Display::scaleY() - Text::height(s.str()), -40 * Display::scaleX()), Text::mesh(s.str(), Color(1, 0, 1)));
        Display::flip();
        Display::handleEvents(nullptr);
    }
}
