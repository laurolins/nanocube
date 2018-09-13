//JQuery
import jquery from 'jquery';
let $ = window.$ = jquery;

//d3
import * as d3 from 'd3';

//Colorbrewer
import colorbrewer from 'colorbrewer';

import Heatmap from './Heatmap';
import PolygonMap from './PolygonMap';
import GroupedBarChart from './GroupedBarChart';
import Timeseries from './Timeseries';
import Expression from './Expression';

let Viewer = function(opts){
    var container = $(opts.div_id);
    //set title
    if(opts.config.title){
        d3.select('head')
            .append('title')
            .html(opts.config.title);
    }
    
    //overlays
    let mapdiv = $('<div>');
    mapdiv.addClass('map-overlay');
    mapdiv.attr('id', 'map-overlay');
    container.append(mapdiv);

    let catdiv = $('<div>');
    let catbtndiv = $('<div>');
    catdiv.attr('id','cat-overlay');
    catbtndiv.addClass('tab');
    catdiv.append(catbtndiv);
    container.append(catdiv);
    
    let timediv = $('<div>');
    let timebtndiv=$('<div>');
    timediv.attr('id', 'time-overlay');
    timebtndiv.addClass('tab');
    timediv.append(timebtndiv);
    container.append(timediv);
    
    //setup
    var nanocubes = opts.nanocubes;
    var variables = [];
    
    this._container = container;
    this._mapoverlay = mapdiv;
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
            this._mapoverlay.append(newdiv);
            options.levels = levels || 25;
            return new Heatmap(options,function(datasrc,bbox,zoom,maptilesize){
                return viewer.getSpatialData(id,datasrc,bbox,zoom);
            },function(args,constraints,datasrc){
                return viewer.update([id],constraints,
                                     id,args,datasrc);
            });

        case 'choropleth':            
            this._mapoverlay.append(newdiv);
            options.levels = levels || 25;
            
            return new PolygonMap(options, function(datasrc){
                return viewer.getCategoricalData(id,datasrc);
            },function(args,constraints,datasrc){
                return viewer.update([id],constraints,
                                     id,args,datasrc);
            });
            
        case 'cat':
            let cattabname = options.tab;
            let catoverlay = this._catoverlay;
            if (catoverlay.find('#'+ cattabname).length == 0){
                //create the button
                let cattabbtn = $('<button>');
                cattabbtn.addClass('tablinks');
                cattabbtn.html(cattabname);
                cattabbtn.click(function(e){ //open tab when click
                    viewer.toggleTab(cattabname,$(e.target));
                });
                catoverlay.find('.tab').append(cattabbtn);
                
                //create the content div
                var catcontentdiv = $('<div>');
                catcontentdiv.addClass('tabcontent');
                catcontentdiv.attr('id',cattabname);                                
                catoverlay.append(catcontentdiv);

                //open the tab
                if(options.open){
                    this.toggleTab(cattabname,cattabbtn);
                }
            }
            
            catoverlay.find('#'+ cattabname).append(newdiv);
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
            let timeoverlay=this._timeoverlay;
            let timetabname = options.tab;

            if(timeoverlay.find('#'+ timetabname).length==0){
                //create the button
                let timetabbtn = $('<button>');
                timetabbtn.addClass('tablinks');
                timetabbtn.html(timetabname);
                timetabbtn.click(function(e){ //open tab when click
                    viewer.toggleTab(timetabname,$(e.target));
                });
                
                timeoverlay.find('.tab').append(timetabbtn);

                //create the content div
                var timecontentdiv = $('<div>');
                timecontentdiv.addClass('tabcontent');
                timecontentdiv.attr('id',timetabname);
                timeoverlay.append(timecontentdiv);

                //open the tab
                if(options.open){
                    this.toggleTab(timetabname,timetabbtn);
                }
            }

            timeoverlay.find('#'+ timetabname).append(newdiv);
            options.timerange = viewer.getTimeRange();
            options.timelimits = viewer.getTimeRange();
            
            return new Timeseries(options,function(datasrc,start,end,interval){
                return viewer.getTemporalData(id,datasrc,start,end,interval);
            },function(args,constraints){
                return viewer.update([id],constraints,id,args);
            });
        default:
            return null;
        }
    },
    
    toggleTab: function(id, currentbtn){
        var viewer = this;
        if($('#'+id+':hidden').length > 0){
            currentbtn.parent().parent()
                .find('.tabcontent').hide();
            currentbtn.parent()
                .find('button').removeClass('active');
            $('#'+id).show();
            viewer.update(); 
            currentbtn.addClass('active');
        }
        else{
            currentbtn.removeClass('active');
            $('#'+id).hide();
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
                return q.categoricalQuery(varname);
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


export default Viewer;
