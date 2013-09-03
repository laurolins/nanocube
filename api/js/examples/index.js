var color_compress = Shade.parameter("float", 1);
var current_mode="pan";
var nanocube, histograms;
var nanocube_selection, nanocube_selection_query = {};
var brush, point_batch, globe;
var canvas;
var gl, interactor;

function init_ui()
{
    canvas = document.getElementById("webgl");

    // buttons
    $("#option-pan").click(function() {
        current_mode = "pan";
        $("#option-pan").addClass("selected-mode");
        $("#option-select").removeClass("selected-mode");
    });
    $("#option-select").click(function() {
        current_mode = "select";
        $("#option-select").addClass("selected-mode");
        $("#option-pan").removeClass("selected-mode");
    });
    $("#option-pan").addClass("selected-mode");
    $("#option-select").removeClass("selected-mode");

    // keystrokes
    $(document).keypress(function(e){
        if (e.which === '<'.charCodeAt(0) || e.which === ','.charCodeAt(0)) {
            color_compress.set(Math.max(0.1, color_compress.get() / 1.333));
            Lux.Scene.invalidate();
        } else if (e.which === '>'.charCodeAt(0) || e.which === '.'.charCodeAt(0)) {
            color_compress.set(Math.min(color_compress.get() * 1.333, 10));
            Lux.Scene.invalidate();
        } else if (e.which === 'k'.charCodeAt(0) || e.which === 'K'.charCodeAt(0)) {
            point_batch && point_batch.coarsen();
        } else if (e.which === 'l'.charCodeAt(0) ||e.which === 'L'.charCodeAt(0)) {
            point_batch && point_batch.refine();
        }
    });

    $(window).resize(function(eventObject) {
        var w = window.innerWidth;
        var h = window.innerHeight;
        gl.resize(w, h);
        point_batch && (point_batch._dirty = true);
        Lux.Scene.invalidate();
    });

    // prevent right-click context menu
    canvas.addEventListener('contextmenu', function(ev) {
        ev.preventDefault();
        return false;
    }, false);

    // setup fullscreen
    var width = window.innerWidth, height = window.innerHeight;
    canvas.width = width;
    canvas.height = height;

}

function init_lux()
{
    interactor = Lux.UI.center_zoom_interactor({
        left: -0.5,
        right: 0.5,
        bottom: -0.5,
        top: 0.5,
        width: canvas.width, 
        height: canvas.height, 
        zoom: 1,
        widest_zoom: 0.2,
        deepest_zoom: 600
    });
    interactor.center.set(Lux.Marks.globe_2d.lat_lon_to_tile_mercator.js_evaluate(
        view_schema.center.lat * Math.PI / 180, 
        view_schema.center.lon * Math.PI / 180
    ));
    interactor.zoom.set(view_schema.center.zoom);
    gl = Lux.init({
        clearColor: [47/255, 54/255, 54/255, 1],
        interactor: interactor,
        highDPS: false
    });
}

$().ready(function () {
    init_ui();
    init_lux();
    nanocube = Nanocube.create({
        url: view_schema.url,
        max_zoom: view_schema.max_zoom,
        ready: function() {
            nanocube_selection = nanocube.selection();
            create_nanocube_view(view_schema, nanocube_selection);
            nanocube_selection
                .add_observer(function(q) {
                    var diff = Nanocube.diff_queries(nanocube_selection_query, q);
                    // majestic hack to get a deep copy:
                    nanocube_selection_query = JSON.parse(JSON.stringify(q));
                    if (diff.when || diff.where) {
                        point_batch && point_batch.reload_tiles();
                    }
                })
                .update_region({ x: [0, 1], y: [0, 1], z: 0 })
                .refresh();
        }
    });

    // Lux.Net.json("two_cities/towers.json", function(towers_array) {
    // Lux.Transform.using(interactor.transform, function() {
    globe = Lux.Marks.globe_2d({
        interactor: interactor, 
        tile_pattern: function(zoom, x, y) {
            // this is a SPDY version of OSM, so it's quite a bit faster on Chrome.
            // if it stops working, comment out the tile_pattern option.
            return "https://skechboy.com/maps/" + zoom +"/" + x + "/" + y + ".png";
        },
        resolution_bias: 0,
        cache_size: 4,
        no_network: false,
        // no_network: true,
        post_process: _.compose(
            Shade.Colors.invert,
            Shade.Colors.desaturate(0.75)
        )
    });
    Lux.Scene.add(globe);

    point_batch = Lux.Scene.add(nanocube_heatmap({
        interactor: interactor,
        max_zoom: view_schema.max_zoom - 9,
        resolution_bias: 0,
        point_color: function(scalar) {
            var logx = scalar.log().div(Shade(10).log());
            var scale = Shade.Scale.linear({domain: [0,
                                                     Shade(1).mul(color_compress),
                                                     Shade(2).mul(color_compress),
                                                     Shade(3).mul(color_compress)],
                                            range: [Shade.vec(0,0,0,0),
                                                    Shade.vec(1,0,0,1),
                                                    Shade.vec(1,1,0,1),
                                                    Shade.vec(1,1,1,1)]});
            var new_color = scale(logx);
            return scalar.eq(0).ifelse(Shade.vec(0,0,0,0), new_color).discard_if(scalar.eq(0));
        },
        tile_pattern: function(zoom, x, y) {
            return function(node, data_callback) {
                var q = { tile: { x: x, y: y, z: zoom },
                          fields: nanocube_selection_query.where
                        };
                if (nanocube_selection_query.when) {
                    q.time = nanocube_selection_query.when;
                }
                nanocube.tile(q, data_callback);
            };
        }
    }));

    Lux.Scene.add(Lux.Marks.rectangle_brush({
        color: Shade.vec(1, 0.8, 0.6, 0.5),
        accept_event: function(event) {
            return (event.button === 2 || current_mode === "select");
        },
        on: {
            brush_ended: function(b1, b2) {
                var z = ~~(globe.current_osm_zoom + 8.5);
                var x1 = ~~(b1[0] * (1 << z)),
                y1 = ~~(b1[1] * (1 << z)),
                x2 = ~~(b2[0] * (1 << z)) + 1,
                y2 = ~~(b2[1] * (1 << z)) + 1;
                var region = { 
                    x: [Math.min(x1, x2), Math.max(x1, x2)],
                    y: [Math.min(y1, y2), Math.max(y1, y2)],
                    z: z
                };
                if (b1[0] === b2[0] && b1[1] === b2[1]) {
                    region.x = [0,1];
                    region.y = [0,1];
                    region.z = 0;
                }
                nanocube_selection.update_region(region).refresh();
            }
        }
    }));
});
