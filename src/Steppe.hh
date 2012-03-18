#ifndef STEPPE_HH
#define STEPPE_HH

#include <exception>	// exception
#include <string>	// string
#include <vector>	// vector
#include <cmath>	// M_PI

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define XXX_HTMLCANVASELEMENT   int
#define XXX_IMAGE               int
#define XXX_OBJECT              int

namespace Steppe
{
    class Exception : public std::exception
    {
    private: 
        const char* _msg;

    public:
        Exception(const char* msg) : _msg(msg) { }

        virtual const char* what() const throw()
        {
            return (std::string("Exception: ") + std::string(_msg)).c_str();
        }
    };

    struct Camera
    {
        int angle;
        int x;
        int y;
        int z;
    };

    class Compositor
    {
    private:
        unsigned char* _heightmap;
        unsigned char* _outOfBoundsHeightmap;
        std::vector<XXX_OBJECT> _textureArray;

    public:
        Compositor();
        ~Compositor();

        static Uint32 getPixel(SDL_Surface* surface, int x, int y);
        static void putPixel(SDL_Surface* surface, int x, int y, Uint32 color);

        Compositor* addTexture(int height, XXX_IMAGE textureImage);
        void composite(XXX_HTMLCANVASELEMENT texturemapCanvas);
        unsigned char* getHeightmap();					// DONE
        unsigned char* getOutOfBoundsHeightmap();			// DONE
        Compositor* putMask(XXX_IMAGE mask, int x, int y, float scaleFactor);
        Compositor* setHeightmap(SDL_Surface* heightmapCanvas);		// DONE
    };

    inline Uint32 Compositor::getPixel(SDL_Surface* surface, int x, int y)
    {
        int bytesPerPixel = surface->format->BytesPerPixel;
        Uint8* data = (Uint8*) surface->pixels + y * surface->pitch + x *
            bytesPerPixel;
        Uint32 color;

        switch (bytesPerPixel) {
        case 1:
            color = *data;
            break;
        case 2:
            color = *(Uint16*) data;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                color = (data[0] << 16) |
                        (data[1] <<  8) |
                         data[2];
            } else {
                color =  data[0]        |
                        (data[1] <<  8) |
                        (data[2] << 16);
            }
            break;
        case 4:
            color = *(Uint32*) data;
            break;
        default:
            throw Steppe::Exception("Invalid bytes-per-pixel");
        }
        return color;
    }

    inline void Compositor::putPixel(SDL_Surface* surface, int x, int y,
        Uint32 color)
    {
        int bytesPerPixel = surface->format->BytesPerPixel;
        Uint8* data = (Uint8*) surface->pixels + y * surface->pitch + x *
            bytesPerPixel;

        switch (bytesPerPixel) {
        case 1:
            *data = color;
            break;
        case 2:
            *(Uint16*) data = color;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                data[0] = (color >> 16) & 0xff;
                data[1] = (color >>  8) & 0xff;
                data[2] =  color        & 0xff;
            } else {
                data[0] =  color        & 0xff;
                data[1] = (color >>  8) & 0xff;
                data[2] = (color >> 16) & 0xff;
            }
            break;
        case 4:
            *(Uint32*) data = color;
            break;
        default:
            throw Steppe::Exception("Invalid bytes-per-pixel");
        }
    }

    class Renderer
    {
    private:
        static const int _CANVAS_WIDTH  = 320;
        static const int _CANVAS_HEIGHT = 200;
        static const int _ANGLE_OF_VIEW = 60;
        static const float _ONE_DEGREE_ANGLE = 1.0F / _ANGLE_OF_VIEW *
            _CANVAS_WIDTH;
        static const float _THIRTY_DEGREE_ANGLE = 1.0F / _ANGLE_OF_VIEW *
            _CANVAS_WIDTH * 30.0F;
        static const float _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE = 1.0F /
            _ANGLE_OF_VIEW * _CANVAS_WIDTH * 360.0F;
        static const float _ANGULAR_INCREMENT = 1.0F * _ANGLE_OF_VIEW /
            _CANVAS_WIDTH;
        static const float _DEGREES_TO_RADIANS = M_PI / 180.0F;
        static const float _FAKE_DEGREES_TO_RADIANS = (2.0F * M_PI) /
            ((360.0F / _ANGLE_OF_VIEW) * _CANVAS_WIDTH);
        static const float _RADIANS_TO_DEGREES = 180.0F / M_PI;
        static const float _RADIANS_TO_FAKE_DEGREES =
            ((360.0F / _ANGLE_OF_VIEW) * _CANVAS_WIDTH) / (2.0F * M_PI);
        static const int _SCALE_FACTOR = 35;
        static const int _CAMERA_Y     = 175;
        static const int _DISTANCE     = 75;
        static const int _MAXIMUM_ROW  = _CANVAS_HEIGHT +
            _CANVAS_HEIGHT / 2 + 1;
        static const int _WATER_HEIGHT = 64;

        static const int _FASTEST   = 4;
        static const int _DONT_CARE = 2;
        static const int _NICEST    = 1;

        struct Camera _camera;
        float* _cosineLookupTable;
        int _fogColor;
        SDL_Surface* _framebuffer;
        unsigned char* _heightmap;
        float* _inverseDistortionLookupTable;
        unsigned char* _outOfBoundsHeightmap;
        SDL_Surface* _outOfBoundsTexturemap;
        float** _rayLengthLookupTable;
        float* _sineLookupTable;
        SDL_Surface* _sky;
        std::vector<XXX_OBJECT> _spriteList;
        SDL_Surface* _temporaryFramebuffer;
        SDL_Surface* _texturemap;
        std::vector<XXX_OBJECT> _visibleSpriteList;

        bool _fog;
        int _quality;
        float _smooth;
        int _waterHeight;

        inline int _alphaBlend(int firstColor, int secondColor,
            int alpha);							// DONE
        int _getPixelFromOutOfBoundsTexturemap(int x, int y);		// DONE
        int _getPixelFromSky(int x, int y);				// DONE
        int _getPixelFromTexturemap(int x, int y);			// DONE
        inline int _getRow(int x, int z, int ray);			// DONE
        void _initInverseDistortionLookupTable();			// DONE
        void _initRayLengthLookupTable(int distance);			// DONE
        void _initSineAndCosineLookupTables();				// DONE
        void _renderBackToFront(int initialAngle);
        void _renderFrontToBack(int initialAngle);			// DONE
        void _renderSky();						// DONE

    public:
        Renderer(SDL_Surface* canvas);
        ~Renderer();

        Renderer* addSprite(XXX_IMAGE image, int x, int z);
        Renderer* disable(std::string capability);			// DONE
        Renderer* enable(std::string capability);			// DONE
        struct Camera getCamera();					// DONE
        int getHeight(int x, int z);					// DONE
        bool isEnabled(std::string capability);				// DONE
        void render();
        Renderer* setCamera(const struct Camera& camera);		// DONE
        Renderer* setFogColor(std::string cssColor);			// DONE
        Renderer* setHeightmap(const unsigned char* heightmap);		// DONE
        Renderer* setOutOfBoundsHeightmap(
            const unsigned char* outOfBoundsHeightmap);			// DONE
        Renderer* setOutOfBoundsTexturemap(
            SDL_Surface* outOfBoundsTexturemapCanvas);			// DONE
        Renderer* setQuality(std::string quality);			// DONE
        Renderer* setSky(SDL_Surface* skyCanvas);			// DONE
        Renderer* setTexturemap(SDL_Surface* texturemapCanvas);		// DONE
        Renderer* setWaterHeight(int height);				// DONE
    };
}

#endif
