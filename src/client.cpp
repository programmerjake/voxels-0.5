#include "client.h"
#include "network_protocol.h"
#include "render_object.h"
#include "platform.h"
#include "game_version.h"
#include "text.h"
#include "world.h"
#include "compressed_stream.h"
#include <thread>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>

using namespace std;

atomic_uint_fast64_t Client::nextId(1);

struct ClientState
{
    float theta, phi;
    float dtheta, dphi;
    int renderDistance;
    PositionF pos;
    VectorF velocity;
    bool needMeshes;
    Client client;
    recursive_mutex & lock;
    condition_variable_any cond;
    bool done;
    bool paused;
    UpdateList neededChunkList;
    bool forwardDown, backwardDown, leftDown, rightDown, jumpDown, sneakDown;
    ClientState()
        : lock(client.getLock())
    {
        pos = PositionF(0.5, AverageGroundHeight + 5.5, 0.5, Dimension::Overworld);
        velocity = VectorF(0);
        theta = phi = dtheta = dphi = 0;
        renderDistance = 30;
        needMeshes = true;
        done = false;
        paused = GameVersion::DEBUG;
    }
};

namespace ClientImplementation
{
void clientProcessWriter(Writer *pwriter, ClientState * state)
{
    Writer &writer = *pwriter;
    //Client &client = state->client;
    unordered_set<PositionI> chunksAlreadyRequsted;

    try
    {
        state->lock.lock();
        while(!state->done)
        {
            PositionF pos = state->pos;
            VectorF velocity = state->velocity;
            if(state->paused)
                velocity = VectorF(0);
            UpdateList neededChunkList = state->neededChunkList;
            state->neededChunkList.clear();
            state->lock.unlock();
            NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdatePositionAndVelocity);
            writer.writeF32(pos.x);
            writer.writeF32(pos.y);
            writer.writeF32(pos.z);
            writer.writeDimension(pos.d);
            writer.writeF32(velocity.x);
            writer.writeF32(velocity.y);
            writer.writeF32(velocity.z);
            writer.flush();
            for(PositionI p : neededChunkList.updatesList)
            {
                if(!get<1>(chunksAlreadyRequsted.insert(p)))
                    continue;
                NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::RequestChunk);
                writer.writeS32(p.x);
                writer.writeS32(p.y);
                writer.writeS32(p.z);
                writer.writeDimension(p.d);
                writer.writeU32(RenderObjectWorld::Chunk::size);
                writer.flush();
                //cout << "Client : send chunk request\n";
            }
            this_thread::sleep_for(chrono::milliseconds(50));
            state->lock.lock();
        }
        state->lock.unlock();
    }
    catch(exception &e)
    {
        cerr << e.what() << "\n";
    }

    state->lock.lock();
    state->done = true;
    state->lock.unlock();
}

void clientProcessReader(Reader *preader, ClientState * state)
{
    Reader &reader = *preader;
    Client &client = state->client;
    NetworkProtocol::NetworkEvent event;

    try
    {
        state->lock.lock();
        while(!state->done)
        {
            state->lock.unlock();
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
                cout << "Client : received " << readCount << " updated render objects\n";
                state->lock.lock();
                continue;
            }

            case NetworkProtocol::NetworkEvent::Last:
                assert(false);
            }
            assert(false);
        }
        state->lock.unlock();
    }
    catch(exception &e)
    {
        cerr << e.what() << "\n";
    }

    state->lock.lock();
    state->done = true;
    state->lock.unlock();
}

