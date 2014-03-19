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
#include "block.h"
#include "entity.h"
#include "render_object.h"
#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "chunk.h"
#include <unordered_map>
#include <mutex>

using namespace std;

const int WorldHeight = ChunkHeight;
const int AverageGroundHeight = 64;
constexpr VectorF gravityVector = VectorF(0, -9.8, 0);

class BlockIterator;
class World;

#include "world_generator.h"

class World final : public enable_shared_from_this<World>
{
    friend class Chunk;
    friend class BlockIterator;
    World(const World &) = delete;
    const World &operator =(const World &) = delete;
public:
    recursive_mutex lock;
    WorldRandom random;
    const WorldGenerator generator;
private:
    list<shared_ptr<Chunk>> chunksList;
    struct EntityCompare final
    {
        int operator()(shared_ptr<EntityData> a, const PositionF &b) const
        {
            if(a->position.x < b.x)
            {
                return -1;
            }

            if(a->position.x > b.x)
            {
                return 1;
            }

            if(a->position.y < b.y)
            {
                return -1;
            }

            if(a->position.y > b.y)
            {
                return 1;
            }

            if(a->position.z < b.z)
            {
                return -1;
            }

            if(a->position.z > b.z)
            {
                return 1;
            }

            if(a->position.d < b.d)
            {
                return -1;
            }

            if(a->position.d > b.d)
            {
                return 1;
            }

            return 0;
        }
        int operator()(shared_ptr<EntityData> a, shared_ptr<EntityData> b) const
        {
            int retval = operator()(a, b->position);
            if(retval == 0)
            {
                if(a < b)
                    return -1;
                if(a > b)
                    return 1;
                return 0;
            }
            return retval;
        }
    };
    balanced_tree<shared_ptr<EntityData>, EntityCompare> entities;
    vector<shared_ptr<RenderObjectEntity>> destroyedEntities;
    unordered_map<ChunkPosition, shared_ptr<Chunk>> chunksMap;
    shared_ptr<Chunk> getChunk(ChunkPosition pos)
    {
        lock.lock();
        shared_ptr<Chunk> &c = chunksMap[pos];

        if(c != nullptr)
        {
            lock.unlock();
            return c;
        }

        c = shared_ptr<Chunk>(new Chunk(pos));
        c->nx = chunksMap[pos.nx()];
        c->px = chunksMap[pos.px()];
        c->nz = chunksMap[pos.nz()];
        c->pz = chunksMap[pos.pz()];
        chunksList.push_back(c);
        lock.unlock();
        return c;
    }
    UpdateList clientsUpdates;
    World(uint32_t seed, const WorldGenerator &generator)
        : lock(), random(seed, lock), generator(generator)
    {
    }
public:
    UpdateList generatedChunks;
    UpdateList generatingChunks;
    UpdateList needGenerateChunks;
    void addGenerateChunk(PositionI pos)
    {
        lock_guard<recursive_mutex> lockIt(lock);

        if(generatedChunks.updatesSet.find(pos) != generatedChunks.updatesSet.end())
        {
            return;
        }

        if(generatingChunks.updatesSet.find(pos) != generatingChunks.updatesSet.end())
        {
            return;
        }

        needGenerateChunks.add(pos);
    }
    static shared_ptr<World> make(uint32_t seed = makeSeed(),
                                  const WorldGenerator &generator = WorldGenerator::makeDefault())
    {
        return shared_ptr<World>(new World(seed, generator));
    }
    ~World()
    {
    }
    void addUpdate(PositionI pos)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        clientsUpdates.add(pos);
    }
    BlockIterator get(PositionI pos);
    UpdateList copyOutUpdates()
    {
        lock_guard<recursive_mutex> lockIt(lock);
        UpdateList retval = std::move(clientsUpdates);
        clientsUpdates.clear();
        return std::move(retval);
    }
    vector<shared_ptr<RenderObjectEntity>> copyOutDestroyedEntities()
    {
        lock_guard<recursive_mutex> lockIt(lock);
        vector<shared_ptr<RenderObjectEntity>> retval = move(destroyedEntities);
        destroyedEntities.clear();
        return move(retval);
    }
    void merge(shared_ptr<World> world);
    shared_ptr<World> makeWorldForGenerate()
    {
        lock_guard<recursive_mutex> lockIt(lock);
        return make(random.seed, generator);
    }
    template <typename Function>
    int forEachEntityInRange(Function fn, VectorF min, VectorF max, Dimension d)
    {
        lock_guard<recursive_mutex> lockIt(lock);

        for(auto i = entities.rangeBegin(PositionF(min, d)), end = entities.rangeEnd(PositionF(max, d));
                i != end;)
        {
            shared_ptr<EntityData> e = *i;
            if(!e->good())
            {
                destroyedEntities.push_back(e->entity);
                i = entities.erase(i);
            }
            else if(e->position.d == d && e->position.y >= min.y && e->position.y <= max.y && e->position.z >= min.z
                    && e->position.z <= max.z)
            {
                i = entities.erase(i);
                int retval;

                try
                {
                    retval = fn(e);
                }
                catch(...)
                {
                    if(e->good())
                    {
                        entities.insert(e);
                    }
                    else
                    {
                        destroyedEntities.push_back(e->entity);
                    }

                    throw;
                }

                if(e->good())
                {
                    entities.insert(e);
                }
                else
                {
                    destroyedEntities.push_back(e->entity);
                }

                if(retval != 0)
                {
                    return retval;
                }
            }
            else
            {
                i++;
            }
        }

        return 0;
    }
    template <typename Function>
    int forEachEntity(Function fn)
    {
        lock_guard<recursive_mutex> lockIt(lock);

        for(auto i = entities.begin(), end = entities.end(); i != end;)
        {
            shared_ptr<EntityData> e = *i;
            if(!e->good())
            {
                destroyedEntities.push_back(e->entity);
                i = entities.erase(i);
            }
            else
            {
                i = entities.erase(i);
                int retval;

                try
                {
                    retval = fn(e);
                }
                catch(...)
                {
                if(e->good())
                {
                    entities.insert(e);
                }
                else
                {
                    destroyedEntities.push_back(e->entity);
                }

                    throw;
                }

                if(e->good())
                {
                    entities.insert(e);
                }
                else
                {
                    destroyedEntities.push_back(e->entity);
                }
                if(retval != 0)
                {
                    return retval;
                }
            }
        }

        return 0;
    }
    void addEntity(const EntityData &e)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        entities.insert(make_shared<EntityData>(e));
    }
    void addEntity(EntityData &&e)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        entities.insert(make_shared<EntityData>(move(e)));
    }
};

