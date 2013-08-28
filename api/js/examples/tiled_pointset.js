function tiled_pointset(opts)
{
    var ctx = Lux._globals.ctx;
    opts = _.defaults(opts || {}, {
        resolution_bias: -1,
        patch_size: 10,
        cache_size: 256,
        post_process: function(c) { return c; },
        point_color: function(t) {
            var avg = t.swizzle("xyz").div(t.a());
            var new_alpha = t.a().gt(10).ifelse(1, t.a().tanh());
            var new_color = Shade.vec(avg, new_alpha);
            return Shade.ifelse(t.a().eq(0), Shade.vec(0,0,0,0), new_color);
        },
        project: function(v) { return v; },
        unproject: function(v) { return v; }
    });

    if (opts.interactor) {
        opts.center = opts.interactor.center;
        opts.zoom   = opts.interactor.zoom;
        opts.camera = opts.interactor.camera;
    } else {
        throw "currently this requires an interactor, sorry!";
    }

    if (_.isUndefined(opts.tile_pattern)) {
        throw "tiled_pointset requires parameter tile_pattern";
    }

    var patch = Lux.model({
        type: "triangles",
        uv: [[0,0,1,0,1,1,0,0,1,1,0,1], 2],
        vertex: function(min, max) {
            return this.uv.mul(max.sub(min)).add(min);
        }
    });

    var inverter = Lux.Transform.get_inverse();
    var unproject = Shade(function(p) {
        return opts.unproject(inverter({ position: p }).position);
    }).js_evaluate;

    var cache_size = opts.cache_size;

    var tiles = tile_cache({
        compare_tile: function(r1, r2) {
            var z = result.current_osm_zoom;
            var c = result._screen_center;
            var pc = { x: c[0], y: c[1], zoom: z };
            function d(c1, c2) {
                var dc = ((c1.x - c2.x) * (c1.x - c2.x) +
                          (c1.y - c2.y) * (c1.x - c2.y));
                var dz = (c1.zoom - c2.zoom) * (c1.zoom - c2.zoom);
                return Math.sqrt(dc * Math.pow(2, dz));
            }
            var x = d(pc, r1) - d(pc, r2);
            return x;
        }
    });

    var min_x = Shade.parameter("float");
    var max_x = Shade.parameter("float");
    var min_y = Shade.parameter("float");
    var max_y = Shade.parameter("float");
    var point_x = opts.point_pos.x();
    var point_y = opts.point_pos.y();

    var min_pt = Shade.vec(min_x, min_y);
    var max_pt = Shade.vec(max_x, max_y);
    var point_pos = Shade.vec(Shade.mix(min_x, max_x, point_x),
                              Shade.mix(max_y, min_y, point_y));
    var v = patch.vertex(min_pt, max_pt);

    // We need two batches: one to blank the area in which we'll draw the points,
    // and another for the points themselves. If we don't blank the whole square, 
    // we get bleeding across zoom levels
    var blank_batch = Lux.bake(patch, {
        position: opts.project(v),
        color: Shade.vec(0,0,0,0),
        mode: Lux.DrawingMode.pass // over_no_depth
    });

    var pointset_model = Lux.model({
        type: "triangles",
        vertex: opts.project(point_pos), 
        color: Shade.color("#ffffff").mul(opts.point_weight),
        elements: 1
    });

    var fractional_zoom = Shade.parameter("float", 0);

    var pointset_batch = Lux.bake(pointset_model, {
        position: pointset_model.vertex,
        color: pointset_model.color,
        mode: Lux.DrawingMode.pass
    });

    var width = ctx.viewportWidth;
    var height = ctx.viewportHeight;
    var point_view_lod = 1;
    var rb1 = Lux.render_buffer({ width: width * point_view_lod, height: height * point_view_lod, type: ctx.FLOAT });

    var rb1_batch = rb1.make_screen_batch(function(texel_accessor) {
        return opts.point_color(texel_accessor().a());
    }, Lux.DrawingMode.over);

    var result = {
        _prev_center: opts.center.get(),
        _prev_zoom: 0, // 0 will force a draw the first time this is called.,
        fractional_zoom: 0,
        _dirty: false,
        _screen_corner_1: [0,0],
        _screen_corner_2: [0,0],
        tiles: tiles,
        queue: [],
        current_osm_zoom: opts.zoom.get(),
        lat_lon_position: function(lat, lon) {
            return Shade.Scale.Geo.latlong_to_mercator(lat, lon);
        },
        refine: function() {
            if (this.resolution_bias < 1) {
                this._dirty = true;
                this.resolution_bias += 1;
                Lux.Scene.invalidate();
            }
        },
        coarsen: function() {
            if (this.resolution_bias > -5) {
                this._dirty = true;
                this.resolution_bias -= 1;
                Lux.Scene.invalidate();
            }
        },
        resolution_bias: opts.resolution_bias,
        new_center: function(center_zoom) {
            var ctx = Lux._globals.ctx;
            var screen_resolution_bias = Math.log(ctx.viewportHeight / 256) / Math.log(2);
            var log2_z = (Math.log(center_zoom) / Math.log(2)); 
            var zoom = this.resolution_bias + screen_resolution_bias + log2_z;
            this.zoom = log2_z;
            this.fractional_zoom = zoom - ~~zoom + Math.log(screen_resolution_bias) / Math.log(2);
            zoom = ~~zoom;
            this.current_osm_zoom = zoom;

            var c1 = unproject(vec.make([0,0]));
            this._screen_corner_1 = c1;
            var c2 = unproject(vec.make([ctx.parameters.width.get(), ctx.parameters.height.get()]));
            this._screen_corner_2 = c2;
            this._screen_center = vec.scaling(vec.plus(result._screen_corner_1, result._screen_corner_2), 0.5);

            var min_x_at_zoom = Math.floor(c1[0] * (1 << zoom));
            var max_x_at_zoom = Math.ceil(c2[0] * (1 << zoom));
            var min_y_at_zoom = (1 << zoom) - Math.ceil(c2[1] * (1 << zoom)) - 1;
            var max_y_at_zoom = (1 << zoom) - Math.floor(c1[1] * (1 << zoom)) - 1;

            for (var rx=min_x_at_zoom; rx<=max_x_at_zoom; ++rx) {
                if (rx < 0 || rx >= (1 << zoom)) continue;
                for (var ry=min_y_at_zoom; ry<=max_y_at_zoom; ++ry) {
                    if (ry < 0 || ry >= (1 << zoom)) continue;
                    this.request(~~zoom, ~~rx, ~~ry);
                }
            }
        },
        init: function() {
            for (var z=0; z<3; ++z)
                for (var i=0; i<(1 << z); ++i)
                    for (var j=0; j<(1 << z); ++j)
                        this.request(z, i, j);
        },
        request: function(zoom, x, y) {
            if (zoom > opts.max_zoom) {
                var shrinkage = zoom - opts.max_zoom;
                x = (x / (1 << shrinkage)) | 0;
                y = (y / (1 << shrinkage)) | 0;
                zoom = opts.max_zoom;
            }
                
            var that = this;

            function on_success(node, data) {
                if (_.isUndefined(node.client_data))
                    node.client_data = opts.construct_node(data, zoom, x, y);
                else
                    node.client_data.update(data);
                that._dirty = true;
                Lux.Scene.invalidate();
            };

            this.tiles.insert(zoom, x, y, opts.tile_pattern(zoom, x, y), on_success);
            this.tiles.trim(function(node) {
                if (!_.isUndefined(node.client_data))
                    node.client_data.clear_data();
            });
        },
        clear_cache: function() {
            this.tiles.clear(function(node) {
                if (!_.isUndefined(node.client_data))
                    node.client_data.clear_data();
            });
        },
        reload_tiles: function() {
            this.clear_cache();
            this.new_center(opts.zoom.get());
        },
        draw_dots: function() {
            var that = this;
            this.new_center(opts.zoom.get());
            function draw_visit(t) {
                var done = (that.current_osm_zoom >= opts.max_zoom) || (t.zoom < that.current_osm_zoom);
                if (Math.abs(t.zoom - that.current_osm_zoom) < 5 ||
                    t.zoom === opts.max_zoom) {

                    min_x.set(t.x / (1 << t.zoom));
                    max_x.set((t.x + 1) / (1 << t.zoom));
                    max_y.set(1 - t.y / (1 << t.zoom));
                    min_y.set(1 - (t.y + 1) / (1 << t.zoom));
                    
                    blank_batch.draw();
                    var n_points = t.client_data.element_count();
                    
                    if (n_points > 0) {
                        pointset_model.elements = n_points;
                        t.client_data.set_attributes(done);
                        pointset_batch.draw();
                    }
                }
                return done;
            }
            this.tiles.visit_all_visible(this._screen_corner_1[0],
                                         this._screen_corner_2[0],
                                         this._screen_corner_1[1],
                                         this._screen_corner_2[1], draw_visit);
        },
        draw: function() {
            var this_center = opts.interactor.center.get();
            var this_zoom = opts.interactor.zoom.get();
            var that = this;
            if (this._dirty ||
                !vec.equal(this._prev_center, this_center) ||
                this._prev_zoom !== this_zoom) {
                this._prev_center = this_center;
                this._prev_zoom = this_zoom;
                rb1.with_bound_buffer(function() {
                    ctx.clearColor(0,0,0,0);
                    ctx.clear(ctx.COLOR_BUFFER_BIT);
                    fractional_zoom.set(that.fractional_zoom);
                    that.draw_dots();
                });
            }
            rb1_batch.draw();
            this._dirty = false;
        }
    };
    result.init();

    return result;
};
