#ifndef PHYSICS_H_INCLUDED
#define PHYSICS_H_INCLUDED

#include "position.h"
#include "util.h"
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
    static constexpr INFINITE_MASS = 1e20;
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
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother) const = 0;
    enum final Type
    {
        Box
    };
    virtual Type type() const = 0;
};

class PhysicsBox final : public PhysicsObject
{
private:
    VectorF center, extents;
    VectorF velocity;
public:
    PhysicsBox(VectorF center, VectorF extents, Dimension dimension, PhysicsProperties properties)
        : PhysicsObject(dimension, properties), center(center), extents(extents)
    {
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother) const override
    {
        if(movable == false)
            return PhysicsCollision();
        if(other == nullptr)
            return PhysicsCollision();
        if(other->dimension() != dimension())
            return PhysicsCollision();
        switch(other->type())
        {
        case Type::Box:
        {
            const PhysicsBox & other = *dynamic_cast<const PhysicsBox *>(pother.get());
            VectorF min = center - extents;
            VectorF max = center + extents;
            VectorF otherMin = other.center - other.extents;
            VectorF otherMax = other.center + other.extents;
            if(min.x < otherMax.x && min.y < otherMax.y && min.z < otherMax.z && max.x > otherMin.x && max.y > otherMin.y && max.z > otherMin.z) // intersects now
            {
                VectorF newVelocity = VectorF(0);
                VectorF newPosition;
                if(other.movable)
                {
                    newVelocity = (velocity * mass + other.velocity * other.mass) / (mass + other.mass);
                    VectorF newPosition
                }
                else
                {
                    #error finish fn and converting to use PhysicsProperties
                }
            }
            float xn =
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
