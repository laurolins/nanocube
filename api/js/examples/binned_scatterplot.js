function binned_scatterplot(parent, spec)
{
    var div = parent.append("div").attr("class", "vis-pane-bg");
    var svg = div.append("svg")
        .attr("width", 300)
        .attr("height", spec.height);
    var margin_x = spec.margin_x || 20;
    var margin_y = spec.margin_y || 10;
    var border = _.isUndefined(spec.border)? 2 : spec.border;
    var index_x = {}, index_y = {};
    _.each(spec.field_x.values, function(d, i) {
        index_x[d] = i;
    });
    _.each(spec.field_y.values, function(d, i) {
        index_y[d] = i;
    });

    var gr = svg.append("g");
    var glx = svg.append("g").attr("transform", "translate(0," + (spec.height-margin_y + border + 2) + ")");
    var gly = svg.append("g").attr("transform", "translate(" + (300-margin_x + border + 2) + ",0)");
    var data = [];
    var w = spec.field_x.values.length,
        h = spec.field_y.values.length;
    for (var i=0; i < w * h; ++i) data.push({
        x: i % w,
        y: ~~(i / w),
        index: i,
        value_x: spec.field_x.values[i % w],
        value_y: spec.field_y.values[~~(i / w)]
    });

    var colormap = spec.colormap || "absolute";

    var x = d3.scale.linear().domain([0, w]).range([0,300-margin_x]);
    var y = d3.scale.linear().domain([0, h]).range([0,spec.height-margin_y]);
    var color = d3.scale.linear().domain([0, 1]).range(["black", "white"]);

    // convert from sparse result given by server to
    // dense result needed by the update function.
    function map_to_dense(d) {
        var result = [];
        for (var i=0; i<w*h; ++i) {
            result.push(0);
        }
        _.each(d, function(entry) {
            result[index_x[entry[spec.field_x.name]] +
                   index_y[entry[spec.field_y.name]] * w] = entry.values[0];
        });
        return result;
    }
    function map_to_dense_field_x(d) {
        var result = [];
        for (var i=0; i<w; ++i) {
            result.push(0);
        }
        _.each(d, function(entry) {
            result[index_x[entry.key]] = entry.values;
        });
        return result;
    }
    function map_to_dense_field_y(d) {
        var result = [];
        for (var i=0; i<h; ++i) {
            result.push(0);
        }
        _.each(d, function(entry) {
            result[index_y[entry.key]] = entry.values;
        });
        return result;
    }

    gr.selectAll("rect")
        .data(data)
        .enter()
        .append("rect")
        .attr("width", x(1) - x(0) - border + 1)
        .attr("height", y(1) - y(0) - border + 1)
        .attr("x", function(d, i) {
            return x(d.x);
        })
        .attr("y", function(d, i) {
            return y(d.y);
        })
        .style("fill", "black");

    glx.selectAll("text")
        .data(spec.field_x.values)
        .enter()
        .append("text")
        .attr("x", function(d, i) { return x(i) + 1; })
        .attr("y", y(0.5))
        .style("fill", "white")
        .style("font-size", "0.5em")
        .text(function(d) { return d; })
    ;

    gly.selectAll("text")
        .data(spec.field_y.values)
        .enter()
        .append("text")
        .attr("y", function(d, i) { return y(i+0.8); })
        .attr("x", 0)
        .style("fill", "white")
        .style("font-size", "0.5em")
        .text(function(d) { return d; })
    ;

    var colormap_dispatch = {
        "absolute": update_absolute_colormap,
        "relative-field-x": update_relative_field_x_colormap,
        "relative-field-y": update_relative_field_y_colormap
    };

    function update_absolute_colormap(d) {
        var mx = d3.max(d, function(v) { return v.values[0]; });
        var dense = map_to_dense(d);
        color.domain([0, mx]);
        gr.selectAll("rect")
            .transition()
            .style("fill", function(d) {
                return color(dense[d.index]);
            });        
    }

    function update_relative_field_x_colormap(d) {
        var by_field_x = d3.nest()
            .key(function(d) { return d[spec.field_x.name]; })
            .rollup(function(d) { return d3.sum(d, function(d) { return d.values[0]; }); })
            .entries(d);
        var field_x_sums = map_to_dense_field_x(by_field_x);
        var mx = d3.max(field_x_sums);
        var data = map_to_dense(d);
        color.domain([0, 1]);
        gr.selectAll("rect")
            .transition()
            .style("fill", function(d) {
                return color(data[d.index] / field_x_sums[d.x]);
            });
    }

    function update_relative_field_y_colormap(d) {
        var by_field_y = d3.nest()
            .key(function(d) { return d[spec.field_y.name]; })
            .rollup(function(d) { return d3.sum(d, function(d) { return d.values[0]; }); })
            .entries(d);
        var field_y_sums = map_to_dense_field_y(by_field_y);
        var mx = d3.max(field_y_sums);
        var data = map_to_dense(d);
        color.domain([0, 1]);
        gr.selectAll("rect")
            .transition()
            .style("fill", function(d) {
                return color(data[d.index] / field_y_sums[d.x]);
            });
    }

    return {
        new_data: function(d) {
            colormap_dispatch[colormap](d);
        }
    };
}
