#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#include "generate.h"
#include <cwchar>
#include <string>

using namespace std;

namespace Text
{
    struct TextProperties
    {
        float tabWidth = 8;
    };
    extern const TextProperties defaultTextProperties;
    float width(wstring str, const TextProperties & properties = defaultTextProperties);
    float height(wstring str, const TextProperties & properties = defaultTextProperties);
    float xPos(wstring str, const TextProperties & properties = defaultTextProperties);
    float yPos(wstring str, const TextProperties & properties = defaultTextProperties);
    Mesh mesh(wstring str, Color color = Color(1), const TextProperties & properties = defaultTextProperties);
}

#endif // TEXT_H_INCLUDED
