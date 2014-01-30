#ifndef UTIL_H
#define UTIL_H

#include <cmath>
#include <random>
#include <cstdint>
#include <list>
#include <set>
#include <functional>
#include <cassert>

using namespace std;

const float eps = 1e-4;

template <typename T>
inline const T limit(const T v, const T minV, const T maxV)
{
    if(v > maxV)
        return maxV;
    if(minV > v)
        return minV;
    return v;
}

inline int ifloor(float v)
{
    return static_cast<int>(floor(v));
}

inline int iceil(float v)
{
    return static_cast<int>(ceil(v));
}

template <typename T>
inline const T interpolate(const float t, const T a, const T b)
{
    return a + t * (b - a);
}

extern default_random_engine defaultRandom;

class initializer
{
private:
    void (*finalizeFn)();
    initializer(const initializer & rt) = delete;
    void operator =(const initializer & rt) = delete;
public:
    initializer(void (*initFn)(), void (*finalizeFn)() = nullptr)
        : finalizeFn(finalizeFn)
    {
        initFn();
    }
    ~initializer()
    {
        if(finalizeFn)
            finalizeFn();
    }
};

class finalizer
{
private:
    void (*finalizeFn)();
    finalizer(const finalizer & rt) = delete;
    void operator =(const finalizer & rt) = delete;
public:
    finalizer(void (*finalizeFn)())
        : finalizeFn(finalizeFn)
    {
        assert(finalizeFn);
    }
    ~finalizer()
    {
        finalizeFn();
    }
};

#endif // UTIL_H
