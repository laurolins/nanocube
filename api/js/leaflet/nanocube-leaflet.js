function nanocube_leaflet_layer(opts) {
    var resolution = opts.resolution || opts.nanocube.resolution || 6;
    var query = {};
    var nanocube_selection = opts.nanocube_selection;
    var nanocube = opts.nanocube;

    var colormap_offset = 0;

    nanocube_selection
        .add_observer(function(q) {
            var diff = Nanocube.diff_queries(query, q);
            query = JSON.parse(JSON.stringify(q));
            if (diff.when || diff.where) {
                canvas_layer.redraw();
            }
        });

    var colormap = opts.colormap || function(count) {
        var lc = Math.log(count + 1) / Math.log(10) + colormap_offset,
        u, s;
        if (lc < 0) {
            s = "rgba(0,0,0,0)";
        } else if (lc < 1) {
            u = ~~(lc * 255);
            s = "rgba(" + u + ",0,0," + lc + ")";
        } else if (lc < 2) {
            u = ~~((lc - 1) * 255);
            s = "rgba(255," + u + ",0,1)";
        } else if (lc < 3) {
            s = "yellow";
            u = ~~((lc - 2) * 255);
        } else
            s = "white";
        return s;
    };

    var canvas_layer = L.tileLayer.canvas();

    canvas_layer.drawTile = function(tile, tilePoint) {
        var x = tilePoint.x;
        var y = tilePoint.y;
        var z = this._map._zoom;
        while (x < 0) x += 1 << z;
        while (y < 0) y += 1 << z;
        x = x % (1 << z);
        y = y % (1 << z);
        var r = Math.max(1, Math.min(nanocube.schema.sbin - z, resolution));

        var q = { tile : { x: x, y: y, z: z, resolution: r },
                  fields : query.where
                };
        if (query.when) q.time = query.when;

        nanocube.tile(q, function(data) {
            if (data.x.length === 0)
                return;
            var c = tile.getContext("2d");
            for (var i=0; i<data.x.length; ++i) {
                c.fillStyle = colormap(data.count[i]);
                c.fillRect(data.x[i], data.y[i], 1 << (8 - r), 1 << (8 - r));
            }
        });
    };


    canvas_layer.makeFiner = function() {
        resolution = Math.min(resolution+1, 8);
        canvas_layer.redraw();
    };
    canvas_layer.makeCoarser = function() {
        resolution = Math.max(resolution-1, 2);
        canvas_layer.redraw();
    };
    canvas_layer.makeFainter = function() {
        colormap_offset -= 1/3;
        canvas_layer.redraw();
    };
    canvas_layer.makeBrighter = function() {
        colormap_offset += 1/3;
        canvas_layer.redraw();
    };

    return canvas_layer;
}
