<?php
require_once 'library/Steppe/Compositor.php';

$heightmapImage = imagecreatefrompng('images/heightmap.png');
$heightmapCanvas = imagecreatetruecolor(1024, 1024);
imagecopyresampled($heightmapCanvas, $heightmapImage, 0, 0, 0, 0, 1024, 1024,
    imagesx($heightmapImage), imagesy($heightmapImage));
imagefilter($heightmapCanvas, IMG_FILTER_SMOOTH, 5);

$grassImage   = imagecreatefrompng('images/textures/grass.png');
$grass2Image  = imagecreatefrompng('images/textures/grass2.png');
$shingleImage = imagecreatefrompng('images/textures/shingle.png');

$texturemapCanvas = imagecreatetruecolor(1024, 1024);

$compositor = new Steppe_Compositor;
$compositor->setHeightmap($heightmapCanvas);
$compositor->addTexture(255, $grassImage);
$compositor->addTexture(127, $grass2Image);
$compositor->addTexture(95, $grassImage);
$compositor->addTexture(63, $shingleImage);
$compositor->composite($texturemapCanvas);

header('Content-Type: image/png');
imagepng($texturemapCanvas);

imagedestroy($heightmapImage);
imagedestroy($heightmapCanvas);
imagedestroy($grassImage);
imagedestroy($grass2Image);
imagedestroy($shingleImage);
imagedestroy($texturemapCanvas);
