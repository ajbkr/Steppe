#include <iostream>	// cerr, endl, hex
#include <cstdlib>	// strtol()
#include <sstream>	// stringstream
#include <cctype>	// isxdigit()
#include <cmath>	// cosf(), roundf(), sinf()

#include "Steppe.hh"

using namespace std;

namespace Steppe
{
    Renderer::Renderer(SDL_Surface* canvas)
    {
#ifndef NDEBUG
        cerr << "_CANVAS_WIDTH = " << _CANVAS_WIDTH << endl;
        cerr << "_CANVAS_HEIGHT = " << _CANVAS_HEIGHT << endl;
        cerr << "_ANGLE_OF_VIEW = " << _ANGLE_OF_VIEW << endl;
        cerr << "_ONE_DEGREE_ANGLE = " << _ONE_DEGREE_ANGLE << endl;
        cerr << "_THIRTY_DEGREE_ANGLE = " << _THIRTY_DEGREE_ANGLE << endl;
        cerr << "_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE = " <<
            _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE << endl;
        cerr << "_ANGULAR_INCREMENT = " << _ANGULAR_INCREMENT << endl;
        cerr << "_DEGREES_TO_RADIANS = " << _DEGREES_TO_RADIANS << endl;
        cerr << "_FAKE_DEGREES_TO_RADIANS = " << _FAKE_DEGREES_TO_RADIANS <<
            endl;
        cerr << "_RADIANS_TO_DEGREES = " << _RADIANS_TO_DEGREES <<
            endl;	// XXX ?!
        cerr << "_RADIANS_TO_FAKE_DEGREES = " << _RADIANS_TO_FAKE_DEGREES <<
            endl;	// XXX ?!
        cerr << "_SCALE_FACTOR = " << _SCALE_FACTOR << endl;
        cerr << "_CAMERA_Y = " << _CAMERA_Y << endl;
        cerr << "_DISTANCE = " << _DISTANCE << endl;
        cerr << "_WATER_HEIGHT = " << _WATER_HEIGHT << endl;
#endif

        _camera.angle = 0;
        _camera.x = _camera.z = 1024;
        _camera.y = _CAMERA_Y;
        _cosineLookupTable = new float[
            (int) _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE];
        _fogColor = 0x7f7f7fff;
        _framebuffer = NULL;
        _heightmap = NULL;
        _inverseDistortionLookupTable = new float[
            (int) _THIRTY_DEGREE_ANGLE * 2];
        _outOfBoundsHeightmap = NULL;
        _outOfBoundsTexturemap = NULL;
        _rayLengthLookupTable = new float *[300 - 200 + 1];
        _sineLookupTable = new float[
            (int) _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE];
        _sky = NULL;
        // XXX _spriteList
        _temporaryFramebuffer = NULL;
        _texturemap = NULL;
        // XXX _visibleSpriteList

        _fog = false;
        _quality = _DONT_CARE;
        _smooth = 0.0F;
        _waterHeight = -1;

        if (canvas->w != _CANVAS_WIDTH) {
            stringstream ss;

            ss << "Canvas width not equal to " << _CANVAS_WIDTH;
            throw Exception(ss.str().c_str());
        }
        if (canvas->h != _CANVAS_HEIGHT) {
            stringstream ss;

            ss << "Canvas height not equal to " << _CANVAS_HEIGHT;
            throw Exception(ss.str().c_str());
        }

        _framebuffer = canvas;

        const SDL_PixelFormat& pixelFormat = *(canvas->format);
        _temporaryFramebuffer = SDL_CreateRGBSurface(0, canvas->w, canvas->h,
            pixelFormat.BitsPerPixel, pixelFormat.Rmask, pixelFormat.Gmask,
            pixelFormat.Bmask, pixelFormat.Amask);

        _initSineAndCosineLookupTables();
        _initInverseDistortionLookupTable();
        _initRayLengthLookupTable(_DISTANCE);
    }

