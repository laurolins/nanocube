function init_histogram(opts)
{
    var c = d3.scale.category10()(1);
    var color = opts.color || function() { return c; };
    var unselected_color = opts.unselected_color || function() { return "#888"; };
    var field = opts.field;
    var n_cats = field.values.length;
    var category = field.values;
    var indices = {};
    var selected = [];
    var any_selected = false;
    _.each(field.values, function(v, i) { 
        indices[v] = i; 
        selected[i] = false;
    });
    var element = opts.element;
    var width = opts.width || 300;
    var height = opts.height || 120;
    var border = _.isUndefined(opts.border) ? 2 : opts.border;

    var fmt = d3.format(',');

    function make_histogram() {
        var w = width, h = height;
        element.append("div").text(field.title || field.name);
        var g = 
            element
            .append("svg")
            .attr("width", w)
            .attr("height", h)
            .append("g");

        var x = d3.scale.linear().domain([0, 10000]).range([0, w]);
        var y = d3.scale.linear().domain([0, n_cats]).range([h-10, 5]);
        var current_data = [];
        for (var i=0; i<category.length; ++i) current_data[i] = 0;

        function toggle_select(d, i) {
            selected[i] = !selected[i];
            if (!d3.event.shiftKey) {
                for (var j=0; j<selected.length; ++j)
                    if (j !== i)
                        selected[j] = false;
            }

            var selected_fields = _.filter(category, function(v, i) {
                return selected[i];
            });
            any_selected = selected_fields.length > 0;
            _.each(event_handlers.select, function(f) {
                f(selected_fields);
            });
        }

        var d = g.selectAll("g")
            .data(current_data)
            .enter()
            .append("g");

        // g.selectAll("rect")
        //     .data(current_data)
        //     .enter()
        //     .append("rect")
        //     .on("click", toggle_select)
        // ;

        d.append("rect")
            .attr("x", 0)
            .attr("y", function(d, i) { return y(i) - (y(0)-y(1)); })
            .attr("width", x)
            .attr("height", Math.abs(y(0)-y(1)) + 1 - border)
            .attr("fill", function(d, i) { 
                if (selected[i])
                    return color(i);
                else
                    return unselected_color(i); 
            })
            .on("mouseover", function(d, i) { 
                d3.select(this.parentNode)
                    .selectAll("text")
                    .text(category[i] + " - " + fmt(current_data[i]));
            })
            .on("mouseout", function(d, i) { 
                d3.select(this.parentNode)
                    .selectAll("text")
                    .text(category[i]);
            })
            .style("cursor", "pointer")
            .on("click", toggle_select)
        ;

        d.append("svg:text")
            .attr("x", ".35em")
            .attr("dy", ".35em")
            .attr("y", function(d, i) { return y(i) - (y(0)-y(1))/2; })
            .attr("fill", "white")
            .text(function(d, i) { return category[i]; })
            .on("mouseover", function(d, i) { 
                d3.select(this)
                    .text(category[i] + " - " + fmt(current_data[i])); 
            })
            .on("mouseout", function(d, i) { 
                d3.select(this)
                    .text(category[i]); 
            })
            .style("cursor", "pointer")
            .on("click", toggle_select)
        ;

        return {
            new_data: function(data) {
                current_data = data;
                var mx = d3.max(data);
                if (mx === 0)
                    mx = 1;
                x.domain([0, mx]);
                g.selectAll("rect")
                    .data(data)
                    .transition()
                    .attr("width", x)
                    .attr("fill", function(d, i) { 
                        if (selected[i])
                            return color(i);
                        else
                            return unselected_color(i); 
                    })
                ;
            }
        };
    }

    var g = make_histogram();

    var event_handlers = {
        select: []
    };

    var result = {
        new_data: function(data) {
            var vs = [];
            for (var i=0; i<n_cats; ++i) vs[i] = 0;
            _.each(data, function(e) {
                vs[indices[e[field.name]]] = e.values[0];
            });
            g.new_data(vs);
            return this;
        }, on: function(event, callback) {
            event_handlers[event].push(callback);
            return this;
        }
    };

    return result;
}
