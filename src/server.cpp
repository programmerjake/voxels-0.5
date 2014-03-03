#include "server.h"
#include "world.h"
#include "client.h"
#include "network_protocol.h"
#include "platform.h"
#include "builtin_blocks.h"
#include "compressed_stream.h"
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
        retval = make_shared<PositionF>(PositionF(0.5, AverageGroundHeight + 0.5, 0.5, Dimension::Overworld));
        id = client.makeId(retval, Client::DataType::PositionF);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<PositionF>(id, Client::DataType::PositionF);

    if(retval == nullptr)
    {
        retval = make_shared<PositionF>(PositionF(0.5, AverageGroundHeight + 0.5, 0.5, Dimension::Overworld));
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

void runServerReaderThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient, shared_ptr<World> world)
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
                LockedClient lockIt(client);
                getClientPosition(client) = pos;
                getClientVelocity(client) = velocity;
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
                LockedClient lockIt(client);
                UpdateList & updateList = getClientUpdateList(client);
                for(int x = 0; x < size; x++)
                {
                    for(int y = 0; y < size; y++)
                    {
                        for(int z = 0; z < size; z++)
                        {
                            updateList.add(origin + VectorI(x, y, z));
                        }
                    }
                }
                cout << "Server : Got Chunk Request : " << origin.x << ", " << origin.y << ", " << origin.z << ", " << (int)origin.d << endl;
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
    const PositionF * pos;
    bool operator ()(PositionI a, PositionI b)
    {
        float aDistanceSq = absSquared((VectorF)a - (VectorF)*pos);
        float bDistanceSq = absSquared((VectorF)b - (VectorF)*pos);
        if(a.d != pos->d)
            aDistanceSq *= 50;
        if(b.d != pos->d)
            bDistanceSq *= 50;
        return aDistanceSq < bDistanceSq;
    }
    PlayerDistanceOrdering(const PositionF * pos)
        : pos(pos)
    {
    }
};

void runServerWriterThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient, shared_ptr<World> world)
{
    Writer &writer = connection->writer();
    Client &client = *pclient;
    flag &terminated = getClientTerminatedFlag(client);
    cout << "connected\n";
    UpdateList &clientUpdateList = getClientUpdateList(client);
    //PositionF &clientPosition = getClientPosition(client);

    UpdateList updateList;

    try
    {
        while(!terminated)
        {
            list<shared_ptr<RenderObject>> objects;
            client.lock();
            //PositionF curClientPosition = clientPosition;
            updateList = clientUpdateList;
            clientUpdateList.clear();
            client.unlock();
            if(!updateList.updatesList.empty())
            {
                world->lock.lock();
                BlockIterator bi = world->get(updateList.updatesList.front());
                UpdateList newClientUpdateList;
                for(PositionI pos : updateList.updatesList)
                {
                    bi = pos;
                    if(bi.get().good())
                    {
                        shared_ptr<RenderObjectBlockMesh> mesh = bi.get().desc->getBlockMesh(bi);
                        shared_ptr<RenderObject> object = static_pointer_cast<RenderObject>(make_shared<RenderObjectBlock>(mesh, pos));
                        objects.push_back(object);
                    }
                    else
                    {
                        newClientUpdateList.add(pos);
                    }
                }
                world->lock.unlock();
                client.lock();
                clientUpdateList.merge(newClientUpdateList);
                client.unlock();
            }

            if(!objects.empty())
            {
                cout << "Server : writing " << objects.size() << " render objects\n";
                NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
                writer.writeU64(objects.size());
                for(shared_ptr<RenderObject> object : objects)
                {
                    object->write(writer, client);
                }
                writer.flush();
            }
            else
                writer.flush();
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

    void runAtFPS(float fps)
    {
        double sleepTime;
        double curTime = Display::realtimeTimer();
        sleepTime = 1 / fps - (curTime - lastFlipTime);

        if(sleepTime <= eps)
        {
            lastFlipTime = curTime;
        }

        if(sleepTime > eps)
        {
            this_thread::sleep_for(chrono::nanoseconds(static_cast<int64_t>(sleepTime * 1e9)));
            lastFlipTime = Display::realtimeTimer();
        }
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
            return false;
        if(needJoin)
            theThread.join();
        needJoin = true;
        chunkOrigin = pos;
        theThread = thread([](ChunkGenerator * v){v->run();}, this);
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
            theThread.detach();
    }
    bool inUse()
    {
        return !generated;
    }
};

void generateInitialWorld(shared_ptr<World> world)
{
    vector<shared_ptr<ChunkGenerator>> generators;
    const int generateSize = 32;
    PositionI pos(0, 0, 0, Dimension::Overworld);
    for(pos.x = -generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.x; pos.x <= (generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.x); pos.x += WorldGeneratorPart::generateChunkSize.x)
    {
        for(pos.y = 0; pos.y < ((WorldHeight + WorldGeneratorPart::generateChunkSize.y - 1) & WorldGeneratorPart::generateChunkSizeFloorMask.x); pos.y += WorldGeneratorPart::generateChunkSize.y)
        {
            for(pos.z = -generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.z; pos.z <= (generateSize & WorldGeneratorPart::generateChunkSizeFloorMask.z); pos.z += WorldGeneratorPart::generateChunkSize.z)
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

        while(true)
        {
            {
                lock_guard<recursive_mutex> lockIt(world->lock);
                UpdateList updateList = world->copyOutUpdates();

                for(shared_ptr<Client> pclient : *clients)
                {
                    UpdateList &cul = getClientUpdateList(*pclient);
                    cul.merge(updateList);
                }
            }
#if 0
            if(frame % 1 == 0)
            {
                lock_guard<recursive_mutex> lockIt(world->lock);
                for(int i = 0; i < 1; i++)
                {
                    PositionI pos = PositionI(rand() % 33 - 16, rand() % 33 - 16 + AverageGroundHeight, rand() % 33 - 16, Dimension::Overworld);
                    if(absSquared(pos - PositionI(0, AverageGroundHeight, 0, Dimension::Overworld)) > 5)
                    {
                        BlockIterator bi = world->get(pos);
                        switch(rand() % 4)
                        {
                        case 0:
                            bi.set(BlockData(BlockDescriptor::getBlock(L"builtin.air")));
                            break;
                        case 1:
                            bi.set(BlockData(BlockDescriptor::getBlock(L"builtin.stone")));
                            break;
                        case 2:
                            bi.set(BlockData(BlockDescriptor::getBlock(L"builtin.glass")));
                            break;
                        default:
                            bi.set(BlockData(BlockDescriptor::getBlock(L"builtin.bedrock")));
                            break;
                        }
                    }
                }
            }
#endif
            periodic.runAtFPS(20);
            frame++;
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


