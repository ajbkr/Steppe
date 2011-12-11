<!DOCTYPE html>
<html lang="en-gb">
  <head>
    <script type="text/javascript" src="http://fleetingfantasy.com/js/jquery/jquery-1.6.2.min.js"></script> 
    <script type="text/javascript" src="http://fleetingfantasy.com/js/jquery/plugins/jquery.loadImages.1.0.1.min.js"></script>
<style type="text/css">
body {
    font-family: Arial, sans;
    font-size: 10pt
}
</style>
    <title>Client-side/Server-side Rendering Comparison - Steppe</title>
  </head>
  <body onload="Comparison.run()">
    <h1>Steppe</h1>
    <h2>Client-side/Server-side Rendering Comparison</h2>
    <h3>JavaScript (HTML5 Canvas)</h3>
    <canvas id="canvas" style="height: 400px; width: 640px" height="200" width="320">
      <p>
        Your Web browser does not support the canvas element.
      </p>
    </canvas>
<!--    <h3>PHP (GD)</h3>
    <img style="height: 400px; width: 640px" src="render.php" />-->
<!-- Uncomment the HTML below to test the compositor. -->
<!--    <img src="composite.php" />-->
<script type="text/javascript">
document.writeln('<' + 'script src="js/Steppe/Compositor.js?' +
    Number(new Date()) + '" type="text/javascript">' + '<' + '/script>');
document.writeln('<' + 'script src="js/Steppe/Renderer.js?' +
    Number(new Date()) + '" type="text/javascript">' + '<' + '/script>');
</script>
<script type="text/javascript">
var Comparison = (function(undefined) {
    var _WATER_HEIGHT = 88;

    return {
        run: function() {
            var canvas = document.getElementById('canvas');

            var offscreenCanvas = document.createElement('canvas');
            offscreenCanvas.width  = 320;
            offscreenCanvas.height = 200;

            var context = canvas.getContext('2d');
            var offscreenContext = offscreenCanvas.getContext('2d');

            context.fillStyle = '#000';
            context.fillRect(0, 0, canvas.width, canvas.height);

            var renderer = Steppe.Renderer(offscreenCanvas);

            var compositor = Steppe.Compositor();

            var images = {
                'sky':        '/images/sky.png',
                'heightmap':  '/images/heightmap.png',
                'texturemap': '/images/texturemap.png',
                'sprite':     '/images/sprite.png'
            };

            $.loadImages($.map(images, function(v) { return v; }), function() {
                $.each(images, function(name, src) {
                    var image = new Image();
                    image.src = src;
                    images[name] = image;
                });

                var skyCanvas = document.createElement('canvas');
                skyCanvas.width  = 1920;
                skyCanvas.height = 100;
                var skyContext = skyCanvas.getContext('2d');

                skyContext.drawImage(images['sky'], 0, 0);

                var heightmapCanvas = document.createElement('canvas');
                heightmapCanvas.width  = 1024;
                heightmapCanvas.height = 1024;
                var heightmapContext = heightmapCanvas.getContext('2d');

                heightmapContext.drawImage(images['heightmap'], 0, 0, 1024,
                    1024);

                compositor.setHeightmap(heightmapCanvas);

                var texturemapCanvas = document.createElement('canvas');
                texturemapCanvas.width  = 1024;
                texturemapCanvas.height = 1024;
                var texturemapContext = texturemapCanvas.getContext('2d');

                texturemapContext.drawImage(images['texturemap'], 0, 0, 1024,
                    1024);

                renderer.setTexturemap(texturemapCanvas)
                    .setOutOfBoundsTexturemap(heightmapCanvas)
                    .setQuality('high')
                    .setSky(skyCanvas)
                    .setHeightmap(compositor.getHeightmap())
                    .setOutOfBoundsHeightmap(
                        compositor.getOutOfBoundsHeightmap())
                    .enable('fog')
//                    .setFogColor('#000000')
//                    .enable('smooth')
//                    .enable('reflection-map')
//                    .setWaterHeight(82)
                ;

                renderer.addSprite(images['sprite'], 1024 + 512 - 128,
                    renderer.getHeight(1024 + 512 - 128, 1024 + 512 + 32),
                    1024 + 512 + 32);
                renderer.addSprite(images['sprite'], 1024 + 512 - 32,
                    renderer.getHeight(1024 + 512 - 32, 1024 + 512 + 128 + 32),
                    1024 + 512 + 128 + 32);
                renderer.addSprite(images['sprite'], 1024 + 512 + 32,
                    renderer.getHeight(1024 + 512 + 32, 1024 + 512 + 256),
                    1024 + 512 + 256);

                renderer.setCamera({
                    angle: 255,
                    x:     1024 + 768 - 128 - 96,
                    y:     renderer.getHeight(1024 + 768 - 128 - 96,
                               1024 + 768 + 128),
                    z:     1024 + 768 + 128
                });

                renderer.render();

                context.drawImage(offscreenCanvas, 0, 0, canvas.width,
                    canvas.height);
            });
        }
    };
})();
</script>
  </body>
</html>
