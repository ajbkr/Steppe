<?php
require_once 'library/Steppe/Compositor.php';
require_once 'library/Steppe/Renderer.php';

define('_WATER_HEIGHT', 88);

$heightmapImage = imagecreatefrompng('images/heightmap.png');
$heightmapCanvas = imagecreatetruecolor(1024, 1024);
imagecopyresampled($heightmapCanvas, $heightmapImage, 0, 0, 0, 0, 1024, 1024,
    imagesx($heightmapImage), imagesy($heightmapImage));
imagefilter($heightmapCanvas, IMG_FILTER_SMOOTH, 5);

$compositor = new Steppe_Compositor;
$compositor->setHeightmap($heightmapCanvas);

$offscreenCanvas = imagecreatetruecolor(320, 200);

$renderer = new Steppe_Renderer($offscreenCanvas);

$texturemapCanvas = imagecreatefrompng('images/texturemap.png');
$skyCanvas        = imagecreatefrompng('images/sky.png');

$renderer->setTexturemap($texturemapCanvas)
    ->setOutOfBoundsTexturemap($heightmapCanvas)
    ->setQuality('high')
    ->setSky($skyCanvas)
    ->setHeightmap($compositor->getHeightmap())
    ->setOutOfBoundsHeightmap($compositor->getOutOfBoundsHeightmap())
    ->enable('smooth')
    ->enable('fog')
    ->enable('reflection-map')
    ->setWaterHeight(_WATER_HEIGHT)
;

$renderer->setCamera(array(
    'angle' => 240,
    'x'     => 1024 + 768 - 128 - 96,
    'y'     => 165,
    'z'     => 1024 + 768 + 128
));

$renderer->render();

imagedestroy($heightmapImage);
imagedestroy($texturemapCanvas);
imagedestroy($skyCanvas);

header('Content-Type: image/png');
imagepng($offscreenCanvas);

imagedestroy($offscreenCanvas);
