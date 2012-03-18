#include <iostream>	// cerr, endl
#include <iomanip>	// setfill(), setw()
#include <sstream>	// stringstream
#include <cstdio>	// fclose(), fopen()

#include "Steppe.hh"

using namespace std;
using namespace Steppe;

#define CANVAS_WIDTH	320
#define CANVAS_HEIGHT	200

SDL_Surface* createFromPng(string path)
{
    SDL_Surface* image;
    SDL_Surface* tempImage;

    if ( (tempImage = IMG_Load(path.c_str())) == NULL) {
        throw Steppe::Exception((string("Unable to open file: ") +
            path).c_str());
    }
#ifndef NDEBUG
    const SDL_PixelFormat& pixelFormat = *(tempImage->format);

    cerr << "BitsPerPixel = "  << (unsigned) pixelFormat.BitsPerPixel  << endl;
    cerr << "BytesPerPixel = " << (unsigned) pixelFormat.BytesPerPixel << endl;
    cerr << "Rloss = "         << (unsigned) pixelFormat.Rloss         << endl;
    cerr << "Gloss = "         << (unsigned) pixelFormat.Gloss         << endl;
    cerr << "Bloss = "         << (unsigned) pixelFormat.Bloss         << endl;
    cerr << "Aloss = "         << (unsigned) pixelFormat.Aloss         << endl;
    cerr << "Rshift = "        << (unsigned) pixelFormat.Rshift        << endl;
    cerr << "Gshift = "        << (unsigned) pixelFormat.Gshift        << endl;
    cerr << "Bshift = "        << (unsigned) pixelFormat.Bshift        << endl;
    cerr << "Ashift = "        << (unsigned) pixelFormat.Ashift        << endl;
    cerr << "Rmask = "         << (unsigned) pixelFormat.Rmask         << endl;
    cerr << "Gmask = "         << (unsigned) pixelFormat.Gmask         << endl;
    cerr << "Bmask = "         << (unsigned) pixelFormat.Bmask         << endl;
    cerr << "Amask = "         << (unsigned) pixelFormat.Amask         << endl;
    cerr << "colorkey = "      << (unsigned) pixelFormat.colorkey      << endl;
    cerr << "alpha = "         << (unsigned) pixelFormat.alpha         << endl;
#endif
    if ( (image = SDL_DisplayFormatAlpha(tempImage)) == NULL) {
#ifndef NDEBUG
        throw Steppe::Exception(std::string(std::string(
            "Unable to create image from png: ") +
            std::string(SDL_GetError()).c_str()).c_str());
#else
        throw Steppe::Exception("Unable to create image from png");
#endif
    }
    SDL_FreeSurface(tempImage);
    return image;
}

/*void printScreen(string path, gdImagePtr image)
{
    FILE *fp;

    if ( (fp = fopen(path.c_str(), "wb")) == NULL) {
        throw Steppe::Exception("Unable to open file");
    }
    gdImagePng(image, fp);
    if (fclose(fp) != 0) {
        throw Steppe::Exception("Unable to close file");
    }
}*/

void smooth(SDL_Surface* canvas, SDL_Surface* image)
{
    int radius = 2;
    SDL_Surface* temporaryImage;
    float total;
    int greyscale;
    const SDL_PixelFormat& pixelFormat = *(canvas->format);

    temporaryImage = SDL_CreateRGBSurface(0, image->w, image->h,
        pixelFormat.BitsPerPixel, pixelFormat.Rmask, pixelFormat.Gmask,
        pixelFormat.Bmask, pixelFormat.Amask);
    if (temporaryImage == NULL) {
        throw Steppe::Exception("Unable to create temporary image");
    }

    if (SDL_MUSTLOCK(temporaryImage)) {
        SDL_LockSurface(temporaryImage);
    }
    for (int y = 0; y < image->h; ++y) {
        for (int x = radius; x < image->w; ++x) {
            total = 0.0F;
            for (int kx = -radius; kx <= radius; ++kx) {
                total += (float) (Compositor::getPixel(image, x + kx, y) &
                    0xff);
            }
            greyscale = (int) (total / ((float) radius * 2.0F + 1.0F));
            Compositor::putPixel(temporaryImage, x, y,
                (greyscale << 16) | (greyscale << 8) | greyscale);
        }
    }
    if (SDL_MUSTLOCK(temporaryImage)) {
        SDL_UnlockSurface(temporaryImage);
    }

    if (SDL_MUSTLOCK(image)) {
        SDL_LockSurface(image);
    }
    for (int y = radius; y < image->h; ++y) {
        for (int x = 0; x < image->w; ++x) {
            total = 0.0F;
            for (int ky = -radius; ky <= radius; ++ky) {
                total += (float) (Compositor::getPixel(temporaryImage, x,
                    y + ky) & 0xff);
            }
            greyscale = (int) (total / ((float) radius * 2.0F + 1.0F));
            Compositor::putPixel(image, x, y,
                (greyscale << 16) | (greyscale << 8) | greyscale);
        }
    }
    if (SDL_MUSTLOCK(image)) {
        SDL_UnlockSurface(image);
    }
    SDL_FreeSurface(temporaryImage);
}

