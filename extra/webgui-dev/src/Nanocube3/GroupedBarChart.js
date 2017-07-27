/*global d3 $ */

function GroupedBarChart(opts, getDataCallback, updateCallback, getXYCallback){
    this.getDataCallback=getDataCallback;
    this.updateCallback=updateCallback;
    this.getXYCallback = getXYCallback;

    var name=opts.name;
    var id = "#"+name.replace(/\./g,'\\.');
    var margin = {top: 20, right: 20, bottom: 30, left: 40};

    this.id = id;
    this.margin = margin;

    //set param
    this.selection = {global:[]};
    this.tempselection = {};
    if(opts.args){ // set selection from arguments
        this._decodeArgs(opts.args);
    }

    this.retbrush = {
        color:'',
        x:'',
        y:''
    };

    this.retx = ['default'];
    this.rety = ['default'];
    
    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","barchart resize-drag");
    
    d3.select(id).on("divresize",function(){
        widget.update();
    });

    this.toplayer = d3.select(id).append("div")
        .style("width", $(id).width() + "px")
        .style("height", 40 + "px")
        .attr("class", "toplayer");

    this.botlayer = d3.select(id).append("div")
        .style("width", $(id).width() + "px")
        .style("height", $(id).height() + "px");

    //Add clear button
    this.clearbtn = this.toplayer
        .append('button')
        .attr('class','clear-btn')
        .on('click',function(){
            d3.event.stopPropagation();
            
            delete widget.selection.brush; //clear selection
            widget.update(); //redraw itself
            widget.updateCallback(widget._encodeArgs());            
        }).html('clear');
    
    //Add sort button
    this.sortbtn = this.toplayer
        .append('button')
        .attr('class','sort-btn')
        .on('click',function(){
            d3.event.stopPropagation();
            widget._opts.alpha_order = !widget._opts.alpha_order;
            widget.redraw(widget.lastres);
        });

    this.cmpbtn = this.toplayer
        .append('button')
        .attr('class','cmp-btn')
        .on('click',function(){
            widget.runCompare();
        }).html('Compare');

    this.finbtn = this.toplayer
        .append('button')
        .attr('id',(name + 'fin'))
        .on('click',function(){
            Object.keys(widget.tempselection).map(function(k){
                widget.selection[k] = widget.tempselection[k];
                delete widget.tempselection[k];
            });
            widget.compare = true;
            widget.adjust = false;
            widget.cmpbtn.html("Reset");
            delete widget.selection.brush;
            $('#' + name + 'fin').hide();
            widget.update();
            widget.updateCallback(widget._encodeArgs(), [], widget.compare);
        }).html('Compare!');

    $('#' + name + 'fin').hide();

    this.toplayer.append("text")
        .attr("x", $(id).width() / 2)
        .attr("y", 16)
        .attr("font-family", "sans-serif")
        .attr("font-size", "16px")
        .attr("text-anchor", "center")
        .attr("fill", "#fff")
        .text(opts.name);
    
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
    var svg = {};
    var y0 = {};
    var y1 = {};
    var yAxis = {};
    this.margin.left = {};
    for(var j in this.rety){
        svg[this.rety[j]] = {};
        y0[this.rety[j]] = {};
        y1[this.rety[j]] = {};
        yAxis[this.rety[j]] = {};
        this.margin.left[this.rety[j]] = {};
        for(var i in this.retx){
            svg[this.rety[j]][this.retx[i]] = this.botlayer
                .append("svg")
                .attr("class", "barsvg")
                .append("g");
            //Title
            svg[this.rety[j]][this.retx[i]].append('text')
                .attr('y',-8)
                .attr("font-size", "10px")
                .attr('text-anchor', 'middle')
                .attr('fill', '#fff')
                .attr("class", "total");
            svg[this.rety[j]][this.retx[i]].append('text')
                .attr('y',-2)
                .attr('x', -5)
                .attr('text-anchor', 'end')
                .attr("font-size", "10px")
                .attr("class", "xtext")
                .text("X COLOR");
            svg[this.rety[j]][this.retx[i]].append('text')
                .attr('y',-2)
                .attr('x', 5)
                .attr('text-anchor', 'start')
                .attr("font-size", "10px")
                .attr("class", "ytext")
                .text("Y COLOR");
            
            //Axes
            svg[this.rety[j]][this.retx[i]].append("g").attr("class", "y axis")
                .attr("transform", "translate(-3,0)");
            svg[this.rety[j]][this.retx[i]].append("g").attr("class", "x axis");

            y0[this.rety[j]][this.retx[i]] = d3.scaleBand();
            y1[this.rety[j]][this.retx[i]] = d3.scaleBand();
            yAxis[this.rety[j]][this.retx[i]] = d3.axisLeft();
            this.margin.left[this.rety[j]][this.retx[i]] = 40;
        }
    }
    
    //Scales
    var x = d3.scaleLinear();
    if (opts.logaxis){
        x = d3.scaleLog();
    }

    //Axis
    var xAxis = d3.axisBottom()
        .ticks(3,opts.numformat);


    //set default values 
    opts.numformat = opts.numformat || ",";    
    if(!opts.hasOwnProperty('alpha_order')) {
        opts.alpha_order = true;
    }

    //Save vars to "this"
    
    this.svg=svg;
    this.y0=y0;
    this.y1=y1;
    this.x=x;
    this.xAxis = xAxis;
    this.yAxis = yAxis;
    this.compare = false;
    
    this._datasrc = opts.datasrc;
    this._opts = opts;
    this._logaxis = opts.logaxis;
    this._name = name;

    widget.update();
}

