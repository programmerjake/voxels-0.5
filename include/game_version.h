#ifndef GAME_VERSION_H_INCLUDED
#define GAME_VERSION_H_INCLUDED

#include <cwchar>
#include <string>
#include <cstdint>
using namespace std;

namespace GameVersion
{
    extern const wstring VERSION;
    extern const uint32_t FILE_VERSION;
    constexpr uint16_t port = 12345;
#ifdef DEBUG_VERSION
    const bool DEBUG = true;
#else
    const bool DEBUG = false;
#endif
};

#endif // GAME_VERSION_H_INCLUDED
