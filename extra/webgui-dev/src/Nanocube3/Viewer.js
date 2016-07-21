/*global $ jsep colorbrewer Expression Map Timeseries GroupedBarChart */

var Viewer = function(opts){
    var container = $(opts.div_id);
    var nanocubes = opts.nanocubes;
    var variables = [];
 
    this._container = container;
    this._nanocubes = nanocubes;
    this._urlargs = opts.urlargs;
    this._widget = {};
    var viewer = this;
    

    //Expressions input
    this._data = opts.config.data;
    var data = this._data;    
    for (var d in this._data){
        var exp = this._data[d].expr;
        try{
            exp = new Expression(exp);
            data[d].expr = exp;
            if(typeof data[d].colormap == 'string'){
                data[d].colormap = colorbrewer[data[d].colormap][9].slice(0);
                data[d].colormap.reverse();
            }
        }
        catch(err){
            console.log('Cannot parse '+ exp + '--' + err);            
        }
    }

    //Setup each widget
    for (var w in opts.config.widget){
        viewer._widget[w] = viewer.setupWidget(w,opts.config.widget[w],
                                               opts.config.widget[w].levels);
    }
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
    
    setupWidget:function(id, widget, levels){
        var options = $.extend(true, {}, widget.div);
        var viewer = this;
        options.name = id;
        options.model = viewer;
        options.args = viewer._urlargs[id] || null;

        //add the div
        var newdiv = $('<div>');
        newdiv.attr('id', id);
        newdiv.css(widget.div);
        this._container.append(newdiv);
        
        //Create the widget
        switch(widget.type){
        case 'spatial':            
            options.levels = levels || 25;
            return new Map(options,function(bbox,zoom,maptilesize){
                return viewer.getSpatialData(id,bbox,zoom);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
            
        case 'cat':
            return new GroupedBarChart(options, function(){
                return viewer.getCategoricalData(id);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
            
        case 'id':
            return new GroupedBarChart(options, function(){
                return viewer.getTopKData(id,options.topk);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
            
        case 'time':
            options.timerange = viewer.getTimeRange();
            return new Timeseries(options,function(start,end,interval){
                return viewer.getTemporalData(id,start,end,interval);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
        default:
            return null;
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

        //generate queries for each selections
        var res = {};
        var data = viewer._data;
        Object.keys(selq).forEach(function(s){
            Object.keys(data).forEach(function(d){
                if(data[d].disabled){
                    return;
                }
                var expr = data[d].expr;
                var cidx = data[d].colormap.length/2;
                var c = data[d].colormap[Math.floor(cidx)];                
                console.log(s+'-'+c);
                res[s+'-'+c] = expr.getData(selq[s],function(q){
                    return q.spatialQuery(varname,bbox,zoom,maptilesize);
                });
            });
        });
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

        //generate queries for each selections
        var res = {};        
        var data = viewer._data;
        Object.keys(selq).forEach(function(s){            
            Object.keys(data).forEach(function(d){
                if(data[d].disabled){
                    return;
                }
                var expr = data[d].expr;
                var cidx = data[d].colormap.length/2;
                var c = data[d].colormap[Math.floor(cidx)];
                res[s+'-'+c] = expr.getData(selq[s],function(q){
                    return q.temporalQuery(varname,start,end,intervalsec);
                });
            });
        });
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

        //generate queries for each selections
        var res = {};
        Object.keys(selq).forEach(function(s){
            res[s] = expr.getData(selq[s],function(q){
                return q.topKQuery(varname,n);
            });
        });
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

        //generate queries for each selections
        var res = {};
        var data = viewer._data;
        Object.keys(selq).forEach(function(s){
            Object.keys(data).forEach(function(d){
                if(data[d].disabled){
                    return;
                }
                var expr = data[d].expr;
                var cidx = data[d].colormap.length/2;
                var c = data[d].colormap[Math.floor(cidx)];
                res[s+'-'+c] = expr.getData(selq[s],function(q){
                    return q.categorialQuery(varname);
                });
            });
        });
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
                                 window.location.pathname.replace(/\/$/,'')+
                                 argstr);
    }
};
