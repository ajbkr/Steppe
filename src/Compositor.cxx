#include <iostream>

#include "Steppe.hh"

using namespace std;

namespace Steppe
{
    Compositor::Compositor()
    {
        _heightmap = new unsigned char[1024 * 1024];
        _outOfBoundsHeightmap = new unsigned char[1024 * 1024];
        // ...
    }

    Compositor::~Compositor()
    {
        delete [] _heightmap;
        delete [] _outOfBoundsHeightmap;
        // ...
    }

    Compositor *Compositor::addTexture(int height, XXX_IMAGE textureImage)
    {
        // ...

        return this;
    }

    void Compositor::composite(XXX_HTMLCANVASELEMENT texturemapCanvas)
    {
        // ...
    }

    unsigned char* Compositor::getHeightmap()
    {
        return _heightmap;
    }

    unsigned char* Compositor::getOutOfBoundsHeightmap()
    {
        return _outOfBoundsHeightmap;
    }

    Compositor* Compositor::putMask(XXX_IMAGE mask, int x, int y,
        float scaleFactor)
    {
        // ...

        return this;
    }

    Compositor* Compositor::setHeightmap(SDL_Surface* heightmapCanvas)
    {
        for (int y = 0; y < 1024; ++y) {
            for (int x = 0; x < 1024; ++x) {
                int index = (y << 10) + x;

/*                _heightmap[index] = gdImageGetPixel(heightmapCanvas, x, y) &
                    0xff;*/
                _heightmap[index] = ((unsigned int*) heightmapCanvas->pixels)[
                    y * (heightmapCanvas->pitch / sizeof(unsigned int)) + x] &
                    0xff;
                _outOfBoundsHeightmap[index] = _heightmap[index];
            }
        }

#ifndef NDEBUG
        cerr << "heightmap[42] = " << (int) _heightmap[42] << endl;
#endif

        return this;
    }
}
