#ifndef PHYSICS_H_INCLUDED
#define PHYSICS_H_INCLUDED

#include "position.h"
#include "util.h"
#include "ray_casting.h"
#include <memory>

using namespace std;

struct PhysicsCollision final
{
    PositionF newPosition;
    VectorF newVelocity;
    bool valid;
    float time;
    PhysicsCollision()
        : valid(false), time(-1)
    {
    }
    PhysicsCollision(PositionF newPosition, VectorF newVelocity, float time)
        : newPosition(newPosition), newVelocity(newVelocity), valid(true), time(time)
    {
    }
};

struct PhysicsProperties final
{
    float mass, friction, bounciness;
    static constexpr float INFINITE_MASS = 1e20;
    PhysicsProperties(float mass, float friction, float bounciness)
        : mass(mass), friction(friction), bounciness(bounciness)
    {
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
private:
    VectorF center, extents;
    VectorF velocity;
public:
    PhysicsBox(VectorF center, VectorF extents/** size is 2 * extents */, Dimension dimension, PhysicsProperties properties)
        : PhysicsObject(dimension, properties), center(center), extents(extents)
    {
        assert(extents.x > eps && extents.y > eps && extents.z > eps);
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother, float maxTime) const override
    {
        if(pother == nullptr)
            return PhysicsCollision();
        if(pother->dimension() != dimension())
            return PhysicsCollision();
        switch(pother->type())
        {
        case Type::None:
            return PhysicsCollision();
        case Type::Box:
        {
            const PhysicsBox & other = *dynamic_cast<const PhysicsBox *>(pother.get());
            VectorF min = center - extents;
            VectorF max = center + extents;
            VectorF otherMin = other.center - other.extents;
            VectorF otherMax = other.center + other.extents;
            if(min.x < otherMax.x && min.y < otherMax.y && min.z < otherMax.z && max.x > otherMin.x && max.y > otherMin.y && max.z > otherMin.z) // intersects now
            {
                VectorF newVelocity = (velocity * properties().mass + other.velocity * other.properties().mass) / (properties().mass + other.properties().mass);
                VectorF deltaCenter = center - other.center;
                VectorF deltaFixedPosition;
                if(deltaCenter.x > 0)
                {
                    deltaFixedPosition.x = otherMax.x - min.x;
                }
                else
                {
                    deltaFixedPosition.x = otherMin.x - max.x;
                }
                if(deltaCenter.y > 0)
                {
                    deltaFixedPosition.y = otherMax.y - min.y;
                }
                else
                {
                    deltaFixedPosition.y = otherMin.y - max.y;
                }
                if(deltaCenter.z > 0)
                {
                    deltaFixedPosition.z = otherMax.z - min.z;
                }
                else
                {
                    deltaFixedPosition.z = otherMin.z - max.z;
                }
                VectorF fixedPosition = center;
                if(abs(deltaCenter.x) > abs(deltaCenter.y))
                {
                    if(abs(deltaCenter.x) > abs(deltaCenter.z))
                    {
                        fixedPosition.x += deltaFixedPosition.x;
                    }
                    else
                    {
                        fixedPosition.z += deltaFixedPosition.z;
                    }
                }
                else if(abs(deltaCenter.y) > abs(deltaCenter.z))
                {
                    fixedPosition.y += deltaFixedPosition.y;
                }
                else
                {
                    fixedPosition.z += deltaFixedPosition.z;
                }
                VectorF newPosition = (center * properties().mass + fixedPosition * other.properties().mass) / (properties().mass + other.properties().mass);
                return PhysicsCollision(PositionF(newPosition, dimension()), newVelocity, 0);
            }
            Ray ray(velocity - other.velocity, PositionF(center, dimension()));
            if(!ray.good())
                return PhysicsCollision();
            BoxRayCollision brc = rayHitBox(other.center - extents - other.extents, other.center + extents + other.extents, ray);
            if(!brc.good())
                return PhysicsCollision();
            float collisionTime = brc.t;
            VectorF collisionNormal = VectorF(dx(brc.enterFace), dy(brc.enterFace), dz(brc.enterFace));
            if(collisionTime > maxTime)
                return PhysicsCollision();
            VectorF nonReflectedVelocity = (velocity * properties().mass + other.velocity * other.properties().mass) / (properties().mass + other.properties().mass);
            VectorF reflectedVelocity = (velocity * (properties().mass - other.properties().mass) + (other.properties().mass + other.properties().mass) * other.velocity) / (properties().mass + other.properties().mass);
            VectorF partiallyReflectedVelocity = nonReflectedVelocity + properties().bounciness * other.properties().bounciness * (reflectedVelocity - nonReflectedVelocity);
            float reflectedDot = dot(partiallyReflectedVelocity, collisionNormal);
            VectorF frictionlessVelocity = velocity;
            VectorF stuckVelocity = nonReflectedVelocity;
            VectorF newVelocity = frictionlessVelocity + properties().friction * other.properties().friction * (stuckVelocity - frictionlessVelocity);
            newVelocity -= collisionNormal * dot(newVelocity, collisionNormal);
            newVelocity += collisionNormal * reflectedDot;
            VectorF newPosition = velocity * collisionTime + center;
            return PhysicsCollision(PositionF(newPosition, dimension()), newVelocity, collisionTime);
        }
        }
        assert(false);
        return PhysicsCollision();
    }
    virtual Type type() const override
    {
        return Type::Box;
    }
};

#endif // PHYSICS_H_INCLUDED
