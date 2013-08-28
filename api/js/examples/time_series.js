function timeseries(parent, spec)
{
    var boundary = spec.time_range;
    var max_width = boundary[1].getTime() - boundary[0].getTime();
    var div = parent.append("div").style("position", "relative").attr("class", "vis-pane-bg");
    var div_buttons = div.append("div")
        .style("z-index", "2")
        .style("position", "absolute")
        .style("right", "5px")
        .style("top", "2px")
        .style("cursor", "pointer")
    ;
    div_buttons.append("div").text("+").on("click", function() {
        result.zoom_in();
    });
    div_buttons.append("div").text("-").on("click", function() {
        result.zoom_out();
    });

    var y_axis_margin = 25;
    var x_axis_margin = 50;

    var svg = div.append("svg")
        .attr("width", 300)
        .attr("height", spec.height);
    var x = d3.time.scale().domain(boundary).range([0, 300 - x_axis_margin - 2]);
    var y = d3.scale.linear().domain([0, 1]).range([spec.height - y_axis_margin - 2, 0]);
    var line = d3.svg.line()
        .x(function(d) { return x(d.time); })
        .y(function(d) { return y(d.count); })
        .interpolate("linear")
    ;
    var pan_start_x, select_start_x;
    var time_selection;

    function pan() {
        var pan_current_x = d3.event.x;
        var delta_x = pan_current_x - pan_start_x;
        gr.attr("transform", "translate(" + delta_x + ",0)");
        ga_pan.attr("transform", "translate(" + delta_x + ",0)");
    }

    function clear_time_selection() {
        time_selection_rect.attr("display", "none");
        time_selection = undefined;
    }

    function select() {
        var select_current_x = x.invert(d3.event.offsetX);
        time_selection_rect
            .attr("height", spec.height - y_axis_margin - 2)
            .attr("x", Math.min(x(select_start_x), x(select_current_x)))
            .attr("width", Math.abs(x(select_current_x) - x(select_start_x)));
    }

    var main_g = svg.append("g");
    var gr = main_g.append("g");
    var group_x_axis = main_g.append("g").attr("transform", "translate(" + (300 - x_axis_margin) + ",0)");
    var ga_pan = main_g.append("g");
    var group_y_axis = ga_pan.append("g").attr("transform", "translate(0, " + (spec.height - y_axis_margin) + ")");

    var grab = svg.append("g").append("rect")
        .style("cursor", "all-scroll")
        .attr("width", 300 - x_axis_margin - 2)
        .attr("height", spec.height - y_axis_margin - 2)
        .style("fill-opacity", 0)
        .on("mousedown", function() {
            pan_start_x = d3.event.x;
            d3.select(this)
                .on("mousemove", pan);
        }).on("mouseup", function() {
            ga_pan.attr("transform", null);
            gr.attr("transform", null);
            d3.select(this)
                .on("mousemove", null);
            var pan_current_x = d3.event.x;
            var delta_x = pan_current_x - pan_start_x;
            var delta_t = (x.invert(1).getTime() - x.invert(0).getTime()) * delta_x;
            result.update_range(result.range[0].getTime() - delta_t,
                                result.range[1].getTime() - delta_t);
        });

    var grab_select = svg.append("g").append("rect")
        .style("cursor", "col-resize")
        .attr("y", spec.height - y_axis_margin - 2)
        .attr("width", 300 - x_axis_margin - 2)
        .attr("height", y_axis_margin - 2)
        .style("fill-opacity", 0)
        .on("mousedown", function() {
            select_start_x = x.invert(d3.event.offsetX);
            time_selection_rect
                .style("display", "inline")
                .attr("width", 0)
            ;
            d3.select(this)
                .on("mousemove", select);
        }).on("mouseup", function() {
            var select_end_x = x.invert(d3.event.offsetX);
            d3.select(this)
                .on("mousemove", null);
            if (x(select_end_x) === x(select_start_x)) {
                clear_time_selection();
            } else {
                if (select_end_x < select_start_x) {
                    time_selection = [select_end_x, select_start_x];
                } else {
                    time_selection = [select_start_x, select_end_x];
                }
            }
            _.each(handlers.select, function(f) {
                f(time_selection);
            });
        });

    var time_selection_rect = gr.append("g").append("rect")
        .style("fill", "white")
        .style("fill-opacity", 0.5)
        .style("display", "none")
        .attr("y", 0);

    var empty_data = [{time: boundary[0], count:0},
                      {time: boundary[1], count:0}];
    var path = gr.append("svg:path")
        .style("stroke", "white")
        .style("fill", "none")
        .attr("d", line(empty_data));

    var fmt = d3.time.format("%Y-%m-%d");
    var y_axis = d3.svg.axis()
        .scale(x)
        .tickFormat(fmt)
        .ticks(3)
    ;

    var x_axis = d3.svg.axis()
        .orient("right")
        .scale(y)
        .ticks(3)
    ;

    group_y_axis.call(y_axis);
    group_x_axis.call(x_axis);

    var clip = gr.append("defs").append("svg:clipPath")
        .attr("id", "clip")
        .append("svg:rect")
        .attr("id", "clip-rect")
        .attr("x", "0")
        .attr("y", "0")
        .attr("width", 300 - x_axis_margin - 2)
        .attr("height", spec.height - y_axis_margin - 2);

    gr.attr("clip-path", "url(#clip)");

    var old_d = empty_data;
    var handlers = { zoom: [], select: [] };
    
    var result = {
        range: [boundary[0], boundary[1]],
        resolution: 300 - x_axis_margin - 2,
        update_range: function(new_left, new_right) {
            var center = (new_left + new_right) / 2;
            var current_width = (new_right - new_left);
            if (current_width > max_width) {
                new_left += (current_width - max_width) / 2;
                new_right -= (current_width - max_width) / 2;
            }
            if (new_left < boundary[0].getTime()) {
                var delta = boundary[0].getTime() - new_left;
                new_left += delta;
                new_right += delta;
            }
            if (new_right > boundary[1].getTime()) {
                var delta = new_right - boundary[1].getTime();
                new_left -= delta;
                new_right -= delta;
            }
            var new_range = [new Date(new_left), new Date(new_right)];
            this.range = new_range;
            x.domain(this.range);
            path.attr("d", line(old_d));
            group_y_axis.call(y_axis);
            group_x_axis.call(x_axis);
            if (time_selection) {
                time_selection_rect
                    .attr("height", spec.height - y_axis_margin - 2)
                    .attr("x", x(time_selection[0]))
                    .attr("width", x(time_selection[1]) - x(time_selection[0]))
                    .attr("y", 0);
            }
            _.each(handlers.zoom, function(f) { f(); });
            return this;
        },
        zoom_in: function() {
            var from = this.range[0].getTime();
            var to = this.range[1].getTime();
            var old_width = to - from;
            var new_width = old_width / 2;
            var delta_width = new_width - old_width;
            var new_from = from - delta_width/2;
            var new_to = to + delta_width/2;
            return this.update_range(new_from, new_to);
        },
        zoom_out: function() {
            var from = this.range[0].getTime();
            var to = this.range[1].getTime();
            var old_width = to - from;
            var new_width = old_width * 2;
            var delta_width = new_width - old_width;
            var new_from = from - delta_width/2;
            var new_to = to + delta_width/2;
            return this.update_range(new_from, new_to);
        },
        new_data: function(d) {
            y.domain([0, d3.max(d, function(entry) { return entry.count; })]);
            group_x_axis.call(x_axis);
            if (old_d.length === d.length) {
                path.attr("d", line(d));
            } else {
                x.domain(this.range);
                group_y_axis.call(y_axis);
                path.attr("d", line(d));
            }
            old_d = d;
            return this;
        },
        on: function(event, handler) {
            handlers[event].push(handler);
            return this;
        }
    };

    return result;
}
