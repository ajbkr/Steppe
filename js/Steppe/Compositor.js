/**
 * Compositor object.
 *
 * @author Andrew J. Baker
 */

/** @namespace Steppe */
var Steppe = (function(Steppe) {
    /** @class */
    Steppe.Compositor = function(undefined) {
        var _heightmap = [],
            _outOfBoundsHeightmap = [],
            _textureArray = [];

        return {
            /**
             * Add a texture to the texture-array.
             *
             * @param {number} height The 'height' at which to apply the
             *                        texture.
             * @param {Image} textureImage The image to use as a texture.
             * @return {Compositor} This (fluent interface).
             */
            addTexture: function(height, textureImage) {
                if ( !(textureImage instanceof HTMLImageElement)) {
                    throw('Invalid textureImage: not an instance of ' +
                        'HTMLImageElement');
                }

                if (height < 0 || height > 255) {
                    throw('Invalid height; must be in the range 0..255');
                }

                if (textureImage.width != 256 ||
                    textureImage.height != 256) {
                    throw('Invalid texture dimensions; must be 256x256');
                }

                var textureCanvas    = document.createElement('canvas');
                textureCanvas.width  = textureImage.width;
                textureCanvas.height = textureImage.height;

                var textureContext = textureCanvas.getContext('2d');

                textureContext.drawImage(textureImage, 0, 0,
                    textureCanvas.width, textureCanvas.height);

                _textureArray[height] = {
                    data:   textureContext.getImageData(0, 0,
                                textureCanvas.width,
                                textureCanvas.height).data,
                    height: textureCanvas.height,
                    width:  textureCanvas.width
                };

                return this;
            },

            /**
             * Create a composite texturemap from the heightmap and
             * texture-array images.
             *
             * @param {HTMLCanvasElement} texturemapCanvas The texturemap
             *                            canvas to which the heightmap and
             *                            texture-array images are
             *                            composited.
             */
            composite: function(texturemapCanvas) {
                if ( !(texturemapCanvas instanceof HTMLCanvasElement)) {
                    throw('Invalid texturemapCanvas: not an instance of ' +
                        'HTMLCanvasElement');
                }

                var texturemapContext = texturemapCanvas.getContext('2d');

                var textureCanvas = document.createElement('canvas');
                textureCanvas.width = 256;
                textureCanvas.height = 256;

                var textureContext = textureCanvas.getContext('2d');

                if (_textureArray[255] === undefined) {
                    throw('No texture added at height 255; unable to ' +
                        'composite');
                }
                for (var i = 254; i >= 0; --i) {
                    if (_textureArray[i] === undefined) {
                        _textureArray[i] = _textureArray[i + 1];
                    }
                }

                for (var y = 0; y < 1024; ++y) {
                    for (var x = 0; x < 1024; ++x) {
                        var height = _heightmap[(y << 10) + x];

                        var index = ((y & 255) << 10) + ((x & 255) << 2);

                        texturemapContext.fillStyle = 'rgb(' +
                            _textureArray[height].data[index] + ',' +
                            _textureArray[height].data[index + 1] + ',' +
                            _textureArray[height].data[index + 2] + ')';

                        texturemapContext.fillRect(x, y, 1, 1);
                    }
                }
            },

            /**
             * Get the heightmap as an array.
             *
             * @return {array} The heightmap canvas converted to an array.
             */
            getHeightmap: function() {
                return _heightmap;
            },

            /**
             * Get the out-of-bounds heightmap as an array.
             *
             * @return {array} The out-of-bounds heightmap canvas converted to
             *                 an array.
             */
            getOutOfBoundsHeightmap: function() {
                return _outOfBoundsHeightmap;
            },

            /**
             * Put a mask (a 2.5D sprite's heightmap).
             *
             * @param {HTMLImageElement} mask The 2.5D sprite's heightmap;
             *                                should contain a greyscale
             *                                image.
             * @param {number} x The x-ordinate.
             * @param {number} y The y-ordinate.
             * @param {number} scaleFactor The scale factor to multiply each
             *                             value in the mask by.
             * @return {Compositor} This (fluent interface).
             */
            putMask: function(mask, x, y, scaleFactor) {
                var maskCanvas = document.createElement('canvas');
                maskCanvas.width  = mask.width;
                maskCanvas.height = mask.height;

                var maskContext = maskCanvas.getContext('2d');

                maskContext.drawImage(mask, 0, 0);

                var data = maskContext.getImageData(0, 0, maskCanvas.width,
                    maskCanvas.height).data;

                for (var y2 = 0; y2 < maskCanvas.height; ++y2) {
                    for (var x2 = 0; x2 < maskCanvas.width; ++x2) {
                        var index = y2 * maskCanvas.width + x2;

                        if (data[index * 4 + 3]) {
                            var index = ((y2 + y) << 10) + (x2 + x);

                            _heightmap[index] = 192 +
                                data[(y2 * maskCanvas.width + x2) * 4] *
                                scaleFactor;
                        }
                    }
                }

                return this;
            },

            /**
             * Set the heightmap to use for compositing [and
             * out-of-bounds].
             *
             * @param {HTMLCanvasElement} heightmapCanvas The heightmap
             *                                            canvas; should
             *                                            contain a
             *                                            greyscale image.
             * @return {Compositor} This (fluent interface).
             */
            setHeightmap: function(heightmapCanvas) {
                var data = heightmapCanvas.getContext('2d')
                    .getImageData(0, 0, 1024, 1024).data;

                for (var y = 0; y < 1024; ++y) {
                    for (var x = 0; x < 1024; ++x) {
                        var index = (y << 10) + x;

                        _heightmap[index] = data[index << 2];
                        _outOfBoundsHeightmap[index] = _heightmap[index];
                    }
                }

                return this;
            }
        };
    };

    return Steppe;
} (Steppe || { }) );
