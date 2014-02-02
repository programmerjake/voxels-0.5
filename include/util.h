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
#ifndef UTIL_H
#define UTIL_H

#include <cmath>
#include <random>
#include <cstdint>
#include <list>
#include <set>
#include <functional>
#include <cassert>
#include <cwchar>
#include <string>
#include <cstring>

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

inline string wcsrtombs(wstring wstr)
{
    size_t destLen = wstr.length() * 4 + 1;
    char * str = new char[destLen];
    const wchar_t * ptr = wstr.c_str();
    mbstate_t mbstate;
    memset((void *)&mbstate, 0, sizeof(mbstate));
    size_t v = wcsrtombs(str, &ptr, destLen - 1, &mbstate);
    if(v == (size_t)-1)
    {
        delete []str;
        throw new runtime_error("can't convert wide character string to multi-byte string");
    }
    str[v] = '\0';
    string retval = str;
    delete []str;
    return retval;
}

inline wstring mbsrtowcs(string str)
{
    size_t destLen = str.length() + 1;
    wchar_t * wstr = new wchar_t[destLen];
    const char * ptr = str.c_str();
    mbstate_t mbstate;
    memset((void *)&mbstate, 0, sizeof(mbstate));
    size_t v = mbsrtowcs(wstr, &ptr, destLen - 1, &mbstate);
    if(v == (size_t)-1)
    {
        delete []wstr;
        throw new runtime_error("can't convert multi-byte string to wide character string");
    }
    wstr[v] = '\0';
    wstring retval = wstr;
    delete []wstr;
    return retval;
}

#endif // UTIL_H
