/*global define module exports require */

function loadCss(url) {
    var link = document.createElement("link");
    link.type = "text/css";
    link.rel = "stylesheet";
    link.href = url;
    document.getElementsByTagName("head")[0].appendChild(link);
}

(function(root, factory) {    
    if (typeof define === 'function' && define.amd) {
	// AMD. Register as an anonymous module.
	define(['jquery','colorbrewer','d3',
		'jsep','leafletdraw','canvaslayer'], factory);
    } else if (typeof exports === 'object') {
	// Node. Does not work with strict CommonJS, but
	// only CommonJS-like environments that support module.exports,
	// like Node.
	module.exports = factory(require('jquery'),
				 require('colorbrewer'),
				 require('d3'),
				 require('jsep'),
				 require('leaflet'),
				 require('leafletdraw'),
				 require('canvaslayer'));
    } else {
	// Browser globals (root is window)
	root.Nanocube3 = factory(root.$,root.colorbrewer,root.d3,
				 root.jsep,root.L);
    }
} (this, function($,colorbrewer,d3,jsep,L) {
    loadCss('node_modules/leaflet/dist/leaflet.css');
    loadCss('node_modules/leaflet-draw/dist/leaflet.draw.css');

    var Nanocube3 = {};

/*global $ jsep  */

var Expression = function(expr){
    this.parsetree = jsep(expr);
};

Expression.prototype = {
    getData: function(q,qfunc){
        return this._process(this.parsetree,q,qfunc);
    },

    _process: function(expr,q,qfunc){
        var p;
        switch(expr.type) {
        case 'CallExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'BinaryExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'LogicalExpression':
            p =  this._binExp(expr,q,qfunc);
            break;
        case 'MemberExpression':
            p = this._memExp(expr,q,qfunc);
            break;
        case 'Literal':
            var dfd = new $.Deferred();
            p = dfd.promise();
            dfd.resolve(expr.value);
            break;
        case 'Identifier':
            p = qfunc(q[expr.name]);
            break;
        default:
            throw "Cannot parse expression";
        }
        return p;
    },

    _memExp: function(memexp, q, qfunc){

        //function for recursive processing
        function memExpQuery(memexp, q){
            //process the type
            var newq = null ;
            if (memexp.object.type == 'MemberExpression'){
                newq = memExpQuery(memexp.object, q);
            }
            else if (memexp.object.type == 'Identifier'){
                //select the base query
                newq = $.extend(true,{}, q[memexp.object.name]);
            }

            //process the properties
            var prop = memexp.property;
            if (prop.type=='BinaryExpression'&& prop.operator==  '==' ){
                var catvar = prop.left.name;
                var catval;

                if(prop.right.type == 'Identifier'){
                    catval = [prop.right.name];
                }

                if(prop.right.type == 'Literal'){
                    catval = [prop.right.value];
                }

                if(prop.right.type == 'ArrayExpression'){
                    catval = prop.right.elements.map(function(d){
                        if (d.name){
                            return d.name;
                        }
                        else{
                            return d.value;
                        }
                    });
                }
                
                catval = catval.map(function(d){ return {cat: d , id: null };});
                
                newq.setCatConst(catvar,catval);
            }
            return newq;
        }

        //process the query
        var resq = memExpQuery(memexp,q);

        //exec the spatial query
        return qfunc(resq);
    },

    _binExp: function(binexp, q, qfunc){
        var dfd = new $.Deferred();

        //process left and right
        var left = this._process(binexp.left,q,qfunc);
        var right = this._process(binexp.right,q,qfunc);

        var expr = this;
        $.when(left,right).done(function(){
            var results = arguments;
            var resleft = results[0];
            var resright = results[1];

            function getOpFunc(operator){
                switch (operator){
                case '+':
                    return function(a,b) {return a+b;};
                case '-':
                    return function(a,b) {return a-b;};
                case '*':
                    return function(a,b) {return a*b;};
                case '/':
                    return function(a,b) {
                        if(isNaN(a/b)){
                            return 0;
                        }
                        else{
                            return a/b;
                        }
                    };
                case '||':
                    return function(a,b) { return Math.max(a,b); };
                case '&&':
                    return function(a,b) { return Math.min(a,b); };

                default:
                    throw "Unsupported Operation";
                }
            }

            var opfunc = getOpFunc(binexp.operator);
            if (!opfunc){
                dfd.resolve(null);
            }

            var res = null;
            if (opfunc){
                res = expr._op(opfunc,resleft,resright);
            }
            dfd.resolve(res);
        });
        return dfd.promise();
    },

    _callExp: function(callexp, q, qfunc){
        var dfd = new $.Deferred();

        //process the arguments
        var args = callexp.arguments.forEach(function(d){
            return this._process(d,q,qfunc);
        });

        var expr = this;
        $.when.apply($,args).done(function(){
            var results = arguments;

            function getOpFunc(operator){
                switch (operator){
                case '+':
                    return function(a,b) {return a+b;};
                case '-':
                    return function(a,b) {return a-b;};
                case '*':
                    return function(a,b) {return a*b;};
                case '/':
                    return function(a,b) {return (a+1e-4)/(b+1e-4);};
                default:
                    throw "Unsupported Operation";
                }
            }

            var opfunc = getOpFunc(binexp.operator);
            if (!opfunc){
                dfd.resolve(null);
            }

            var res = null;
            if (opfunc){
                res = expr._op(opfunc,resleft,resright);
            }
            dfd.resolve(res);
        });
        return dfd.promise();
    },

    _opTemporal: function(opfunc,left,right){
        var lefthash = {};
        if (typeof left === 'number'){
            right.data.forEach(function(d,i){
                lefthash[d.time] = left;
            });
        }
        else{
            left.data.forEach(function(d,i){
                lefthash[d.time] = d.val;
            });
        }
       var righthash = {};
        if (typeof right == 'number'){
            left.data.forEach(function(d,i){
                righthash[d.time] = right;
            });
        }
        else{
            right.data.forEach(function(d,i){
                righthash[d.time] = d.val;
            });
        }


        var allkeys = {};
        Object.keys(righthash).forEach(function(d){ allkeys[d]=1; });
        Object.keys(lefthash).forEach(function(d){ allkeys[d]=1; });


        var res = {};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val =  opfunc(l,r);

            return {time: new Date(k),val: val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        //res.data = res.data.sort(function(a,b){return a.time - b.time;});

        return res;
    },

    _opCategorical: function(opfunc,left,right){
        if (typeof left === 'number'){
            var leftval = left;
            left = $.extend(true, {}, right);
            left.data = left.data.map(function(d) {
                d.val = leftval;
                return d;
            });
        }
        
        if (typeof right == 'number'){
            var rightval = right;
            right = $.extend(true, {}, left);
            right.data = right.data.map(function(d) {
                d.val = rightval;
                return d;
            });

        }
        var lefthash = {};
        left.data.forEach(function(d) {
            lefthash[d.id]=d.val;
        });
        var righthash = {};
        right.data.forEach(function(d) {
            righthash[d.id]=d.val;
        });
        
        var allkeys = {};
        left.data.forEach(function(d){
            allkeys[d.id] = d.cat;
        });

        right.data.forEach(function(d){
            allkeys[d.id] = d.cat;
        });

        var res = {};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val = opfunc(l,r);

            return {id:k, cat:allkeys[k],val:val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        return res;
    },

    _opSpatial: function(opfunc,left,right){
        var lefthash = {};
        if (typeof left === 'number'){
            right.data.forEach(function(d,i){
                lefthash[[d.x,d.y]] = left;
            });
        }
        else{
            left.data.forEach(function(d,i){
                lefthash[[d.x,d.y]] = d.val;
            });
        }

        var righthash = {};
        if (typeof right == 'number'){
            left.data.forEach(function(d,i){
                righthash[[d.x,d.y]] = right;
            });
        }
        else{
            right.data.forEach(function(d,i){
                righthash[[d.x,d.y]] = d.val;
            });
        }


        var allkeys = {};
        Object.keys(righthash).forEach(function(d){ allkeys[d]=1; });
        Object.keys(lefthash).forEach(function(d){ allkeys[d]=1; });



        var res = {opts: left.opts || right.opts};
        res.data = Object.keys(allkeys).map(function(k){
            var l = lefthash[k] || 0 ;
            var r = righthash[k] || 0;
            var val =  opfunc(l,r);

            var coord = k.split(',');
            return {x: +coord[0],y: +coord[1],val: val};
        });
        res.data = res.data.filter(function(d){return isFinite(d.val);});
        res.data = res.data.filter(function(d){return d.val !== 0;});
        res.type = left.type || right.type;
        return res;
    },

    _op: function(opfunc,left,right){
        var type = left.type || right.type;

        switch(type){
        case 'spatial':
            return this._opSpatial(opfunc,left,right);
        case 'temporal':
            return this._opTemporal(opfunc,left,right);
        case 'cat':
            return this._opCategorical(opfunc,left,right);

        default:
            return null;
        }
    }
};

/*global d3 $ */

function GroupedBarChart(opts, getDataCallback, updateCallback){
    this.getDataCallback=getDataCallback;
    this.updateCallback=updateCallback;

    var name=opts.name;
    var id = "#"+name.replace(/\./g,'\\.');
    var margin = {top: 20, right: 20, bottom: 30, left: 40};

    //set param
    this.selection = {global:[]};
    if(opts.args){ // set selection from arguments
        this._decodeArgs(opts.args);
    }
    
    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","barchart resize-drag");
    
    d3.select(id).on("divresize",function(){
        widget.update();
    });

    //Add clear button
    this.clearbtn = d3.select(id)
        .append('button')
        .attr('class','clear-btn')
        .on('click',function(){
            d3.event.stopPropagation();
            
            delete widget.selection.brush; //clear selection
            widget.update(); //redraw itself
            widget.updateCallback(widget._encodeArgs());            
        }).html('clear');
    
    //Add sort button
    this.sortbtn = d3.select(id)
        .append('button')
        .attr('class','sort-btn')
        .on('click',function(){
            d3.event.stopPropagation();
            widget._opts.alpha_order = !widget._opts.alpha_order;
            widget.redraw(widget.lastres);
        });
    
    //Collapse on dbl click
    d3.select(id).on('dblclick',function(d){
        var currentheight = d3.select(id).style("height");
        if ( currentheight != "40px"){
            widget.restoreHeight =currentheight ;
            d3.select(id).style('height','40px');
        }
        else{
            d3.select(id).style("height",widget.restoreHeight);
        }
    });

    
    //SVG container
    var svg = d3.select(id).append("svg").append("g");

    //Title
    svg.append('text').attr('y',-5);
    
    //Axes
    svg.append("g").attr("class", "y axis")
        .attr("transform", "translate(-3,0)");
    svg.append("g").attr("class", "x axis");
    
    //Scales
    var y0 = d3.scale.ordinal();
    var y1 = d3.scale.ordinal();
    var x = d3.scale.linear();
    if (opts.logaxis){
        x = d3.scale.log();
    }

    //Axis
    var xAxis = d3.svg.axis();
    var yAxis = d3.svg.axis();

    //set default values 
    opts.numformat = opts.numformat || ",";    
    if(!opts.hasOwnProperty('alpha_order')) {
        opts.alpha_order = true;
    }

    xAxis.orient("bottom")
        .ticks(3,opts.numformat);
    yAxis.orient("left");

    //Save vars to "this"
    this.margin = margin;
    this.svg=svg;
    this.y0=y0;
    this.y1=y1;
    this.x=x;
    this.xAxis = xAxis;
    this.yAxis = yAxis;
    
    this._datasrc = opts.datasrc;
    this._opts = opts;
    this._logaxis = opts.logaxis;
    this._name = name;
}

GroupedBarChart.prototype = {
    getSelection: function(){        
        return this.selection;
    },
    
    _encodeArgs: function(){
        return JSON.stringify(this.getSelection());
    },
    
    _decodeArgs: function(s){
        this.selection = JSON.parse(s);
    },
    
    update: function(){        
        var widget = this;        
        var promises = {};
        
        //generate promise for each expr
        for (var d in widget._datasrc){
            if (widget._datasrc[d].disabled){
                continue;
            }
            var p = this.getDataCallback(d);
            for (var k in p){
                promises[k] = p[k];
            }
        }

        var promarray = Object.keys(promises).map(function(k){
            return promises[k];
        });
        
        var promkeys = Object.keys(promises);
        $.when.apply($,promarray).done(function(){
            var results = arguments;
            var res = {};
            promkeys.forEach(function(d,i){
                res[d] = results[i];
            });
            
            widget.lastres = res;
            widget.redraw(res);
        });
    },
    
    flattenData: function(res){
        var widget = this;        
        return Object.keys(res).reduce(function(prev,curr){         
            var label = curr.split('&-&'); 
            var c = label[0];

            var isColor  = /^#[0-9A-F]{6}$/i.test(label[0]);                
            if(!isColor){
                var colormap = widget._datasrc[label[1]].colormap;
                var cidx = Math.floor(colormap.length/2);
                c = colormap[cidx];
            }
            
            //Add color
            var row = res[curr].data.map(function(d){
                d.color = c;
                return d;
            });
            return prev.concat(row);
        }, []);
    },

    redraw :function(res){
        var topn = this._opts.topn;
        if(topn !== undefined ){
            var agg = {};
            Object.keys(res).forEach(function(k){
                res[k].data.forEach(function(d){
                    agg[d.cat]= (agg[d.cat] + d.val) || d.val;
                });
            });
            var kvlist =Object.keys(agg)
                .map(function(d){return {cat: d, val:agg[d]};});
            kvlist.sort(function(x,y) { return y.val - x.val; });
            kvlist = kvlist.slice(0,topn);
            var kvhash = {};
            kvlist.forEach(function(d){ kvhash[d.cat] = d.val; });
            Object.keys(res).forEach(function(k){
                res[k].data = res[k].data.filter(function(d){
                    return (d.cat in kvhash);
                });
            });
            console.log(res);
        }

        var fdata = this.flattenData(res);
        
        var x =this.x;
        var y0 =this.y0;
        var y1 =this.y1;
        var svg =this.svg;
        var selection = this.selection;
        
        
        //update the axis and svgframe
        this.updateYAxis(fdata);
        this.updateXAxis(fdata);
        this.updateSVG();

        var widget = this;
        
        //bind data
        var bars = this.svg.selectAll('.bar').data(fdata);
        
        //append new bars
        bars.enter()
            .append('rect')
            .attr('class', 'bar')
            .on('click', function(d) { widget.clickFunc(d);})//toggle callback
            .append("svg:title"); //tooltip

        //set shape
        bars.attr('x', 0)
            .attr('y', function(d){return widget.y0(d.cat) + //category
                                   widget.y1(d.color);}) //selection group
            .style('fill', function(d){
                if (!widget.selection.brush || //no selection
                    widget.selection.brush.findIndex(function(b){
                        return (b.cat == d.cat); }) != -1){//in selection
                    return d.color;
                }
                else{
                    return 'gray';
                }
            })
            .attr('height',function(d){
                return widget.y1.rangeBand()-1;
            })
            .transition().duration(250)
            .attr('width',function(d){
                var w = widget.x(d.val);
                if(isNaN(w) && d.val <=0 ){
                    w = 0;
                }
                return w;
            });
        
        //add tool tip
        bars.select('title').text(function(d){
            return d3.format(widget._opts.numformat)(d.val);
        });

        //remove bars with no data
        bars.exit().remove();
    },

    clickFunc:function(d){
        var widget = this;
        if(!widget.selection.brush){
            widget.selection.brush = [];
        }
            
        var idx = widget.selection.brush.findIndex(function(b){
            return (b.cat == d.cat);
        });
        
        if (idx != -1){
            widget.selection.brush.splice(idx,1);
        }
        else{
            if(d3.event.shiftKey){
                widget.selection.brush.push({id:d.id, cat:d.cat});
            }
            else{
                widget.selection.brush = [{id:d.id, cat:d.cat}];
            }                        
        }
        
        if(widget.selection.brush.length < 1){
            delete widget.selection.brush;
        }            
            
        widget.update(); //redraw itself
        widget.updateCallback(widget._encodeArgs());            
    },

    updateSVG : function(){
        var svg = this.svg;
        var margin = this.margin;

        var svgframe = d3.select(svg.node().parentNode);
        var width=d3.select(svgframe.node().parentNode).style('width');       
        width = parseFloat(width);
        
        //calculate width and height in side the margin
        width = width-margin.left-margin.right;
        var height = this.totalheight;

        //resize the frame
        svgframe.attr("width", width + margin.left + margin.right);
        svgframe.attr("height", height + margin.top + margin.bottom);

        svg.attr("transform", "translate("+margin.left+","+margin.top+")");

        this.width = width;
        this.height = height;
    },

    updateXAxis: function(data){
        var margin = this.margin;
        var x=this.x;
        var xAxis=this.xAxis;
        var svg=this.svg;

        var svgframe = d3.select(svg.node().parentNode);
        var rect=svgframe.node().parentNode.getBoundingClientRect();
        var width = rect.width - this.margin.left-this.margin.right;

        var d = [d3.min(data, function(d) {return +d.val;}),
                 d3.max(data, function(d) {return +d.val;})];

        if(this._opts.logaxis){ // prevent zeros for log
            d[0] = Math.max(d[0]-1e-6,1e-6);
        }
        else{
            d[0] = Math.min(d[0],d[1]-Math.abs(d[1]/2));
            d[0] = d[0]-0.1*Math.abs(d[0]);
        }
        
        //set domain from options
        if(this._opts.domain){
            if(this._opts.domain.min !== undefined){
                d[0] = this._opts.domain.min;
            }
                
            if(this._opts.domain.max !== undefined){
                d[1] = this._opts.domain.max;
            }
        }
        
        //set domain
        x.domain(d);        
        x.range([0,width]);
        
        xAxis.scale(x);

        //move and draw the axis
        svg.select('.x.axis')
            .attr("transform", "translate(0,"+this.totalheight+")")
            .call(xAxis);
        this.width=width;
    },

    updateYAxis:function(data){
        var y0=this.y0;
        var y1=this.y1;
        var yAxis=this.yAxis;
        var svg = this.svg;
        var opts = this._opts;
        var sortbtn = this.sortbtn;
        
        
        //Sort y axis
        if (opts.alpha_order){            
            y0.domain(data.map(function(d){return d.cat;}).sort());
            if (y0.domain().every(function(d) {return !isNaN(d);})){
                y0.domain(y0.domain().sort(function(a,b){return a-b;}));
            }
            sortbtn.html('#');
        }
        else{ //sort by data value
            var d = data.sort(function(x,y){ return y.val - x.val;});
            y0.domain(d.map(function(d){return d.cat;}));
            sortbtn.html('A');
        }
        
        
        y1.domain(data.map(function(d){return d.color;}));
        var totalheight = y0.domain().length* y1.domain().length * 18;

        y0.rangeRoundBands([0, totalheight]);
        y1.rangeRoundBands([0, y0.rangeBand()]);
        yAxis.scale(y0);
        svg.select('.y.axis').call(yAxis);

        //enable axis click
        var widget = this;
        svg.select('.y.axis').selectAll('.tick')
            .on('click',function(d){
                var obj = data.filter(function(e){return e.cat==d;})[0];
                widget.clickFunc(obj);
            });
        
        this.totalheight = totalheight;
        this.margin.left = svg.select('.y.axis').node().getBBox().width+3;

        //update title with cat count
        svg.select('text').text(this._opts.title+' ('+y0.domain().length+')');
    }    
};

/*global $ L colorbrewer d3 window */

var Map=function(opts,getDataCallback,updateCallback){
    this.getDataCallback = getDataCallback;
    this.updateCallback = updateCallback;

    this._datasrc = opts.datasrc;
    this._coarse_offset = opts.coarse_offset || 0;
    this._name = opts.name || 'defaultmap';
    this._tilesurl = opts.tilesurl ||
        'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';

    this._layers = this._genLayers(this._datasrc);
    this._maxlevels = opts.levels || 25;
    this._logheatmap = true;
    this._opts = opts;
    
    
    var map = this._initMap();

    this._map = map;

    //add Legend
    if (opts.legend){
        this._addLegend(map);
    }

    
    //set according to url
    if(opts.args){
        this._decodeArgs(opts.args);
    }
    else{
        if ('viewbox' in opts){
            this.setSelection('global',opts.viewbox);
        }
        else if ('view' in opts){
            this.setSelection('global', opts.view);
        }
        else{
            this.setSelection('global', {c:{lat:0,lng:0},z:0});
        }
    }

    if('layers' in opts){
        if ('markers' in opts.layers){
            opts.layers.markers.forEach(function(d){
                var m = L.marker(d.coordinate);
                m.bindPopup(d.popup);
                m.addTo(map);
            });
        }                        
    }
};

//Setup static variables and functions
Map.brushcolors = colorbrewer.Paired[12].slice(0);
Map.nextcolor = function(){
    var c = Map.brushcolors.shift();
    Map.brushcolors.push(c);
    return c;
};

Map.prototype = {
    _genLayers: function(data){
        var widget = this;
        var layers = {};
        function drawfunc(layer,options){
                widget._canvasDraw(layer,options);
        }

        function colorfunc(d,i,array){
            var m = d.match(/rgba\((.+),(.+),(.+),(.+)\)/);
            if(m){
                    d={r:+m[1],g:+m[2],b:+m[3],a:+m[4]*255};
                return d;
            }
            else{
                d = d3.rgb(d);
                return {r:d.r, g:d.g, b:d.b, a:i/array.length*255};
            }
        }
        
        for (var d in data){
            var layer = L.canvasOverlay(drawfunc,{opacity:0.7});

            //set datasrc
            layer._datasrc = d;

            //set color
            var midx = Math.floor(widget._datasrc[d].colormap.length /2);
            layer._color = widget._datasrc[d].colormap[midx];
            
            //set colormap
            layer._colormap = widget._datasrc[d].colormap.map(colorfunc);

            //htmllabel
            var htmllabel='<i style="background:'+
                    layer._color+'"> </i>' + d;
            
            layers[htmllabel] = layer;
        }
        return layers;
    },
    
    _initMap: function(viewbbox){
        var widget = this;
        
        //Leaflet stuffs
        var map = L.map(this._name,{detectRetina:true,
                                    attribution: '<a href="https://www.mapbox.com/about/maps/">Terms and Feedback</a>'});

        //make the background black
        $('.leaflet-container').css('background','#000');

        //add an OpenStreetMap tile layer
        var mapt = L.tileLayer(this._tilesurl,{
            noWrap:true,
            opacity:0.4,
            maxZoom: Math.min(this._maxlevels-8, 18)
        });

        //add base layer
        map.addLayer(mapt);

        //add nanocube layers
        for (var l in this._layers){
            map.addLayer(this._layers[l]);
        }

        //Layer
        if (Object.keys(this._layers).length > 1){
            L.control.layers(null,this._layers,
                             {
                                 collapsed: false,
                                 position: 'bottomright'
                             })
                .addTo(map);
        }

        map.on('overlayadd', function (e) {
            widget._datasrc[e.layer._datasrc].disabled=false;
            widget.updateCallback(widget._encodeArgs(),[],
                                  widget._datasrc);
        });

        map.on('overlayremove', function (e) {
            widget._datasrc[e.layer._datasrc].disabled=true;
            widget.updateCallback(widget._encodeArgs(),[],
                                  widget._datasrc);
            
        });

        //Refresh after move
        map.on('moveend', function(){ //update other views
            widget.updateCallback(widget._encodeArgs(),[]);
        });

        //add keyboard hooks with JQuery
        $(map._container).keydown(function(e){
            widget._keyboardShortcuts(e);
        });

        this._maptiles = mapt;
        this._initDrawingControls(map);
        this._renormalize=true;

        //add info
        //$('#'+this._name).append('<p class="info">info test</p>');
        //style
        /*var infodiv = $('#'+this._name+" .info");
        infodiv.css({
            position: 'absolute',
            'z-index':1,
            color: 'white',
            'right': '20ch',
            'top': '0.5em',
            'padding':'0px',
            'margin':'0px'
        });*/

        //add title
        d3.select('#'+this._name)
            .append('div')
            .attr('class','maptitle')
            .text(this._name);
        
        return map;
    },

    _encodeArgs: function(){
        var map = this._map;
        var args= {};
        args.global = {c:map.getCenter(),z:map.getZoom()};

        return JSON.stringify(args);
    },

    _decodeArgs: function(s){
        var map = this._map;
        var args = JSON.parse(s);
        var v = args.global;
        
        map.setView(v.c,v.z);
    },
       
    _keyboardShortcuts: function(e){
        console.log(e);
        switch(e.keyCode){
        case 190: //.
            this.changeHeatmapRes(1);
            break;
        case 188: //,
            this.changeHeatmapRes(-1);
            break;
        case 66: //b
            this.changeMapOpacity(0.1);
            break;
        case 68: //d
            this.changeMapOpacity(-0.1);
            break;
        case 76: //l
            this._logheatmap = !this._logheatmap;
            this._renormalize = true;
            this.update();
            break;
        case 78: //n
            this._renormalize = true;
            this.update();
            break;
        default:
            return;
        }
    },

    _initDrawingControls: function(map){
        var drawnItems = new L.FeatureGroup();
        map.addLayer(drawnItems);
        var widget = this;
        
        var drawingoptions = function(){
            return { shapeOptions:{ color: Map.nextcolor() } };
        };

        map.drawControl = new L.Control.Draw({
            draw: {
                rectangle: drawingoptions(),
                polygon: drawingoptions(),
                polyline:false,
                circle:false,
                marker:false
            },
            edit: {
                featureGroup: drawnItems,
                edit:{
                    selectedPathOptions: {maintainColor: true}
                }
            }
        });

        map.addControl(map.drawControl);

        map.on('draw:created', function (e) {
            drawnItems.addLayer(e.layer);

            //add constraints to the other maps
            widget.updateCallback(widget._encodeArgs(),
                                  [{
                                      type:"SPATIAL",
                                      key:e.layer.options.color
                                  }]);
            
            //Set color for the next shape
            var options = {};
            options[e.layerType] = drawingoptions();
            map.drawControl.setDrawingOptions(options);
        });

        map.on('draw:edited', function (e) {
            widget.updateCallback(widget._encodeArgs()) ;          
        });

        map.on('draw:editing', function (e) {
            widget.updateCallback(widget._encodeArgs()) ;          
        });

        map.on('draw:deleted', function(e) {
            //add constraints to the other maps
            widget.updateCallback(widget._encodeArgs());
        });


        this._drawnItems = drawnItems;
    },

    addConstraint:function(constraint){
        if(constraint.type != "SPATIAL"){
            return;
        }

        var map = this._map;

        var key = constraint.key;
        var event = constraint.event || null;

        var shape;
        if(!event){  // create a rect
            var s = map.getSize();
            var nw = map.containerPointToLatLng([s.x*0.25,s.y*0.25]); 
            var se = map.containerPointToLatLng([s.x*0.75,s.y*0.75]); 
            shape = L.rectangle([nw,se],{color:key});
        }

        this._drawnItems.addLayer(shape);
    },

    setSelection: function(key,v){
        var map =this._map;

        if (key == 'global'){
            if('c' in v && 'z' in v){
                //set the view
                map.setView(v.c,v.z);
            }
            else if (v.length==2){  //viewbox
                map.fitBounds(v);
            }
        }
    },

    getSelection: function(){
        var res = {};
        var map = this._map;

        var bb = map.getBounds();
        var sw = bb.getSouthWest();
        var ne = bb.getNorthEast();


        res.global = {};
        res.global.coord = [[sw.lat,sw.lng],
                            [sw.lat,ne.lng],
                            [ne.lat,ne.lng],
                            [ne.lat,sw.lng]];

        res.global.zoom = map.getZoom() + 8;

        //add polygonal constraints  
        this._drawnItems.getLayers().forEach(function(d){
            res[d.options.color]={};
            res[d.options.color] = {
                coord: d._latlngs.map(function(d){
                    return [d.lat,d.lng];
                }),
                zoom: map.getZoom() + 8
            };                                     
        });
        return res;
    },

    update: function(){
        //force redraw
        this._map.fire('resize');        
        
        for(var l in this._layers){
            var layer = this._layers[l];
            if (!this._datasrc[layer._datasrc].disabled){
                layer._reset();
            }
        }
    },

    drawCanvasLayer: function(res,canvas,cmap,opacity){
        var arr = this.dataToArray(res.opts.pb,res.data);
        this.render(arr,res.opts.pb,cmap,canvas,opacity);
    },

    dataToArray: function(pb,data){
        var origin = pb.min;
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //var arr = new Array(width*height).map(function () { return 0;});
        var arr = [];
        //Explicit Loop for better performance
        var idx = Object.keys(data);
        for (var i = 0, len = idx.length; i < len; i++) {
            var ii= idx[i];
            var d = data[ii];
            var _i = d.x - origin.x;
            var _j = d.y - origin.y;
            if(_i <0 || _j <0 || _i >=width || _j>=height){
                continue;
            }
            var _idx =  _j*width+_i;
            arr[_idx] = d.val;
        }
        return arr;
    },

    normalizeColorMap: function(data,colors,log){
        var ext = d3.extent(data,function(d){
            return d.val;
        });

        var minv = ext[0];
        if (log){ //log
            ext = ext.map(function(d){return Math.log(d-minv+2);});
        }

        //compute domain
        var interval = (ext[1]-ext[0])/(colors.length-1);
        var domain=Array.apply(null,Array(colors.length)).map(function(d,i){
            return i*interval+ext[0];
        });

        if (log){ //anti log
            domain = domain.map(function(d){return Math.exp(d)+minv-2;});
        }

        return d3.scale.linear().domain(domain).range(colors);
    },

    render: function(arr,pb,colormap,canvas,opacity){
        var realctx = canvas.getContext("2d");        
        var width = pb.max.x-pb.min.x+1;
        var height = pb.max.y-pb.min.y+1;

        //create a proxy canvas
        var c = $('<canvas>').attr("width", width).attr("height", height)[0];
        var proxyctx = c.getContext("2d");
        var imgData = proxyctx.createImageData(width,height);

        var buf = new ArrayBuffer(imgData.data.length);
        var buf8 = new Uint8ClampedArray(buf);
        var pixels = new Uint32Array(buf);

        //Explicit Loop for better performance
        var idx = Object.keys(arr);
        var dom = d3.extent(colormap.domain());

        for (var i = 0, len = idx.length; i < len; i++) {
            var ii= idx[i];
            var v = arr[ii];
            v = Math.max(v,dom[0]);
            v = Math.min(v,dom[1]);            
            var color = colormap(v);
            color.a *= opacity;
            pixels[ii] =
                (color.a << 24) |         // alpha
                (color.b << 16) |         // blue
                (color.g <<  8) |         // green
                color.r;                  // red
        }

        imgData.data.set(buf8);
        proxyctx.putImageData(imgData, 0, 0);

        //Clear
        realctx.imageSmoothingEnabled = false;
        realctx.mozImageSmoothingEnabled = false;
        realctx.clearRect(0,0,canvas.width,canvas.height);

        //draw onto the real canvas ...
        realctx.drawImage(c,0,0,canvas.width,canvas.height);
    },

    _canvasDraw: function(layer,options){
        var canvas = options.canvas;
        var ctx = canvas.getContext('2d');
        ctx.clearRect(0,0,canvas.width,canvas.height);

        var map = this._map;

        var z = map.getZoom();
        z = Math.min(z, this._maxlevels-8);
        z -= this._coarse_offset;

        var startdata = window.performance.now();
        var widget = this;

        var bb = map.getBounds();
        var nw = bb.getNorthWest();
        var se = bb.getSouthEast();

        var bbox = { min:[nw.lat,nw.lng], max:[se.lat,se.lng] } ;

        try{
            var promises = widget.getDataCallback(layer._datasrc,bbox,z);
            var promarray = Object.keys(promises).map(function(k){
                return promises[k];
            });
            var promkeys = Object.keys(promises);
            $.when.apply($,promarray).done(function(){
                var results = arguments;
                promkeys.forEach(function(d,i){
                    console.log('tiletime:',window.performance.now()-startdata);
                    
                    var res = results[i];
                    widget._renormalize= true;

                    if(widget._renormalize){
                        var cmap = widget.normalizeColorMap(res.data,
                                                            layer._colormap,
                                                            widget._logheatmap);
                        layer._cmap = cmap;
                        widget._renormalize = false;


                        if(widget._opts.legend){
                            //update the legend
                            var ext = d3.extent(res.data,function(d){
                                return d.val;
                            });
                            
                            if (widget._logheatmap){ //log
                                ext = ext.map(function(d){ return Math.log(d); });
                            }
                            var valcolor = Array.apply(null, Array(5)).map(function (_, i) {return ext[0]+i * (ext[1]-ext[0])/5;});
                            
                            if (widget._logheatmap){ //anti log
                                valcolor = valcolor.map(function(d){ return Math.floor(Math.exp(d)+0.5); });
                            }
                            
                            valcolor = valcolor.map(function(d) {return {val:d, color: JSON.parse(JSON.stringify(cmap(d)))};});
                            widget.updateLegend(widget._map,valcolor);
                            console.log(widget._map);
                        }
                    }
                    
                    var startrender = window.performance.now();
                    widget.drawCanvasLayer(res,canvas,layer._cmap,
                                           layer.options.opacity);

                    console.log('rendertime:',
                                window.performance.now()-startrender);

                    //res.total_count =  res.data.reduce(function(p,c){
                    //    return p+c.val;
                    //},0);
                    //widget.updateInfo('Total: '+
                    //                  d3.format(',')(res.total_count));
                    
                });
            });
        }
        catch(err){
            console.log(err);
        }
    },
    changeHeatmapRes: function(levels){
        var offset = this._coarse_offset+levels;
        offset = Math.max(0,offset);
        offset = Math.min(8,offset);
        this._coarse_offset = offset;
        this.update(); //force redraw
    },
    changeMapOpacity: function(o){
        var op = this._maptiles.options.opacity+o;
        op = Math.max(0.0,op);
        op = Math.min(1.0,op);
        this._maptiles.setOpacity(op);
    },

    updateInfo: function(html_str){
        $('#'+this._name+" .info").html(html_str);
    },

    _addLegend: function(map){
        var legend = L.control({position: 'bottomleft'});
        
        legend.onAdd = function (map) {
            var div = L.DomUtil.create('div', 'legendinfo legend');
            return div;
        };          

        legend.addTo(map);
    },
    updateLegend: function(map,valcolor){
        var legend = d3.select(map._container).select('.legend');
        var htmlstr= valcolor.map(function(d) {
            var colorstr = 'rgb('+parseInt(d.color.r) +','+parseInt(d.color.g)+','+parseInt(d.color.b)+')';
            return '<i style="background:'+colorstr+'"></i>' + d.val;
        });
        legend.html(htmlstr.join('<br />'));
    }
};

/*global $ */


var cache = {};

//Query
var Query = function(nc){
    this.nanocube = nc;
    this.dimension = null ;
    this.drilldown_flag = false;
    this.query_elements = {};

    //constrains
    this.catconst = {};
    this.idconst = {};
    this.spatialconst = {};
    this.temporalconst = {};
};

Query.prototype = {
    //Functions for setting Constraints
    setConstraint: function(varname,c){
        if(!(varname in this.nanocube.dimensions)){
            return this;
        }

        switch(this.nanocube.dimensions[varname].vartype){
        case 'quadtree':
            return this.setSpatialConst(varname, c);
        case 'cat':
            return this.setCatConst(varname, c);
        case 'time':
            return this.setTimeConst(varname, c);
        case 'id':
            return this.setIdConst(varname, c);
        default:
            return this;
        }
    },

    setSpatialConst: function(varname, sel) {
        var tiles = sel.coord.map(function(c){

        });

        var coordstr = sel.coord.map(function(c){
            c[0] = Math.max(-85,c[0]);
            c[0] = Math.min(85,c[0]);
            c[1] = Math.max(-180,c[1]);
            c[1] = Math.min(180,c[1]);
            return c[1].toFixed(4) +","+ c[0].toFixed(4);
        });
        coordstr = coordstr.join(',');

        var zoom = sel.zoom;
        var constraint = 'r(\"' + varname + '\",degrees_mask(\"' +
                coordstr + '\",' + zoom + '))';

        this.query_elements[varname] = constraint;

        //record constraint
        var constlist = this.spatialconst[varname]  || [];
        constlist.push(tiles);
        this.spatialconst[varname]=constlist;
        return this;
    },

    setTimeConst: function(varname, timeconst) {
        var start = this.nanocube.timeToBin(timeconst.start);
        var end = this.nanocube.timeToBin(timeconst.end);

        
        start = Math.floor(start);
        end = Math.ceil(end);
        if(end < 0){
            end=1;
            start=2;
        }

        start = Math.max(start,0);
        var constraint = 'r(\"' + varname + '\",interval(' +
                start + ',' + end + '))';
        this.query_elements[varname] = constraint;

        //record timeconst
        this.temporalconst[varname]={start:start, end:end, binsize: 1};
        return this;
    },

    setCatConst: function(varname, catvalues) {
        var q = this;
        var valnames = q.nanocube.dimensions[varname].valnames;
        
        var values = catvalues.map(function(d){
            return {cat: d.cat, id: valnames[d.cat] };
        });   
                                   
        if (values.length > 0){
            var constraint = 'r("'+varname+'",'+'set('+values.map(function(d){
                return d.id;
            }).join(',') +'))';

            this.query_elements[varname] = constraint;
        }

        //record catconst
        this.catconst[varname]= catvalues;
        return this;
    },


    setIdConst: function(varname, idvalues) {
        //console.log(idvalues);
        var values = idvalues.map(function(d){ return d.id; });
                                   
        if (values.length > 0){
            var constraint = 'ids('+values.join(',') +')';
            this.query_elements[varname] = constraint;
        }

        //record catconst
        this.idconst[varname]= idvalues;
        
        return this;
    },
        
    queryTime: function(varname, base, bucketsize, count) {
        var constraint = 'r(\"' + varname + '\",mt_interval_sequence(' +
                base + ',' + bucketsize + ',' + count + '))';
        this.timebucketsize = bucketsize;
        this.query_elements[varname] = constraint;

        //record timeconst
        this.timeconst={start:base, end:base+bucketsize*count-1,
                        bucketsize:bucketsize};

        var dfd = new $.Deferred();
        
        if((base+count) < 0){
            dfd.resolve({timeconst: this.timeconst,
                         timearray: []});
            return dfd.promise();
        }
        base = Math.max(0,base);

        
        this._run_query(this).done(function(data){
            var q = this;
            if (!('children' in data.root)){
                dfd.resolve({timeconst:q.timeconst, timearray:[]});
                return;
            }

            data = data.root.children;
            var timearray = data.map(function(d){
                var t = d.path[0];
                var v = d.val; //old style
                if(typeof(d.val.volume_count) != 'undefined'){
                    v = d.val.volume_count;
                }

                return { time: t, val: v };
            });

            
            dfd.resolve({timeconst: q.timeconst,
                         timearray: timearray});
            return;
        });
        return dfd.promise();
    },

    queryTile:function(varname,t,drill) {
        var z = t.z;
        var h =  1 << z;
        var th =  1 << drill;
        var x = Math.min(Math.max(0,t.x),h);
        var y = Math.min(Math.max(0,h-1-t.y),h);  //Flip Y


        var tile2d = "tile2d(" + x + "," + y + "," + z + ")";

        var constraint = "a(\"" + varname + "\",dive(" + tile2d +
                "," + drill + "),\"img\")";

        this.query_elements[varname] = constraint;
        this.tile = {x:x,y:y,z:z};
        this.drill = drill;

        var dfd = new $.Deferred();

        this._run_query(this).done(function(data){
            if (!data.root.children){
                dfd.resolve([]);
                return;
            }

            data = data.root.children;
            //flip Y
            var z = this.tile.z+this.drill;
            var query = this;
            var offset = {x:this.tile.x*256,y:(h-1-this.tile.y)*256};

            data = data.map(function(d){
                if(d.path){
                    d.x = d.path[0];
                    d.y = d.path[1];
                }

                if(typeof(d.val.volume_count) != 'undefined'){
                    d.val = d.val.volume_count;
                }

                d.x =  d.x + offset.x;
                d.y = th-d.y + offset.y;
                d.z = z;

                return d;
            });
            
            data = data.filter (function(d){
                return d.val !== 0;
            });
            dfd.resolve(data);
            return;
        });
        return dfd.promise();
    },
    _pathToXY: function(path){
        var xypath = path.map(function(d,i){
            var nthbit = path.length-1-i;
            var x = ((d >> 0) & 1) << nthbit; //x 0th bit
            var y = ((d >> 1) & 1) << nthbit; //y 1st bit
            return {x:x, y:y};
        });
        return xypath.reduce(function(p,c){
            return {x: p.x|c.x, y:p.y|c.y};
        });
    },
    toString: function(type) {
        var qelem = this.query_elements;
        var dims = Object.keys(qelem);
        var vals = dims.map(function(d) {
            return qelem[d];
        });

        var query_string = vals.join('.');
        return this.nanocube.url + '/' + type + '.' + query_string;
    },

    _run_query: function(ctx,query_cmd){
        query_cmd = query_cmd || 'count';

        var query_string = this.toString(query_cmd);

        var dfd = $.Deferred();
        if (cache[query_string]){
            //console.log('cached');
            var res = $.extend(true, {}, cache[query_string]);
            dfd.resolveWith(ctx, [res]);
            return dfd.promise();
        }
        else{
            console.log(query_string);
            $.ajax({url: query_string, context: ctx}).done(function(res){
                if(Object.keys(cache).length > 10){
                    var idx = Math.floor(Math.random() * (10+1)) ;
                    var k = Object.keys(cache)[idx];
                    delete cache[k];
                }
                cache[query_string] = $.extend(true, {}, res);
                dfd.resolveWith(ctx, [res]);
            });

            return dfd.promise();
        }
    },


    categorialQuery: function(varname){
        var constraint = "a(\"" + varname + "\",dive([],1)) ";
        this.query_elements[varname] = constraint;

        var dfd = new $.Deferred();

        this.valnames = this.nanocube.dimensions[varname].valnames;
        this._run_query(this).done(function(data){
            if (!data.root.children){
                return dfd.resolve({type:'cat',data:[]});
            }

            data = data.root.children;
            var q = this;

            //set up a val to name map
            var valToName = {};
            for (var name in q.valnames){
                valToName[q.valnames[name]] = name;
            }

            var catarray = data.map(function(d){
                return { id: d.path[0], cat: valToName[d.path[0]], val: d.val };
            });

            return dfd.resolve({type:'cat', data:catarray});
        });
        return dfd.promise();
    },

    //Top K query
    topKQuery: function(varname, n){
        var constraint = "k("+n+")";
        this.query_elements[varname] = constraint;

        var dfd = new $.Deferred();

        this.valnames = this.nanocube.dimensions[varname].valnames;
        this._run_query(this,'topk').done(function(data){
            if (!data.root.val.volume_keys){
                return dfd.resolve({type:'id', data: []});
            }
            
            data = data.root.val.volume_keys;
            var q = this;
            var idarray = data.map(function(d){
                return {id:d.key,cat:d.word,val:d.count};
            });
            
            return dfd.resolve({type:'id', data: idarray});
        });
        return dfd.promise();
    },
    
    //temporal queries, return an array of {date, val}
    temporalQuery: function(varname,start,end,interval_sec){
        var q = this;
        var timeinfo = q.nanocube.getTbinInfo();
        
        var startbin = q.nanocube.timeToBin(start);
        
        var bucketsize = interval_sec / timeinfo.bin_sec;
        bucketsize = Math.max(1,Math.floor(bucketsize+0.5));

        var endbin = q.nanocube.timeToBin(end);

        startbin = Math.floor(startbin);
        endbin = Math.floor(endbin);
        
        var count = (endbin - startbin) /bucketsize + 1 ;
        count = Math.floor(count);

        var dfd = new $.Deferred();
        if(endbin==startbin){
            dfd.resolved(null);
            return dfd.promise();
        }
        startbin = Math.max(startbin,0);

        q.queryTime(varname,startbin,bucketsize,count).done(function(res){
            //make date and count for each record
            var nbins = res.timeconst.end - res.timeconst.start;
            nbins = nbins/res.timeconst.bucketsize+1;
            nbins = Math.floor(nbins);
            var datecount = new Array(nbins);
            for(var i=0; i < nbins; i++){
                var t = q.nanocube.bucketToTime(i,res.timeconst.start,
                                                res.timeconst.bucketsize);
                datecount[i]= {time:t,  val:0};
            }

            res.timearray.forEach(function(d,i){
                datecount[d.time].val = d.val;
            });

            //kill zeros
            datecount = datecount.filter(function(d){return d.val !== 0;});
            ///////
            dfd.resolve({type:'temporal', data:datecount,
                         timeconst:res.timeconst });
        });
        return dfd.promise();
    },

    spatialQuery: function(varname,bb,z, maptilesize){
        maptilesize = maptilesize || 256;

        var q = this;

        var tilesize_offset = Math.log(maptilesize)/Math.log(2);
        var pb = { min:{ x: long2tile(bb.min[1],z+tilesize_offset),
                         y: lat2tile(bb.min[0],z+tilesize_offset) },
                   max:{ x: long2tile(bb.max[1],z+tilesize_offset),
                         y: lat2tile(bb.max[0],z+tilesize_offset) }
                 };


        var queries = [];
        var maxlevel = this.nanocube.dimensions[varname].varsize;
        var drill = Math.max(0,Math.min(z+8,8));

        var tilesize = 1 << drill;
        var tbbox = {min:{x: Math.floor(pb.min.x / tilesize),
                          y: Math.floor(pb.min.y / tilesize)},
                     max:{x: Math.floor(pb.max.x / tilesize),
                          y: Math.floor(pb.max.y / tilesize)}};

        z = Math.max(0,Math.min(z,maxlevel-8) );

        var h = 1 << z;

        for (var i=Math.floor(tbbox.min.x);i<=Math.floor(tbbox.max.x);i++){
            for (var j=Math.floor(tbbox.min.y);j<=Math.floor(tbbox.max.y);j++){
                if (i < 0 || j < 0 || i >=h || j>=h){
                    continue;
                }

                var clone_q = $.extend({},q);
                queries.push(clone_q.queryTile(varname,{x:i,y:j,z:z},drill));
            }
        }

        var dfd = new $.Deferred();
        $.when.apply($, queries).done(function(){
            var results = arguments;
            var merged = [];
            merged = merged.concat.apply(merged, results);
            dfd.resolve({type: 'spatial', opts:{pb:pb}, data:merged});
        });
        return dfd.promise();
    }
};

var Nanocube = function(opts) {
    this.schema = null ;
    this.dimensions = null ;
};

Nanocube.initNanocube = function(url){
    var nc = new Nanocube();
    return nc.setUrl(url);
};

Nanocube.prototype = {
    setUrl: function(url){
        var dfd  = new $.Deferred();
        this.url = url;
        var schema_q = this.url + '/schema';

        $.ajax({url: schema_q, context:this}).done(function(schema) {
            var nc = this;
            this.setSchema(schema);
            this.setTimeInfo().done(function() {
                dfd.resolve(nc);
            });
        }).fail(function() {
            console.log('Failed to get Schema from ', url);
        });

        return dfd.promise();
    },
    query: function() {
        return new Query(this);
    },

    setSchema:function(json) {
        this.schema = json;
        var dim = this.schema.fields.filter(function(f) {
            return f.type.match(/^path\(|^id\(|^nc_dim/);
        });
        
        var dimensions = {};
        dim.forEach(function(d){
            dimensions[d.name] = d;
            //Match the variable type and process 
            switch(d.type.match(/^path\(|^id\(|^nc_dim_/)[0]){
            case 'path(': //new style for time / spatial / cat
                var m =  d.type.match(/path\(([0-9]+),([0-9]+)\)/i);
                var bits = +m[1];
                var levels = +m[2];
                
                switch(bits){
                case 1: //time dim
                    dimensions[d.name].vartype = 'time';
                    dimensions[d.name].varsize=levels/8;
                    break;
                case 2: //spatial dim
                    dimensions[d.name].vartype = 'quadtree';
                    dimensions[d.name].varsize=levels;
                    break;
                default: //cat dim
                    dimensions[d.name].vartype = 'cat';
                    dimensions[d.name].varsize = Math.pow(bits,levels)/8;
                }
                break;

            case 'id(': // topk id
                dimensions[d.name].vartype = 'id';
                break;

            case 'nc_dim_': //old style
                var oldm = d.type.match(/nc_dim_(.*)_([0-9]+)/i);
                
                dimensions[d.name].vartype = oldm[1];
                dimensions[d.name].varsize = +oldm[2];
            }
        });
        this.dimensions = dimensions;
    },

    
    setTimeInfo: function() {
        var dim = this.dimensions;

        var tvar = Object.keys(dim).filter(function(k){
            return dim[k].vartype === 'time';
        });

        tvar = dim[tvar[0]];
        //var twidth = +tvar.type.match(/_([0-9]+)/)[1];
        
        var twidth = tvar.varsize;   //+tvar.type.match(/_([0-9]+)/)[1];
        var maxtime = Math.pow(2,twidth*8)-1;

        var dfd = new $.Deferred();

        this.timeinfo = this.getTbinInfo();
        var tinfo = this.timeinfo;

        this.getTimeBounds(tvar.name,0,maxtime).done(function(t){
            tinfo.start = t.mintime;
            tinfo.end = t.maxtime;
            tinfo.nbins = (t.maxtime-t.mintime+1);
            dfd.resolve();
            return;
        });
        return dfd.promise();
    },

    getTimeBounds: function(tvarname,mintime,maxtime){
        var dfd = new $.Deferred();
        var minp = this.getMinTime(tvarname,mintime,maxtime);
        var maxp = this.getMaxTime(tvarname,mintime,maxtime);
        $.when(minp,maxp).done(function(mintime,maxtime){
            dfd.resolve({mintime:mintime,maxtime:maxtime});
        });
        return dfd.promise();
    },

    getMinTime: function(tvarname,mintime,maxtime){
        var q = this.query();

        var dfd = new $.Deferred();
        //base case
        if((maxtime - mintime) < 2){
            return dfd.resolve(mintime);
        }

        var nc = this;
        var interval = Math.ceil((maxtime-mintime)/100000);
        q.queryTime(tvarname,mintime,interval,100000).done(function(res){
            var timearray = res.timearray;
            var timeconst = res.timeconst;
            var minp = timearray.reduce(function(p,c){
                if (p.time < c.time){
                    return p;
                }
                else{
                    return c;
                }
            });

            var mint = minp.time *timeconst.bucketsize;
            var end = (minp.time+1)*timeconst.bucketsize-1;
            mint += timeconst.start;
            end += timeconst.start;
            nc.getMinTime(tvarname,mint,end).done(function(m){
                return dfd.resolve(m);
            });
        });
        return dfd.promise();
    },

    getMaxTime: function(tvarname,mintime,maxtime){
        var q = this.query();

        var dfd = new $.Deferred();
        //base case
        if((maxtime - mintime) < 2){
            return dfd.resolve(maxtime);
        }

        var nc = this;
        var interval = Math.ceil((maxtime-mintime)/100000);
        q.queryTime(tvarname,mintime,interval,100000).done(function(res){
            var timearray = res.timearray;
            var timeconst = res.timeconst;
            var maxp = timearray.reduce(function(p,c){
                if (p.time > c.time){
                    return p;
                }
                else{
                    return c;
                }
            });

            var maxt = maxp.time * timeconst.bucketsize;
            var end = (maxp.time +1) * timeconst.bucketsize-1;
            maxt += timeconst.start;
            end += timeconst.start;
            nc.getMaxTime(tvarname,maxt,end).done(function(m){
                return dfd.resolve(m);
            });
        });
        return dfd.promise();
    },

    getTbinInfo: function() {
        if (this.timeinfo){
            return this.timeinfo;
        }
        
        var tbininfo = this.schema.metadata.filter(function(f) {
            return ( f.key === 'tbin') ;
        });

        var s = tbininfo[0].value.split('_');
        var offset = new Date(s[0]+'T'+s[1]+'Z');

        var res;
        var sec = 0;
        res = s[2].match(/([0-9]+)m/);
        if (res) {
            sec += +res[1]*60;
        }
        res = s[2].match(/([0-9]+)s/);
        if (res) {
            sec = +res[1];
        }

        res = s[2].match(/([0-9]+)h/);
        if (res) {
            sec = +res[1]*60*60;
        }

        res = s[2].match(/([0-9]+)[D,d]/);
        if (res) {
            sec = +res[1]*60*60*24;
        }
        return {
            date_offset: offset,
            bin_sec: sec
        };
    },

    timeToBin: function(t){
        //translate time to bin
        var timeinfo = this.timeinfo;
        var sec = (t - timeinfo.date_offset) / 1000.0;
        var bin = sec / timeinfo.bin_sec;
        bin = Math.max(bin,timeinfo.start-1);
        bin = Math.min(bin,timeinfo.end+1);
        return bin;
        
    },

    bucketToTime: function(t, start, bucketsize){
        start = start || 0;
        bucketsize = bucketsize || 1;
        var timeinfo = this.timeinfo;

        //Translate timebins to real dates
        var base= new Date(timeinfo.date_offset.getTime());

        //add time offset from query
        base.setSeconds(start * timeinfo.bin_sec);

        //make date and count for each record
        var offset = timeinfo.bin_sec * bucketsize * t;
        var time= new Date(base.getTime());
        time.setSeconds(offset);
        return time;
    }
};

//Lat Long to Tile functions from
//https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Lon..2Flat._to_tile_numbers

function long2tile(lon,zoom) {
    return (Math.floor((lon+180)/360*Math.pow(2,zoom)));
}

function lat2tile(lat,zoom)  {
    return (Math.floor((1-Math.log(Math.tan(lat*Math.PI/180) +
                                   1/Math.cos(lat*Math.PI/180))/Math.PI)/2*
                       Math.pow(2,zoom)));
}

function tile2long(x,z) {
    return (x/Math.pow(2,z)*360-180);
}

function tile2lat(y,z) {
    var n=Math.PI-2*Math.PI*y/Math.pow(2,z);
    return (180/Math.PI*Math.atan(0.5*(Math.exp(n)-Math.exp(-n))));
}

function latlong2tile(latlong,zoom) {
    return { x: long2tile(latlong[1],zoom),
             y: lat2tile(latlong[0],zoom),
             z: zoom};
}

/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback){
    var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","timeseries");
    
    d3.select(id).on("divresize",function(){ widget.redraw(); });

    //Collapse on dbl click
    d3.select(id).on('dblclick',function(d){
        var currentheight = d3.select(id).style("height");
        if ( currentheight != "40px"){
            widget.restoreHeight =currentheight ;
            d3.select(id).style('height','40px');
        }
        else{
            d3.select(id).style('height',widget.restoreHeight);
        }
    });


    opts.numformat = opts.numformat || ",";

    this._datasrc = opts.datasrc;

    widget.getDataCallback = getDataCallback;
    widget.updateCallback =  updateCallback;


    var margin = opts.margin;
    if (margin===undefined){
        margin = {top: 30, right: 10, bottom: 30, left: 70};
    }

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height() - margin.top - margin.bottom;

    widget.x = d3.time.scale.utc().range([0, width]);
    widget.y = d3.scale.linear().range([height, 0]);

    widget.xAxis = d3.svg.axis().scale(widget.x)
        .orient("bottom")
        .innerTickSize(-height);

    widget.yAxis = d3.svg.axis().scale(widget.y)
        .orient("left")
        .ticks(3)
        .tickFormat(d3.format(opts.numformat))
        .innerTickSize(-width-3);
    
    // gridlines in x axis function
    function make_x_gridlines() {		
        return d3.axisBottom(widget.x);
    }

    // gridlines in y axis function
    function make_y_gridlines() {		
        return d3.axisLeft(widget.y)
            .ticks(3);
    }

    
    //Zoom
    widget.zoom=d3.behavior.zoom()
        .x(widget.x)
        .on('zoom', function(){
            widget.svg.select(".x.axis").call(widget.xAxis);
            widget.redraw(widget.lastres);
        })
        .on('zoomend', function(){
            widget.update();
            widget.updateCallback(widget._encodeArgs());
            widget.brush.x(widget.x);
        });

    
    //Brush
    widget.brush = d3.svg.brush().x(widget.x);

    widget.brush.on('brushstart', function(){
        if(d3.event.sourceEvent){
            d3.event.sourceEvent.stopPropagation();
        }
    });

    widget.brush.on('brushend', function(){
        console.log(widget.brush.extent());

        widget.updateCallback(widget._encodeArgs());
    });

    //SVG
    widget.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")").call(widget.zoom);

    //Load config
    if(opts.args){
        widget._decodeArgs(opts.args);
    }
    else{
        //set initial domain    
        widget.x.domain(opts.timerange);
    }
    //Update scales
    widget.zoom.x(widget.x);
    widget.brush.x(widget.x);
    
    //add svg stuffs    
    //add title
    widget.svg.append("text")
        .attr("x", -10)
        .attr("y", -10)
        .text(opts.title);

    widget.svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + (height) + ")")
        .call(widget.xAxis);

    widget.svg.append("g")
        .attr("class", "y axis")
        .attr("transform", "translate(-3,0)")
        .call(widget.yAxis);

    //add the X gridlines
    //widget.svg.append("g")			
        //.attr("class", "grid")
      //  .attr("transform", "translate(0," + height + ")")
        //.call(make_x_gridlines().tickSize(-height).tickFormat(""));
    
    // add the Y gridlines
    //widget.append("g")			
    //    .attr("class", "grid")
    //    .call(make_y_gridlines()
    //          .tickSize(-width).tickFormat(""));
    
    //brush
    widget.svg.append("g").attr("class", "x brush")
        .call(widget.brush)
        .selectAll("rect")
        .attr("y", 0)
        .attr("height", height);

    widget.width = width;
}

Timeseries.prototype={
    update: function(){
        var widget = this;
        var sel = this.getSelection();
        var start = sel.global.start;
        var end = sel.global.end;
        var interval = (end - start+1) / 1000 / this.width * 1;

        var promises = {};

        //generate promise for each expr
        for (var d in widget._datasrc){
            if (widget._datasrc[d].disabled){
                continue;
            }
            var p = this.getDataCallback(d,start, end, interval);
            for (var k in p){
                promises[k] = p[k];
            }
        }
            
        var promarray = Object.keys(promises).map(function(k){
            return promises[k];
        });

        var promkeys = Object.keys(promises);
        $.when.apply($,promarray).done(function(){
            var results = arguments;
            var res = {};
            promkeys.forEach(function(d,i){
                res[d] = results[i];

                var label = d.split('&-&');
                var isColor  = /^#[0-9A-F]{6}$/i.test(label[0]);                
                if(isColor){
                    res[d].color = label[0];
                }
                else{
                    var colormap = widget._datasrc[label[1]].colormap;
                    var cidx = Math.floor(colormap.length/2);
                    res[d].color = colormap[cidx];
                }
            });

            widget.lastres = res;
            widget.redraw(res);
        });
    },

    getSelection: function(){
        var sel = {};
        var timedom = this.x.domain();
        sel.global = {start:timedom[0], end:timedom[1]};

        if (!this.brush.empty()){
            var bext = this.brush.extent();
            sel.brush = {start:bext[0], end:bext[1]};
        }
        return sel;
    },

    _encodeArgs: function(){
        var args= this.getSelection();
        return JSON.stringify(args);
    },
    
    _decodeArgs: function(s){
        var args = JSON.parse(s);
        this.x.domain([new Date(args.global.start),
                       new Date(args.global.end)]);
        if(args.brush){
            this.brush.extent([new Date(args.brush.start),
                               new Date(args.brush.end)]);

            this._updateBrush();
        }
    },
    
    _updateBrush: function(){
        //update brush
        this.svg.select("g.x.brush")
            .call(this.brush)
            .call(this.brush.event);
    },
    
    redraw: function(lines){            
        Object.keys(lines).forEach(function(k){
            if(lines[k].data.length > 1){ 
                var last = lines[k].data[lines[k].data.length-1];
                lines[k].data.push(last); //dup the last point for step line
            }
        });

        //update y axis
        var yext = Object.keys(lines).reduce(function(p,c){
            var e = d3.extent(lines[c].data, function(d){
                return (d.val || 0);
            });
            return [ Math.min(p[0],e[0]),
                     Math.max(p[1],e[1])];
        }, [Infinity,-Infinity]);


        yext[0]= yext[0]-0.05*(yext[1]-yext[0]); //show the line around min
        yext[0]= Math.min(yext[0],yext[1]*0.5);
        
        this.y.domain(yext);

        //update the axis
        this.svg.select("g.x.axis").call(this.xAxis);
        this.svg.select("g.y.axis").call(this.yAxis);

        var widget = this;

        //Remove paths obsolete paths
        var paths = widget.svg.selectAll('path.line');
        paths.each(function(){
            var p = this;
            var exists = Object.keys(lines).some(function(d){
                return d3.select(p).classed(d);
            });
            if (!exists){ // remove obsolete
                d3.select(p).remove();
            }
        });
        
        
        //Draw Lines
        Object.keys(lines).forEach(function(k){
            lines[k].data.sort(function(a,b){return a.time - b.time;});
            widget.drawLine(lines[k].data,lines[k].color);
        });
    },

    drawLine:function(data,color){
        var colorid = 'color_'+color.replace('#','');
        
        if (data.length < 2){
            return;
        }

        var widget = this;
        
        //create unexisted paths
        var path = widget.svg.select('path.line.'+colorid);
        if (path.empty()){
            path = widget.svg.append('path');
            path.attr('class', 'line '+colorid);
            
            path.style('stroke-width','2px')
                .style('fill','none')
                .style('stroke',color);
        }


        //Transit to new data
        var lineFunc = d3.svg.line()
                .x(function(d) { return widget.x(d.time); })
                .y(function(d) { return widget.y(d.val); })
                .interpolate("step-before");
        var zeroFunc = d3.svg.line()
                .x(function(d) { return widget.x(d.time); })
                .y(function(d) { return widget.y(0); });

        path.transition()
            .duration(500)
            .attr('d', lineFunc(data));
    }
};

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

     Nanocube3.Nanocube = Nanocube;
     Nanocube3.Viewer = Viewer;
     return Nanocube3;
}));

//# sourceMappingURL=Nanocube.js.map