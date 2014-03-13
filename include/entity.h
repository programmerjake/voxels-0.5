#ifndef ENTITY_H_INCLUDED
#define ENTITY_H_INCLUDED

#include "render_object.h"
#include <memory>
#include <cstdint>
#include <cwchar>
#include <string>
#include <map>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

class EntityDescriptor;

typedef shared_ptr<const EntityDescriptor> EntityDescriptorPtr;
typedef vector<EntityDescriptorPtr>::const_iterator EntityDescriptorPtrIterator;

class ExtraEntityData
{
public:
    virtual ~ExtraEntityData()
    {
    }
};

struct EntityData
{
    shared_ptr<const EntityDescriptor> desc;
    shared_ptr<RenderObjectEntity> entity;
    PositionF position;
    VectorF velocity;
    shared_ptr<ExtraEntityData> extraData;
    explicit EntityData(shared_ptr<const EntityDescriptor> desc = shared_ptr<EntityDescriptor>(), PositionF position = PositionF(), VectorF velocity = VectorF(0), shared_ptr<ExtraEntityData> extraData = nullptr)
        : desc(desc), position(position), velocity(velocity), extraData(extraData)
    {
    }
    bool good() const
    {
        return desc != nullptr;
    }
};

#include "world.h"
#include "game_stream.h"
#include "util.h"

class EntityDescriptor : public enable_shared_from_this<EntityDescriptor>
{
private:
    static map<wstring, EntityDescriptorPtr> *entities;
    static vector<EntityDescriptorPtr> *entitiesList;
protected:
    static void initBlock(EntityDescriptorPtr ed) /// call with all constructed EntityDescriptor daughter classes
    {
        if(entities == nullptr) // so that we don't have problems with static initialization order
        {
            entities = new map<wstring, EntityDescriptorPtr>;
            entitiesList = new vector<EntityDescriptorPtr>;
        }
        shared_ptr<const EntityDescriptor> & e = (*entities)[ed->name];
        if(e)
        {
            cerr << "Error : duplicate entity name : \"" << wcsrtombs(ed->name) << "\"\n";
            exit(1);
        }
        e = ed;
        entitiesList->push_back(ed);
    }
public:
    const wstring name;
private:
    friend class EntityDescriptors_t;
    static EntityDescriptorPtr getEntity(wstring name)
    {
        if(entities == nullptr) // so that we don't have problems with static initialization order
        {
            entities = new map<wstring, EntityDescriptorPtr>;
            entitiesList = new vector<EntityDescriptorPtr>;
        }
        if(entities->find(name) == entities->end())
            return nullptr;
        return entities->at(name);
    }
    static EntityDescriptorPtrIterator entitiesBegin()
    {
        if(entities == nullptr) // so that we don't have problems with static initialization order
        {
            entities = new map<wstring, EntityDescriptorPtr>;
            entitiesList = new vector<EntityDescriptorPtr>;
        }
        return entitiesList->cbegin();
    }
    static EntityDescriptorPtrIterator entitiesEnd()
    {
        if(entities == nullptr) // so that we don't have problems with static initialization order
        {
            entities = new map<wstring, EntityDescriptorPtr>;
            entitiesList = new vector<EntityDescriptorPtr>;
        }
        return entitiesList->cend();
    }
public:
    virtual ~EntityDescriptor()
    {
    }
    virtual shared_ptr<RenderObjectEntity> getEntity(EntityData & entity, World & world) const = 0;
protected:
    EntityDescriptor(wstring name)
        : name(name)
    {
    }
    virtual EntityData loadInternal(GameLoadStream & gls) const = 0;
    virtual void storeInternal(EntityData data, GameStoreStream & gss) const = 0;
public:
    virtual void onMove(EntityData & entity, World & world) const = 0;
    static EntityData load(GameLoadStream & gls)
    {
        return gls.readEntityDescriptor()->loadInternal(gls);
    }

    static void store(EntityData data, GameStoreStream & gss)
    {
        gss.writeEntityDescriptor(data.desc);
        data.desc->storeInternal(data, gss);
    }
};

struct EntityDescriptors_t final
{
    EntityDescriptorPtr get(wstring name) const
    {
        return EntityDescriptor::getEntity(name);
    }
    EntityDescriptorPtrIterator begin() const
    {
        return EntityDescriptor::entitiesBegin();
    }
    EntityDescriptorPtrIterator end() const
    {
        return EntityDescriptor::entitiesEnd();
    }
};

extern const EntityDescriptors_t EntityDescriptors;

inline EntityDescriptorPtrIterator begin(const EntityDescriptors_t & ed)
{
    return ed.begin();
}

inline EntityDescriptorPtrIterator end(const EntityDescriptors_t & ed)
{
    return ed.end();
}

#endif // ENTITY_H_INCLUDED
