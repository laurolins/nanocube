var Nanocube = {};

Nanocube.diff_queries = function(q1, q2)
{
    var result = {
        when: ((_.isUndefined(q1.when) && !_.isUndefined(q2.when)) ||
               (_.isUndefined(q2.when) && !_.isUndefined(q1.when)) ||
               (!_.isUndefined(q1.when) && !_.isUndefined(q2.when) && 
                _.any(q1.when, function(v, k) {
                    return q2.when[k] !== v;
                }))),
        where: _.any(q1.where, function(v1, k) {
            var v2 = q2.where[k] || [];
            return v1.length != v2.length || _.any(v1, function(v_elt, i) {
                return v1[i] !== v2[i];
            });
        }) || _.any(q2.where, function(v1, k) {
            var v2 = q1.where[k] || [];
            return v1.length != v2.length || _.any(v1, function(v_elt, i) {
                return v1[i] !== v2[i];
            });
        }),
        region: ((_.isUndefined(q1.region) && !_.isUndefined(q2.region)) ||
                 (_.isUndefined(q2.region) && !_.isUndefined(q1.region)) ||
                 (!_.isUndefined(q1.region) && !_.isUndefined(q2.region) && (
                     (q1.region.z !== q2.region.z) ||
                         (q1.region.x[0] !== q2.region.x[0]) ||
                         (q1.region.x[1] !== q2.region.x[1]) ||
                         (q1.region.y[0] !== q2.region.y[0]) ||
                         (q1.region.y[1] !== q2.region.y[1]))))
    };
    return result;
};

