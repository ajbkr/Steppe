<?php
class Steppe_Renderer
{
    const _CANVAS_WIDTH  = 320;
    const _ANGLE_OF_VIEW = 60;
    private $_ONE_DEGREE_ANGLE;
    private $_THIRTY_DEGREE_ANGLE;
    private $_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
    private $_ANGULAR_INCREMENT;
    private $_DEGREES_TO_RADIANS;
    private $_FAKE_DEGREES_TO_RADIANS;
    private $_RADIANS_TO_DEGREES;
    private $_RADIANS_TO_FAKE_DEGREES;
    const _SCALE_FACTOR = 35;
    const _CAMERA_Y     = 175;
    const _DISTANCE     = 75;
    const _WATER_HEIGHT = 64;

    const _FASTEST   = 4;
    const _DONT_CARE = 2;
    const _NICEST    = 1;

    private $_camera;
    private $_cosineLookupTable;
    private $_framebuffer;
    private $_heightmap;
    private $_inverseDistortionLookupTable;
    private $_outOfBoundsHeightmap;
    private $_outOfBoundsTexturemap;
    private $_rayLengthLookupTable;
    private $_sineLookupTable;
    private $_sky;
    private $_texturemap;

    private $_fog;
    private $_quality;
    private $_smooth;
    private $_waterHeight;

    public function __construct($canvas)
    {
        $this->_ONE_DEGREE_ANGLE = 1 / self::_ANGLE_OF_VIEW *
            self::_CANVAS_WIDTH;
        $this->_THIRTY_DEGREE_ANGLE = $this->_ONE_DEGREE_ANGLE * 30;
        $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE =
            $this->_ONE_DEGREE_ANGLE * 360;
        $this->_ANGULAR_INCREMENT = self::_ANGLE_OF_VIEW / self::_CANVAS_WIDTH;
        $this->_DEGREES_TO_RADIANS = M_PI / 180;
        $this->_FAKE_DEGREES_TO_RADIANS = (2 * M_PI) /
            ((360 / self::_ANGLE_OF_VIEW) * self::_CANVAS_WIDTH);
        $this->_RADIANS_TO_DEGREES = 180 / M_PI;
        $this->_RADIANS_TO_FAKE_DEGREES = ((360 / self::_ANGLE_OF_VIEW) *
            self::_CANVAS_WIDTH) / (2 * M_PI);

        $this->_camera = array(
            'angle' => 0,
            'x' => 0,
            'y' => self::_CAMERA_Y,
            'z' => 0
        );
        $this->_cosineLookupTable = array();
        $this->_framebuffer = NULL;
        $this->_heightmap = array();
        $this->_inverseDistortionLookupTable = array();
        $this->_outOfBoundsHeightmap = array();
        $this->_outOfBoundsTexturemap = NULL;
        $this->_rayLengthLookupTable = array();
        $this->_sineLookupTable = array();
        $this->_sky = NULL;
        $this->_texturemap = NULL;

        $this->_fog = FALSE;			// disabled (default)
        $this->_quality = self::_DONT_CARE;	// medium quality (default)
        $this->_smooth = 0;			// disabled (default)
        $this->_waterHeight = -1;		// disabled (default)

        if (imagesx($canvas) != self::_CANVAS_WIDTH) {
            throw new Exception('Canvas width not equal to ' .
                self::_CANVAS_WIDTH);
        }
        if (imagesy($canvas) != 200) {
            throw new Exception('Canvas height not equal to 200');
        }

        $this->_framebuffer = $canvas;

        $this->_initSineAndCosineLookupTables();
        $this->_initInverseDistortionLookupTable();
        $this->_initRayLengthLookupTable(self::_CAMERA_Y, self::_DISTANCE);
    }