    Renderer::~Renderer()
    {
        delete [] _cosineLookupTable;
        _cosineLookupTable = NULL;
        delete [] _inverseDistortionLookupTable;
        _inverseDistortionLookupTable = NULL;
        for (int y = 200; y <= 300; ++y) {
            delete [] _rayLengthLookupTable[y - 200];
        }
        delete [] _rayLengthLookupTable;
        _rayLengthLookupTable = NULL;
        delete [] _sineLookupTable;
        _sineLookupTable = NULL;
        SDL_FreeSurface(_temporaryFramebuffer);

        // ...
    }

    inline int Renderer::_alphaBlend(int firstColor, int secondColor,
        int alpha)
    {
        if (alpha < 0) {
            alpha = 0;
        } else if (alpha > 255) {
            alpha = 255;
        }

        float normalisedAlpha = alpha / 255.0F;
        float adjustedAlpha   = 1.0F - normalisedAlpha;

        int mixedRed   = (int) (((firstColor >> 24) & 0xff) * normalisedAlpha);
        int mixedGreen = (int) (((firstColor >> 16) & 0xff) * normalisedAlpha);
        int mixedBlue  = (int) (((firstColor >>  8) & 0xff) * normalisedAlpha);

        mixedRed   += (int) floorf(((secondColor >> 24) & 0xff) *
            adjustedAlpha);
        mixedGreen += (int) floorf(((secondColor >> 16) & 0xff) *
            adjustedAlpha);
        mixedBlue  += (int) floorf(((secondColor >>  8) & 0xff) *
            adjustedAlpha);

        return (mixedRed << 24) |
            (mixedGreen << 16)  |
            (mixedBlue  <<  8)  | 0xff;
    }

    int Renderer::_getPixelFromOutOfBoundsTexturemap(int x, int y)
    {
        if (_outOfBoundsTexturemap != NULL) {
            return (Compositor::getPixel(_outOfBoundsTexturemap, x, y) << 8) |
                0xff;
        } else {
            return 0x7f7f7fff;
        }
    }

