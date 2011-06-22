<?php
class Steppe_Compositor
{
    private $_heightmap;
    private $_outOfBoundsHeightmap;
    private $_textureArray;

    /**
     * Add a texture to the texture-array.
     *
     * @param integer $height The 'height' at which to apply the texture.
     * @param resource $textureImage The image to use as a texture.
     * @return Steppe_Compositor This (fluent interface).
     */
    public function addTexture($height, $textureImage)
    {
        if ( !is_resource($textureImage)) {
            throw new Exception('Invalid textureImage: not a resource');
        }

        if ($height < 0 || $height > 255) {
            throw new Exception('Invalid height; must be in the range 0..255');
        }

        if (imagesx($textureImage) != 256 || imagesy($textureImage) != 256) {
            throw new Exception('Invalid texture dimensions; must be 256x256');
        }

        $textureCanvas = imagecreatetruecolor(imagesx($textureImage),
            imagesy($textureImage));

        imagecopy($textureCanvas, $textureImage, 0, 0, 0, 0,
            imagesx($textureImage), imagesy($textureImage));

        $this->_textureArray[$height] = array(
            'data'   => $textureCanvas,
            'height' => imagesy($textureCanvas),
            'width'  => imagesx($textureCanvas)
        );

        return $this;
    }

    /**
     * Create a composite texturemap from the heightmap and texture-array
     * images.
     *
     * @param resource $texturemapCanvas The texturemap canvas to which the
     *                                   heightmap and texture-array images are
     *                                   composited.
     * @return void
     */
    public function composite($texturemapCanvas)
    {
        if ( !is_resource($texturemapCanvas)) {
            throw new Exception('Invalid texturemapCanvas: not a resource');
        }

        $textureCanvas = imagecreatetruecolor(256, 256);

        if ( !isset($this->_textureArray[255])) {
            throw new Exception('No texture added at height 255; unable to ' .
                'composite');
        }
        for ($i = 254; $i >= 0; --$i) {
            if ( !isset($this->_textureArray[$i])) {
                $this->_textureArray[$i] = $this->_textureArray[$i + 1];
            }
        }

        for ($y = 0; $y < 1024; ++$y) {
            for ($x = 0; $x < 1024; ++$x) {
                $height = $this->_heightmap[($y << 10) + $x];

                imagesetpixel($texturemapCanvas, $x, $y, imagecolorat(
                    $this->_textureArray[$height]['data'], $x & 255,
                    $y & 255));
            }
        }
    }

    /**
     * Get the heightmap as an array.
     *
     * @return array ...
     */
    public function getHeightmap()
    {
        return $this->_heightmap;
    }

    /**
     * Get the out-of-bounds heightmap as an array.
     *
     * @return array ...
     */
    public function getOutOfBoundsHeightmap()
    {
        return $this->_outOfBoundsHeightmap;
    }

    /**
     * Put a mask (a 2.5D sprite's heightmap).
     *
     * @param resource $mask The 2.5D sprite's heightmap; should contain a
     *                       greyscale image.
     * @param integer $x The x-ordinate.
     * @param integer $y The y-ordinate.
     * @param float $scaleFactor ...
     * @return Steppe_Compositor This (fluent interface).
     */
    public function putMask($mask, $x, $y, $scaleFactor)
    {
        for ($y2 = 0; $y2 < imagesy($mask); ++$y2) {
            for ($x2 = 0; $x2 < imagesx($mask); ++$x2) {
                if (imagecolorat($mask, $x2, $y2) >> 24) {
                    $index = (($y2 + $y) << 10) + ($x2 + $x);

                    $this->_heightmap[$index] = 192 +
                        (imagecolorat($mask, $x2, $y2) & 0xff) * $scaleFactor;
                }
            }
        }

        return $this;
    }

    /**
     * Set the heightmap to use for compositing [and out-of-bounds].
     *
     * @param resource $heightmapCanvas The heightmap canvas; should contain a
     *                                  greyscale image.
     * @return Steppe_Compositor This (fluent interface).
     */
    public function setHeightmap($heightmapCanvas)
    {
        for ($y = 0; $y < 1024; ++$y) {
            for ($x = 0; $x < 1024; ++$x) {
                $index = ($y << 10) + $x;

                $this->_heightmap[$index] = imagecolorat(
                    $heightmapCanvas, $x, $y) & 255;
                $this->_outOfBoundsHeightmap[$index] =
                    $this->_heightmap[$index];
            }
        }
    }
}
