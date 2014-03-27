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
#include "server.h"
#include "world.h"
#include "client.h"
#include "network_protocol.h"
#include "platform.h"
#include "builtin_blocks.h"
#include "builtin_entities.h"
#include "compressed_stream.h"
#include "generate.h"
#include "texture_atlas.h"
#include "player.h"
#include <thread>
#include <list>

using namespace std;

namespace
{
inline flag &getClientTerminatedFlag(Client &client)
{
    static Client::IdType terminatedId = Client::NullId;
    shared_ptr<flag> retval;

    if(terminatedId == Client::NullId)
    {
        retval = make_shared<flag>(false);
        terminatedId = client.makeId(retval, Client::DataType::ServerFlag);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<flag>(terminatedId, Client::DataType::ServerFlag);

    if(retval == nullptr)
    {
        retval = make_shared<flag>(false);
        client.setPtr(retval, terminatedId, Client::DataType::ServerFlag);
    }

    return *retval;
}
}

bool isClientValid(Client &client)
{
    return !getClientTerminatedFlag(client);
}

namespace
{
inline UpdateList &getClientUpdateList(Client &client)
{
    static Client::IdType id = Client::NullId;
    shared_ptr<UpdateList> retval;

    if(id == Client::NullId)
    {
        retval = make_shared<UpdateList>();
        id = client.makeId(retval, Client::DataType::UpdateList);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<UpdateList>(id, Client::DataType::UpdateList);

    if(retval == nullptr)
    {
        retval = make_shared<UpdateList>();
        client.setPtr(retval, id, Client::DataType::UpdateList);
    }

    return *retval;
}

inline PositionF &getClientPosition(Client &client)
{
    static Client::IdType id = Client::NullId;
    shared_ptr<PositionF> retval;

    if(id == Client::NullId)
    {
        retval = make_shared<PositionF>(PositionF(0.5, AverageGroundHeight + 0.5, 0.5,
                                        Dimension::Overworld));
        id = client.makeId(retval, Client::DataType::PositionF);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<PositionF>(id, Client::DataType::PositionF);

    if(retval == nullptr)
    {
        retval = make_shared<PositionF>(PositionF(0.5, AverageGroundHeight + 0.5, 0.5,
                                        Dimension::Overworld));
        client.setPtr(retval, id, Client::DataType::PositionF);
    }

    return *retval;
}

inline VectorF &getClientVelocity(Client &client)
{
    static Client::IdType id = Client::NullId;
    shared_ptr<VectorF> retval;

    if(id == Client::NullId)
    {
        retval = make_shared<VectorF>(VectorF(0));
        id = client.makeId(retval, Client::DataType::VectorF);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<VectorF>(id, Client::DataType::VectorF);

    if(retval == nullptr)
    {
        retval = make_shared<VectorF>(VectorF(0));
        client.setPtr(retval, id, Client::DataType::VectorF);
    }

    return *retval;
}

inline float &getClientViewPhi(Client &client)
{
    return client.getPropertyReference<float, 0>(Client::DataType::Float);
}

inline float &getClientViewTheta(Client &client)
{
    return client.getPropertyReference<float, 1>(Client::DataType::Float);
}

inline float &getClientViewDistance(Client &client)
{
    return client.getPropertyReference<float, 2>(Client::DataType::Float);
}

inline flag &getClientNeedStateFlag(Client &client)
{
    return client.getPropertyReference<flag, 0>(Client::DataType::ServerFlag, [](){return make_shared<flag>(true);});
}

inline flag &getClientGotStateFlag(Client &client)
{
    return client.getPropertyReference<flag, 1>(Client::DataType::ServerFlag);
}

void runServerReaderThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient,
                           shared_ptr<World> world)
{
    Reader &reader = connection->reader();
    Client &client = *pclient;
    flag &terminated = getClientTerminatedFlag(client);

    try
    {
        while(!terminated)
        {
            NetworkProtocol::NetworkEvent event = NetworkProtocol::readNetworkEvent(reader);

            switch(event)
            {
            case NetworkProtocol::NetworkEvent::UpdatePositionAndVelocity:
            {
                //cout << "Server : read position and velocity\n";
                PositionF pos;
                pos.x = reader.readFiniteF32();
                pos.y = reader.readFiniteF32();
                pos.z = reader.readFiniteF32();
                pos.d = reader.readDimension();
                VectorF velocity;
                velocity.x = reader.readFiniteF32();
                velocity.y = reader.readFiniteF32();
                velocity.z = reader.readFiniteF32();
                float phi, theta, viewDistance;
                phi = reader.readLimitedF32(-M_PI / 2 - eps, M_PI / 2 + eps);
                theta = reader.readLimitedF32(-2 * M_PI - eps, 2 * M_PI + eps);
                viewDistance = reader.readLimitedF32(0, 1000);
                bool flying = reader.readBool();
                float age = reader.readLimitedF32(0, 1e10);
                {
                    lock_guard<recursive_mutex> lockIt(world->lock);
                    shared_ptr<EntityData> player = EntityPlayer::get(client);
                    float serverAge = player->entity ? player->entity->age : 0;
                    EntityPlayer::update(player, pos, velocity, theta, phi, flying);
                }
                {
                    LockedClient lockIt(client);
                    getClientPosition(client) = pos;
                    getClientVelocity(client) = velocity;
                    getClientViewPhi(client) = phi;
                    getClientViewTheta(client) = theta;
                    getClientViewDistance(client) = viewDistance;
                    getClientGotStateFlag(client) = true;
                    getClientNeedStateFlag(client) = false;
                }
                continue;
            }

            case NetworkProtocol::NetworkEvent::RequestChunk:
            {
                PositionI origin;
                origin.x = reader.readS32();
                origin.y = reader.readS32();
                origin.z = reader.readS32();
                origin.d = reader.readDimension();
                int size = reader.readU32();
                {
                    lock_guard<recursive_mutex> lockIt(world->lock);
                    ChunkPosition cPos(origin);
                    world->addGenerateChunk((PositionI)cPos);
                }
                UpdateList &updateList = getClientUpdateList(client);

                for(int x = 0; x < size; x++)
                {
                    LockedClient lockIt(client);
                    for(int y = 0; y < size; y++)
                    {
                        for(int z = 0; z < size; z++)
                        {
                            updateList.add(origin + VectorI(x, y, z));
                        }
                    }
                }

                //cout << "Server : Got Chunk Request : " << origin.x << ", " << origin.y << ", " << origin.z << ", "
                //     << (int)origin.d << endl;
                continue;
            }

            case NetworkProtocol::NetworkEvent::Last:
                assert(false);
            }

            throw runtime_error("Network Event not implemented");
        }
    }
    catch(exception &e)
    {
        cerr << "Error : " << e.what() << endl;
    }

    terminated = true;
}

struct PlayerDistanceOrdering final
{
    const PositionF *pos;
    bool operator()(PositionI a, PositionI b)
    {
        float aDistanceSq = absSquared((VectorF)a - (VectorF) * pos);
        float bDistanceSq = absSquared((VectorF)b - (VectorF) * pos);

        if(a.d != pos->d)
        {
            aDistanceSq *= 50;
        }

        if(b.d != pos->d)
        {
            bDistanceSq *= 50;
        }

        return aDistanceSq < bDistanceSq;
    }
    PlayerDistanceOrdering(const PositionF *pos)
        : pos(pos)
    {
    }
};

void runServerWriterThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient,
                           shared_ptr<World> world)
{
    Writer &writer = connection->writer();
    Client &client = *pclient;
    flag &terminated = getClientTerminatedFlag(client);
    flag &needState = getClientNeedStateFlag(client);
    cout << "connected\n";
    UpdateList &clientUpdateList = getClientUpdateList(client);
    set<shared_ptr<RenderObjectEntity>> &entitiesList = client.getPropertyReference<set<shared_ptr<RenderObjectEntity>>, 0>(Client::DataType::RenderObjectEntitySet);
    //PositionF &clientPosition = getClientPosition(client);
    UpdateList updateList;
    unordered_set<PositionI> neededUpdates;

    try
    {
        {
            shared_ptr<RenderObjectEntity> roplayer;
            {
                LockedClient lockClient(client);
                lock_guard<recursive_mutex> lockWorld(world->lock);
                shared_ptr<EntityData> eplayer = EntityPlayer::get(client);
                EntityPlayer::update(eplayer, PositionF(0.5, 0.5 + AverageGroundHeight + 10, 0.5, Dimension::Overworld), VectorF(0), 0, 0, false);
                world->addEntity(eplayer);
                roplayer = eplayer->desc->getEntity(*eplayer, world);
            }
            NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::SendPlayer);
            roplayer->write(writer, client);
        }
#if 0
        {
            shared_ptr<RenderObjectEntityMesh> entityMesh = make_shared<RenderObjectEntityMesh>(VectorF(0),
                    VectorF(0));
            entityMesh->addPart(Generate::unitBox(TextureAtlas::Wool.td(), TextureAtlas::Wool.td(),
                                                  TextureAtlas::Wool.td(), TextureAtlas::Wool.td(), TextureAtlas::Wool.td(), TextureAtlas::Wool.td()),
                                Script::parse(
                                    L"io.transform = make_translate(<-0.5, -0.5, -0.5>) ~ make_rotatey(io.age / 5 * 2 * pi) ~ make_translate(io.position);io.colorR=io.colorG=1-(io.colorB=0.5+0.5*sin(io.age*2*pi))"));
            shared_ptr<RenderObjectEntity> entity = make_shared<RenderObjectEntity>(entityMesh, PositionF(0.5,
                                                    AverageGroundHeight + 10.5, 0.5, Dimension::Overworld), VectorF(0,-0.1,0), 0);
            NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
            writer.writeU64(1);
            entity->write(writer, client);
        }
#endif
        while(!terminated)
        {
            list<shared_ptr<RenderObject>> objects;
            bool didAnything = false;
            client.lock();
            //PositionF curClientPosition = clientPosition;
            updateList.merge(clientUpdateList);
            clientUpdateList.clear();
            for(auto e : entitiesList)
            {
                objects.push_back(e);
            }
            entitiesList.clear();
            client.unlock();

            if(!updateList.updatesList.empty())
            {
                bool locked = true;
                world->lock.lock();
                BlockIterator bi = world->get(updateList.updatesList.front());
                ssize_t count = 0;
                for(auto i = updateList.updatesList.begin(); i != updateList.updatesList.end();)
                {
                    PositionI pos = *i;
                    bi = pos;

                    if(count >= max<ssize_t>(50, 100 - (ssize_t)objects.size() / 2))
                    {
                        if(locked)
                        {
                            world->lock.unlock();
                            locked = false;
                        }
                        break;
                    }
                    else if(bi.get().good())
                    {
                        assert(locked);
                        shared_ptr<RenderObjectBlockMesh> mesh = bi.get().desc->getBlockMesh(bi);
                        shared_ptr<RenderObject> object = static_pointer_cast<RenderObject>(make_shared<RenderObjectBlock>
                                                          (mesh, pos, bi.get().light));
                        objects.push_back(object);
                        count++;
                        updateList.updatesSet.erase(pos);
                        i = updateList.updatesList.erase(i);
                    }
                    else
                        i++;
                }

                if(locked)
                    world->lock.unlock();
            }

            if(!objects.empty())
            {
                didAnything = true;
                //cout << "Server : writing " << objects.size() << " render objects\n";
                NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
                writer.writeU64(objects.size());

                for(shared_ptr<RenderObject> object : objects)
                {
                    object->write(writer, client);
                }
            }
            if(needState.exchange(false))
            {
                NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::RequestState);
                didAnything = true;
            }
            writer.flush();
            if(!didAnything)
                this_thread::sleep_for(chrono::milliseconds(1));
        }
    }
    catch(exception &e)
    {
        cerr << "Error : " << e.what() << endl;
    }

