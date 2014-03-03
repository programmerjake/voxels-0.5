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
#include "position.h"
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
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iterator>

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
    return floor(v);
}

inline int iceil(float v)
{
    return ceil(v);
}

template <typename T>
inline const T interpolate(const float t, const T a, const T b)
{
    return a + t * (b - a);
}

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
    size_t destLen = wstr.length() * 4 + 1 + 32/*for extra buffer space*/;
    char * str = new char[destLen];
    const wchar_t * ptr = wstr.c_str();
    mbstate_t mbstate;
    memset((void *)&mbstate, 0, sizeof(mbstate));
    size_t v = wcsrtombs(str, &ptr, destLen - 1, &mbstate);
    if(v == (size_t)-1)
    {
        delete []str;
        throw runtime_error("can't convert wide character string to multi-byte string");
    }
    str[v] = '\0';
    string retval = str;
    delete []str;
    return retval;
}

inline wstring mbsrtowcs(string str)
{
    size_t destLen = str.length() + 1 + 32/* for extra buffer space*/;
    wchar_t * wstr = new wchar_t[destLen];
    const char * ptr = str.c_str();
    mbstate_t mbstate;
    memset((void *)&mbstate, 0, sizeof(mbstate));
    size_t v = mbsrtowcs(wstr, &ptr, destLen - 1, &mbstate);
    if(v == (size_t)-1)
    {
        delete []wstr;
        throw runtime_error("can't convert multi-byte string to wide character string");
    }
    wstr[v] = '\0';
    wstring retval = wstr;
    delete []wstr;
    return retval;
}

class flag final
{
private:
    mutex lock;
    condition_variable_any cond;
    atomic_bool value;
public:
    flag(bool value = false)
        : value(value)
    {
    }
    const flag & operator =(bool v)
    {
        if(value.exchange(v) != v)
            cond.notify_all();
        return *this;
    }
    bool exchange(bool v)
    {
        bool retval = value.exchange(v);
        if(retval != v)
            cond.notify_all();
        return retval;
    }
    operator bool()
    {
        bool retval = value;
        return retval;
    }
    bool operator !()
    {
        bool retval = value;
        return !retval;
    }
    void wait(bool v = true) /// waits until value == v
    {
        if(v == value)
            return;
        lock.lock();
        while(v != value)
        {
            cond.wait(lock);
        }
        lock.unlock();
    }
    void set()
    {
        *this = true;
    }
    void reset()
    {
        *this = false;
    }
};

template <typename T, size_t arraySize>
class circularDeque final
{
public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;
    static constexpr size_type capacity()
    {
        return arraySize;
    }
private:
    size_type frontIndex, backIndex;
    value_type array[arraySize];
public:
    friend class iterator;
    class iterator final : public std::iterator<random_access_iterator_tag, value_type>
    {
        friend class circularDeque;
        friend class const_iterator;
    private:
        circularDeque * container;
        size_t index;
        iterator(circularDeque * container, size_t index)
            : container(container), index(index)
        {
        }
    public:
        iterator()
            : container(nullptr)
        {
        }
        iterator & operator +=(difference_type n)
        {
            if(-n > (difference_type)index)
                n = n % arraySize + arraySize;
            index += n;
            index %= arraySize;
            if(index < 0)
                index += arraySize;
            return *this;
        }
        iterator & operator -=(difference_type n)
        {
            return *this += -n;
        }
        friend iterator operator +(difference_type n, iterator i)
        {
            return i += n;
        }
        friend iterator operator +(iterator i, difference_type n)
        {
            return i += n;
        }
        friend iterator operator -(iterator i, difference_type n)
        {
            return i -= n;
        }
        difference_type operator -(const iterator & r) const
        {
            assert(container == r.container && container != nullptr);
            difference_type loc = index + arraySize - container->frontIndex;
            if(loc >= arraySize)
                loc -= arraySize;
            if(loc >= arraySize)
                loc -= arraySize;
            difference_type rloc = r.index + arraySize - container->frontIndex;
            if(rloc >= arraySize)
                rloc -= arraySize;
            if(rloc >= arraySize)
                rloc -= arraySize;
            return loc - rloc;
        }
        T & operator [](difference_type n) const
        {
            return *(*this + n);
        }
        T & operator *() const
        {
            return container->array[index];
        }
        T * operator ->() const
        {
            return container->array + index;
        }
        const iterator & operator --()
        {
            if(index == 0)
                index = arraySize - 1;
            else
                index--;
            return *this;
        }
        iterator operator --(int)
        {
            iterator retval = *this;
            if(index == 0)
                index = arraySize - 1;
            else
                index--;
            return retval;
        }
        const iterator & operator ++()
        {
            if(index >= arraySize - 1)
                index = 0;
            else
                index++;
            return *this;
        }
        iterator operator ++(int)
        {
            iterator retval = *this;
            if(index >= arraySize - 1)
                index = 0;
            else
                index++;
            return retval;
        }
        friend bool operator ==(const iterator & l, const iterator & r)
        {
            return l.index == r.index;
        }
        friend bool operator !=(const iterator & l, const iterator & r)
        {
            return l.index != r.index;
        }
        friend bool operator >(const iterator & l, const iterator & r)
        {
            return (l - r) > 0;
        }
        friend bool operator >=(const iterator & l, const iterator & r)
        {
            return (l - r) >= 0;
        }
        friend bool operator <(const iterator & l, const iterator & r)
        {
            return (l - r) < 0;
        }
        friend bool operator <=(const iterator & l, const iterator & r)
        {
            return (l - r) <= 0;
        }
    };