int main()
{
    SDL_Surface* heightmapCanvas;
    SDL_Surface* offscreenCanvas;
    SDL_Surface* skyCanvas;
    SDL_Surface* temporaryHeightmapCanvas;
    SDL_Surface* texturemapCanvas;
    struct Camera camera;
    unsigned char* heightmap;
    Compositor* compositor;
    Renderer* renderer;

    SDL_Init(SDL_INIT_EVERYTHING);

    offscreenCanvas          = SDL_SetVideoMode(CANVAS_WIDTH, CANVAS_HEIGHT,
                                   32, SDL_SWSURFACE);
    temporaryHeightmapCanvas = createFromPng("../images/heightmap.png");
    skyCanvas                = createFromPng("../images/sky.png");
    texturemapCanvas         = createFromPng("../images/texturemap.png");

//    printScreen("texturemap.png", texturemapCanvas);

    const SDL_PixelFormat& pixelFormat = *(offscreenCanvas->format);
    heightmapCanvas = SDL_CreateRGBSurface(0, 1024, 1024,
        pixelFormat.BitsPerPixel, pixelFormat.Rmask, pixelFormat.Gmask,
        pixelFormat.Bmask, pixelFormat.Amask);

/*    gdImageCopyResampled(heightmapCanvas, temporaryHeightmapCanvas,
        0, 0, 0, 0, gdImageSX(heightmapCanvas), gdImageSY(heightmapCanvas),
        gdImageSX(temporaryHeightmapCanvas),
        gdImageSY(temporaryHeightmapCanvas));*/
    if (SDL_MUSTLOCK(heightmapCanvas)) {
        SDL_LockSurface(heightmapCanvas);
    }
    for (int y = 0; y < heightmapCanvas->h; ++y) {
        for (int x = 0; x < heightmapCanvas->w; ++x) {
            Compositor::putPixel(heightmapCanvas, x, y, Compositor::getPixel(
                temporaryHeightmapCanvas, (x >> 2), (y >> 2)));
        }
    }
    if (SDL_MUSTLOCK(heightmapCanvas)) {
        SDL_UnlockSurface(heightmapCanvas);
    }
    smooth(offscreenCanvas, heightmapCanvas);

    compositor = new Compositor();
    compositor->setHeightmap(heightmapCanvas);
    heightmap = compositor->getHeightmap();

    renderer = new Renderer(offscreenCanvas);

    renderer->setTexturemap(texturemapCanvas)
        ->setOutOfBoundsTexturemap(heightmapCanvas)
        ->setQuality("high")
        ->setSky(skyCanvas)
        ->setHeightmap(compositor->getHeightmap())
        ->setOutOfBoundsHeightmap(compositor->getOutOfBoundsHeightmap())
        ->enable("smooth")
        ->enable("fog")
        ->setFogColor("#d7a67b")
        ->enable("reflection-map")
        ->setWaterHeight(80)
    ;

    camera = renderer->getCamera();
    camera.x = camera.z = 1024 + 512;
    camera.y = 225;
    renderer->setCamera(camera);

    for (int angle = 0; angle < 360; angle += 1) {
        camera.angle = angle;
        renderer->setCamera(camera);
        renderer->render();
        SDL_Flip(offscreenCanvas);

/*        stringstream ss;

        ss << setfill('0') << setw(3) << angle << ".png";
        printScreen(ss.str().c_str(), offscreenCanvas);*/
//        SDL_Delay(100);
    }

    delete compositor;
    delete renderer;
    SDL_FreeSurface(heightmapCanvas);
    SDL_FreeSurface(offscreenCanvas);
    SDL_FreeSurface(skyCanvas);
    SDL_FreeSurface(temporaryHeightmapCanvas);
    SDL_FreeSurface(texturemapCanvas);
    return 0;
}
