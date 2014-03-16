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
#include "util.h"
#include <chrono>
#include <iostream>
#include <cstdlib>

using namespace std;

uint32_t makeSeed()
{
    uint64_t v = chrono::system_clock::now().time_since_epoch().count();
    return (uint32_t)v ^ (uint32_t)(v >> 32);
}

#if 0 // testing balanced_tree
namespace
{
initializer init1([]()
{
    balanced_tree<float> t, t2;
    t.insert(1.0);
    t.insert(1.0);
    t.insert(5.0);
    t.insert(2.0);
    t.insert(4.0);
    t.insert(3.0);
    t.insert(6.0);
    cout << "tree :";
    for(auto i = t.begin(); i != t.end(); i++)
        cout << " " << *i;
    cout << endl;
    cout << "tree :";
    for(auto i = t.begin(); i != t.end();)
    {
        auto v = *i;
        i = t.erase(i);
        cout << " " << v;
        t.insert(v);
    }
    cout << endl;
    t2 = move(t);
    t = t2;
    cout << "tree range :";
    for(auto i = t.rangeBegin(2.0); i != t.rangeEnd(30); i++)
        cout << " " << *i;
    cout << endl;
    t.erase(3);
    cout << "tree :";
    for(auto i = t.begin(); i != t.end(); i++)
        cout << " " << *i;
    cout << endl;

    exit(0);
});
}
#endif // testing balanced_tree
