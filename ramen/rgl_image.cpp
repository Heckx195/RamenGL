
#include "rgl_image.h"
#include "rgl_filesystem.h"

#include <stb_image.h>

#include <assert.h>

#include <stdio.h>
#include <string.h> /* For memcpy */

Image::Image()
{
    m_Width       = 0;
    m_Height      = 0;
    m_NumChannels = 4;
    m_Data        = nullptr;
}

Image::Image(size_t width, size_t height, size_t numChannels)
{
    Init(width, height, numChannels);
}

Image::~Image()
{
    if ( m_Data )
    {
        free(m_Data);
    }
}

bool Image::Load(const char* file)
{
    int x, y, n;

    File imgFile = Filesystem::Instance()->Read(file);

    stbi_uc* data = stbi_load_from_memory((unsigned char*)(imgFile.data), imgFile.m_size, &x, &y, &n, 4);
    if ( !data )
    {
        fprintf(stderr, "STBI: Failed to load image.\n");
        return false;
    }

    // TODO: Use n for numbers of channels. But forcing to 4 makes gl texture generation easier for now.

    m_Data   = (unsigned char*)malloc(4 * x * y * sizeof(unsigned char));
    m_Width  = x;
    m_Height = y;
    memcpy(m_Data, data, 4 * x * y);

    stbi_image_free(data);

    return true;
}

void Image::Init(size_t width, size_t height, size_t numChannels)
{
    m_Width       = width;
    m_Height      = height;
    m_NumChannels = numChannels;
    m_Data        = (unsigned char*)malloc(width * height * numChannels);
    memset(m_Data, 0, width * height * numChannels);
}

void Image::SetPixel32ABGR(size_t x, size_t y, uint32_t value)
{
    assert(m_Data != nullptr);
    uint32_t* pixel = (uint32_t*)(m_Data + (y * 4 * m_Width + x * 4));
    *pixel          = value;
}

int Image::GetWidth()
{
    return m_Width;
}

int Image::GetHeight()
{
    return m_Height;
}

unsigned char* Image::Data()
{
    return m_Data;
}
