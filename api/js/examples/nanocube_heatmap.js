function nanocube_heatmap(opts) {
    var global_id = 0;
    var point_x = Shade.attribute("float");
    var point_y = Shade.attribute("float");
    var point_offset = Shade.attribute("vec2");
    var point_weight = Shade.attribute("float");
    var color = Shade.color("#ffffff").mul(point_weight);

    return tiled_pointset({
        resolution_bias: opts.resolution_bias,
        interactor: opts.interactor,
        max_zoom: opts.max_zoom,
        point_pos: Shade.vec(point_x, point_y).add(point_offset),
        point_weight: point_weight,
        point_color: opts.point_color,
        tile_pattern: opts.tile_pattern,
        project: opts.project || function(v) { return v; },
        unproject: opts.unproject || function(v) { return v; },
        construct_node: function(data, zoom, x, y) {
            var node = {
                id: global_id++,
                element_count: function() { return this.n_points; },
                clear_data: function() {
                    delete this.x_buffer;
                    delete this.y_buffer;
                    delete this.count_buffer;
                    delete this.n_points;
                },
                // set_attributes should be called pre_draw_visit() or something like that
                set_attributes: function(is_leaf) {
                    point_offset.set(this.offset);
                    point_x.set(this.x_buffer);
                    point_y.set(this.y_buffer);
                    point_weight.set(this.count_buffer);
                },
                update: function(data) {
                    var x_list = [], y_list = [], count = [],
                    offset = [];
                    _.each(data.x, function(x, i) {
                        x_list.push(x/256, x/256, x/256, x/256, x/256, x/256);
                        offset.push(0/256, 0/256,
                                    1/256, 0/256,
                                    1/256, 1/256,
                                    0/256, 0/256,
                                    1/256, 1/256,
                                    0/256, 1/256);
                        // offset.push(0,0,0,0,0,0,0,0,0,0,0,0);
                        count.push(data.count[i], data.count[i], data.count[i],
                                   data.count[i], data.count[i], data.count[i]);
                        
                        var y = data.y[i];

                        // WHAT THE HELL
                        if (y > 0) y = y - 1;
                        else y = 255;
                        y = y / 256;
                        y_list.push(y, y, y, y, y, y);
                    });
                    var weight_list = new Float32Array(count);
                    if (!this.x_buffer) {
                        this.offset = Lux.attribute_buffer({
                            vertex_array: offset,
                            item_type: 'float',
                            normalized: false,
                            item_size: 2,
                            keep_array: true
                        });
                        this.x_buffer = Lux.attribute_buffer({
                            vertex_array: x_list,
                            item_type: 'float',
                            // normalized: true,
                            item_size: 1,
                            keep_array: true
                        });
                        this.y_buffer = Lux.attribute_buffer({
                            vertex_array: y_list,
                            item_type: 'float',
                            // normalized: true,
                            item_size: 1,
                            keep_array: true
                        });
                        this.count_buffer = Lux.attribute_buffer({
                            vertex_array: weight_list,
                            item_type: 'float',
                            normalized: false,
                            item_size: 1,
                            keep_array: true
                        });
                        this.max_count = d3.max(weight_list);
                    } else {
                        this.offset.set(offset);
                        this.x_buffer.set(x_list);
                        this.y_buffer.set(y_list);
                        this.count_buffer.set(weight_list);
                        this.max_count = d3.max(weight_list);
                    }
                    this.n_points = x_list.length;
                }
            };
            node.update(data);
            return node;
        }
    });
}