    friend class const_iterator;
    class const_iterator final : public std::iterator<random_access_iterator_tag, const value_type>
    {
        friend class circularDeque;
    private:
        const circularDeque * container;
        size_t index;
        const_iterator(const circularDeque * container, size_t index)
            : container(container), index(index)
        {
        }
    public:
        const_iterator()
            : container(nullptr)
        {
        }
        const_iterator(const iterator & v)
            : container(v.container), index(v.index)
        {
        }
        const_iterator & operator +=(difference_type n)
        {
            if(-n > (difference_type)index)
                n = n % arraySize + arraySize;
            index += n;
            index %= arraySize;
            if(index < 0)
                index += arraySize;
            return *this;
        }
        const_iterator & operator -=(difference_type n)
        {
            return *this += -n;
        }
        friend const_iterator operator +(difference_type n, const_iterator i)
        {
            return i += n;
        }
        friend const_iterator operator +(const_iterator i, difference_type n)
        {
            return i += n;
        }
        friend const_iterator operator -(const_iterator i, difference_type n)
        {
            return i -= n;
        }
        difference_type operator -(const const_iterator & r) const
        {
            assert(container == r.container && container != nullptr);
            difference_type loc = index + arraySize - container->frontIndex;
            if(loc >= arraySize)
                loc -= arraySize;
            if(loc >= arraySize)
                loc -= arraySize;
            difference_type rloc = r.index + arraySize - container->frontIndex;
            if(rloc >= arraySize)
                rloc -= arraySize;
            if(rloc >= arraySize)
                rloc -= arraySize;
            return loc - rloc;
        }
        const T & operator [](difference_type n) const
        {
            return *(*this + n);
        }
        const T & operator *() const
        {
            return container->array[index];
        }
        const T * operator ->() const
        {
            return container->array + index;
        }
        const const_iterator & operator --()
        {
            if(index == 0)
                index = arraySize - 1;
            else
                index--;
            return *this;
        }
        const_iterator operator --(int)
        {
            const_iterator retval = *this;
            if(index == 0)
                index = arraySize - 1;
            else
                index--;
            return retval;
        }
        const const_iterator & operator ++()
        {
            if(index >= arraySize - 1)
                index = 0;
            else
                index++;
            return *this;
        }
        const_iterator operator ++(int)
        {
            const_iterator retval = *this;
            if(index >= arraySize - 1)
                index = 0;
            else
                index++;
            return retval;
        }
        friend bool operator ==(const const_iterator & l, const const_iterator & r)
        {
            return l.index == r.index;
        }
        friend bool operator !=(const const_iterator & l, const const_iterator & r)
        {
            return l.index != r.index;
        }
        friend bool operator >(const const_iterator & l, const const_iterator & r)
        {
            return (l - r) > 0;
        }
        friend bool operator >=(const const_iterator & l, const const_iterator & r)
        {
            return (l - r) >= 0;
        }
        friend bool operator <(const const_iterator & l, const const_iterator & r)
        {
            return (l - r) < 0;
        }
        friend bool operator <=(const const_iterator & l, const const_iterator & r)
        {
            return (l - r) <= 0;
        }
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    circularDeque()
        : frontIndex(0), backIndex(0)
    {
    }

    iterator begin()
    {
        return iterator(this, frontIndex);
    }

    const_iterator begin() const
    {
        return const_iterator(this, frontIndex);
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, frontIndex);
    }

    iterator end()
    {
        return iterator(this, backIndex);
    }

    const_iterator end() const
    {
        return const_iterator(this, backIndex);
    }

    const_iterator cend() const
    {
        return const_iterator(this, backIndex);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(cbegin());
    }

    T & front()
    {
        return *begin();
    }

    const T & front() const
    {
        return *begin();
    }

    T & back()
    {
        return end()[-1];
    }

    const T & back() const
    {
        return end()[-1];
    }

    size_type size() const
    {
        return cend() - cbegin();
    }

    T & at(size_type pos)
    {
        if(pos >= size())
            throw out_of_range("position out of range in circularDeque::at");
        return begin()[pos];
    }

    const T & at(size_type pos) const
    {
        if(pos >= size())
            throw out_of_range("position out of range in circularDeque::at");
        return cbegin()[pos];
    }

    T & operator [](size_type pos)
    {
        return begin()[pos];
    }

    const T & operator [](size_type pos) const
    {
        return cbegin()[pos];
    }

    bool empty() const
    {
        return frontIndex == backIndex;
    }

    void clear()
    {
        frontIndex = backIndex = 0;
    }

    void push_front(const T & v)
    {
        if(frontIndex-- == 0)
            frontIndex = arraySize - 1;
        array[frontIndex] = v;
    }

    void push_front(T && v)
    {
        if(frontIndex-- == 0)
            frontIndex = arraySize - 1;
        array[frontIndex] = move(v);
    }

    void push_back(const T & v)
    {
        array[backIndex] = v;
        if(++backIndex >= arraySize)
            backIndex = 0;
    }

    void push_back(T && v)
    {
        array[backIndex] = move(v);
        if(++backIndex >= arraySize)
            backIndex = 0;
    }

    void pop_front()
    {
        array[frontIndex] = T();
        if(++frontIndex >= arraySize)
            frontIndex = 0;
    }

    void pop_back()
    {
        if(backIndex-- == 0)
            backIndex = arraySize - 1;
        array[backIndex] = T();
    }

    void swap(circularDeque & other)
    {
        circularDeque<T, arraySize> temp = move(*this);
        *this = move(other);
        other = move(temp);
    }
};

uint32_t makeSeed();

inline uint32_t makeSeed(wstring str)
{
    if(str == L"")
        return makeSeed();
    uint32_t retval = 0;
    for(wchar_t ch : str)
    {
        retval *= 9;
        retval += ch;
    }
    return retval;
}

#endif // UTIL_H
