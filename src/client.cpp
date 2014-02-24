#include "client.h"
#include "network_protocol.h"
#include "render_object.h"
#include "platform.h"
#include "game_version.h"
#include "text.h"
#include <thread>
#include <iostream>
#include <sstream>

using namespace std;

atomic_uint_fast64_t Client::nextId(1);

namespace
{
void clientProcessReader(Reader *preader, Client *pclient, atomic_bool *pdone)
{
    Reader &reader = *preader;
    Client &client = *pclient;
    atomic_bool &done = *pdone;
    NetworkProtocol::NetworkEvent event;

    try
    {
        while(!done)
        {
            event = NetworkProtocol::readNetworkEvent(reader);

            switch(event)
            {
            case NetworkProtocol::NetworkEvent::UpdateRenderObjects:
            {
                uint64_t readCount = reader.readU64();

                for(uint64_t i = 0; i < readCount; i++)
                {
                    shared_ptr<RenderObject> ro = RenderObject::read(reader, client);
                }

                break;
            }

            case NetworkProtocol::NetworkEvent::Last:
                assert(false);
            }
        }
    }
    catch(exception *e)
    {
        cerr << e->what() << "\n";
        delete e;
    }

    done = true;
}

Mesh makeChunkRenderMesh(Client &client, PositionI chunkPos, RenderLayer rl)
{
    LockedClient lock(client);
    shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
    RenderObjectWorld::BlockIterator biX(world, chunkPos);
    shared_ptr<RenderObjectWorld::Chunk> chunk = biX.getChunk();

    if(chunk->cachedMesh[(int)rl] != nullptr)
    {
        return chunk->cachedMesh[(int)rl];
    }

    Mesh retval = Mesh(new Mesh_t);

    for(int dx = 0; dx < RenderObjectWorld::Chunk::size; dx++, biX.movePX())
    {
        RenderObjectWorld::BlockIterator biXY = biX;

        for(int dy = 0; dy < RenderObjectWorld::Chunk::size; dy++, biXY.movePY())
        {
            RenderObjectWorld::BlockIterator biXYZ = biXY;

            for(int dz = 0; dz < RenderObjectWorld::Chunk::size; dz++, biXYZ.movePZ())
            {
                shared_ptr<RenderObjectBlockMesh> mesh = biXYZ.getMesh();

                if(mesh != nullptr)
                {
                    mesh->render(retval, rl, biXYZ);
                }
            }
        }
    }

    chunk->cachedMesh[(int)rl] = retval;
    chunk->cachedMeshValid = true;
    return retval;
}

Mesh makeRenderMesh(Client &client, RenderLayer rl, PositionI pos, int renderDistance = 40)
{
    Mesh retval = Mesh(new Mesh_t);
    PositionI curChunkPos = pos;

    for(curChunkPos.x = (pos.x - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask; curChunkPos.x <= ((pos.x + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask); curChunkPos.x += RenderObjectWorld::Chunk::size)
    {
        for(curChunkPos.y = (pos.y - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask; curChunkPos.y <= ((pos.x + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask); curChunkPos.y += RenderObjectWorld::Chunk::size)
        {
            for(curChunkPos.z = (pos.z - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask; curChunkPos.z <= ((pos.x + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask); curChunkPos.z += RenderObjectWorld::Chunk::size)
            {
                retval->add(makeChunkRenderMesh(client, curChunkPos, rl));
            }
        }
    }

    return retval;
}

class ExampleEventHandler final : public EventHandler
{
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Up    : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Down  : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Move\n";
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Scroll : (" << event.scrollX << ", " << event.scrollY << ")\n";
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        cout << "Key Up  : " << event.key << " : " << event.mods << endl;
        return true;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        cout << "Key Down : " << event.key << " : " << event.mods << (event.isRepetition ? " : Repeated\n" : " : First\n");
        return true;
    }
    virtual bool handleKeyPress(KeyPressEvent &event) override
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
}

void clientProcess(Reader &reader, Writer &writer)
{
    Client client;
    atomic_bool done(false);
    thread networkThread(clientProcessReader, &reader, &client, &done);
    Renderer r;
    const int size = 5;

    while(!done)
    {
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Matrix tform = Matrix::rotateY(M_PI / 40 * Display::timer()).concat(Matrix::rotateX(Display::timer() / 10)).concat(Matrix::translate(0, 0, -4 * size)).concat(Matrix::scale(1.0f / size));
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            Mesh mesh = makeRenderMesh(client, rl, PositionI(0, 0, 0, Dimension::Overworld));
            renderLayerSetup(rl);
            r << transform(tform, mesh);
        }
        renderLayerSetup(RenderLayer::Last);
        Display::initOverlay();
        wstringstream s;
        s << L"Voxels " << GameVersion::VERSION;

        if(GameVersion::DEBUG)
        {
            s << L" Debug";
        }

        s << "\nFPS : " << Display::averageFPS() << endl;
        r << transform(Matrix::translate(-40 * Display::scaleX(), 40 * Display::scaleY() - Text::height(s.str()), -40 * Display::scaleX()), Text::mesh(s.str(), Color(1, 0, 1)));
        Display::flip();
        Display::handleEvents(shared_ptr<EventHandler>(new ExampleEventHandler));
    }
}

