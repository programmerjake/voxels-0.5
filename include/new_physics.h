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
#ifndef NEW_PHYSICS_H_INCLUDED
#define NEW_PHYSICS_H_INCLUDED

#error finish new_physics

#include "position.h"
#include "util.h"
#include "stream.h"
#include <list>
#include <memory>

namespace Physics
{
const constexpr float ContactEPS = 1e-3;

struct Properties final
{
    float mass, friction, bounciness;
    uint_fast32_t contactMask;
    static constexpr float INFINITE_MASS = 1e20;
    constexpr Properties(float mass, float friction, float bounciness, uint32_t contactMask = 1)
        : mass(mass), friction(friction), bounciness(bounciness), contactMask(contactMask)
    {
    }
    static Properties read(Reader &reader)
    {
        float mass, friction, bounciness;
        mass = reader.readLimitedF32(eps, INFINITE_MASS);
        friction = reader.readLimitedF32(0, 1);
        bounciness = reader.readLimitedF32(0, 1);
        uint32_t contactMask = reader.readU32();
        return Properties(mass, friction, bounciness, contactMask);
    }
    void write(Writer &writer)
    {
        writer.writeF32(mass);
        writer.writeF32(friction);
        writer.writeF32(bounciness);
        writer.writeU32(contactMask);
    }
};

struct Contact
{
    PositionF position;
    VectorF otherNormal = VectorF(0);
    Contact()
    {
    }
    Contact(PositionF position, VectorF otherNormal)
        : position(position), otherNormal(otherNormal)
    {
    }
    bool valid() const
    {
        return otherNormal != VectorF(0);
    }
};

struct Collision final : public Contact
{
    VectorF velocity;
    float time;
    Collision()
        : Contact(), time(-1)
    {
    }
    Collision(PositionF position, VectorF otherNormal, VectorF velocity, float time)
        : Contact(position, otherNormal), velocity(velocity), time(time)
    {
    }
};

struct Object final
{
public:
    Object()
    {
    }
    enum class Type
    {
        Empty,
        AABox,
    };
    virtual Type type() const = 0;
    double lastCalcTime = 0;
    Properties properties;
    VectorF gravity;
    virtual void calcPos(double currentTime) = 0;
    virtual Contact getContact(Object & other) = 0;
    virtual Collision getFirstCollision(Object & other) = 0;
};

struct Empty final : public Object
{
private:
    virtual Type type() const override
    {
        return Type::Empty;
    }
    virtual void calcPos(double currentTime)
    {
        lastCalcTime = currentTime;
    }
    virtual Contact getContact(Object & other) override
    {
        return Contact();
    }
    virtual Collision getFirstCollision(Object & other) override
    {
        return Collision();
    }
};

struct AABox final : public Object
{
    PositionF position;
    VectorF size;
    VectorF velocity;
    AABox(PositionF position, VectorF size, VectorF velocity)
    {
    }
    virtual Type type() const override
    {
        return Type::Empty;
    }
    virtual void calcPos(double currentTime)
    {
        float deltaTime = currentTime - lastCalcTime;
        lastCalcTime = currentTime;
        position += velocity * deltaTime + 0.5f * deltaTime * deltaTime * gravity;
        velocity += gravity * deltaTime;
    }
    virtual Contact getContact(Object & other) override;
    virtual Collision getFirstCollision(Object & other) override;
};

struct World final
{
    double currentTime = 0;
    list<shared_ptr<Object>> objects;
    void add(shared_ptr<Object> o)
    {
        assert(o);
        o->lastCalcTime = currentTime;
        objects.push_back(o);
    }
    void move(double runToTime)
    {
    }
};
}

#endif // NEW_PHYSICS_H_INCLUDED
