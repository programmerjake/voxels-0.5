#ifndef PNG_DECODER_H_INCLUDED
#define PNG_DECODER_H_INCLUDED

#include <string>
#include <cstdint>
#include <stdexcept>

using namespace std;

class PngLoadError final : public runtime_error
{
public:
    explicit PngLoadError(const string & arg)
        : runtime_error(arg)
    {
    }
};

/** read and decode png files<br/>
    bytes in RGBA format
 */
class PngDecoder final
{
private:
    unsigned w, h;
    uint8_t * data;
    PngDecoder(const PngDecoder &) = delete;
    const PngDecoder &operator =(const PngDecoder &) = delete;
public:
    explicit PngDecoder(string fileName);
    PngDecoder(PngDecoder && rt)
    {
        w = rt.w;
        h = rt.h;
        data = rt.data;
        rt.data = nullptr;
    }
    ~PngDecoder()
    {
        delete []data;
    }
    uint8_t operator()(int x, int y, int byteNum) const
    {
        if(x < 0 || (unsigned)x >= w || y < 0 || (unsigned)y >= h || byteNum < 0 || byteNum >= 4)
            throw new range_error("index out of range in PngDecoder::operator()(int x, int y, int byteNum) const");
        size_t index = y;
        index *= w;
        index += x;
        index *= 4;
        index += byteNum;
        return data[index];
    }
    int width() const
    {
        return w;
    }
    int height() const
    {
        return h;
    }
    uint8_t * removeData()
    {
        uint8_t * retval = data;
        data = nullptr;
        return retval;
    }
};

#endif // PNG_DECODER_H_INCLUDED
