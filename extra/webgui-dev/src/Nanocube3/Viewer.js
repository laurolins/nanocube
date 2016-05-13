/*global $ jsep Expression Map Timeseries GroupedBarChart */

var Viewer = function(opts){
    var container = $(opts.div_id);
    var nanocubes = opts.nanocubes;
    var variables = [];

    this._container = container;
    this._nanocubes = nanocubes;
    this._urlargs = opts.urlargs;

    this.setupDivs(opts.config.widget);

    //View and controller
    var widget = {};
    this._widget = widget;

    var viewer = this;

    var k  = Object.keys(nanocubes);
    var nc = nanocubes[k[0]];
    Object.keys(nc.dimensions).forEach(function(dim){
        var d = nc.dimensions[dim];
        var dimtype = d.vartype;
        var dimsize = d.varsize;

        var options;

        switch(dimtype){
        case 'quadtree':
            if (!(d.name in opts.config.widget)){
                break;
            }

            options = $.extend(true, {}, opts.config.widget[d.name].div);
            options.name = d.name;
            options.model = viewer;
            options.levels = dimsize;
            options.args = viewer._urlargs[d.name] || null;

            widget[d.name]=new Map(options,function(bbox,zoom,
                                                    maptilesize){
                return viewer.getSpatialData(d.name,bbox,zoom);
            },function(args,constraints){
                return viewer.update([d.name],constraints,d.name,args);
            });
            break;

        case 'cat':
            if (!(d.name in opts.config.widget)){
                break;
            }
            
            options = $.extend(true, {}, opts.config.widget[d.name].div);
            options.name = d.name;
            options.model = viewer;
            options.args = viewer._urlargs[d.name] || null;

            widget[d.name]=new GroupedBarChart(options, function(){
                return viewer.getCategoricalData(d.name);
            },function(args,constraints){
                return viewer.update([d.name],constraints,d.name,args);
            });
            break;

        case 'id':
            if (!(d.name in opts.config.widget)){
                break;
            }

            options = $.extend(true, {}, opts.config.widget[d.name].div);
            options.name = d.name;
            options.model = viewer;
            options.args = viewer._urlargs[d.name] || null;

            widget[d.name]=new GroupedBarChart(options, function(){
                return viewer.getTopKData(d.name,options.topk);
            },function(args,constraints){
                return viewer.update([d.name],constraints,d.name,args);
            });
            break;

        case 'time':
            if (!(d.name in opts.config.widget)){
                break;
            }

            options = $.extend(true, {}, opts.config.widget[d.name].div);
            options.name = d.name;
            options.model = viewer;
            options.timerange = viewer.getTimeRange();
            options.args = viewer._urlargs[d.name] || null;

            widget[d.name]=new Timeseries(options,function(start,end,interval){
                return viewer.getTemporalData(d.name, start,end,interval);
            },function(args,constraints){
                return viewer.update([d.name],constraints,d.name,args);
            });
            break;
        }
    });

    //Expression input
    var expr_input=$('<input>').attr('id','expr_input').attr('size',100);
    $('#expr').append(expr_input);

    expr_input.on('change',function(e){
        try{
            viewer.expression = new Expression(expr_input[0].value);
            viewer.update();
        }
        catch(err){
            console.log(err);
        }
    });

    //init expression
    expr_input[0].value = k.join('+');
    viewer.expression = new Expression(expr_input[0].value);
};