class BlockIterator final
{
    friend class World;
private:
    shared_ptr<World> worldInternal;
public:
    shared_ptr<World> world() const
    {
        return worldInternal;
    }
    operator bool() const
    {
        return world() != nullptr;
    }
    bool operator !() const
    {
        return world() == nullptr;
    }
    BlockIterator()
    {
    }
private:
    PositionI pos;
public:
    explicit operator PositionI() const
    {
        return pos;
    }
    PositionI position() const
    {
        return pos;
    }
private:
    shared_ptr<Chunk> chunk;
    BlockIterator(shared_ptr<World> w, PositionI pos);
    static BlockData makeBedrock();
    static BlockData makeLitAir();
public:
    BlockData get()
    {
        if(pos.y < 0)
        {
            return makeBedrock();
        }

        if(pos.y >= ChunkHeight)
        {
            return makeLitAir();
        }

        lock_guard<recursive_mutex> lock(world()->lock);
        VectorI rPos = (VectorI)pos - (VectorI)(PositionI)chunk->pos;
        return chunk->blocks[rPos.x][rPos.y][rPos.z];
    }
    void set(BlockData newBlock)
    {
        if(pos.y < 0)
        {
            return;
        }

        if(pos.y >= ChunkHeight)
        {
            return;
        }

        lock_guard<recursive_mutex> lock(world()->lock);
        VectorI rPos = (VectorI)pos - (VectorI)(PositionI)chunk->pos;
        chunk->blocks[rPos.x][rPos.y][rPos.z] = newBlock;
        world()->addUpdate(pos);
    }
    BlockIterator &operator =(VectorI newPos)
    {
        pos = PositionI(newPos, pos.d);
        ChunkPosition cPos(pos);

        if(chunk == nullptr || chunk->pos != cPos)
        {
            chunk = world()->getChunk(cPos);
        }

        return *this;
    }
    BlockIterator &operator +=(VectorI deltaPos)
    {
        shared_ptr<lock_guard<recursive_mutex>> lock = nullptr;
        pos += deltaPos;

        if(deltaPos.x < -ChunkSize || deltaPos.x > ChunkSize || deltaPos.z < -ChunkSize
                || deltaPos.z > ChunkSize)
        {
            return *this = pos;
        }

        if(pos.x < chunk->pos.x)
        {
            if(lock == nullptr)
            {
                lock = make_shared<lock_guard<recursive_mutex>>(world()->lock);
            }

            chunk = chunk->nx.lock();

            if(chunk == nullptr)
            {
                return *this = pos;
            }
        }

        if(pos.x >= chunk->pos.x + ChunkSize)
        {
            if(lock == nullptr)
            {
                lock = make_shared<lock_guard<recursive_mutex>>(world()->lock);
            }

            chunk = chunk->px.lock();

            if(chunk == nullptr)
            {
                return *this = pos;
            }
        }

        if(pos.z < chunk->pos.z)
        {
            if(lock == nullptr)
            {
                lock = make_shared<lock_guard<recursive_mutex>>(world()->lock);
            }

            chunk = chunk->nz.lock();

            if(chunk == nullptr)
            {
                return *this = pos;
            }
        }

        if(pos.z >= chunk->pos.z + ChunkSize)
        {
            if(lock == nullptr)
            {
                lock = make_shared<lock_guard<recursive_mutex>>(world()->lock);
            }

            chunk = chunk->pz.lock();

            if(chunk == nullptr)
            {
                return *this = pos;
            }
        }
        return *this;
    }
    BlockIterator &operator -=(VectorI v)
    {
        return operator +=(-v);
    }
    BlockIterator &operator +=(BlockFace face)
    {
        switch(face)
        {
        case BlockFace::NX:
        {
            if(pos.x-- == chunk->pos.x)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->nx.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::PX:
        {
            if(++pos.x == chunk->pos.x + ChunkSize)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->px.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::NY:
        {
            pos.y--;
            return *this;
        }

        case BlockFace::PY:
        {
            pos.y++;
            return *this;
        }

        case BlockFace::NZ:
        {
            if(pos.z-- == chunk->pos.z)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->nz.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::PZ:
        {
            if(++pos.z == chunk->pos.z + ChunkSize)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->pz.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }
        }

        assert(false);
        return *this;
    }
    BlockIterator &operator -=(BlockFace face)
    {
        switch(face)
        {
        case BlockFace::PX:
        {
            if(pos.x-- == chunk->pos.x)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->nx.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::NX:
        {
            if(++pos.x == chunk->pos.x + ChunkSize)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->px.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::PY:
        {
            pos.y--;
            return *this;
        }

        case BlockFace::NY:
        {
            pos.y++;
            return *this;
        }

        case BlockFace::PZ:
        {
            if(pos.z-- == chunk->pos.z)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->nz.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }

        case BlockFace::NZ:
        {
            if(++pos.z == chunk->pos.z + ChunkSize)
            {
                lock_guard<recursive_mutex> lockIt(world()->lock);
                chunk = chunk->pz.lock();

                if(chunk == nullptr)
                {
                    return *this = pos;
                }
            }

            return *this;
        }
        }

        assert(false);
        return *this;
    }
    void invalidate()
    {
        lock_guard<recursive_mutex> lock(world()->lock);
        world()->addUpdate(pos);
    }
    recursive_mutex &getWorldLock()
    {
        return world()->lock;
    }
};

inline BlockIterator World::get(PositionI pos)
{
    return BlockIterator(shared_from_this(), pos);
}

inline BlockIterator::BlockIterator(shared_ptr<World> w, PositionI pos)
    : worldInternal(w), pos(pos)
{
    ChunkPosition cPos(pos);
    chunk = world()->getChunk(cPos);
}

inline void World::merge(shared_ptr<World> world)
{
    lock_guard<recursive_mutex> lockIt(lock);
    lock_guard<recursive_mutex> lockOther(world->lock);
    clientsUpdates.merge(world->clientsUpdates);

    for(shared_ptr<Chunk> chunk : world->chunksList)
    {
        BlockIterator bi = get((PositionI)chunk->pos);
        BlockIterator bi2 = world->get((PositionI)chunk->pos);

        for(int x = 0; x < ChunkSize; x++)
        {
            for(int y = 0; y < ChunkHeight; y++)
            {
                for(int z = 0; z < ChunkSize; z++)
                {
                    bi = (PositionI)chunk->pos + VectorI(x, y, z);
                    bi2 = (PositionI)chunk->pos + VectorI(x, y, z);

                    if(bi2.get().good())
                    {
                        bi.set(bi2.get());
                    }
                }
            }
        }
    }

    for(auto i = world->entities.begin(); i != world->entities.end();)
    {
        entities.insert(*i);
        i = world->entities.erase(i);
    }
}

#endif // WORLD_H_INCLUDED
