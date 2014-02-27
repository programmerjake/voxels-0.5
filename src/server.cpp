#include "server.h"
#include "world.h"
#include "client.h"
#include "network_protocol.h"
#include "platform.h"
#include "builtin_blocks.h"
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
        retval = make_shared<PositionF>(PositionF(0.5, WorldHeight / 2 + 0.5, 0.5, Dimension::Overworld));
        id = client.makeId(retval, Client::DataType::PositionF);
        return *retval;
    }

    LockedClient lock(client);
    retval = client.getPtr<PositionF>(id, Client::DataType::PositionF);

    if(retval == nullptr)
    {
        retval = make_shared<PositionF>(PositionF(0.5, WorldHeight / 2 + 0.5, 0.5, Dimension::Overworld));
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
                break;
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
                break;
            }
            default:
                throw new runtime_error("Network Event not implemented");
            }
        }
    }
    catch(exception *e)
    {
        cerr << "Error : " << e->what() << endl;
        delete e;
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
    const int size = 50;
    cout << "connected\n";
    UpdateList &clientUpdateList = getClientUpdateList(client);
    PositionF &clientPosition = getClientPosition(client);

    UpdateList updateList;

    try
    {
        while(!terminated)
        {
            vector<shared_ptr<RenderObject>> objects;
            client.lock();
            PositionF curClientPosition = clientPosition;
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
                NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
                writer.writeU64(objects.size());
                for(shared_ptr<RenderObject> object : objects)
                {
                    object->write(writer, client);
                }
                writer.flush();
            }
            writer.flush();
        }
    }
    catch(exception *e)
    {
        cerr << "Error : " << e->what() << endl;
        delete e;
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

const int worldSize = 5;

void generateInitialWorld(shared_ptr<World> world)
{
    lock_guard<recursive_mutex> lockIt(world->lock);
    BlockIterator biX = world->get(PositionI(-worldSize, WorldHeight / 2 - worldSize, -worldSize, Dimension::Overworld));

    for(; biX.position().x <= worldSize; biX += BlockFace::PX)
    {
        BlockIterator biY = biX;

        for(; biY.position().y <= worldSize + WorldHeight / 2; biY += BlockFace::PY)
        {
            BlockIterator bi = biY;

            for(; bi.position().z <= worldSize; bi += BlockFace::PZ)
            {
                if(absSquared((VectorI)bi.position()) < worldSize * worldSize)
                {
                    bi.set(BlockData(AirBlock::ptr));
                }
                else
                {
                    bi.set(BlockData(StoneBlock::ptr));
                }
            }
        }
    }
    cout << "Server : world Generated\n";
}

void serverSimulateThreadFn(shared_ptr<list<shared_ptr<Client>>> clients, shared_ptr<World> world)
{
    try
    {
        Periodic periodic;
        generateInitialWorld(world);

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
            periodic.runAtFPS(20);
        }
    }
    catch(exception *e)
    {
        cerr << "fatal error : " << e->what() << endl;
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
            lock_guard<recursive_mutex> lockIt(world->lock);
            shared_ptr<Client> pclient = make_shared<Client>();
            clients->push_back(pclient);
            threads->push_back(thread(runServerWriterThread, stream, pclient, world));
            threads->push_back(thread(runServerReaderThread, stream, pclient, world));
        }
    }
    catch(NoStreamsLeftException *e)
    {
        delete e;
    }

    for(thread &t : *threads)
    {
        t.join();
    }

    serverSimulateThread.join();
}


