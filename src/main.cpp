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
#include "event.h"
#include "util.h"

using namespace std;

class ExampleEventHandler final : public EventHandler
{
    virtual bool handleMouseUp(MouseUpEvent & event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Up    : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent & event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Down  : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent & event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Move\n";
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent & event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Scroll : (" << event.scrollX << ", " << event.scrollY << ")\n";
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent & event) override
    {
        cout << "Key Up  : " << event.key << " : " << event.mods << endl;
        return true;
    }
    virtual bool handleKeyDown(KeyDownEvent & event) override
    {
        cout << "Key Down : " << event.key << " : " << event.mods << (event.isRepetition ? " : Repeated\n" : " : First\n");
        return true;
    }
    virtual bool handleKeyPress(KeyPressEvent & event) override
    {
        cout << "Key : \'" << wcsrtombs(wstring(L"") + event.character) << "\'\n";
        return true;
    }
    virtual bool handleQuit(QuitEvent &) override
    {
        cout << "Quit\n";
        return false; // so that the program will actually quit
    }
};

int main()
{
    Mesh mesh = Mesh(new Mesh_t());
    int size = 10;
    for(int dx = -size; dx <= size; dx++)
    {
        for(int dy = -size; dy <= size; dy++)
        {
            for(int dz = -size; dz <= size; dz++)
            {
                if(dx != -size && dx != size && dy != -size && dy != size && dz != -size && dz != size)
                {
                    dz = size;
                }
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
        r << transform(Matrix::rotateY(M_PI / 40 * Display::timer()).concat(Matrix::rotateX(Display::timer() / 10)).concat(Matrix::translate(0, 0, -4 * size)).concat(Matrix::scale(1.0f / size)), mesh);
        Display::initOverlay();
        wstringstream s;
        s << L"Voxels " << GameVersion::VERSION;
        if(GameVersion::DEBUG)
            s << L" Debug";
        s << "\nFPS : " << Display::averageFPS() << endl;
        r << transform(Matrix::translate(-40 * Display::scaleX(), 40 * Display::scaleY() - Text::height(s.str()), -40 * Display::scaleX()), Text::mesh(s.str(), Color(1, 0, 1)));
        Display::flip();
        Display::handleEvents(shared_ptr<EventHandler>(new ExampleEventHandler));
    }
}
