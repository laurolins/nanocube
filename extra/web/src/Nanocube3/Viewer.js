/*global $ d3 jsep colorbrewer Expression Map Timeseries GroupedBarChart */

var Viewer = function(opts){
    var container = $(opts.div_id);
    //set title
    if(opts.config.title){
        d3.select('head')
            .append('title')
            .html(opts.config.title);
    }
    
    //overlays
    var catdiv = $('<div>');
    catdiv.addClass('chart-overlay');
    catdiv.attr('id', 'cat_overlay');
    container.append(catdiv);
    
    var timediv = $('<div>');
    timediv.addClass('chart-overlay');
    timediv.attr('id', 'time_overlay');
    container.append(timediv);
    

    //setup
    var nanocubes = opts.nanocubes;
    var variables = [];
    
    this._container = container;
    this._catoverlay = catdiv;
    this._timeoverlay = timediv;

    this._nanocubes = nanocubes;
    this._urlargs = opts.urlargs;
    this._widget = {};
    this._datasrc = opts.config.datasrc;
    var viewer = this;
    
    //Expressions input
    var datasrc = this._datasrc;
    for (var d in datasrc){
        var exp = datasrc[d].expr;
        var colormap = datasrc[d].colormap;
        try{
            //make an expression
            datasrc[d].expr = new Expression(datasrc[d].expr);
            if(typeof colormap == 'string'){
                //make a copy of the colormap
                datasrc[d].colormap = colorbrewer[colormap][9].slice(0);
                datasrc[d].colormap.reverse();
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
        var options = $.extend(true, {}, widget);
        var viewer = this;
        
        options.name = id;
        options.model = viewer;
        options.args = viewer._urlargs[id] || null;
        options.datasrc = viewer._datasrc;

        //add the div
        var newdiv = $('<div>');
        newdiv.attr('id', id);
        newdiv.css(widget.css);
        
        //Create the widget
        switch(widget.type){
        case 'spatial':            
            this._container.append(newdiv);
            options.levels = levels || 25;
            return new Map(options,function(datasrc,bbox,zoom,maptilesize){
                return viewer.getSpatialData(id,datasrc,bbox,zoom);
            },function(args,constraints,datasrc){
                return viewer.update([id],constraints,
                                     id,args,datasrc);
            });
            
        case 'cat':
            this._catoverlay.append(newdiv);
            return new GroupedBarChart(options,function(datasrc){
                return viewer.getCategoricalData(id,datasrc);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
            
        case 'id':
            this._catoverlay.append(newdiv);
            return new GroupedBarChart(options, function(datasrc){
                return viewer.getTopKData(id,datasrc,options.topk);
            },function(args,constraints){
                return viewer.update([id],constraints,
                                     id,args);
            });
            
        case 'time':
            this._timeoverlay.append(newdiv);
            options.timerange = viewer.getTimeRange();
            return new Timeseries(options,function(datasrc,start,end,interval){
                return viewer.getTemporalData(id,datasrc,start,end,interval);
            },function(args,constraints){
                return viewer.update([id],constraints,id,args);
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

    update: function(skip,constraints,name,args,datasrc){
        //console.log("skip: ",skip);

        skip = skip || [];
        constraints = constraints || [];
        var viewer = this;

        //change datasrc configuration
        if(datasrc){
            for (var d in viewer._datasrc){
                viewer._datasrc[d].disabled = datasrc[d].disabled;
            }
        }
        
        //update the url
        viewer.updateURL(name,args);

        //add constraints ....
        for (var c in constraints){
            viewer.broadcastConstraint(skip,constraints[c]);
        }

        Object.keys(viewer._widget).forEach(function(d){
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
        
        //console.log(queries.global,skip);
        
        if (Object.keys(queries).length > 1){
            delete queries.global;
        }

        return queries;
    },

    getSpatialData:function(varname, datasrc, bbox, zoom, maptilesize){
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
        var data = viewer._datasrc;
        var expr = data[datasrc].expr;
        Object.keys(selq).forEach(function(s){
            res[s+'&-&'+datasrc] = expr.getData(selq[s],function(q){
                return q.spatialQuery(varname,bbox,zoom,maptilesize);
            });
        });
        return res;
    },

    getTemporalData:function(varname,datasrc,start,end,intervalsec){
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
        var data = viewer._datasrc;
        Object.keys(selq).forEach(function(s){            
            var expr = data[datasrc].expr;
            res[s+'&-&'+datasrc] = expr.getData(selq[s],function(q){
                return q.temporalQuery(varname,start,end,intervalsec);
            });
        });
        return res;
    },

    getTopKData:function(varname,datasrc,n){
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
        var data = viewer._datasrc;
        Object.keys(selq).forEach(function(s){
            var expr = data[datasrc].expr;
            res[s+'&-&'+datasrc] =expr.getData(selq[s],function(q){
                return q.topKQuery(varname,n);
            });
        });
        return res;
    },

    getCategoricalData:function(varname,datasrc){
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
        var data = viewer._datasrc;
        Object.keys(selq).forEach(function(s){
            var expr = data[datasrc].expr;
            res[s+'&-&'+datasrc] = expr.getData(selq[s],function(q){
                return q.categorialQuery(varname);
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
                                 window.location.pathname+
                                 argstr);
    }
};