    /**
     * Blend two colours together using an alpha value.
     *
     * @param integer $firstColor First (or source) colour.
     * @param integer $secondColor Second (or destination) colour.
     * @param integer $alpha Alpha value in the range 0..255.
     * @return integer Mixed colour.
     */
    private function _alphaBlend($firstColor, $secondColor, $alpha)
    {
        if ($alpha < 0) {
            $alpha = 0;
        } else if ($alpha > 255) {
            $alpha = 255;
        }

        $normalisedAlpha = $alpha / 255;
        $adjustedAlpha = 1 - $normalisedAlpha;

        $mixedRed   = floor((($firstColor >> 16) & 0xff) * $normalisedAlpha);
        $mixedGreen = floor((($firstColor >> 8)  & 0xff) * $normalisedAlpha);
        $mixedBlue  = floor(( $firstColor        & 0xff) * $normalisedAlpha);

        $mixedRed   += floor((($secondColor >> 16) & 0xff) * $adjustedAlpha);
        $mixedGreen += floor((($secondColor >> 8)  & 0xff) * $adjustedAlpha);
        $mixedBlue  += floor(( $secondColor        & 0xff) * $adjustedAlpha);

        return ($mixedRed << 16) | ($mixedGreen << 8) | $mixedBlue;
    }

    /**
     * Get a pixel from the out-of-bounds texturemap.
     *
     * @param integer $x ...
     * @param integer $y ...
     * @return integer ...
     */
    private function _getPixelFromOutOfBoundsTexturemap($x, $y)
    {
        if (isset($this->_outOfBoundsTexturemap)) {
            return imagecolorat($this->_outOfBoundsTexturemap, $x, $y);
        } else {
            return 0x007f7f7f;
        }
    }

