#include "client.h"
#include "network_protocol.h"
#include "render_object.h"
#include "platform.h"
#include "game_version.h"
#include "text.h"
#include <thread>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>

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

void meshMakerThread(atomic_bool * pdone, Mesh meshes[], bool * pneedMeshes, const int * prenderDistance, const PositionI * ppos, condition_variable_any * cond, mutex * lock, Client * pclient)
{
    atomic_bool & done = *pdone;
    bool & needMeshes = *pneedMeshes;
    lock->lock();
    const int & renderDistance = *prenderDistance;
    const PositionI & pos = *ppos;
    Client &client = *pclient;
    while(!done)
    {
        while(!done && !needMeshes)
            cond->wait(*lock);
        if(done)
            break;
        needMeshes = false;
        lock->unlock();
        Mesh tempMeshes[(int)RenderLayer::Last];
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            tempMeshes[(int)rl] = makeRenderMesh(client, rl, pos, renderDistance);
        }
        lock->lock();
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            meshes[(int)rl] = tempMeshes[(int)rl];
        }
    }
    lock->unlock();
}

class ClientEventHandler final : public EventHandler
{
    float & dtheta;
    float & dphi;
    bool & paused;
public:
    ClientEventHandler(float & dtheta, float & dphi, bool &paused)
        : dtheta(dtheta), dphi(dphi), paused(paused)
    {
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        //cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Up    : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        //cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Down  : " << event.button << endl;
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        //cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Move\n";
        if(paused)
            return true;
        if(event.deltaX > 0)
            dtheta += limit(event.deltaX, 0.0f, 10.0f) * event.deltaX / 1000.0 * M_PI;
        else
            dtheta -= limit(event.deltaX, -10.0f, 0.0f) * event.deltaX / 1000.0 * M_PI;
        if(event.deltaY > 0)
            dphi -= limit(event.deltaY, 0.0f, 10.0f) * event.deltaY / 1000.0 * M_PI;
        else
            dphi += limit(event.deltaY, -10.0f, 0.0f) * event.deltaY / 1000.0 * M_PI;
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        //cout << "Mouse (" << event.x << ", " << event.y << ") (" << event.deltaX << ", " << event.deltaY << ") : Scroll : (" << event.scrollX << ", " << event.scrollY << ")\n";
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        //cout << "Key Up  : " << event.key << " : " << event.mods << endl;
        return true;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        //cout << "Key Down : " << event.key << " : " << event.mods << (event.isRepetition ? " : Repeated\n" : " : First\n");
        if(event.key == KeyboardKey::KeyboardKey_P)
        {
            paused = !paused;
        }
        return false;
    }
    virtual bool handleKeyPress(KeyPressEvent &event) override
    {
        //cout << "Key : \'" << wcsrtombs(wstring(L"") + event.character) << "\'\n";
        return true;
    }
    virtual bool handleQuit(QuitEvent &) override
    {
        //cout << "Quit\n";
        return false; // so that the program will actually quit
    }
};
}

void clientProcess(StreamRW & streamRW)
{
    startGraphics();
    Reader &reader = streamRW.reader();
    Writer &writer = streamRW.writer();
    Client client;
    atomic_bool done(false);
    thread networkThread(clientProcessReader, &reader, &client, &done);
    Renderer r;
    float theta = 0, phi = 0, dtheta = 0, dphi = 0;
    bool paused = true;
    mutex meshGenLock;
    condition_variable_any meshGenCond;
    bool needMeshes = true;
    int renderDistance = 20;
    PositionI pos(0, 0, 0, Dimension::Overworld);
    Mesh meshes[(int)RenderLayer::Last];
    thread renderThread(meshMakerThread, &done, (Mesh *)meshes, &needMeshes, &renderDistance, &pos, &meshGenCond, &meshGenLock, &client);

    while(!done)
    {
        meshGenLock.lock();
        needMeshes = true;
        meshGenCond.notify_all();
        meshGenLock.unlock();
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Matrix tform = Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::rotateY(theta)).concat(Matrix::rotateX(-phi));
        meshGenLock.lock();
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            Mesh mesh = meshes[(int)rl];
            renderLayerSetup(rl);
            r << transform(tform, mesh);
        }
        meshGenLock.unlock();
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
        Display::handleEvents(shared_ptr<EventHandler>(new ClientEventHandler(dtheta, dphi, paused)));
        theta = fmod(theta + dtheta / 2, M_PI * 2);
        phi = limit(phi + dphi / 2, -(float)M_PI / 2, (float)M_PI / 2);
        dtheta /= 2;
        dphi /= 2;
        if(paused == Display::grabMouse())
            Display::grabMouse(!paused);
    }
    meshGenCond.notify_all();
    networkThread.join();
    renderThread.join();
}

