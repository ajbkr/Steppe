function runAll() {
    module('General');

    test('Steppe availability', function() {
        expect(3);

        ok(Steppe, 'Steppe defined');
        ok(Steppe.Compositor, 'Steppe.Compositor defined');
        ok(Steppe.Renderer, 'Steppe.Renderer defined');
    });

    module('Compositor');

    // ...

    module('Renderer');

    test('Instantiation, success', function() {
        var canvas = document.createElement('canvas');
        canvas.width  = 320;
        canvas.height = 200;

        var renderer = new Steppe.Renderer(canvas);

        expect(3);

        ok(renderer, "renderer 'set'");
        equals(typeof(renderer), 'object', "renderer is type of 'object'");
        ok(renderer.render, 'Renderer.render() method defined');
    });

    test('Instantiation, omitted canvas argument', function() {
        expect(1);

        raises(function() {
            var renderer = new Steppe.Renderer();
        }, 'Omitted canvas argument raises exception');
    });

    test('Instantiation, invalid canvas arguments', function() {
        var v = [
            null,
            42,
            'foo',
            NaN,
            Infinity,
            4.2,
            { },
            new Image(),
            /[A-Z]/
        ];

        expect(v.length);

        for (var i = 0; i < v.length; ++i) {
            raises(function() {
                var renderer = new Steppe.Renderer(v[i]);
            }, "'" + v[i] + "' canvas argument raises exception");
        }
    });

    test('Instantiation, too many arguments', function() {
        var canvas = document.createElement('canvas');
        canvas.width  = 320;
        canvas.height = 200;

        expect(2);

        raises(function() {
            var renderer = new Steppe.Renderer(canvas, 42, 'foo');
        }, 'Too many arguments raises exception');

        raises(function() {
            var renderer = new Steppe.Renderer(canvas, undefined);
        }, 'Too many arguments raises exception');
    });
}
