Steppe
======

Steppe is a custom HTML5 canvas 2.5D landscape renderer and compositor. The
JavaScript source code for Steppe is unobfuscated and can be used free of
charge in your own projects (MIT license).

Steppe, like [Fleeting Fantasy](http://fleetingfantasy.com/), is actively under
development so we recommend you wait until it's production-ready before using
it for anything serious. That doesn't mean you shouldn't play with it now
though; go right ahead!

### Screenshots ###

![screenshot](http://fleetingfantasy.com/images/screenshots/007.jpg)

![screenshot](http://fleetingfantasy.com/images/screenshots/001.jpg)

### Features ###

* Four degrees of freedom (4DoF); translation along the x, y, and z axes and rotation about the y axis
* Reflection-mapped, semi-transparent water with globally variable height
* Full 360-degree panoramic sky
* Floating horizon and faux camera tilt
* Coloured, distance fog for better depth perception
* Texture-mapped terrain with client-side compositor accepting multiple textures
* Antialiasing for smoother rendering results (currently unavailable)
* Configurable render quality from low, through medium, to high
* 2D billboarded sprites (JS only)
* Alternative texturemap for out-of-bounds terrain
* PHP port for graceful degradation where HTML5 canvas isn't supported
* MIT license
* All HTML5 2D canvas; no WebGL!

### Coming Soon ###

* Fog applied to sprites
* Sanitised API
* Pre-calculated lightmap support for shadows

### Steppe Mars Demo ###

[Try Demo](http://steppe.fleetingfantasy.com/)
