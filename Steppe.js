var Steppe = (function() {
    return {
        Compositor: function(undefined) {
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
                        throw('Invalid texturemapCanvas: not an instance of' +
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
                 * @return {array} ...
                 */
                getHeightmap: function() {
                    return _heightmap;
                },

                /**
                 * Get the out-of-bounds heightmap as an array.
                 *
                 * @return {array} ...
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
                 * @param {number} scaleFactor ...
                 * @return {Renderer} This (fluent interface).
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
                 * Set the heightmap to use for compositing
                 * [and out-of-bounds].
                 *
                 * @param {HTMLCanvasElement} heightmapCanvas The heightmap
                 *                                            canvas; should
                 *                                            contain a
                 *                                            greyscale image.
                 * @return {Compositor} This (fluent interface).
                 */
                setHeightmap: function(heightmapCanvas) {
                    var data = heightmapCanvas.getContext('2d').getImageData(
                        0, 0, 1024, 1024).data;

                    for (var y = 0; y < 1024; ++y) {
                        for (var x = 0; x < 1024; ++x) {
                            var index = (y << 10) + x;

                            _heightmap[index] = data[index << 2];
                            _outOfBoundsHeightmap[index] = data[index << 2];
                        }
                    }

                    return this;
                }
            };
        },

        Renderer: function(canvas, undefined) {
            var _CANVAS_WIDTH        = 320,	// 320 pixels
                _ANGLE_OF_VIEW       = 60,	// 60 degrees
                _ONE_DEGREE_ANGLE    = 1 / _ANGLE_OF_VIEW * _CANVAS_WIDTH,
                _THIRTY_DEGREE_ANGLE = _ONE_DEGREE_ANGLE * 30,
                _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE = _ONE_DEGREE_ANGLE * 360,
                _ANGULAR_INCREMENT   = _ANGLE_OF_VIEW / _CANVAS_WIDTH,
                _DEGREES_TO_RADIANS  = Math.PI / 180,
                _FAKE_DEGREES_TO_RADIANS = (2 * Math.PI) /
                    ((360 / _ANGLE_OF_VIEW) * _CANVAS_WIDTH),
                _RADIANS_TO_DEGREES  = 180 / Math.PI,
                _RADIANS_TO_FAKE_DEGREES = ((360 / _ANGLE_OF_VIEW) *
                    _CANVAS_WIDTH) / (2 * Math.PI),
                _SCALE_FACTOR = 35,
                _CAMERA_Y     = 175,
                _DISTANCE     = 75,
                _WATER_HEIGHT = 64;

            var _FASTEST   = 4,
                _DONT_CARE = 2,
                _NICEST    = 1;

            var _camera = { angle: 0, x: 0, y: _CAMERA_Y, z: 0 },
                _cosineLookupTable = [],
                _framebuffer = undefined,
                _heightmap = [],
                _inverseDistortionLookupTable = [],
                _outOfBoundsHeightmap = [],
                _outOfBoundsTexturemap,
                _rayLengthLookupTable = [],
                _sineLookupTable = [],
                _sky,
                _texturemap;

            var _fog = false,		// disabled (default)
                _quality = _DONT_CARE,	// medium quality (default)
                _smooth = 0,		// disabled (default)
                _waterHeight = -1;	// disabled (default)

            /**
             * Blend two colours together using an alpha value.
             *
             * @param {object} firstColor First (or source) colour
             * @param {object} secondColor Second (or destination) colour
             * @param {number} alpha Alpha value in the range 0..255
             * @return {object} Mixed colour
             */
            var _alphaBlend = function(firstColor, secondColor, alpha)
            {
                var normalisedAlpha = alpha / 255,
                    adjustedAlpha = 1 - normalisedAlpha;

                var mixedRed   = firstColor.red   * normalisedAlpha | 0,
                    mixedGreen = firstColor.green * normalisedAlpha | 0,
                    mixedBlue  = firstColor.blue  * normalisedAlpha | 0;

                mixedRed   += Math.floor(secondColor.red   * adjustedAlpha);
                mixedGreen += Math.floor(secondColor.green * adjustedAlpha);
                mixedBlue  += Math.floor(secondColor.blue  * adjustedAlpha);

                return { red: mixedRed, green: mixedGreen, blue: mixedBlue };
            };

            /**
             * Get a pixel from the out-of-bounds texturemap.
             *
             * @param {number} x ...
             * @param {number} y ...
             * @return {object} ...
             */
            var _getPixelFromOutOfBoundsTexturemap = function(x, y) {
                if (_outOfBoundsTexturemap !== undefined) {
                    var index = (y << 12) + (x << 2);

                    return {
                        red:   _outOfBoundsTexturemap[index],
                        green: _outOfBoundsTexturemap[index + 1],
                        blue:  _outOfBoundsTexturemap[index + 2]
                    };
                } else {
                    return {
                        red:   127,
                        green: 127,
                        blue:  127
                    };
                }
            };

            /**
             * ...
             *
             * @param {number} x ...
             * @param {number} y ...
             */
            var _getPixelFromSky = function(x, y) {
                var currentAngle = _camera.angle - _THIRTY_DEGREE_ANGLE;

                if (currentAngle < 0) {
                    currentAngle += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
                }

                if (y < 0) {
                    y = 0;
                } else if (y >= 100) {
                    y = 100 - 1;
                }

                var data = _sky.getContext('2d').getImageData(
                    (currentAngle + x | 0) % 1920, y, 1, 1).data;

                return { red: data[0], green: data[1], blue: data[2] };
            };

            /**
             * ...
             *
             * @param {number} x ...
             * @param {number} y ...
             * @return {object} ...
             */
            var _getPixelFromTexturemap = function(x, y) {
                var index = (y << 12) + (x << 2);

                return {
                    red:   _texturemap[index],
                    green: _texturemap[index + 1],
                    blue:  _texturemap[index + 2]
                };
            };

            /**
             * Initialise the inverse distortion lookup table (for removing
             * fisheye).
             */
            var _initInverseDistortionLookupTable = function() {
                for (var angleOfRotation = 0;
                    angleOfRotation < _THIRTY_DEGREE_ANGLE;
                    ++angleOfRotation) {
                    var angleOfRotationInRadians = angleOfRotation *
                        _ANGULAR_INCREMENT * _DEGREES_TO_RADIANS;

                    _inverseDistortionLookupTable[angleOfRotation +
                        _THIRTY_DEGREE_ANGLE] = 1 /
                        Math.cos(angleOfRotationInRadians);
                    _inverseDistortionLookupTable[
                        _THIRTY_DEGREE_ANGLE - angleOfRotation] =
                        1 / Math.cos(angleOfRotationInRadians);
                }
            };

            /**
             * Initialise (or recalculate) the ray-length lookup table.
             *
             * @param {number} y ...
             * @param {number} distance ...
             */
            var _initRayLengthLookupTable = function(y, distance) {
                for (var ray = 1; ray < 320; ++ray) {
                    for (var row = 50; row < 200 + 100; ++row) {
                        var invertedRow = 200 - row;

                        var rayLength = _inverseDistortionLookupTable[ray] *
                            ((distance * y) / (y - invertedRow));

                        _rayLengthLookupTable[row * 320 + ray] = rayLength;
                    }
                }
            };

            /**
             * Initialise the sine and cosine lookup tables.
             */
            var _initSineAndCosineLookupTables = function() {
                for (var angleOfRotation = 0;
                    angleOfRotation < _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
                    ++angleOfRotation) {
                    var angleOfRotationInRadians = angleOfRotation *
                        _ANGULAR_INCREMENT * _DEGREES_TO_RADIANS;

                    _sineLookupTable[angleOfRotation]   = Math.sin(
                        angleOfRotationInRadians);
                    _cosineLookupTable[angleOfRotation] = Math.cos(
                        angleOfRotationInRadians);
                }
            };

            /**
             * Render the 360-degree panoramic sky based on the camera's angle
             * of rotation.
             */
            var _renderSky = function() {
                var angleOfRotation = _camera.angle - _THIRTY_DEGREE_ANGLE;

                if (angleOfRotation < 0) {
                    angleOfRotation += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
                }

                angleOfRotation |= 0;

                var skyWidth  = _sky.width;
                var skyHeight = _sky.height;

                if (angleOfRotation + 320 <= skyWidth) {
                    _framebuffer.drawImage(_sky, angleOfRotation, 0, 320,
                        skyHeight, 0, 0, 320, skyHeight);
                } else {
                    _framebuffer.drawImage(_sky, angleOfRotation, 0,
                        skyWidth - angleOfRotation, skyHeight, 0, 0,
                        skyWidth - angleOfRotation, skyHeight);
                    _framebuffer.drawImage(_sky, 0, 0,
                        320 - (skyWidth - angleOfRotation),
                        skyHeight, skyWidth - angleOfRotation, 0,
                        320 - (skyWidth - angleOfRotation), skyHeight);
                }
            };

            /**
             * Render the terrain (landscape).
             */
            var _renderTerrain = function() {
                _framebuffer.fillStyle = '#7f7f7f';
                _framebuffer.fillRect(0, 100, 320, 100);

                var initialAngle = _camera.angle - _THIRTY_DEGREE_ANGLE;

                if (initialAngle < 0) {
                    initialAngle += _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
                }

                initialAngle |= 0;

                var currentAngle = initialAngle;

                for (var ray = _quality; ray < 320; ray += _quality) {
                    var previousTop = 200 + 100 - 1;

                    for (var row = 200 + 100 - 1; row >= 50; --row) {
                        rayLength = _rayLengthLookupTable[
                            (row << 8) + (row << 6) + ray];

                        var rayX = _camera.x + rayLength *
                            _cosineLookupTable[currentAngle] | 0;
                        var rayZ = _camera.z - rayLength *
                            _sineLookupTable[currentAngle] | 0;

                        var u = rayX & 1023;
                        var v = rayZ & 1023;

                        var height;
                        if ((rayX < 1024 || rayX > 1024 + 1024 ||
                            rayZ < 1024 || rayZ > 1024 + 1024) &&
                            _outOfBoundsHeightmap.length > 0) {
                            height = _outOfBoundsHeightmap[(v << 10) + u];
                        } else {
                            height = _heightmap[(v << 10) + u];
                        }

                        var scale = height * _SCALE_FACTOR /
                            (rayLength + 1) | 0;

                        var top = 50 + row - scale,
                            bottom = top + scale;

                        if (top < previousTop) {
                            bottom = previousTop;
                            previousTop = top;

                            var color = '';

                            if (rayX < 1024 || rayX > 1024 + 1024 ||
                                rayZ < 1024 || rayZ > 1024 + 1024) {
                                var texel = _getPixelFromOutOfBoundsTexturemap(
                                    u, v);

                                if (_fog) {
                                    var foggedTexel = _alphaBlend(texel,
                                        { red: 127, green: 127, blue: 127 },
                                        ~~(row / 150 * 255));

                                    texel = foggedTexel;
                                }

                                color = 'rgb(' + texel.red +
                                    ', ' + texel.green + ', ' +
                                    texel.blue + ')';
                            } else {
                                if (height < _waterHeight) {
                                    var data = _getPixelFromSky(ray,
                                        200 - top);

                                    var texel = _getPixelFromTexturemap(u, v);

                                    var mixedColor = _alphaBlend(data,
                                        texel, ~~((_waterHeight - height) /
                                        _waterHeight * 255 * 2));

                                    texel = mixedColor;

                                    if (_fog) {
                                        var foggedTexel = _alphaBlend(
                                            mixedColor,
                                            { red: 128, green: 128, blue: 128 },
                                            ~~(row / 100 * 255));

                                        texel = foggedTexel;
                                    }

                                    height = _waterHeight;

                                    color = 'rgb(' + texel.red + ', ' +
                                        texel.green + ', ' +
                                        texel.blue + ')';
                                } else {
                                    var texel = _getPixelFromTexturemap(u, v);

                                    if (_fog) {
                                        var foggedTexel = _alphaBlend(texel,
                                            { red: 127, green: 127, blue: 127 },
                                            ~~(row / 150 * 255));

                                        texel = foggedTexel;
                                    }

                                    color = 'rgb(' + texel.red +
                                        ', ' + texel.green + ', ' +
                                        texel.blue + ')';
                                }
                            }

                            // Render sliver...
                            if (bottom > 199) {
                                bottom = 199;
                            }

                            _framebuffer.fillStyle = color;
                            if (ray > _quality) {
                                // Not the left-most ray...
                                _framebuffer.fillRect(ray, top - _smooth,
                                    _quality, bottom - top + 1);
                            } else {
                                // Left-most ray: we don't cast rays for
                                // column 0!
                                _framebuffer.fillRect(0, top - _smooth,
                                    _quality << 1, bottom - top + 1);
                            }
                        }
                    }

                    currentAngle += _quality;
                    if (currentAngle >=
                        _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE) {
                        currentAngle = 0;
                    }
                }
            };

            if (canvas.width !== _CANVAS_WIDTH) {
                throw('Canvas width not equal to ' + _CANVAS_WIDTH);
            }
            if (canvas.height !== 200) {
                throw('Canvas height not equal to 200');
            }

            _framebuffer = canvas.getContext('2d');

            _initSineAndCosineLookupTables();
            _initInverseDistortionLookupTable();
            _initRayLengthLookupTable(_CAMERA_Y, _DISTANCE);

            return {
                /**
                 * Disable a Steppe capability.
                 *
                 * @param {string} capability Specifies a string indicating a
                 *                            Steppe capability; 'fog',
                 *                            'reflection-map' and 'smooth' are
                 *                            currently implemented.
                 * @return {Renderer} This (fluent interface).
                 */
                disable: function(capability) {
                    if (capability === 'fog') {
                        _fog = false;
                    } else if (capability === 'reflection-map') {
                        _waterHeight = -1;
                    } else if (capability === 'smooth') {
                        _smooth = 0;
                    } else {
                        throw("Can't disable unknown capability");
                    }
                    return this;
                },

                /**
                 * Enable a Steppe capability.
                 *
                 * @param {string} capability Specifies a string indicating a
                 *                            Steppe capability; 'fog',
                 *                            'reflection-map' and 'smooth' are
                 *                            currently implemented.
                 * @return {Renderer} This (fluent interface).
                 */
                enable: function(capability) {
                    if (capability === 'fog') {
                        _fog = true;
                    } else if (capability === 'reflection-map') {
                        _waterHeight = _WATER_HEIGHT;
                    } else if (capability === 'smooth') {
                        _smooth = 0.5;
                    } else {
                        throw("Can't enable unknown capability");
                    }
                    return this;
                },

                /**
                 * Get the current camera.
                 *
                 * @return {object} ...
                 */
                getCamera: function() {
                    return {
                        angle: _camera.angle /
                            _THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE * 360 +
                            0.5 | 0,
                        x:     _camera.x,
                        y:     _camera.y,
                        z:     _camera.z
                    };
                },

                /**
                 * ...
                 *
                 * @param {number} x ...
                 * @param {number} z ...
                 * @return {number} ...
                 */
                getHeight: function(x, z) {
                    var u = x & 1023;
                    var v = z & 1023;

                    return _heightmap[(v << 10) + u];
                },

                /**
                 * Set render quality.
                 *
                 * @param {string} quality Specifies a string indicating the
                 *                         render quality from 'low', through
                 *                         'medium', to 'high'.
                 * @return {Renderer} This (fluent interface).
                 */
                setQuality: function(quality) {
                    if (quality === 'medium') {
                        _quality = _DONT_CARE;
                    } else if (quality === 'low') {
                        _quality = _FASTEST;
                    } else if (quality === 'high') {
                        _quality = _NICEST;
                    } else {
                        throw("Invalid quality; must be 'low', 'medium', " +
                            "or 'high'");
                    }

                    return this;
                },

                /**
                 * Test whether a capability is enabled.
                 *
                 * @param {string} capability Specifies a string indicating a
                 *                            Steppe capability; 'smooth' and
                 *                            'reflection-map' are currently
                 *                            implemented.
                 * @return {boolean} Returns true if capability is an enabled
                 *                   capability and returns false otherwise.
                 */
                isEnabled: function(capability) {
                    if (capability === 'fog') {
                        return _fog;
                    } else if (capability === 'reflection-map') {
                        return (_waterHeight > -1);
                    } else if (capability === 'smooth') {
                        return (_smooth === 0.5);
                    }
                    throw('Unknown capability');
                },

                /**
                 * Render the terrain (landscape) including the sky.
                 */
                render: function() {
                    _renderSky();
                    _renderTerrain();
                },

                /**
                 * Set the current camera.
                 *
                 * @param {object} camera ...
                 * @return {Renderer} This (fluent interface).
                 */
                setCamera: function(camera) {
                    if (typeof(camera) != 'object') {
                        throw('Invalid camera: not an object');
                    }

                    _camera.angle = (camera.angle !== undefined &&
                        typeof(camera.angle) == 'number') ?
                        (Math.abs(~~(camera.angle + 0.5)) % 360 /
                        _ANGLE_OF_VIEW * 320) :
                        (_camera.angle);
                    _camera.x = (camera.x !== undefined &&
                        typeof(camera.x) == 'number') ?
                        (~~(camera.x + 0.5)) : (_camera.x);

                    if (camera.y !== undefined &&
                        typeof(camera.y) == 'number') {
                        _camera.y = ~~(camera.y + 0.5);
                        _initRayLengthLookupTable(_camera.y, _DISTANCE);
                    }

                    _camera.y = (camera.y !== undefined &&
                        typeof(camera.y) == 'number') ?
                        (~~(camera.y + 0.5)) : (_camera.y);
                    _camera.z = (camera.z !== undefined &&
                        typeof(camera.z) == 'number') ?
                        (~~(camera.z + 0.5)) : (_camera.z);

                    return this;
                },

                /**
                 * Set the heightmap to use for terrain rendering.
                 *
                 * @param {HTMLCanvasElement} heightmapCanvas The heightmap
                 *                                            canvas; should
                 *                                            contain a
                 *                                            greyscale image.
                 * @return {Renderer} This (fluent interface).
                 */
                setHeightmap: function(heightmap) {
                    _heightmap = heightmap;

                    return this;
                },

                /**
                 * ...
                 *
                 * @param {array} outOfBoundsHeightmap ...
                 * @return {Renderer} This (fluent interface).
                 */
                setOutOfBoundsHeightmap: function(outOfBoundsHeightmap) {
                    _outOfBoundsHeightmap = outOfBoundsHeightmap;

                    return this;
                },

                /**
                 * ...
                 *
                 * @param {HTMLCanvasElement} texturemapCanvas ...
                 * @return {Renderer} This (fluent interface).
                 */
                setOutOfBoundsTexturemap: function(
                    outOfBoundsTexturemapCanvas) {
                    if ( !(outOfBoundsTexturemapCanvas instanceof
                        HTMLCanvasElement)) {
                        throw('Invalid outOfBoundsTexturemapCanvas: not an ' +
                            'instance of HTMLCanvasElement');
                    }

                    if (outOfBoundsTexturemapCanvas.width != 1024) {
                        throw('outOfBoundsTexturemapCanvas width not equal ' +
                            'to 1024');
                    }
                    if (outOfBoundsTexturemapCanvas.height != 1024) {
                        throw('outOfBoundsTexturemapCanvas height not equal ' +
                            'to 1024');
                    }

                    _outOfBoundsTexturemap =
                        outOfBoundsTexturemapCanvas.getContext('2d')
                        .getImageData(0, 0, outOfBoundsTexturemapCanvas.width,
                        outOfBoundsTexturemapCanvas.height).data;

                    return this;
                },

                /**
                 * Set the canvas to use for 360-degree panoramic sky.
                 *
                 * @param {HTMLCanvasElement} skyCanvas The sky canvas; must be
                 *                                      1920x100.
                 */
                setSky: function(skyCanvas) {
                    if ( !(skyCanvas instanceof HTMLCanvasElement)) {
                        throw('Invalid skyCanvas: not an instance of ' +
                            'HTMLCanvasElement');
                    }

                    if (skyCanvas.width != 1920) {
                        throw('skyCanvas width not equal to 1920');
                    }
                    if (skyCanvas.height != 100) {
                        throw('skyCanvas height not equal to 100');
                    }

                    _sky = skyCanvas;

                    return this;
                },

                /**
                 * ...
                 *
                 * @param {HTMLCanvasElement} texturemapCanvas ...
                 * @return {Renderer} This (fluent interface).
                 */
                setTexturemap: function(texturemapCanvas) {
                    if ( !(texturemapCanvas instanceof HTMLCanvasElement)) {
                        throw('Invalid texturemapCanvas: not an instance of ' +
                            'HTMLCanvasElement');
                    }

                    if (texturemapCanvas.width != 1024) {
                        throw('texturemapCanvas width not equal to 1024');
                    }
                    if (texturemapCanvas.height != 1024) {
                        throw('texturemapCanvas height not equal to 1024');
                    }

                    _texturemap = texturemapCanvas.getContext('2d')
                        .getImageData(0, 0, texturemapCanvas.width,
                        texturemapCanvas.height).data;

                    return this;
                },

                /**
                 * Set height of the reflection-mapped water.
                 *
                 * @param {number} height Globally-defined height of the
                 *                        reflection-mapped water. It must be
                 *                        in the range 0..255.
                 * @return {Renderer} This (fluent interface).
                 */
                setWaterHeight: function(height) {
                    if (_waterHeight == -1) {
                        throw('Capability not enabled');
                    }

                    if (typeof(height) != 'number') {
                        throw('Invalid height: not a number');
                    }

                    if (height < 0 || height > 255) {
                        throw('Invalid height: must be in the range 0..255');
                    }

                    _waterHeight = height;

                    return this;
                }
            };
        }
    };
})();