GroupedBarChart.brushcolors = colorbrewer.Set1[5].slice(0);
// GroupedBarChart.nextcolor = function(){
//     var c = GroupedBarChart.brushcolors.shift();
//     GroupedBarChart.brushcolors.push(c);
//     return c;
// };
function arraysEqual(arr1, arr2) {
    if(arr1.length !== arr2.length)
        return false;
    for(var i = arr1.length; i--;) {
        if(arr1[i] !== arr2[i])
            return false;
    }

    return true;
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
        var xydata = this.getXYCallback();
         if(!arraysEqual(this.retx,xydata[0]) || !arraysEqual(this.rety,xydata[1])){
            console.log("Rebuilding..");
            this.retx = xydata[0];
            this.rety = xydata[1];

            d3.select(this.id).selectAll(".barsvg").remove();

            var svg = {};
            var y0 = {};
            var y1 = {};
            var yAxis = {};
            this.margin.left = {};
            for(var j in this.rety){
                svg[this.rety[j]] = {};
                y0[this.rety[j]] = {};
                y1[this.rety[j]] = {};
                yAxis[this.rety[j]] = {};
                this.margin.left[this.rety[j]] = {};
                for(var i in this.retx){
                    svg[this.rety[j]][this.retx[i]] = this.botlayer
                        .append("svg")
                        .attr("class", "barsvg")
                        .append("g");
                    //Title
                    svg[this.rety[j]][this.retx[i]].append('text')
                        .attr('y',-8)
                        .attr("font-size", "10px")
                        .attr('text-anchor', 'middle')
                        .attr('fill', '#fff')
                        .attr("class", "total");
                    svg[this.rety[j]][this.retx[i]].append('text')
                        .attr('y',-2)
                        .attr('x', -5)
                        .attr('text-anchor', 'end')
                        .attr("font-size", "10px")
                        .attr("class", "xtext")
                        .text("X COLOR");
                    svg[this.rety[j]][this.retx[i]].append('text')
                        .attr('y',-2)
                        .attr('x', 5)
                        .attr('text-anchor', 'start')
                        .attr("font-size", "10px")
                        .attr("class", "ytext")
                        .text("Y COLOR");
                    
                    //Axes
                    svg[this.rety[j]][this.retx[i]].append("g").attr("class", "y axis")
                        .attr("transform", "translate(-3,0)");
                    svg[this.rety[j]][this.retx[i]].append("g").attr("class", "x axis");

                    y0[this.rety[j]][this.retx[i]] = d3.scaleBand();
                    y1[this.rety[j]][this.retx[i]] = d3.scaleBand();
                    yAxis[this.rety[j]][this.retx[i]] = d3.axisLeft();
                    this.margin.left[this.rety[j]][this.retx[i]] = 40;
                }
            }

            this.svg = svg;
            this.y0 = y0;
            this.y1 = y1;
            this.yAxis = yAxis;

        }
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
            Object.keys(widget.svg).map(function(a){
                res[a] = {};
                Object.keys(widget.svg[a]).map(function(b){
                    res[a][b] = {};
                    promkeys.forEach(function(d,i){
                        var label = d.split('&-&');
                        var xyc = label[0].split('&');
                        var ret = {};
                        xyc.map(function(k){
                            ret[k.charAt(0)] = k.substring(1);
                        });

                        //check ret.x, ret.y
                        if(ret.x != b && b != 'default')
                            return;
                        if(ret.y != a && a != 'default')
                            return;
                        if(ret.c)
                            res[a][b][ret.c] = results[i];
                        else
                            res[a][b]["global&-&" + label[1]] = results[i];
                    });
                });
            });
            
            widget.lastres = res;
            widget.redraw(res);
        });
    },
    
    flattenData: function(res){
        var widget = this;        
        return Object.keys(res).reduce(function(prev,curr){
            var c = curr;

            var isColor  = /^#[0-9A-F]{6}$/i.test(c);                
            if(!isColor){
                var label = curr.split('&-&');
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
        var widget = this;
        var topn = this._opts.topn;

        if(topn !== undefined ){
            Object.keys(res).map(function(i){
                Object.keys(res[i]).map(function(j){
                    var agg = {};
                    Object.keys(res[i][j]).forEach(function(k){
                        res[i][j][k].data.forEach(function(d){
                            agg[d.cat]= (agg[d.cat] + d.val) || d.val;
                        });
                    });
                    var kvlist =Object.keys(agg)
                        .map(function(d){return {cat: d, val:agg[d]};});
                    kvlist.sort(function(x,y) { return y.val - x.val; });
                    kvlist = kvlist.slice(0,topn);
                    var kvhash = {};
                    kvlist.forEach(function(d){ kvhash[d.cat] = d.val; });
                    Object.keys(res[i][j]).forEach(function(k){
                        res[i][j][k].data = res[i][j][k].data.filter(function(d){
                            return (d.cat in kvhash);
                        });
                    });
                    console.log(res[i][j]);
                });
            });
        }
        var fdata = {};
        Object.keys(res).map(function(i){
            fdata[i] = {};
            Object.keys(res[i]).map(function(j){
                fdata[i][j] = widget.flattenData(res[i][j]);
            });
        });

        var x =this.x;
        var y0 =this.y0;
        var y1 =this.y1;
        var svg =this.svg;
        var selection = this.selection;
        
        
        //update the axis and svgframe
        this.updateYAxis(fdata);
        this.updateXAxis(fdata);
        this.updateSVG();


        Object.keys(fdata).map(function(i){
            Object.keys(fdata[i]).map(function(j){


                //bind data
                var bars = widget.svg[i][j].selectAll('.bar').data(fdata[i][j]);

                // if(bars._groups[0].length === 0)
                //     return;

                //append new bars
                bars.enter()
                    .append('rect')
                    .attr('class', 'bar')
                    .on('click', function(d) { widget.clickFunc(d);})//toggle callback
                    .append("svg:title"); //tooltip

                bars = widget.svg[i][j].selectAll('.bar').data(fdata[i][j]);

                //set shape
                bars.attr('x', 0)
                    .attr('y', function(d){return widget.y0[i][j](d.cat) + //category
                                           widget.y1[i][j](d.color);}) //selection group
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
                        return widget.y1[i][j].bandwidth()-1;
                    })
                    .attr('width',function(d){
                        var w = widget.x(d.val);
                        if(isNaN(w) && d.val <=0 ){
                            w = 0;
                        }
                        return w;
                    });

                if(widget.compare){
                    Object.keys(widget.selection).filter(function(n){
                        return (n != 'brush') && (n != 'global');
                    }).forEach(function(s){
                        var cats = Object.keys(widget.selection[s]).map(function(k){
                            return widget.selection[s][k].cat;
                        });
                        svg[i][j].select('.y.axis')
                            .selectAll("text")
                            .filter(function(n){
                                return (cats.indexOf(n) != -1);
                            })
                            .style("fill", s);
                    });

                    
                    // bars.style('fill', function(d){
                    //     var col;
                    //     Object.keys(widget.selection).filter(function(n){
                    //         return (n != 'brush') && (n != 'global');
                    //     }).forEach(function(s){
                    //         if(widget.selection[s] == [] || 
                    //            widget.selection[s].findIndex(function(b){
                    //                 return (b.cat == d.cat);}) != -1){
                    //             col = s;
                    //         }
                            
                    //     });

                    //     return col || 'gray';
                    // });
                }
                
                //add tool tip
                bars.select('title').text(function(d){
                    return d3.format(widget._opts.numformat)(d.val);
                });

                //remove bars with no data
                bars.exit().remove();
            });
        });
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
        var widget = this;
        var height = this.totalheight;
        var width = this.width;

        this.toplayer.style("width", $(this.id).width() + "px");
        this.botlayer.style("width", $(this.id).width() + "px");

        Object.keys(svg).map(function(i){
            Object.keys(svg[i]).map(function(j){
                var svgframe = d3.select(svg[i][j].node().parentNode);
                //resize the frame
                svgframe.attr("width", width + widget.maxLeft + margin.right);
                svgframe.attr("height", height + margin.top + margin.bottom);
                svg[i][j].attr("transform", "translate("+widget.maxLeft+","+margin.top+")");
            });
        });
    },

    updateXAxis: function(data){
        var margin = this.margin;
        var x=this.x;
        var xAxis=this.xAxis;
        var svg=this.svg;
        var widget = this;



        var anysvg = this.getAny(svg);

        var width = $(this.id).width();
        // console.log(width);
        for(var i in this.retx)
            width -= (widget.maxLeft + widget.margin.right);

        width /= this.retx.length;
        width -= 5;
        // console.log(width);
        if(width < 0)
            width = 1;

        var dlistmin = [];
        var dlistmax = [];
        Object.keys(data).map(function(i){
            Object.keys(data[i]).map(function(j){
                dlistmin.push(d3.min(data[i][j], function(d) {return +d.val;}));
                dlistmax.push(d3.max(data[i][j], function(d) {return +d.val;}));
            });
        });

        var d = [Math.min.apply(null,dlistmin),
                 Math.max.apply(null,dlistmax)];

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
        Object.keys(svg).map(function(i){
            Object.keys(svg[i]).map(function(j){
                svg[i][j].select('.x.axis')
                    .attr("transform", "translate(0,"+widget.totalheight+")")
                    .call(xAxis);
            });
        });
        
        this.width=width;
    },

    updateYAxis:function(data){
        var y0=this.y0;
        var y1=this.y1;
        var yAxis=this.yAxis;
        var svg = this.svg;
        var opts = this._opts;
        var sortbtn = this.sortbtn;
        var widget = this;
        
        Object.keys(data).map(function(i){
            Object.keys(data[i]).map(function(j){
                //Sort y axis
                if (opts.alpha_order){            
                    y0[i][j].domain(data[i][j].map(function(d){return d.cat;}).sort());
                    if (y0[i][j].domain().every(function(d) {return !isNaN(d);})){
                        y0[i][j].domain(y0[i][j].domain().sort(function(a,b){return a-b;}));
                    }
                    sortbtn.html('#');
                }
                else{ //sort by data value
                    var d = data[i][j].sort(function(x,y){ return y.val - x.val;});
                    y0[i][j].domain(d.map(function(d){return d.cat;}));
                    sortbtn.html('A');
                }

                y1[i][j].domain(data[i][j].map(function(d){return d.color;}));
            });
        });


        var maxy0length = 0;
        var maxy1length = 0;

        Object.keys(y0).map(function(i){
            Object.keys(y0[i]).map(function(j){
                maxy0length = Math.max(maxy0length, y0[i][j].domain().length);
                maxy1length = Math.max(maxy1length, y1[i][j].domain().length);
            });
        });

        var totalheight = maxy0length * maxy1length * 18;

        Object.keys(y0).map(function(i){
            Object.keys(y0[i]).map(function(j){
                y0[i][j].rangeRound([0, totalheight]);
                y1[i][j].rangeRound([0, y0[i][j].bandwidth()]);
                yAxis[i][j].scale(y0[i][j]);

                svg[i][j].select('.y.axis').call(yAxis[i][j]);



                //enable axis click
                svg[i][j].select('.y.axis').selectAll('.tick')
                    .on('click',function(d){
                        var obj = data[i][j].filter(function(e){return e.cat==d;})[0];
                        widget.clickFunc(obj);
                    });

                widget.margin.left[i][j] = svg[i][j].select('.y.axis').node().getBBox().width+3;
                //update title with cat count
                svg[i][j].select('.total').text('('+y0[i][j].domain().length+')');
                var xtext = svg[i][j].select('.xtext');
                var ytext = svg[i][j].select('.ytext');


                if(j != 'default')
                    xtext.attr('fill', j);
                else
                    xtext.attr('fill', '#fff');

                if(i != 'default')
                    ytext.attr('fill', i);
                else
                    ytext.attr('fill', '#fff');

            });
        });

        widget.maxLeft = 0;
        Object.keys(widget.margin.left).map(function(i){
            Object.keys(widget.margin.left[i]).map(function(j){
                widget.maxLeft = Math.max(widget.maxLeft, widget.margin.left[i][j]);
            });
        });

        this.totalheight = totalheight;
    },

    runCompare: function(){
        var widget = this;
        d3.event.stopPropagation();
        if(widget.cmpbtn.html() == "Compare" && 
            (widget.retbrush.color == widget._name || 
             widget.retbrush.x == widget._name ||
             widget.retbrush.y == widget._name)){
            delete widget.selection.brush;
            widget.update();
            widget.cmpbtn.html("Selection 1");
            $('#' + widget._name + 'fin').show();
        }

        else if(widget.cmpbtn.html() == "Selection 5"){
            widget.tempselection[GroupedBarChart.brushcolors[4]] = widget.selection.brush || [];
            Object.keys(widget.tempselection).map(function(k){
                widget.selection[k] = widget.tempselection[k];
                delete widget.tempselection[k];
            });
            widget.compare = true;
            widget.adjust = false;
            widget.cmpbtn.html("Reset");
            $('#' + widget._name + 'fin').hide();
            delete widget.selection.brush;
            widget.update();
            widget.updateCallback(widget._encodeArgs(), [], widget.compare);

        }

        else if (widget.cmpbtn.html().startsWith("Selection")){
            var sel = parseInt(widget.cmpbtn.html().split(' ')[1]);
            widget.tempselection[GroupedBarChart.brushcolors[sel - 1]] = widget.selection.brush;
            if(widget.selection.brush === undefined)
                widget.tempselection[GroupedBarChart.brushcolors[sel - 1]] = [];
            widget.cmpbtn.html("Selection " + (sel + 1));
            delete widget.selection.brush;
            widget.update();
            widget.updateCallback(widget._encodeArgs());

        }
        else{
            //reset
            Object.keys(widget.svg).map(function(i){
                Object.keys(widget.svg[i]).map(function(j){
                    widget.svg[i][j].select('.y.axis').selectAll("text").style("fill", "#fff");
                });
            });
            
            GroupedBarChart.brushcolors.map(function(k){
                delete widget.selection[k];
            });
            widget.compare = false;
            widget.cmpbtn.html("Compare");
            widget.update();
            widget.updateCallback(widget._encodeArgs(), [], widget.compare);
        }
    },

    getAny: function(obj){
        var temp = obj[Object.keys(obj)[0]];
        return temp[Object.keys(temp)[0]];
    },
};