Viewer.prototype = {
    broadcastConstraint: function(skip,constraint){
        var widget=this._widget;
        for (var v in widget){
            if(skip.indexOf(v) == -1){
                if(widget[v].addConstraint){
                    widget[v].addConstraint(constraint);
                }
            }
        }
    },

    setupDivs: function(config){
        for (var d in config){
            var newdiv = $('<div>');
            newdiv.attr('id', d);
            newdiv.css(config[d].div);
            this._container.append(newdiv);
        }
    },

    getTimeRange: function(){
        var nc = this._nanocubes;
        var range = Object.keys(nc).reduce(function(p,c){
            var s = nc[c].timeinfo.start;
            var e = nc[c].timeinfo.end;

            return [Math.min(p[0], nc[c].bucketToTime(s)),
                    Math.max(p[1], nc[c].bucketToTime(e))];
        }, [Infinity, 0]);
        return [new Date(range[0]), new Date(range[1])];
    },

    update: function(skip,constraints,name,args){
        console.log("skip: ",skip);

        skip = skip || [];
        constraints = constraints || [];
        var viewer = this;

        //update the url
        viewer.updateURL(name,args);

        //add constraints ....
        for (var c in constraints){
            viewer.broadcastConstraint(skip,constraints[c]);
        }

        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1){
                //re-render
                viewer._widget[d].update();
            }
        });
    },

    constructQuery: function(nc,skip){
        skip = skip || [];

        var viewer = this;
        var queries = {};
        queries.global = nc.query();

        //brush
        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1){
                var sel = viewer._widget[d].getSelection();

                if(sel.global){
                    queries.global=queries.global.setConstraint(d,sel.global);
                }

                if(sel.brush){
                    queries.global=queries.global.setConstraint(d,sel.brush);
                }                
            }
        });
        
        //then the rest
        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1){
                var sel = viewer._widget[d].getSelection();
                Object.keys(sel).filter(function(d){
                    return (d != 'brush') && (d != 'global');
                }).forEach(function(s){
                    //get an appropriate query
                    var q = queries[s] || $.extend(true,{},queries.global);
                    //add a constraint
                    queries[s] = q.setConstraint(d,sel[s]);
                });
            }
        });
        
        console.log(queries.global,skip);
        
        if (Object.keys(queries).length > 1){
            delete queries.global;
        }

        return queries;
    },

    getSpatialData:function(varname, bbox, zoom, maptilesize){
        var k = Object.keys(this._nanocubes);
        var viewer = this;

        //construct a list of queries
        var cq = {};
        k.forEach(function(d){
            var nc = viewer._nanocubes[d];
            cq[d]=viewer.constructQuery(nc,[varname]);
        });

        //organize the queries by selection
        var selq = {};
        Object.keys(cq).forEach(function(d){
            Object.keys(cq[d]).forEach(function(s){
                selq[s] = selq[s] || {};
                selq[s][d] = cq[d][s];
            });
        });

        //remove global const if there is a selection
        //if (Object.keys(selq).length > 1){
        //    delete selq.global;
        //}

        //generate queries for each selections
        var res = {};
        var expr = this.expression;
        if (!expr) {
            Object.keys(selq).forEach(function(s){
                res[s]=selq[s][k[0]].spatialQuery(varname,bbox,
                                                  zoom,maptilesize);
            });
        }
        else{
            Object.keys(selq).forEach(function(s){
                res[s] = expr.getData(selq[s],function(q){
                    return q.spatialQuery(varname,bbox,zoom,maptilesize);
                });
            });
        }
        return res;
    },

    getTemporalData:function(varname, start,end,intervalsec){
        var k = Object.keys(this._nanocubes);
        var viewer = this;

        //construct a list of queries
        var cq = {};
        k.forEach(function(d){
            var nc = viewer._nanocubes[d];
            cq[d]=viewer.constructQuery(nc,[varname]);
        });

        //organize the queries by selection
        var selq = {};
        Object.keys(cq).forEach(function(d){
            Object.keys(cq[d]).forEach(function(s){
                selq[s] = selq[s] || {};
                selq[s][d] = cq[d][s];
            });
        });

        //remove global const if there is a selection
        //if (Object.keys(selq).length > 1){
        //    delete selq.global;
        //}

        //generate queries for each selections
        var res = {};
        var expr = this.expression;
        if (!expr) {
            Object.keys(selq).forEach(function(s){
                res[s]=selq[s][k[0]].temporalQuery(varname,start,end,
                                                   intervalsec);
            });
        }
        else{
            Object.keys(selq).forEach(function(s){
                res[s] = expr.getData(selq[s],function(q){
                    return q.temporalQuery(varname,start,end,intervalsec);
                });
            });
        }
        return res;
    },

    getTopKData:function(varname, n){
        n = n || 20; // hard code test for now
        var k = Object.keys(this._nanocubes);
        var viewer = this;

        //construct a list of queries
        var cq = {};
        k.forEach(function(d){
            var nc = viewer._nanocubes[d];
            cq[d]=viewer.constructQuery(nc,[varname]);
        });

        //organize the queries by selection
        var selq = {};
        Object.keys(cq).forEach(function(d){
            Object.keys(cq[d]).forEach(function(s){
                selq[s] = selq[s] || {};
                selq[s][d] = cq[d][s];
            });
        });

        //remove global const if there is a selection
        //if (Object.keys(selq).length > 1){
        //    delete selq.global;
        //}

        //generate queries for each selections
        var res = {};
        var expr = this.expression;
        if (!expr) {
            Object.keys(selq).forEach(function(s){
                res[s]=selq[s][k[0]].topKQuery(varname,n);
            });
        }
        else{
            Object.keys(selq).forEach(function(s){
                res[s] = expr.getData(selq[s],function(q){
                    return q.topKQuery(varname,n);
                });
            });
        }
        return res;
    },

    getCategoricalData:function(varname){
        var k = Object.keys(this._nanocubes);
        var viewer = this;

        //construct a list of queries
        var cq = {};
        k.forEach(function(d){
            var nc = viewer._nanocubes[d];
            cq[d]=viewer.constructQuery(nc,[varname]);
        });

        //organize the queries by selection
        var selq = {};
        Object.keys(cq).forEach(function(d){
            Object.keys(cq[d]).forEach(function(s){
                selq[s] = selq[s] || {};
                selq[s][d] = cq[d][s];
            });
        });

        //remove global const if there is a selection
        //if (Object.keys(selq).length > 1){
        //    delete selq.global;
        //}

        //generate queries for each selections
        var res = {};
        var expr = this.expression;
        if (!expr) {
            Object.keys(selq).forEach(function(s){
                res[s]=selq[s][k[0]].categorialQuery(varname);
            });
        }
        else{
            Object.keys(selq).forEach(function(s){
                res[s] = expr.getData(selq[s],function(q){
                    return q.categorialQuery(varname);
                });
            });
        }
        return res;
    },

    updateURL: function(k,argstring){
        if(!k || !argstring){
            return;
        }

        var args = this._urlargs;
        args[k] = argstring;

        var res = Object.keys(args).map(function(k){
            return k+'='+args[k];
        });
        var argstr = '?'+ res.join('&');

        //change the url
        window.history.pushState('test','title',
                                 window.location.pathname+argstr);
    }
};