Mesh makeChunkRenderMesh(Client &client, shared_ptr<unordered_set<PositionI>> neededChunks, PositionI chunkPos, RenderLayer rl)
{
    LockedClient lock(client);
    shared_ptr<RenderObjectWorld> world = RenderObjectWorld::getWorld(client);
    RenderObjectWorld::BlockIterator biX(world, chunkPos);
    shared_ptr<RenderObjectWorld::Chunk> chunk = biX.getChunk();
    if(chunk->retrieved < RenderObjectWorld::Chunk::size * RenderObjectWorld::Chunk::size * RenderObjectWorld::Chunk::size)
        neededChunks->insert(chunk->pos);

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

Mesh makeRenderMesh(Client &client, shared_ptr<unordered_set<PositionI>> neededChunks, RenderLayer rl, PositionI pos, int renderDistance = 40)
{
    Mesh retval = Mesh(new Mesh_t);

    int minX = (pos.x - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;
    int minY = (pos.y - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;
    int minZ = (pos.z - renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;
    int maxX = (pos.x + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;
    int maxY = (pos.y + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;
    int maxZ = (pos.z + renderDistance) & RenderObjectWorld::Chunk::floor_size_mask;

    for(int x = minX; x <= maxX; x+=RenderObjectWorld::Chunk::size)
    {
        for(int y = minY; y <= maxY; y+=RenderObjectWorld::Chunk::size)
        {
            for(int z = minZ; z <= maxZ; z+=RenderObjectWorld::Chunk::size)
            {
                retval->add(makeChunkRenderMesh(client, neededChunks, PositionI(x, y, z, pos.d), rl));
            }
        }
    }

    return retval;
}

void meshMakerThread(Mesh meshes[], ClientState * state)
{
    state->lock.lock();
    while(!state->done)
    {
        while(!state->done && !state->needMeshes)
            state->cond.wait(state->lock);
        if(state->done)
            break;
        state->needMeshes = false;
        Client & client = state->client;
        int renderDistance = state->renderDistance;
        PositionI pos = (PositionI)state->pos;
        state->lock.unlock();
        Mesh tempMeshes[(int)RenderLayer::Last];
        shared_ptr<unordered_set<PositionI>> neededChunks = make_shared<unordered_set<PositionI>>();
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            tempMeshes[(int)rl] = makeRenderMesh(client, neededChunks, rl, pos, renderDistance);
        }
        state->lock.lock();
        for(PositionI p : *neededChunks)
        {
            state->neededChunkList.add(p);
        }
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            meshes[(int)rl] = tempMeshes[(int)rl];
        }
    }
    state->lock.unlock();
}

void updateVelocity(ClientState * state)
{
    lock_guard<recursive_mutex> lockIt(state->lock);
    state->velocity = VectorF(0, state->velocity.y, 0);
    const float speed = 2;
    VectorF forwardVector = Matrix::rotateY(state->theta).invert().apply(VectorF(0, 0, -1)) * speed;
    VectorF leftVector = Matrix::rotateY(state->theta).invert().apply(VectorF(-1, 0, 0)) * speed;
    if(state->backwardDown)
    {
        state->velocity -= forwardVector;
    }
    if(state->forwardDown)
    {
        state->velocity += forwardVector;
    }
    if(state->leftDown)
    {
        state->velocity += leftVector;
    }
    if(state->rightDown)
    {
        state->velocity -= leftVector;
    }
    if(!state->paused)
        state->pos += Display::frameDeltaTime() * state->velocity;
}

class ClientEventHandler final : public EventHandler
{
    ClientState * clientState;
public:
    ClientEventHandler(ClientState * clientState)
        : clientState(clientState)
    {
    }
    virtual bool handleMouseUp(MouseUpEvent &) override
    {
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &) override
    {
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        lock_guard<recursive_mutex> lockIt(clientState->lock);
        if(clientState->paused)
            return true;
        if(event.deltaX > 0)
            clientState->dtheta += limit(event.deltaX, 0.0f, 10.0f) * event.deltaX / 1000.0 * M_PI;
        else
            clientState->dtheta -= limit(event.deltaX, -10.0f, 0.0f) * event.deltaX / 1000.0 * M_PI;
        if(event.deltaY > 0)
            clientState->dphi -= limit(event.deltaY, 0.0f, 10.0f) * event.deltaY / 1000.0 * M_PI;
        else
            clientState->dphi += limit(event.deltaY, -10.0f, 0.0f) * event.deltaY / 1000.0 * M_PI;
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &) override
    {
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent & event) override
    {
        lock_guard<recursive_mutex> lockIt(clientState->lock);
        if(clientState->paused)
            return false;
        if(event.key == KeyboardKey::KeyboardKey_W)
        {
            clientState->forwardDown = false;
        }
        if(event.key == KeyboardKey::KeyboardKey_A)
        {
            clientState->leftDown = false;
        }
        if(event.key == KeyboardKey::KeyboardKey_D)
        {
            clientState->rightDown = false;
        }
        if(event.key == KeyboardKey::KeyboardKey_S)
        {
            clientState->backwardDown = false;
        }
        return false;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        lock_guard<recursive_mutex> lockIt(clientState->lock);
        if(event.key == KeyboardKey::KeyboardKey_P && !event.isRepetition)
        {
            clientState->paused = !clientState->paused;
        }
        if(clientState->paused)
            return false;
        if(event.key == KeyboardKey::KeyboardKey_W)
        {
            clientState->forwardDown = true;
        }
        if(event.key == KeyboardKey::KeyboardKey_A)
        {
            clientState->leftDown = true;
        }
        if(event.key == KeyboardKey::KeyboardKey_D)
        {
            clientState->rightDown = true;
        }
        if(event.key == KeyboardKey::KeyboardKey_S)
        {
            clientState->backwardDown = true;
        }
        return false;
    }
    virtual bool handleKeyPress(KeyPressEvent &) override
    {
        return true;
    }
    virtual bool handleQuit(QuitEvent &) override
    {
        return false; // so that the program will actually quit
    }
};
}

using namespace ClientImplementation;

void clientProcess(StreamRW & streamRW)
{
    startGraphics();
    shared_ptr<Reader> preader = streamRW.preader();
    shared_ptr<Writer> pwriter = streamRW.pwriter();
    //preader = shared_ptr<Reader>(new ExpandReader(preader));
    //pwriter = shared_ptr<Writer>(new CompressWriter(pwriter));
    Reader &reader = *preader;
    Writer &writer = *pwriter;
    ClientState clientState;
    thread networkReaderThread(clientProcessReader, &reader, &clientState);
    thread networkWriterThread(clientProcessWriter, &writer, &clientState);
    Renderer r;
    Mesh meshes[(int)RenderLayer::Last];
    for(Mesh & m : meshes)
    {
        m = Mesh(new Mesh_t);
    }
    thread renderThread(meshMakerThread, (Mesh *)meshes, &clientState);

    clientState.lock.lock();
    while(!clientState.done)
    {
        clientState.needMeshes = true;
        clientState.cond.notify_all();
        clientState.lock.unlock();
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        clientState.lock.lock();
        updateVelocity(&clientState);
        Matrix tform = Matrix::translate(-(VectorF)clientState.pos)
                        .concat(Matrix::rotateY(clientState.theta))
                        .concat(Matrix::rotateX(-clientState.phi));
        Mesh tempMeshes[(int)RenderLayer::Last];
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            tempMeshes[(int)rl] = meshes[(int)rl];
        }
        clientState.lock.unlock();
        int polyCount = 0;
        for(RenderLayer rl = (RenderLayer)0; rl < RenderLayer::Last; rl = (RenderLayer)((int)rl + 1))
        {
            Mesh mesh = tempMeshes[(int)rl];
            assert(mesh);
            polyCount += mesh->size();
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

        s << L"\nFPS : " << Display::averageFPS() << endl;
        s << L"Triangle Count : " << polyCount << endl;
        r << transform(Matrix::translate(-40 * Display::scaleX(), 40 * Display::scaleY() - Text::height(s.str()), -40 * Display::scaleX()), Text::mesh(s.str(), Color(1, 0, 1)));
        Display::flip();
        Display::handleEvents(shared_ptr<EventHandler>(new ClientEventHandler(&clientState)));
        clientState.lock.lock();
        clientState.theta = fmod(clientState.theta + clientState.dtheta / 2, M_PI * 2);
        clientState.phi = limit(clientState.phi + clientState.dphi / 2, -(float)M_PI / 2, (float)M_PI / 2);
        clientState.dtheta /= 2;
        clientState.dphi /= 2;
        bool paused = clientState.paused;
        clientState.lock.unlock();
        if(paused == Display::grabMouse())
            Display::grabMouse(!paused);
        clientState.lock.lock();
        clientState.cond.notify_all();
    }
    clientState.cond.notify_all();
    clientState.lock.unlock();
    networkReaderThread.join();
    networkWriterThread.join();
    renderThread.join();
}