Nanocube.create = function(opts)
{
    var url = opts.url;
    var max_zoom;

    function tile_subquery(opts) {
        opts = opts || {};
        var x = opts.x || 0, y = opts.y || 0, z = opts.z || 0, 
            resolution = _.isUndefined(opts.resolution)?8:opts.resolution;
        return z + 
            "/" + resolution + 
            "/" + x + 
            "/" + ((1 << z) - 1 - y);
    }

    function field_subquery(opts) {
        opts = opts || {};
        var result = "";
        var i = 0;
        for (var k in opts) {
            if (i++) result = result + ";";
            result = result + k + "=" + opts[k].join("|");
        }
        return result;
    }

    function time_subquery(opts) {
        return opts.from + "/" + opts.step + "/" + opts.count;
    }

    function time_range_subquery(opts) {
        if (_.isUndefined(opts) || _.isUndefined(opts.from))
            return "/0/10000000000";
        return "/" + opts.from + "/" + opts.to;
    }

    //////////////////////////////////////////////////////////////////////////
    // query generators for query/ scheme

    function region_subquery(opts) {
        if (_.isUndefined(opts))
            opts = { x: [0, 1 << max_zoom], y: [0, 1 << max_zoom], z: max_zoom };
        if (opts.z > max_zoom) {
            var shrinkage = 1 << (opts.z - max_zoom);
            opts = { x: [(opts.x[0] / shrinkage) | 0, (opts.x[1] / shrinkage) | 0],
                     y: [(opts.y[0] / shrinkage) | 0, (opts.y[1] / shrinkage) | 0],
                     z: max_zoom };
        }
        return "region/" + opts.z + "/" + opts.x[0] + "/" + opts.y[0] + "/" + opts.x[1] + "/" + opts.y[1];
    }

    function where_subquery(opts) {
        var a = _.map(opts, function(v, k) {
            v = v.join("|");
            return k + "=" + v;
        });
        if (a.length)
            return "/where/" + a.join(";");
        else
            return "";
    }

    function when_subquery(opts) {
        if (_.isUndefined(opts) || _.isUndefined(opts.from))
            return "";
        else
            return "/tseries/" + opts.from + "/" + (opts.to - opts.from) + "/1";
    }

    var result = {
        schema: undefined,
        to_tbin: function(time) {
            var delta = (time.getTime() - this.schema.time_schema.epoch.getTime());
            return ~~(delta / this.schema.time_schema.tick);
        },
        from_tbin: function(tbin) {
            var newtime = this.schema.time_schema.epoch.getTime() + (tbin * this.schema.time_schema.tick);
            return new Date(newtime);
        },
        tile: function(opts, k) {
            var tile = tile_subquery(opts.tile);
            var time = time_range_subquery(opts.time);
            var fields = field_subquery(opts.fields);
            var this_url = url + "/tile/" + tile + time + "/" + fields;
            
            Lux.Net.binary(this_url, function(data) {
                if (data === null) {
                    k({x:[], y:[], count:[]});
                    return;
                }
                console.log(data);
                var view = new DataView(data);
                // slow, meh
                var x_array = new Uint8Array(data.byteLength / 6);
                var y_array = new Uint8Array(data.byteLength / 6);
                var count_array = new Uint32Array(data.byteLength / 6);
                for (var i=0; i<data.byteLength/6; ++i) {
                    x_array[i] = view.getUint8(6*i+1);
                    y_array[i] = 256 - view.getUint8(6*i);
                    count_array[i] = view.getUint32(6*i+2, true);
                }
                k({x: x_array, y: y_array, count: count_array});
            });
        },
        time_series: function(opts, k) {
            var time = time_subquery(opts.time);
            var region = region_subquery(opts.region);
            var this_url = url + "/query/tseries/" + time + "/" + region;
            this_url = this_url + where_subquery(opts.where);
            d3.json(this_url, function(data) { k(data); });
        },
        category: function(opts, k) {
            var field = opts.fields.join("/field/");
            var region = region_subquery(opts.region);
            var this_url = url + "/query/field/" + field + "/" + region;
            this_url = this_url + where_subquery(opts.where);
            this_url = this_url + when_subquery(opts.when);
            d3.json(this_url, function(data) { k(data); });
        },
        all: function(opts, k) {
            var region = region_subquery(opts.region);
            var this_url = url + "/query/" + region;
            this_url = this_url + where_subquery(opts.where);
            this_url = this_url + when_subquery(opts.when);
            d3.json(this_url, function(data) { k(data); });
        },
        selection: function() {
            var nanocube = this;
            var observers = [];

            return {
                query: {
                    where: {},
                    region: { x: [0, 1 << max_zoom], y: [0, 1 << max_zoom], z: max_zoom }
                },
                nanocube: nanocube,
                refresh: function() { 
                    var that = this;
                    _.each(observers, function(f) { f(that.query); });
                    return this;
                },
                update_when: function(new_when) {
                    if (_.isUndefined(new_when)) {
                        delete this.query.when;
                    } else {
                        this.query.when = new_when;
                    }
                    return this;
                },
                update_region: function(new_region) {
                    this.query.region = new_region;
                    return this;
                },
                update_where: function(key, value) {
                    this.query.where[key] = value;
                    if (value.length === 0)
                        delete this.query.where[key];
                    return this;
                },
                all: function(k) {
                    var result = function(q) {
                        nanocube.all(q, k);
                    };
                    observers.push(result);
                    return result;
                },
                category: function(fields, k) {
                    var result = function(q) {
                        nanocube.category(_.extend(q, { fields: fields }), k);
                    };
                    observers.push(result);
                    return result;
                },
                remove_observer: function(f) {
                    observers = _.without(observers, f);
                    return this;
                },
                add_observer: function(f) {
                    observers.push(f);
                    return this;
                }
            };
        }
    };
    d3.json(url + "/schema_json", function(error, data) {
        result.schema = data;
        var tbin = data.tbin;
        max_zoom = data.sbin;
        var s = tbin.split('_');
        var date = _.map(s[0].split('-'), Number);
        date[1] -= 1;
        var time = _.map(s[1].split(':'), Number);
        var tick_units = {
            "h": 3600 * 1000,
            "d": 3600 * 1000 * 24,
            "w": 3600 * 1000 * 24 * 7
        }[s[2][s[2].length-1]];
        if (_.isUndefined(tick_units))
            throw "Unrecognized tick unit in " + s[2];
        var ticks = Number(s[2].substr(0, s[2].length-1)) * tick_units;
        result.schema.time_schema = {
            epoch: new Date(date[0], date[1], date[2], time[0], time[1], time[2]),
            tick: ticks
        };
        opts.ready && opts.ready.call(result);
    });
    return result;
};
