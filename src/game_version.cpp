// Automatically generated : don't modify
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
#include "game_version.h"

const wstring GameVersion::VERSION = L"0.5.3.183";
const uint32_t GameVersion::FILE_VERSION = 0;

#ifdef COMPILE_DUMP_VERSION
#include <iostream>

int main(int argc, char ** argv)
{
    const int curVersion = 183;
    if(argc > 1 && string(argv[1]) == "--next-version")
        cout << (curVersion + 1) << endl;
    else if(argc > 1 && string(argv[1]) == "--next-version-str")
        cout << "0.5.3." << (curVersion + 1) << endl;
    else if(argc > 1)
        cout << "0.5.3." << curVersion << endl;
    else
        cout << GameVersion::FILE_VERSION << endl;
    return 0;
}
#endif // COMPILE_DUMP_VERSION

