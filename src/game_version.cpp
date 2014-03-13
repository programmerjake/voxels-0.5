#include "game_version.h"

const wstring GameVersion::VERSION = L"0.5.1.182";
const uint32_t GameVersion::FILE_VERSION = 0;

#ifdef COMPILE_DUMP_VERSION
#include <iostream>

int main(int argc, char ** argv)
{
    const int curVersion = 182;
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

