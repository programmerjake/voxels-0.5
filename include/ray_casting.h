#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "position.h"
#include "block_face.h"
#include "util.h"

struct Ray final
{
    VectorF direction;
    PositionF position;
    Ray()
        : direction(0), position(0)
    {
    }
    Ray(VectorF direction, PositionF position)
        : direction(direction), position(position)
    {
    }
    bool good() const
    {
        return abs(direction.x) >= eps && abs(direction.y) >= 0 && abs(direction.z) >= 0;
    }
};

struct BoxRayCollision final
{
    float t;
    BlockFace enterFace;
    BlockFace exitFace;
    bool good() const
    {
        return t >= eps;
    }
    BoxRayCollision()
        : t(-1), face(BlockFace::NX)
    {
    }
    BoxRayCollision(float t, BlockFace face)
        : t(t), face(face)
    {
    }
};

inline BoxRayCollision rayHitBox(VectorF min, VectorF max, Ray ray)
{
    assert(min.x < max.x && min.y < max.y && min.z < max.z && ray.good());
    if(abs(ray.direction.x) < eps)
        ray.direction.x = eps;
    if(abs(ray.direction.y) < eps)
        ray.direction.y = eps;
    if(abs(ray.direction.z) < eps)
        ray.direction.z = eps;
    VectorF collidePlane = min;
    if(ray.direction.x < 0)
        collidePlane.x = max.x;
    if(ray.direction.y < 0)
        collidePlane.y = max.y;
    if(ray.direction.z < 0)
        collidePlane.z = max.z;
    VectorF t = (collidePlane - (VectorF)ray.position) / ray.direction;
    if(t.x >= eps)
    {
        Vector p = ray.direction * t.x + (VectorF)ray.position;
        if(p.y < min.y || p.y > max.y || p.z < min.z || p.z > max.z)
            t.x = -1;
    }
    if(t.y >= eps)
    {
        Vector p = ray.direction * t.y + (VectorF)ray.position;
        if(p.x < min.x || p.x > max.x || p.z < min.z || p.z > max.z)
            t.y = -1;
    }
    if(t.z >= eps)
    {
        Vector p = ray.direction * t.z + (VectorF)ray.position;
        if(p.x < min.x || p.x > max.x || p.y < min.y || p.y > max.y)
            t.z = -1;
    }
    BoxRayCollision retval;
    if(t.x >= eps && (t.y < eps || t.y > t.x) && (t.z < eps || t.z > t.x)) // x is closest
    {
        retval.t = t.x;
        retval.enterFace = (ray.direction.x > 0) ? BlockFace::NX : BlockFace::PX;
    }
    else if(t.y >= eps && (t.z < eps || t.z > t.y)) // y is closest
    {
        retval.t = t.y;
        retval.enterFace = (ray.direction.y > 0) ? BlockFace::NY : BlockFace::PY;
    }
    else if(t.z >= eps) // z is closest
    {
        retval.t = t.z;
        retval.enterFace = (ray.direction.z > 0) ? BlockFace::NZ : BlockFace::PZ;
    }
    else
        return BoxRayCollision();
    collidePlane = min;
    if(ray.direction.x > 0)
        collidePlane.x = max.x;
    if(ray.direction.y > 0)
        collidePlane.y = max.y;
    if(ray.direction.z > 0)
        collidePlane.z = max.z;
    VectorF t = (collidePlane - (VectorF)ray.position) / ray.direction;
    if(t.x < t.y && t.x < t.z)
    {
        retval.exitFace = (ray.direction.x > 0) ? BlockFace::PX : BlockFace::NX;
    }
    else if(t.y < t.z)
    {
        retval.exitFace = (ray.direction.y > 0) ? BlockFace::PY : BlockFace::NY;
    }
    else
    {
        retval.exitFace = (ray.direction.z > 0) ? BlockFace::PZ : BlockFace::NZ;
    }
    return retval;
}

#endif // RAY_CASTING_H_INCLUDED
