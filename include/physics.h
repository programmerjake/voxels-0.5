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
#ifndef PHYSICS_H_INCLUDED
#define PHYSICS_H_INCLUDED

#include "position.h"
#include "util.h"
#include "ray_casting.h"
#include "stream.h"
#include "client.h"
#include <memory>

using namespace std;

struct PhysicsCollision final
{
    PositionF newPosition;
    VectorF newVelocity;
    VectorF collisionNormal;
    bool valid;
    float time;
    PhysicsCollision()
        : valid(false), time(-1)
    {
    }
    PhysicsCollision(PositionF newPosition, VectorF newVelocity, VectorF collisionNormal, float time)
        : newPosition(newPosition), newVelocity(newVelocity), collisionNormal(collisionNormal), valid(true), time(time)
    {
    }
};

struct PhysicsProperties final
{
    float mass, friction, bounciness;
    static constexpr float INFINITE_MASS = 1e20;
    constexpr PhysicsProperties(float mass, float friction, float bounciness)
        : mass(mass), friction(friction), bounciness(bounciness)
    {
    }
    static PhysicsProperties read(Reader &reader)
    {
        float mass, friction, bounciness;
        mass = reader.readLimitedF32(eps, INFINITE_MASS);
        friction = reader.readLimitedF32(0, 1);
        bounciness = reader.readLimitedF32(0, 1);
        return PhysicsProperties(mass, friction, bounciness);
    }
    void write(Writer &writer)
    {
        writer.writeF32(mass);
        writer.writeF32(friction);
        writer.writeF32(bounciness);
    }
};

class PhysicsObject : public enable_shared_from_this<PhysicsObject>
{
    const PhysicsObject & operator =(const PhysicsObject & rt) = delete;
    PhysicsObject(PhysicsObject && rt) = delete;
    PhysicsObject(const PhysicsObject & rt) = default;
private:
    Dimension d;
    PhysicsProperties props;
public:
    Dimension dimension() const
    {
        return d;
    }
    const PhysicsProperties & properties() const
    {
        return props;
    }

    virtual ~PhysicsObject();
    PhysicsObject(Dimension dimension, PhysicsProperties properties)
        : d(dimension), props(properties)
    {
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother, float maxTime) const = 0;
    enum class Type
    {
        None,
        Box
    };
    virtual Type type() const = 0;
};

class PhysicsEmpty final : public PhysicsObject
{
public:
    PhysicsEmpty()
        : PhysicsObject(Dimension::Overworld, PhysicsProperties(1, 0, 0))
    {
    }
    virtual Type type() const override
    {
        return Type::None;
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject>, float) const
    {
        return PhysicsCollision();
    }
};

class PhysicsBox final : public PhysicsObject
{
public:
    VectorF center, extents;
    VectorF velocity, acceleration, deltaAcceleration, answerOffset;
    PhysicsBox(VectorF center, VectorF extents/** size is 2 * extents */, VectorF velocity, VectorF acceleration, VectorF deltaAcceleration, Dimension dimension, PhysicsProperties properties, VectorF answerOffset = VectorF(0))
        : PhysicsObject(dimension, properties), center(center), extents(extents), velocity(velocity), acceleration(acceleration), deltaAcceleration(deltaAcceleration), answerOffset(answerOffset)
    {
        assert(extents.x > eps && extents.y > eps && extents.z > eps);
    }
    void reInit(VectorF center, VectorF extents/** size is 2 * extents */, VectorF velocity, VectorF acceleration, VectorF deltaAcceleration)
    {
        this->center = center;
        this->extents = extents;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->deltaAcceleration = deltaAcceleration;
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother, float maxTime) const override;
    virtual Type type() const override
    {
        return Type::Box;
    }
};

