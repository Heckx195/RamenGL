#ifndef RGL_IMAGE_H
#define RGL_IMAGE_H

#include <stdint.h>
#include <string.h> /* For size_t */

class Image
{
  public:
    Image();
    Image(size_t width, size_t height, size_t numChannels = 4);
    ~Image();
    bool           Load(const char* file);
    void           Init(size_t width, size_t height, size_t numChannels = 4);
    void           SetPixel32ABGR(size_t x, size_t y, uint32_t value);
    int            GetWidth();
    int            GetHeight();
    unsigned char* Data();

  private:
    int            m_Width;
    int            m_Height;
    int            m_NumChannels;
    unsigned char* m_Data;
};

#endif
