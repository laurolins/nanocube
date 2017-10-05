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

    var retdiv = $('<div>');
    retdiv.addClass('chart-overlay');
    retdiv.attr('id', 'ret-overlay');
    container.append(retdiv);
    

    //setup
    var nanocubes = opts.nanocubes;
    var variables = [];
    
    this._container = container;
    this._catoverlay = catdiv;
    this._timeoverlay = timediv;
    this._retoverlay = retdiv;

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
        var colormap2 = datasrc[d].colormap2;
        try{
            //make an expression
            datasrc[d].expr = new Expression(datasrc[d].expr);
            if(typeof colormap == 'string'){
                //make a copy of the colormap
                datasrc[d].colormap = colorbrewer[colormap][9].slice(0);
                datasrc[d].colormap.reverse();
            }
            if(typeof colormap2 == 'string'){
                //make a copy of the colormap
                datasrc[d].colormap2 = colorbrewer[colormap2][9].slice(0);
                datasrc[d].colormap2.reverse();
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

    var retwidget = {
        "type": 'ret', 
        "css": {
            "opacity": 0.8, 
            "height": "150px", 
            "width": "300px",
            "position": "absolute",
            "left": "50px",
            "top": "20px"
        }, 
        "title": "Retinal Brush"
    };

    viewer._widget.ret = viewer.setupWidget('ret', retwidget);

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
        // console.log(typeof options);
        var viewer = this;
        
        options.name = id;
        options.model = viewer;
        options.args = viewer._urlargs[id] || null;
        options.datasrc = viewer._datasrc;

        //add the div
        var newdiv = $('<div>');
        newdiv.attr('id', id);
        newdiv.css(widget.css);

        // console.log(newdiv);
        
        //Create the widget
        switch(widget.type){
        case 'spatial':
            this._container.append(newdiv);
            options.levels = levels || 25;
            return new Map(options,function(datasrc,bbox,zoom,maptilesize){
                return viewer.getSpatialData(id,datasrc,bbox,zoom);
            },function(args,constraints,datasrc){
                return viewer.update([id, 'ret'],constraints,
                                     id,args,datasrc);
            },function(){
                return viewer.getXYData([id]);
            });
            
        case 'cat':
            options.compare = true;
            this._catoverlay.append(newdiv);
            return new GroupedBarChart(options,function(datasrc){
                return viewer.getCategoricalData(id,datasrc);
            },function(args,constraints){
                return viewer.update([id, 'ret'],constraints,
                                     id,args);
            },function(){
                return viewer.getXYData([id]);
            });
            
        case 'id':
            this._catoverlay.append(newdiv);
            return new GroupedBarChart(options, function(datasrc){
                return viewer.getTopKData(id,datasrc,options.topk);
            },function(args,constraints){
                return viewer.update([id, 'ret'],constraints,
                                     id,args);
            },function(){
                return viewer.getXYData([id]);
            });
            
        case 'time':
            this._timeoverlay.append(newdiv);
            options.timerange = viewer.getTimeRange();
            options.binsec = viewer.getBinTime();
            return new Timeseries(options,function(datasrc,start,end,interval){
                return viewer.getTemporalData(id,datasrc,start,end,interval);
            },function(args,constraints){
                return viewer.update([id, 'ret'],constraints,id,args);
            },function(){
                return viewer.getXYData([id]);
            });

        case 'ret':
            this._retoverlay.append(newdiv);
            return new RetinalBrushes(options, function(args,retbrush){
                return viewer.update([id],false,id,args,false,retbrush);
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

    getBinTime: function(){
        var nc = this._nanocubes;
        var binsec = Object.keys(nc).map(function(c){
            return nc[c].timeinfo.bin_sec;
        });
        return binsec;
    },

    update: function(skip,constraints,name,args,datasrc,retbrush){
        // console.log("skip: ",skip);

        skip = skip || [];
        constraints = constraints || [];
        var viewer = this;

        //change datasrc configuration
        if(datasrc){
            for (var d in viewer._datasrc){
                viewer._datasrc[d].disabled = datasrc[d].disabled;
            }
        }

        if(retbrush){
            Object.keys(viewer._widget).forEach(function(d){
                if(skip.indexOf(d) == -1){
                    viewer._widget[d].retbrush = retbrush;
                }
            });
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
        skip.push('ret');

        // console.log("skip: ",skip);

        var viewer = this;
        var queries = {};
        queries.global = nc.query();

        var retbrush;
        if(this._widget.ret)
            retbrush = this._widget.ret.getSelection();
        else{
            retbrush = {
                color:'',
                x:'',
                y:''
            };
        }
        var retarray = Object.keys(retbrush).map(function(k){
            return retbrush[k];
        });

        // console.log(Object.keys(this._widget));

        //brush
        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1 && retarray.indexOf(d) == -1){
                var sel = viewer._widget[d].getSelection();

                if(sel.brush){
                    queries.global=queries.global.setConstraint(d,sel.brush);
                }else if(sel.global){
                    queries.global=queries.global.setConstraint(d,sel.global);
                }

                              
            }
        });

        // console.log(retarray);
        var xqueries = {};
        var yqueries = {};
        var cqueries = {};
        
        //then the restTimeseries.prototype={
        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1 && retarray.indexOf(d) != -1){
                var sel = viewer._widget[d].getSelection();
                var colarray = Object.keys(sel).filter(function(d){
                    return (d != 'brush') && (d != 'global');
                });
                if(colarray.length == 0){
                    if(sel.brush){
                        queries.global=queries.global.setConstraint(d,sel.brush);
                    }else if(sel.global){
                        queries.global=queries.global.setConstraint(d,sel.global);
                    }
                }
                colarray.forEach(function(s){
                    //get an appropriate query
                    // var q = queries[s] || $.extend(true,{},queries.global);
                    
                    //add a constraint
                    if(retbrush.x == d)
                        xqueries[s] = [d, sel[s]];
                    else if (retbrush.y == d)
                        yqueries[s] = [d, sel[s]];
                    else //color
                        cqueries[s] = [d, sel[s]];
                });
            }
        });

        // console.log(retbrush);
        // console.log(xqueries, yqueries, cqueries);

        if(!jQuery.isEmptyObject(xqueries)){
            Object.keys(xqueries).forEach(function(s){
                var str1 = '&x' + s;
                var q1 = $.extend(true,{},queries.global);
                q1 = q1.setConstraint(xqueries[s][0], xqueries[s][1]);
                if(!jQuery.isEmptyObject(yqueries)){
                    Object.keys(yqueries).forEach(function(s){
                        var str2 = str1 + '&y' + s;
                        var q2 = $.extend(true,{},q1);
                        q2 = q2.setConstraint(yqueries[s][0], yqueries[s][1]);
                        if(!jQuery.isEmptyObject(cqueries)){
                            Object.keys(cqueries).forEach(function(s){
                                var str3 = str2 + '&c' + s;
                                var q3 = $.extend(true,{},q2);
                                queries[str3] = q3.setConstraint(cqueries[s][0], cqueries[s][1]); 
                            });
                        }
                        else{
                            queries[str2] = q2;
                        }
                    });
                }
                else{
                    if(!jQuery.isEmptyObject(cqueries)){
                        Object.keys(cqueries).forEach(function(s){
                            var str2 = str1 + '&c' + s;
                            var q2 = $.extend(true,{},q1);
                            queries[str2] = q2.setConstraint(cqueries[s][0], cqueries[s][1]); 
                        });
                    }
                    else{
                        queries[str1] = q1;
                    }
                }
            });
        }
        else{
            if(!jQuery.isEmptyObject(yqueries)){
                Object.keys(yqueries).forEach(function(s){
                    var str1 = '&y' + s;
                    var q1 = $.extend(true,{},queries.global);
                    q1 = q1.setConstraint(yqueries[s][0], yqueries[s][1]);
                    if(!jQuery.isEmptyObject(cqueries)){
                        Object.keys(cqueries).forEach(function(s){
                            var str2 = str1 + '&c' + s;
                            var q2 = $.extend(true,{},q1);
                            queries[str2] = q2.setConstraint(cqueries[s][0], cqueries[s][1]); 
                        });
                    }
                    else{
                        queries[str1] = q1;
                    }
                });
            }
            else{
                if(!jQuery.isEmptyObject(cqueries)){
                    Object.keys(cqueries).forEach(function(s){
                        var str1 = '&c' + s;
                        var q1 = $.extend(true,{},queries.global);
                        queries[str1] = q1.setConstraint(cqueries[s][0], cqueries[s][1]); 
                    });
                }
                else{
                    // console.log("Do nothing");
                }
            }
        }
        
        //console.log(queries.global,skip);

        
        if (Object.keys(queries).length > 1){
            delete queries.global;
        }

        return queries;
    },

    getXYData: function(skip){
        skip.push('ret');
        var retbrush;
        if(this._widget.ret)
            retbrush = this._widget.ret.getSelection();
        else{
            retbrush = {
                color:'',
                x:'',
                y:''
            };
        }
        var retarray = Object.keys(retbrush).map(function(k){
            return retbrush[k];
        });

        var x = [];
        var y = [];
        
        //then the restTimeseries.prototype={
        Object.keys(this._widget).forEach(function(d){
            if (skip.indexOf(d) == -1 && retarray.indexOf(d) != -1){
                var sel = viewer._widget[d].getSelection();
                Object.keys(sel).filter(function(d){
                    return (d != 'brush') && (d != 'global');
                }).forEach(function(s){
                    if(retbrush.x == d)
                        x.push(s);
                    else if (retbrush.y == d)
                        y.push(s);
                });
            }
        });
        if(y.length === 0)
            y = ['default'];
        if(x.length === 0)
            x = ['default'];
        return [x,y];
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