    int Renderer::_getPixelFromSky(int x, int y)
    {
        int currentAngle = _camera.angle - _THIRTY_DEGREE_ANGLE;

        if (currentAngle < 0) {
            currentAngle += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

        if (y < 0) {
            y = 0;
        } else if (y >= 100) {
            y = 100 - 1;
        }

        return (Compositor::getPixel(_sky, (currentAngle + x) % 1920, y) <<
            8) | 0xff;
    }

    int Renderer::_getPixelFromTexturemap(int x, int y)
    {
        if (_texturemap != NULL) {
            return (Compositor::getPixel(_texturemap, x, y) << 8) | 0xff;
        } else {
            return 0xffffffff;
        }
    }

    inline int Renderer::_getRow(int x, int z, int ray)
    {
        float cameraVectorX = cosf((float) _camera.angle * _ANGULAR_INCREMENT *
            _DEGREES_TO_RADIANS);
        float cameraVectorZ = sinf((float) _camera.angle * _ANGULAR_INCREMENT *
            _DEGREES_TO_RADIANS);

        int spriteVectorX = x - _camera.x;
        int spriteVectorZ = z - _camera.z;

        float vectorLength = sqrtf(spriteVectorX * spriteVectorX +
            spriteVectorZ * spriteVectorZ);

        float newVectorLength = vectorLength /
            _inverseDistortionLookupTable[ray];

        int y = roundf((float) _DISTANCE * _camera.y / newVectorLength);
        int row = y + _CANVAS_HEIGHT - 1 - _camera.y;

        return row;
    }

    void Renderer::_initInverseDistortionLookupTable()
    {
        for (int angleOfRotation = 0; angleOfRotation < _THIRTY_DEGREE_ANGLE;
            ++angleOfRotation) {
            float angleOfRotationInRadians = (float) angleOfRotation *
                _ANGULAR_INCREMENT * _DEGREES_TO_RADIANS;

            _inverseDistortionLookupTable[angleOfRotation +
                (int) _THIRTY_DEGREE_ANGLE] =
                1.0F / cosf(angleOfRotationInRadians);

            float cosine = cosf(angleOfRotationInRadians);
            if (cosine != 0.0F) {
                _inverseDistortionLookupTable[
                    (int) _THIRTY_DEGREE_ANGLE - angleOfRotation] =
                    1.0F / cosine;
            }
        }

        _inverseDistortionLookupTable[0]   = 2.0F;
        _inverseDistortionLookupTable[160] = 1.0F;
    }

    void Renderer::_initRayLengthLookupTable(int distance)
    {
        for (int y = 200; y <= 300; ++y) {
            _rayLengthLookupTable[y - 200] =
                new float[_CANVAS_WIDTH * (_MAXIMUM_ROW + 1)];

            for (int ray = 1; ray < _CANVAS_WIDTH; ++ray) {
                for (int row = 0; row <= _MAXIMUM_ROW; ++row) {
                    int invertedRow = _CANVAS_HEIGHT - 1 - row;

                    float rayLength = _inverseDistortionLookupTable[ray] *
                        ((distance * y) / (y - invertedRow));

                    _rayLengthLookupTable[y - 200][row * _CANVAS_WIDTH + ray] =
                        rayLength;
#if 0
#ifndef NDEBUG
                    if (y == 250 && ray == 159 && row == 149) {
                        cerr << "invertedRow = " << invertedRow << endl;
                        cerr << "rayLength = " << rayLength << endl;
                    }
#endif
#endif
                }
            }
        }
    }

    void Renderer::_initSineAndCosineLookupTables()
    {
        for (int angleOfRotation = 0;
            angleOfRotation < _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
            ++angleOfRotation) {
            float angleOfRotationInRadians = (float) angleOfRotation *
                _ANGULAR_INCREMENT * _DEGREES_TO_RADIANS;

            _sineLookupTable[angleOfRotation]   = sinf(
                angleOfRotationInRadians);
            _cosineLookupTable[angleOfRotation] = cosf(
                angleOfRotationInRadians);

#if 0
#ifndef NDEBUG
            cerr << _sineLookupTable[angleOfRotation] << endl;
            cerr << _cosineLookupTable[angleOfRotation] << endl;
#endif
#endif
        }
    }

    void Renderer::_renderBackToFront(int initialAngle)
    {
        // ...
    }

    void Renderer::_renderFrontToBack(int initialAngle)
    {
        if (SDL_MUSTLOCK(_framebuffer)) {
            SDL_LockSurface(_framebuffer);
        }

        int currentAngle = initialAngle;

        for (int ray = _quality; ray < _CANVAS_WIDTH; ray += _quality) {
            int previousTop = _MAXIMUM_ROW;

            for (int row = _MAXIMUM_ROW; row >= 0; --row) {
                float rayLength = _rayLengthLookupTable[_camera.y - 200][
                    (row << 8) + (row << 6) + ray];

                int rayX = (int) ((float) _camera.x + rayLength *
                    _cosineLookupTable[currentAngle]);
                int rayZ = (int) ((float) _camera.z + rayLength *
                    _sineLookupTable[currentAngle]);

                int u = rayX & 1023;
                int v = rayZ & 1023;

                int height;
//                cerr << "XYZZY" << endl;
                if ((rayX < 1024 || rayX >= 1024 + 1024 ||
                    rayZ < 1024 || rayZ >= 1024 + 1024) &&
                    _outOfBoundsHeightmap != NULL) {
                    height = _outOfBoundsHeightmap[(v << 10) + u];
                } else {
                    height = _heightmap[(v << 10) + u];
                }

                int scale = (int) ((float) height * _SCALE_FACTOR /
                    (rayLength + 1.0F));

                int top = (_CANVAS_HEIGHT >> 1) -
                    (_camera.y - _CANVAS_HEIGHT) + row - scale;
                int bottom = top + scale;

                if (top < previousTop) {
                    bottom = previousTop;
                    previousTop = top;

                    int color = 0x000000ff;

                    int texel;

                    if (rayX < 1024 || rayX >= 1024 + 1024 ||
                        rayZ < 1024 || rayZ >= 1024 + 1024) {
                        texel = _getPixelFromOutOfBoundsTexturemap(u, v);

                        color = texel;
                    } else {
                        if (height < _waterHeight) {
                            int data = _getPixelFromSky(ray, 200 - top);

                            texel = _getPixelFromTexturemap(u, v);

                            int mixedColor = _alphaBlend(data,
                                texel, (int) ((float) (_waterHeight - height) /
                                (float) _waterHeight * 255.0F * 2.0F));
cerr << ((int) ((float) (_waterHeight - height) / (float) _waterHeight *
    255.0F * 2.0F)) << endl;
                            texel = mixedColor;

                            height = _waterHeight;

                            color = texel;
                        } else {
                            texel = _getPixelFromTexturemap(u, v);

                            color = texel;
                        }
                    }

                    if (_fog) {
                        int foggedTexel = _alphaBlend(texel,
                            _fogColor, (int) ((float) row / 100.0F * 255.0F));

                        color = foggedTexel;
                    }

                    if (bottom > 199) {
                        bottom = 199;
                    }

                    if (ray > _quality) {
                        for (int j = 0; j < bottom - top + 1; ++j) {
                            for (int i = 0; i < _quality; ++i) {
                                Compositor::putPixel(_framebuffer, ray + i,
                                    top + j, (color >> 8) & 0x00ffffff);
                            }
                        }
                    } else {
                        for (int j = 0; j < bottom - top + 1; ++j) {
                            for (int i = 0; i < _quality; ++i) {
                                Compositor::putPixel(_framebuffer, ray + i,
                                    top + j, (color >> 8) & 0x00ffffff);
                                Compositor::putPixel(_framebuffer,
                                    ray + i - _quality, top + j,
                                    (color >> 8) & 0x00ffffff);
                            }
                        }
                    }
                }
            }

            currentAngle += _quality;
            if (currentAngle >= _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE) {
                currentAngle = 0;
            }
        }

        if (SDL_MUSTLOCK(_framebuffer)) {
            SDL_UnlockSurface(_framebuffer);
        }
    }

    void Renderer::_renderSky()
    {
        int angleOfRotation = _camera.angle - _THIRTY_DEGREE_ANGLE;

        if (angleOfRotation < 0) {
            angleOfRotation += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

        int skyWidth  = _sky->w;
        int skyHeight = (_CANVAS_HEIGHT >> 1) -
            (_camera.y - _CANVAS_HEIGHT);

        if (skyHeight > _sky->h) {
            skyHeight = _sky->h;
        }

        int sy = _camera.y - _CANVAS_HEIGHT;

        if (sy < 0) {
            sy = 0;
        }

#ifndef NDEBUG
        cerr << "angleOfRotation = " << angleOfRotation << endl;
        cerr << "skyWidth = " << skyWidth << endl;
        cerr << "skyHeight = " << skyHeight << endl;
        cerr << "sy = " << sy << endl;
#endif

        if (SDL_MUSTLOCK(_framebuffer)) {
            SDL_LockSurface(_framebuffer);
        }

        if (angleOfRotation + 320 <= skyWidth) {
            for (int y = 0; y < skyHeight; ++y) {
                for (int x = 0; x < 320; ++x) {
                    Compositor::putPixel(_framebuffer, x, y,
                        Compositor::getPixel(_sky, angleOfRotation + x,
                        sy + y));
                }
            }
        } else {
            for (int y = 0; y < skyHeight; ++y) {
                for (int x = 0; x < skyWidth - angleOfRotation; ++x) {
                    Compositor::putPixel(_framebuffer, x, y,
                        Compositor::getPixel(_sky, angleOfRotation + x,
                        sy + y));
                }
            }
            for (int y = 0; y < skyHeight; ++y) {
                for (int x = 0; x < 320 - (skyWidth - angleOfRotation); ++x) {
                    Compositor::putPixel(_framebuffer,
                        skyWidth - angleOfRotation + x, y,
                        Compositor::getPixel(_sky,
                        (skyWidth - angleOfRotation) + angleOfRotation + x,
                        sy + y));
                }
            }
        }

        if (_fog) {
#ifndef NDEBUG
            cerr << "pixel = " << hex <<
                (Compositor::getPixel(_framebuffer, 0, 0) << 8) << dec << endl;
#endif

            for (int y = 0; y < skyHeight; ++y) {
                int alpha = 255 - (int) (255.0F / 100.0F *
                    (100 - skyHeight + y));

                for (int x = 0; x < 320; ++x) {
                    int pixel = (Compositor::getPixel(_framebuffer, x, y) << 8);

                    int foggedPixel = _alphaBlend(pixel, _fogColor,
                        alpha);

                    Compositor::putPixel(_framebuffer, x, y,
                        ((foggedPixel >> 8) & 0x00ffffff));
                }
            }
        }

        if (SDL_MUSTLOCK(_framebuffer)) {
            SDL_UnlockSurface(_framebuffer);
        }
    }

    Renderer* Renderer::addSprite(XXX_IMAGE image, int x, int z)
    {
        // ...

        return this;
    }

    Renderer* Renderer::disable(string capability)
    {
        if (capability == "fog") {
            _fog = false;
        } else if (capability == "reflection-map") {
            _waterHeight = -1;
        } else if (capability == "smooth") {
            _smooth = 0.0F;
        } else {
            throw Exception("Can't disable unknown capability");
        }

        return this;
    }

    Renderer* Renderer::enable(string capability)
    {
        if (capability == "fog") {
            _fog = true;
        } else if (capability == "reflection-map") {
            _waterHeight = _WATER_HEIGHT;
        } else if (capability == "smooth") {
            _smooth = 0.5F;
        } else {
            throw Exception("Can't enable unknown capability");
        }

        return this;
    }

    struct Camera Renderer::getCamera()
    {
        struct Camera camera;

        camera.angle = roundf(_camera.angle /
            _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE * 360.0F);
        camera.x = _camera.x;
        camera.y = _camera.y;
        camera.z = _camera.z;
        return camera;
    }

    int Renderer::getHeight(int x, int z)
    {
        if (x < 1024 || x >= 1024 + 1024) {
            throw Exception("Invalid x: must be in the range 1024..2047");
        }
        if (z < 1024 || z >= 1024 + 1024) {
            throw Exception("Invalid z: must be in the range 1024..2047");
        }

        int u = x & 1023;
        int v = z & 1023;

        return _heightmap[(v << 10) + u];
    }

    bool Renderer::isEnabled(string capability)
    {
        if (capability == "fog") {
            return _fog;
        } else if (capability == "reflection-map") {
            return (_waterHeight > -1);
        } else if (capability == "smooth") {
            return (_smooth == 0.5F);
        }
        throw Exception("Unknown capability");
    }

    void Renderer::render()
    {
        _renderSky();

        int initialAngle = _camera.angle - _THIRTY_DEGREE_ANGLE;

        if (initialAngle < 0) {
            initialAngle += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

//        cerr << "Calling _renderFrontToBack()..." << endl;

        _renderFrontToBack(initialAngle);

        // ...
    }

    Renderer* Renderer::setCamera(const struct Camera& camera)
    {
        _camera.angle = (int) ((float) (abs(camera.angle) % 360) /
            (float) _ANGLE_OF_VIEW * 320.0F);
        _camera.x = camera.x;
        _camera.y = camera.y;
        _camera.z = camera.z;

#ifndef NDEBUG
        cerr << "_camera.angle = " << _camera.angle << endl;
        cerr << "_camera.x = " << _camera.x << endl;
        cerr << "_camera.y = " << _camera.y << endl;
        cerr << "_camera.z = " << _camera.z << endl;
#endif

        return this;
    }

    Renderer* Renderer::setFogColor(string cssColor)
    {
        if ( !_fog) {
            throw Exception("Capability not enabled");
        }

        if (cssColor.size() != 7 || cssColor[0] != '#' ||
            !isxdigit(cssColor[1]) || !isxdigit(cssColor[2]) ||
            !isxdigit(cssColor[3]) || !isxdigit(cssColor[4]) ||
            !isxdigit(cssColor[5]) || !isxdigit(cssColor[6])) {
            throw Exception("Invalid cssColor: must be in fully-qualified "
                "hexadecimal format (#rrggbb)");
        }

        string red   = cssColor.substr(1, 2);
        string green = cssColor.substr(3, 2);
        string blue  = cssColor.substr(5, 2);

        _fogColor = (((int) strtol(red.c_str(),   NULL, 16) << 24) |
                     ((int) strtol(green.c_str(), NULL, 16) << 16) |
                     ((int) strtol(blue.c_str(),  NULL, 16) <<  8) | 0xff);

#ifndef NDEBUG
        cerr << "fogColor = " << hex << _fogColor << dec << endl;
#endif

        return this;
    }

    Renderer* Renderer::setHeightmap(const unsigned char* heightmap)
    {
        _heightmap = (unsigned char*) heightmap;

        return this;
    }

    Renderer* Renderer::setOutOfBoundsHeightmap(
        const unsigned char* outOfBoundsHeightmap)
    {
        _outOfBoundsHeightmap = (unsigned char*) outOfBoundsHeightmap;

        return this;
    }

    Renderer* Renderer::setOutOfBoundsTexturemap(
        SDL_Surface* outOfBoundsTexturemapCanvas)
    {
        if (outOfBoundsTexturemapCanvas->w != 1024) {
            throw Exception("outOfBoundsTexturemapCanvas width not equal "
                "to 1024");
        }
        if (outOfBoundsTexturemapCanvas->h != 1024) {
            throw Exception("outOfBoundsTexturemapCanvas height not equal "
                "to 1024");
        }

        _outOfBoundsTexturemap = outOfBoundsTexturemapCanvas;

        return this;
    }

    Renderer* Renderer::setQuality(string quality)
    {
        if (quality == "medium") {
            _quality = _DONT_CARE;
        } else if (quality == "low") {
            _quality = _FASTEST;
        } else if (quality == "high") {
            _quality = _NICEST;
        } else {
            throw Exception("Invalid quality: must be 'low', 'medium', or "
                "'high'");
        }

        return this;
    }

    Renderer* Renderer::setSky(SDL_Surface* skyCanvas)
    {
        if (skyCanvas->w != 1920) {
            throw Exception("skyCanvas width not equal to 1920");
        }
        if (skyCanvas->h != 100) {
            throw Exception("skyCanvas height not equal to 100");
        }

        _sky = skyCanvas;

        return this;
    }

    Renderer* Renderer::setTexturemap(SDL_Surface* texturemapCanvas)
    {
        if (texturemapCanvas->w != 1024) {
            throw Exception("texturemapCanvas width not equal to 1024");
        }
        if (texturemapCanvas->h != 1024) {
            throw Exception("texturemapCanvas height not equal to 1024");
        }

        _texturemap = texturemapCanvas;

        return this;
    }

    Renderer* Renderer::setWaterHeight(int height)
    {
        if (_waterHeight == -1) {
            throw Exception("Capability not enabled");
        }

        if (height < 0 || height > 255) {
            throw Exception("Invalid height: must be in the range 0..255");
        }

        _waterHeight = height;

        return this;
    }
}
