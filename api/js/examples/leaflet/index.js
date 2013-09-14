// buttons
var current_mode="pan", leaflet_map, nanocube, nanocube_selection, leaflet_layer;
var enable_rectangle, disable_rectangle, drawn_items;

function init_ui()
{
    $("#option-pan").click(function() {
        current_mode = "pan";
        $("#option-pan").addClass("selected-mode");
        $("#option-select").removeClass("selected-mode");
        disable_rectangle();
    });
    $("#option-select").click(function() {
        current_mode = "select";
        $("#option-select").addClass("selected-mode");
        $("#option-pan").removeClass("selected-mode");
        enable_rectangle();
    });
    $("#option-pan").addClass("selected-mode");
    $("#option-select").removeClass("selected-mode");
    $("#option-clear").click(function() {
        drawn_items.clearLayers();
        nanocube_selection.update_region({x: [0,1], y:[0,1], z:0}).refresh();
    });

    $(document).keypress(function(e) {
        if (e.which === '<'.charCodeAt(0) || 
            e.which === ','.charCodeAt(0))
            leaflet_layer.makeFainter();
        else if (e.which === '>'.charCodeAt(0) ||
                 e.which === '.'.charCodeAt(0))
            leaflet_layer.makeBrighter();
    });
    $("#make-brighter").click(function() {
        leaflet_layer.makeBrighter();
    });
    $("#make-fainter").click(function() {
        leaflet_layer.makeFainter();
    });
    $("#make-coarser").click(function() {
        leaflet_layer.makeCoarser();
    });
    $("#make-finer").click(function() {
        leaflet_layer.makeFiner();
    });
}

function init_nanocube()
{
    nanocube = Nanocube.create({
        url: view_schema.url,
        resolution: 6,
        ready: function() {
            var query = {};
            nanocube_selection = nanocube.selection();
            create_nanocube_view(view_schema, nanocube_selection);
            leaflet_layer = nanocube_leaflet_layer({
                nanocube: nanocube,
                nanocube_selection: nanocube_selection
            });
            var layer = leaflet_layer;
            layer.addTo(leaflet_map);
            nanocube_selection
                .update_region({ x: [0, 1], y: [0, 1], z: 0 })
                .refresh();

            drawn_items = new L.FeatureGroup();
            leaflet_map.addLayer(drawn_items);
            var draw_control = new L.Control.Draw({
                draw: {
                    circle: false,
                    polyline: false,
                    polygon: false,
                    marker: false,
                    rectangle: {
                        shapeOptions: {
                            color: "#ffffff"
                        },
                        repeatMode: true
                    }
                },
                edit: {
                    featureGroup: drawn_items,
                    edit: false,
                    remove: false
                }
            });

            leaflet_map.addControl(draw_control);
            // disgusting hack to let me actually freaking add
            // a "click" command programmatically. JHC, leaflet, will you
            // give it a rest with the 300 layers of gunk?
            enable_rectangle = function() {
                _.filter(draw_control._toolbars, function(t) {
                    return t.options.rectangle;
                })[0]._modes.rectangle.handler.enable();
            };
            disable_rectangle = function() {
                _.filter(draw_control._toolbars, function(t) {
                    return t.options.rectangle;
                })[0]._modes.rectangle.handler.disable();
            };

            $(".leaflet-draw.leaflet-control").hide();

            leaflet_map.on('draw:created', function (e) {
                var layer = e.layer;
                var region;
                var lat1 = e.layer._latlngs[0].lat / 180 * Math.PI,
                    lat2 = e.layer._latlngs[1].lat / 180 * Math.PI, 
                    lng1 = e.layer._latlngs[0].lng / 180 * Math.PI, 
                    lng2 = e.layer._latlngs[2].lng / 180 * Math.PI;
                if (lat1 === lat2 && lng1 === lng2) {
                    region = { x: [0,1], y: [0,1], z: 0 };
                } else {
                    var x1 = (lng1 + Math.PI) / (2 * Math.PI),
                        x2 = (lng2 + Math.PI) / (2 * Math.PI),
                        y1 = (Math.log(Math.tan(lat1/2 + Math.PI/4)) + Math.PI) / (2 * Math.PI),
                        y2 = (Math.log(Math.tan(lat2/2 + Math.PI/4)) + Math.PI) / (2 * Math.PI);
                    var z = Math.min(leaflet_map._zoom + 6, 25); // fixme max_zoom
                    x1 = ~~(x1 * (1 << z));
                    x2 = ~~(x2 * (1 << z)) + 1;
                    y1 = ~~(y1 * (1 << z));
                    y2 = ~~(y2 * (1 << z)) + 1;
                    region = {
                        x: [Math.min(x1, x2), Math.max(x1, x2)],
                        y: [Math.min(y1, y2), Math.max(y1, y2)],
                        z: z
                    };
                }
                nanocube_selection.update_region(region).refresh();
                drawn_items.clearLayers();
                drawn_items.addLayer(layer);
            });
        }
    });
}

function init_leaflet()
{
    leaflet_map = L.map('map', {
        center: [view_schema.center.lat, view_schema.center.lon],
        zoom: view_schema.center.lzoom ? view_schema.center.lzoom : 3
    });

    // add an OpenStreetMap tile layer
    L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png', {
        attribution: '&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
    }).addTo(leaflet_map);
}

$().ready(function() {
    init_ui();
    init_nanocube();
    init_leaflet();
});

//////////////////////////////////////////////////////////////////////////////