    /**
     * ...
     *
     * @param integer $x ...
     * @param integer $y ...
     * @return integer ...
     */
    private function _getPixelFromSky($x, $y)
    {
        $currentAngle = $this->_camera['angle'] - $this->_THIRTY_DEGREE_ANGLE;

        if ($currentAngle < 0) {
            $currentAngle += $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

        if ($y < 0) {
            $y = 0;
        } else if ($y >= 100) {
            $y = 100 - 1;
        }

        return imagecolorat($this->_sky, floor($currentAngle + $x) % 1920,
            $y);
    }

    /**
     * ...
     *
     * @param integer $x
     * @param integer $y
     * @return integer ...
     */
    private function _getPixelFromTexturemap($x, $y)
    {
        return imagecolorat($this->_texturemap, $x, $y);
    }

    /**
     * Initialise the inverse distortion lookup table (for removing fisheye).
     *
     * @return void
     */
    private function _initInverseDistortionLookupTable()
    {
        for ($angleOfRotation = 0;
            $angleOfRotation < $this->_THIRTY_DEGREE_ANGLE;
            ++$angleOfRotation) {
            $angleOfRotationInRadians = $angleOfRotation *
                $this->_ANGULAR_INCREMENT * $this->_DEGREES_TO_RADIANS;

            $this->_inverseDistortionLookupTable[$angleOfRotation +
                $this->_THIRTY_DEGREE_ANGLE] = 1 /
                cos($angleOfRotationInRadians);
            $this->_inverseDistortionLookupTable[
                $this->_THIRTY_DEGREE_ANGLE - $angleOfRotation] =
                1 / cos($angleOfRotationInRadians);
        }
    }

    /**
     * Initialise (or recalculate) the ray-length lookup table.
     *
     * @param integer $y ...
     * @param integer $distance ...
     * @return void
     */
    private function _initRayLengthLookupTable($y, $distance)
    {
        for ($ray = 1; $ray < 320; ++$ray) {
            for ($row = 50; $row < 200 + 100; ++$row) {
                $invertedRow = 200 - $row;

                $rayLength = $this->_inverseDistortionLookupTable[$ray] *
                    (($distance * $y) / ($y - $invertedRow));

                $this->_rayLengthLookupTable[$row * 320 + $ray] = $rayLength;
            }
        }
    }

    /**
     * Initialise the sine and cosine lookup tables.
     *
     * @return void
     */
    private function _initSineAndCosineLookupTables()
    {
        for ($angleOfRotation = 0;
            $angleOfRotation < $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
            ++$angleOfRotation) {
            $angleOfRotationInRadians = $angleOfRotation *
                $this->_ANGULAR_INCREMENT * $this->_DEGREES_TO_RADIANS;

            $this->_sineLookupTable[$angleOfRotation]   = sin(
                $angleOfRotationInRadians);
            $this->_cosineLookupTable[$angleOfRotation] = cos(
                $angleOfRotationInRadians);
        }
    }

    /**
     * Render the 360-degree panoramic sky based on the camera's angle of
     * rotation.
     */
    private function _renderSky()
    {
        $angleOfRotation = $this->_camera['angle'] -
            $this->_THIRTY_DEGREE_ANGLE;

        if ($angleOfRotation < 0) {
            $angleOfRotation += $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

        $angleOfRotation = floor($angleOfRotation);

        $skyWidth  = imagesx($this->_sky);
        $skyHeight = imagesy($this->_sky);

        if ($angleOfRotation + 320 <= $skyWidth) {
            imagecopy($this->_framebuffer, $this->_sky, 0, 0, $angleOfRotation,
                0, 320, $skyHeight);
        } else {
            imagecopy($this->_framebuffer, $this->_sky, 0, 0, $angleOrRotation,
                0, $skyWidth - $angleOfRotation, $skyHeight);
            imagecopy($this->_framebuffer, $this->_sky,
                $skyWidth - $angleOfRotation, 0,
                $skyWidth - $angleOfRotation, 0,
                320 - ($skyWidth - $angleOfRotation), $skyHeight);
        }
    }

    /**
     * Render the terrain (landscape).
     */
    private function _renderTerrain()
    {
        imagefilledrectangle($this->_framebuffer, 0, 100, 319, 124,
            0x007f7f7f);

        $initialAngle = $this->_camera['angle'] - $this->_THIRTY_DEGREE_ANGLE;

        if ($initialAngle < 0) {
            $initialAngle += $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE;
        }

        $initialAngle = floor($initialAngle);

        $currentAngle = $initialAngle;

        for ($ray = $this->_quality; $ray < 320; $ray += $this->_quality) {
            $previousTop = 200 + 100 - 1;

            for ($row = 200 + 100 - 1; $row >= 50; --$row) {
                $rayLength = $this->_rayLengthLookupTable[
                    ($row << 8) + ($row << 6) + $ray];

                $rayX = floor($this->_camera['x'] + $rayLength *
                    $this->_cosineLookupTable[$currentAngle]);
                $rayZ = floor($this->_camera['z'] + $rayLength *
                    $this->_sineLookupTable[$currentAngle]);

                $u = $rayX & 1023;
                $v = $rayZ & 1023;

                $height = NULL;
                if (($rayX < 1024 || $rayX > 1024 + 1024 ||
                    $rayZ < 1024 || $rayZ > 1024 + 1024) &&
                    count($this->_outOfBoundsHeightmap) > 0) {
                    $height = $this->_outOfBoundsHeightmap[($v << 10) + $u];
                } else {
                    $height = $this->_heightmap[($v << 10) + $u];
                }

                $scale = floor($height * self::_SCALE_FACTOR /
                    ($rayLength + 1));

                $top = 50 + $row - $scale;
                $bottom = $top + $scale;

                if ($top < $previousTop) {
                    $bottom = $previousTop;
                    $previousTop = $top;

                    $color = 0;

                    if ($rayX < 1024 || $rayX > 1024 + 1024 ||
                        $rayZ < 1024 || $rayZ > 1024 + 1024) {
                        $texel = $this->_getPixelFromOutOfBoundsTexturemap($u,
                            $v);

                        if ($this->_fog) {
                            $foggedTexel = $this->_alphaBlend($texel,
                                0x007f7f7f, floor($row / 150 * 255));

                            $texel = $foggedTexel;
                        }

                        $color = $texel;
                    } else {
                        if ($height < $this->_waterHeight) {
                            $data = $this->_getPixelFromSky($ray, 200 - $top);

                            $texel = $this->_getPixelFromTexturemap($u, $v);

                            $mixedColor = $this->_alphaBlend($data,
                                $texel, floor(($this->_waterHeight - $height) /
                                $this->_waterHeight * 255 * 2));

                            $texel = $mixedColor;

                            if ($this->_fog) {
                                $foggedTexel = $this->_alphaBlend($mixedColor,
                                    0x007f7f7f, floor($row / 100 * 255));

                                $texel = $foggedTexel;
                            }

                            $height = $this->_waterHeight;

                            $color = $texel;
                        } else {
                            $texel = $this->_getPixelFromTexturemap($u, $v);

                            if ($this->_fog) {
                                $foggedTexel = $this->_alphaBlend($texel,
                                    0x007f7f7f, floor($row / 150 * 255));

                                $texel = $foggedTexel;
                            }

                            $color = $texel;
                        }
                    }

                    // Render sliver...
                    if ($bottom > 199) {
                        $bottom = 199;
                    }

                    if ($ray > $this->_quality) {
                        // Not the left-most ray...
                        imagefilledrectangle($this->_framebuffer, $ray,
                            $top /*- $smooth*/, $ray + $this->_quality - 1,
                            $bottom, $color);
                    } else {
                        // Left-most ray: we don't cast rays for column 0!
                        imagefilledrectangle($this->_framebuffer, 0,
                            $top /*- $smooth*/, ($this->_quality << 1) - 1,
                            $bottom, $color);
                    }
                }
            }

            $currentAngle += $this->_quality;
            if ($currentAngle >=
                $this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE) {
                $currentAngle = 0;
            }
        }
    }

    /**
     * Disable a Steppe capability.
     *
     * @param string $capability Specifies a string indicating a Steppe
     *                           capability; 'fog', 'reflection-map' and
     *                           'smooth' are currently implemented.
     * @return Steppe_Renderer This (fluent interface).
     */
    public function disable($capability)
    {
        switch ($capability) {
        case 'fog':
            $this->_fog = FALSE;
            break;
        case 'reflection-map':
            $this->_waterHeight = -1;
            break;
        case 'smooth':
            $this->_smooth = 0;
            break;
        default:
            throw new Exception("Can't disable unknown capability");
        }

        return $this;
    }

    /**
     * Enable a Steppe capability.
     *
     * @param string $capability Specifes a string indicating a Steppe
     *                           capability; 'fog', 'reflection-map' and
     *                           'smooth' are currently implemented.
     * @return Steppe_Renderer This (fluent interface).
     */
    public function enable($capability)
    {
        switch ($capability) {
        case 'fog':
            $this->_fog = TRUE;
            break;
        case 'reflection-map':
            $this->_waterHeight = self::_WATER_HEIGHT;
            break;
        case 'smooth':
            $this->_smooth = 0.5;
            break;
        default:
            throw new Exception("Can't enable unknown capability");
        }

        return $this;
    }

    /**
     * Get the current camera.
     *
     * @return array ...
     */
    public function getCamera()
    {
        return array(
            'angle' => $this->_camera['angle'] /
                round($this->_THREE_HUNDRED_AND_SIXTY_DEGREE_ANGLE * 360),
            'x'     => $this->_camera['x'],
            'y'     => $this->_camera['y'],
            'z'     => $this->_camera['z']
        );
    }

    /**
     * ...
     *
     * @param integer $x ...
     * @param integer $z ...
     * @return integer ...
     */
    public function getHeight($x, $z)
    {
        $u = $x & 1023;
        $v = $z & 1023;

        return $this->_heightmap[($v << 10) + $u];
    }

    /**
     * Test whether a capability is enabled.
     *
     * @param string $capability Specifies a string indicating a Steppe
     *                           capability; 'smooth' and 'reflection-map' are
     *                           currently implemented.
     * @return boolean Returns TRUE if $capability is an enabled capability and
     *                 returns FALSE otherwise.
     */
    public function isEnabled($capability)
    {
        switch ($capability) {
        case 'fog':
            return $this->_fog;
        case 'reflection-map':
            return ($this->_waterHeight > -1);
        case 'smooth':
            return ($this->_smooth === 0.5);
        }
        throw new Exception('Unknown capability');
    }

    public function render()
    {
        $this->_renderSky();
        $this->_renderTerrain();

        if ($this->_smooth == 0.5) {
            imagefilter($this->_framebuffer, IMG_FILTER_SMOOTH, 15);
        }
    }

    /**
     * Set the current camera.
     *
     * @param array $camera ...
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setCamera($camera)
    {
        if ( !is_array($camera)) {
            throw new Exception('Invalid camera: not an array');
        }

        $this->_camera['angle'] = (isset($camera['angle']) &&
            is_numeric($camera['angle'])) ?
            (abs(round($camera['angle'])) % 360 /
            self::_ANGLE_OF_VIEW * 320) :
            ($this->_camera['angle']);
        $this->_camera['x'] = (isset($camera['x']) &&
            is_numeric($camera['x'])) ?
            (round($camera['x'])) : ($this->_camera['x']);

        if (isset($camera['y']) &&
            is_numeric($camera['y'])) {
            $this->_camera['y'] = round($camera['y']);
            $this->_initRayLengthLookupTable($camera['y'], self::_DISTANCE);
        }

        $this->_camera['z'] = (isset($camera['z']) &&
            is_numeric($camera['z'])) ?
            (round($camera['z'])) : ($this->_camera['z']);

        return $this;
    }

    /**
     * Set the heightmap to use for terrain rendering.
     *
     * @param array $heightmap The heightmap canvas as an array of values in
     *                         the range 0..255.
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setHeightmap($heightmap)
    {
        $this->_heightmap = $heightmap;

        return $this;
    }

    /**
     * ...
     *
     * @param array $outOfBoundsHeightmap ...
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setOutOfBoundsHeightmap($outOfBoundsHeightmap)
    {
        $this->_outOfBoundsHeightmap = $outOfBoundsHeightmap;

        return $this;
    }

    /**
     * ...
     *
     * @param resource $outOfBoundsTexturemapCanvas
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setOutOfBoundsTexturemap($outOfBoundsTexturemapCanvas)
    {
        if ( !is_resource($outOfBoundsTexturemapCanvas)) {
            throw new Exception('Invalid outOfBoundsTexturemapCanvas: not a ' .
                'resource');
        }

        if (imagesx($outOfBoundsTexturemapCanvas) != 1024) {
            throw new Exception('outOfBoundsTexturemapCanvas width not ' .
                'equal to 1024');
        }
        if (imagesy($outOfBoundsTexturemapCanvas) != 1024) {
            throw new Exception('outOfBoundsTexturemapCanvas height not ' .
                'equal to 1024');
        }

        $this->_outOfBoundsTexturemap = $outOfBoundsTexturemapCanvas;

        return $this;
    }

    /**
     * Set render quality.
     *
     * @param string $quality Specifies a string indicating the render quality
     *                        from 'low', through 'medium', to 'high'.
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setQuality($quality)
    {
        switch ($quality) {
        case 'medium':
            $this->_quality = self::_DONT_CARE;
            break;
        case 'low':
            $this->_quality = self::_FASTEST;
            break;
        case 'high':
            $this->_quality = self::_NICEST;
            break;
        default:
            throw new Exception("Invalid quality; must be 'low', 'medium', " .
                "or 'high'");
        }

        return $this;
    }

    /**
     * Set the canvas to use for 360-degree panoramic sky.
     *
     * @param resource $skyCanvas The sky canvas; must be 1920x100.
     */
    public function setSky($skyCanvas)
    {
        if ( !is_resource($skyCanvas)) {
            throw new Exception('Invalid skyCanvas: not a resource');
        }

        if (imagesx($skyCanvas) != 1920) {
            throw new Exception('skyCanvas width not equal to 1920');
        }
        if (imagesy($skyCanvas) != 100) {
            throw new Exception('skyCanvas height not equal to 100');
        }

        $this->_sky = $skyCanvas;

        return $this;
    }

    /**
     * ...
     *
     * @param resource $texturemapCanvas ...
     * @return Steppe_Renderer This (fluent interface).
     */
    public function setTexturemap($texturemapCanvas)
    {
        if ( !is_resource($texturemapCanvas)) {
            throw new Exception('Invalid texturemapCanvas: not a resource');
        }

        if (imagesx($texturemapCanvas) != 1024) {
            throw new Exception('texturemapCanvas width not equal to 1024');
        }
        if (imagesy($texturemapCanvas) != 1024) {
            throw new Exception('texturemapCanvas height not equal to 1024');
        }

        $this->_texturemap = imagecreatetruecolor(imagesx($texturemapCanvas),
            imagesy($texturemapCanvas));
        imagecopy($this->_texturemap, $texturemapCanvas, 0, 0, 0, 0,
            imagesx($texturemapCanvas), imagesy($texturemapCanvas));

        return $this;
    }

    /**
     * Set height of the reflection-mapped water.
     *
     * @param integer $height Globally-defined height of the reflection-mapped
     *                        water. It must be in the range 0..255.
     * @return Renderer This (fluent interface).
     */
    public function setWaterHeight($height)
    {
        if ($this->_waterHeight == -1) {
            throw new Exception('Capability not enabled');
        }

        if ( !is_numeric($height)) {
            throw new Exception('Invalid height: not a number');
        }

        if ($height < 0 || $height > 255) {
            throw new Exception('Invalid height: must be in the range 0..255');
        }

        $this->_waterHeight = $height;

        return $this;
    }
}
