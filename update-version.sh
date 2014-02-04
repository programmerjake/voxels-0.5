#!/bin/sh
mkdir -p obj
g++-4.8 -std=c++11 -DCOMPILE_DUMP_VERSION -Iinclude/ src/game_version.cpp -o obj/dump-version || exit 1
cat > src/game_version.cpp << EOF
#include "game_version.h"

const wstring GameVersion::VERSION = L"`obj/dump-version --next-version-str`";
const uint32_t GameVersion::FILE_VERSION = `obj/dump-version`;

#ifdef COMPILE_DUMP_VERSION
#include <iostream>

int main(int argc, char ** argv)
{
    const int curVersion = `obj/dump-version --next-version`;
    if(argc > 1 && string(argv[1]) == "--next-version")
        cout << (curVersion + 1) << endl;
    else if(argc > 1 && string(argv[1]) == "--next-version-str")
        cout << "0.5.1." << (curVersion + 1) << endl;
    else if(argc > 1)
        cout << "0.5.1." << curVersion << endl;
    else
        cout << GameVersion::FILE_VERSION << endl;
    return 0;
}
#endif // COMPILE_DUMP_VERSION

EOF
