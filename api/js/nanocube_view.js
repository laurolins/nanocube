// FIXME bug on asking too many points from time series.
function create_nanocube_view(view_schema, nanocube_selection) {
    var nanocube = nanocube_selection.nanocube;

    document.title = '"' + view_schema.title + '" nanocube view';
    d3.select("#header").text(view_schema.title);

    function create_timeseries_view(parent, spec) {
        if (_.isUndefined(spec.time_range))
            spec.time_range = view_schema.time_range;
        var ts = timeseries(parent, spec);

        function update_time_series(d) {
            var from = nanocube.to_tbin(ts.range[0]);
            var to = nanocube.to_tbin(ts.range[1]);
            var step = Math.max(1, ~~((to - from) / ts.resolution));
            ts.new_data(_.map(d[0].values, function(count, i) {
                return { time: nanocube.from_tbin(i * step + from),
                         count: count
                       };
            }));
        }

        function refresh_time_series() {
            var time = {};
            var from = nanocube.to_tbin(ts.range[0]);
            var to = nanocube.to_tbin(ts.range[1]);
            var step = ((to - from) / ts.resolution);
            time.from = from;
            if (step < 1) {
                time.count = ts.resolution;
                time.step = 1;
            } else {
                var count_correction = step / ~~step;
                time.count = ~~(ts.resolution * count_correction);
                time.step = ~~step;
            }
            // take rounding into account
            // console.log(ts.resolution, time.count);
            nanocube.time_series(_.extend(nanocube_selection.query, {
                time: time
            }), update_time_series);
        };

        function update_selection(selection) {
            if (_.isUndefined(selection)) {
                nanocube_selection 
                    .update_when(undefined)
                    .refresh();
            } else {
                nanocube_selection 
                    .update_when({ from: nanocube.to_tbin(selection[0]),
                                   to: nanocube.to_tbin(selection[1])
                                 })
                    .refresh();
            }
        }

        ts.on("zoom", refresh_time_series);
        ts.on("select", update_selection);
        nanocube_selection.add_observer(refresh_time_series);
    };

    function create_binned_scatterplot_view(parent, spec) {
        var bs = binned_scatterplot(parent, spec);
        nanocube_selection.category(
            [spec.field_x.name, spec.field_y.name], function(d) {
                bs.new_data(d);
            });
    }

    var fmt = d3.format(',');

    function create_count_view(parent, spec) {
        var div = parent.append("div").attr("class", "vis-pane-bg");
        div.append("span").text("Total count: ");
        var selected_count = div.append("span");
        div.append("span").text(" of ");
        var total_count = div.append("span");
        nanocube.all({}, function(data) {
            total_count.text(fmt(data[0].values[0]));
        });
        nanocube_selection.all(function(data) {
            if (data.length === 0)
                selected_count.text("0");
            else
                selected_count.text(fmt(data[0].values[0]));
        });
    }

    function create_parsets_view(parent, spec) {
        var schema_fields = spec.fields;
        var parsets = d3.parsets()
            .dimensions(schema_fields)
            .value(function(d) { return d.values[0]; })
            .width(300)
            .height(spec.height);
        var parsets_div = parent
            .append("div")
            .attr("class", "vis-pane-bg")
            .attr("style", "background-color: white")
            .append("svg")
            .attr("width", parsets.width())
            .attr("height",parsets.height());
        nanocube_selection.category(schema_fields, function(data) {
            parsets_div.datum(data).call(parsets);
        });
    }

    function create_histogram_view(parent, spec) {
        var element = parent
            .append("div")
            .attr("class", "vis-pane-bg");
        var field = spec.field;
        var histogram = init_histogram({
            element: element,
            field: field,
            height: spec.height,
            border: spec.border
        });
        histogram.on("select", function(selected_fields) {
            nanocube_selection
                .update_where(field.name, selected_fields)
                .refresh();
        });
        nanocube_selection.category(
            [field.name],
            function(data) { histogram.new_data(data); }
        );
    }

    var dispatch = {
        "histogram": create_histogram_view,
        "parallel-sets": create_parsets_view,
        "count": create_count_view,
        "binned-scatterplot": create_binned_scatterplot_view,
        "time-series": create_timeseries_view
    };

    _.each(view_schema.views, function(view) {
        dispatch[view.type](d3.select(view_schema.parent_div), view);
    });
};