    terminated = true;
}

struct Periodic
{
    double lastFlipTime = -1e9;
    double deltaTime = 1 / 60.0;

    void runAtFPS(float fps)
    {
        double sleepTime;
        double curTime = Display::realtimeTimer();
        sleepTime = 1 / fps - (curTime - lastFlipTime);

        if(sleepTime <= eps)
        {
            if(lastFlipTime >= 0)
                deltaTime = curTime - lastFlipTime;
            lastFlipTime = curTime;
        }
        else
        {
            this_thread::sleep_for(chrono::nanoseconds(static_cast<int64_t>(sleepTime * 1e9)));
            curTime = Display::realtimeTimer();
            deltaTime = curTime - lastFlipTime;
            lastFlipTime = curTime;
        }
        //cout << "Delta Time : " << deltaTime << endl;
    }
};

struct ChunkGenerator
{
private:
    PositionI chunkOrigin;
    shared_ptr<World> world;
    void run()
    {
        shared_ptr<World> world2 = world->makeWorldForGenerate();
        lock_guard<recursive_mutex> lockIt(world2->lock);
        //world2->random.dump();
        world2->generator.run(world2, chunkOrigin);
        lock_guard<recursive_mutex> lockIt2(world->lock);
        world->merge(world2);
        world->generatedChunks.add(chunkOrigin);
        world->generatingChunks.remove(chunkOrigin);
        generated = true;
    }
    thread theThread;
    recursive_mutex lock;
    bool needJoin;
public:
    flag generated;
    bool start(PositionI pos)
    {
        lock_guard<recursive_mutex> lockIt(lock);

        if(!generated.exchange(false))
        {
            return false;
        }

        if(needJoin)
        {
            theThread.join();
        }

        needJoin = true;
        chunkOrigin = pos;
        theThread = thread([](ChunkGenerator * v)
        {
            v->run();
        }, this);
        return true;
    }
    ChunkGenerator(shared_ptr<World> world)
        : world(world), needJoin(false), generated(true)
    {
    }
    ~ChunkGenerator()
    {
        lock_guard<recursive_mutex> lockIt(lock);

        if(needJoin)
        {
            theThread.detach();
        }
    }
    bool inUse()
    {
        return !generated;
    }
};

void generateInitialWorld(shared_ptr<World> world)
{
#if 0
    {
        lock_guard<recursive_mutex> lockIt(world->lock);
        world->addEntity(EntityBlock::make(GlassBlock::ptr, PositionF(0, AverageGroundHeight, 0, Dimension::Overworld)));
    }
#endif
    vector<shared_ptr<ChunkGenerator>> generators;
    const int generateSize = 16;
    PositionI pos(0, 0, 0, Dimension::Overworld);

    for(pos.x = -generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.x;
            pos.x <= (generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.x);
            pos.x += WorldGeneratorPart::generateChunkSize.x)
    {
        for(pos.y = 0;
                pos.y < ((WorldHeight + WorldGeneratorPart::generateChunkSize.y - 1) &
                         WorldGeneratorPart::generateChunkSizeFloorMask.x); pos.y += WorldGeneratorPart::generateChunkSize.y)
        {
            for(pos.z = -generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.z;
                    pos.z <= (generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.z);
                    pos.z += WorldGeneratorPart::generateChunkSize.z)
            {
                shared_ptr<ChunkGenerator> generator = make_shared<ChunkGenerator>(world);
                bool successful = generator->start(pos);
                assert(successful);
                generators.push_back(generator);
            }
        }
    }

    for(shared_ptr<ChunkGenerator> generator : generators)
    {
        generator->generated.wait(true);
    }

    cout << "Server : world Generated\n";
}

void serverSimulateThreadFn(shared_ptr<list<shared_ptr<Client>>> clients, shared_ptr<World> world)
{
    array<shared_ptr<ChunkGenerator>, GenerateThreadCount> generators;

    try
    {
        Periodic periodic;
        generateInitialWorld(world);
        uint64_t frame = 0;
        set<shared_ptr<EntityData>> entitiesSet;

        while(true)
        {
            {
                lock_guard<recursive_mutex> lockIt(world->lock);
                UpdateList updateList = world->copyOutUpdates();
                vector<shared_ptr<RenderObjectEntity>> destroyedEntities = world->copyOutDestroyedEntities();
                vector<shared_ptr<EntityData>> playerEntities;

                for(auto i = clients->begin(); i != clients->end();)
                {
                    Client & client = **i;
                    LockedClient lockClient(client);
                    if(getClientTerminatedFlag(client))
                        i = clients->erase(i);
                    else
                    {
                        playerEntities.push_back(EntityPlayer::get(client));
                        i++;
                    }
                }

                for(shared_ptr<Client> pclient : *clients)
                {
                    LockedClient lockClient(*pclient);
                    if(getClientGotStateFlag(*pclient))
                    {
                        getClientNeedStateFlag(*pclient) = true;
                        getClientGotStateFlag(*pclient) = false;
                    }
#if 1
                    if(frame % 40 == 0)
                    {
                        BlockDescriptorPtr block;
                        if(rand() % 3 == 0)
                            block = BlockDescriptors.get(L"builtin.bedrock");
                        else if(rand() % 2 == 0)
                            block = BlockDescriptors.get(L"builtin.stone");
                        else
                            block = BlockDescriptors.get(L"builtin.glass");
                        VectorF lookDir = Matrix::rotateX(getClientViewPhi(*pclient)).concat(Matrix::rotateY(-getClientViewTheta(*pclient))).apply(VectorF(0, 0, -1));
                        for(int i = 5; i <= 50; i++)
                            world->addEntity(EntityBlock::make(block, getClientPosition(*pclient) + lookDir, (i) * lookDir));
                    }
#endif
                    UpdateList &cul = getClientUpdateList(*pclient);
                    set<shared_ptr<RenderObjectEntity>> &entitiesList = pclient->getPropertyReference<set<shared_ptr<RenderObjectEntity>>, 0>(Client::DataType::RenderObjectEntitySet);
                    cul.merge(updateList);
                    for(auto e : destroyedEntities)
                    {
                        entitiesList.insert(e);
                    }
                    for(auto e : playerEntities)
                    {
                        entitiesList.insert(e->desc->getEntity(*e, world));
                    }
                    PositionF &clientPosition = getClientPosition(*pclient);
                    VectorF min = (VectorF)clientPosition - VectorF(getClientViewDistance(*pclient));
                    VectorF max = (VectorF)clientPosition + VectorF(getClientViewDistance(*pclient));
                    world->forEachEntityInRange([&entitiesList, world](shared_ptr<EntityData> e)->int
                    {
                        entitiesList.insert(e->desc->getEntity(*e, world));
                        return 0;
                    }, min, max, clientPosition.d);
                    for(auto e : pclient->getAllPtrs<RenderObjectEntity>(Client::DataType::RenderObjectEntity))
                    {
                        if(!e->good())
                            entitiesList.insert(e);
                        else if((e->position.x < min.x || e->position.x > max.x || e->position.y < min.y || e->position.y > max.y || e->position.z < min.z || e->position.z > max.z || e->position.d != clientPosition.d) && !e->isPlayer())
                        {
                            e->clear();
                            entitiesList.insert(e);
                        }
                    }
                }
            }

            float deltaTime = periodic.deltaTime;
            entitiesSet.clear();
            world->forEachEntity([world, &entitiesSet, deltaTime](shared_ptr<EntityData> e)->int
            {
                if(get<1>(entitiesSet.insert(e)))
                    e->desc->onMove(*e, world, deltaTime);
                return 0;
            });

            for(shared_ptr<ChunkGenerator> &generator : generators)
            {
                if(generator != nullptr && generator->inUse())
                {
                    continue;
                }

                lock_guard<recursive_mutex> lockIt(world->lock);

                if(generator == nullptr)
                {
                    generator = make_shared<ChunkGenerator>(world);
                }

                UpdateList &needGeneratedChunks = world->needGenerateChunks;
                UpdateList &generatingChunks = world->generatingChunks;

                if(needGeneratedChunks.empty())
                {
                    break;
                }

                PositionI pos = needGeneratedChunks.updatesList.front();
                needGeneratedChunks.remove(pos);
                generatingChunks.add(pos);
                bool retval = generator->start(pos);
                assert(retval);
            }

            periodic.runAtFPS(20);
            frame++;
            //cout << "server frame : " << frame << endl;
        }
    }
    catch(exception &e)
    {
        cerr << "fatal error : " << e.what() << endl;
        exit(1);
    }
}
}

void runServer(StreamServer &server)
{
    shared_ptr<list<thread>> threads = make_shared<list<thread>>();
    shared_ptr<list<shared_ptr<Client>>> clients = make_shared<list<shared_ptr<Client>>>();
    shared_ptr<World> world = World::make();
    thread serverSimulateThread(serverSimulateThreadFn, clients, world);

    try
    {
        while(true)
        {
            shared_ptr<StreamRW> stream = server.accept();
            //stream = shared_ptr<StreamRW>(new StreamRWWrapper(shared_ptr<Reader>(new ExpandReader(stream->preader())), shared_ptr<Writer>(new CompressWriter(stream->pwriter()))));
            lock_guard<recursive_mutex> lockIt(world->lock);
            shared_ptr<Client> pclient = make_shared<Client>();
            clients->push_back(pclient);
            threads->push_back(thread(runServerWriterThread, stream, pclient, world));
            threads->push_back(thread(runServerReaderThread, stream, pclient, world));
        }
    }
    catch(NoStreamsLeftException &e)
    {
        // do nothing
    }

    for(thread &t : *threads)
    {
        t.join();
    }

    serverSimulateThread.join();
}