inline bool isBoxCollision(VectorF center1, VectorF extents1, VectorF center2, VectorF extents2, VectorF & normal)
{
    VectorF extents = extents1 + extents2 + VectorF(eps);
    VectorF deltaPos = center1 - center2;
    VectorF aDeltaPos = VectorF(abs(deltaPos.x), abs(deltaPos.y), abs(deltaPos.z));
    if(aDeltaPos.x <= extents.x && aDeltaPos.y <= extents.y && aDeltaPos.z <= extents.z)
    {
        VectorF dist = extents - aDeltaPos;
        if(dist.x < dist.y && dist.x < dist.z)
        {
            if(deltaPos.x > 0)
                normal = VectorF(1, 0, 0);
            else
                normal = VectorF(-1, 0, 0);
        }
        else if(dist.y < dist.z)
        {
            if(deltaPos.y > 0)
                normal = VectorF(0, 1, 0);
            else
                normal = VectorF(0, -1, 0);
        }
        else
        {
            if(deltaPos.z > 0)
                normal = VectorF(0, 0, 1);
            else
                normal = VectorF(0, 0, -1);
        }
        return true;
    }
    return false;
}

struct PhysicsObjectConstructor final : public enable_shared_from_this<PhysicsObjectConstructor>
{
private:
    PhysicsProperties properties;
    VectorF extents, offset;
    enum class Type : uint8_t
    {
        Box,
        Empty,
        Last
    };
    const Type type;
    PhysicsObjectConstructor(Type type)
        : type(type)
    {
    }
public:
    static shared_ptr<PhysicsObjectConstructor> box(PhysicsProperties properties, VectorF extents, VectorF offset = VectorF(0))
    {
        shared_ptr<PhysicsObjectConstructor> retval(new PhysicsObjectConstructor(Type::Box));
        retval->properties = properties;
        retval->extents = extents;
        retval->offset = offset;
        return retval;
    }
    static shared_ptr<PhysicsObjectConstructor> empty()
    {
        return shared_ptr<PhysicsObjectConstructor>(new PhysicsObjectConstructor(Type::Empty));
    }
    shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity = VectorF(0), VectorF acceleration = VectorF(0), VectorF deltaAcceleration = VectorF(0))
    {
        switch(type)
        {
        case Type::Last:
            assert(false);
        case Type::Box:
            return shared_ptr<PhysicsObject>(new PhysicsBox(offset + (VectorF)position, extents, velocity, acceleration, deltaAcceleration, position.d, properties, -offset));
        case Type::Empty:
            return shared_ptr<PhysicsObject>(new PhysicsEmpty());
        }
        assert(false);
    }
    static shared_ptr<PhysicsObjectConstructor> read(Reader &reader)
    {
        Type type = (Type)reader.readLimitedU8(0, (uint8_t)Type::Last - 1);
        shared_ptr<PhysicsObjectConstructor> retval(new PhysicsObjectConstructor(type));
        switch(type)
        {
        case Type::Empty:
            return retval;
        default:
            break;
        }
        retval->properties = PhysicsProperties::read(reader);
        switch(type)
        {
        case Type::Empty:
        case Type::Last:
            break;
        case Type::Box:
            retval->extents.x = reader.readFiniteF32();
            retval->extents.y = reader.readFiniteF32();
            retval->extents.z = reader.readFiniteF32();
            retval->offset.x = reader.readFiniteF32();
            retval->offset.y = reader.readFiniteF32();
            retval->offset.z = reader.readFiniteF32();
            return retval;
        }
        assert(false);
    }
    void write(Writer &writer)
    {
        writer.writeU8(type);
        if(type == Type::Empty)
            return;
        properties.write(writer);
        switch(type)
        {
        case Type::Empty:
        case Type::Last:
            assert(false);
        case Type::Box:
            writer.writeF32(extents.x);
            writer.writeF32(extents.y);
            writer.writeF32(extents.z);
            writer.writeF32(offset.x);
            writer.writeF32(offset.y);
            writer.writeF32(offset.z);
            return;
        }
        assert(false);
    }
};

#endif // PHYSICS_H_INCLUDED
