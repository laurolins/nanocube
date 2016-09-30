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

    //Collapse on dbl click
    d3.select(id).on('dblclick',function(d){
        var currentheight = d3.select(id).style("height");
        if ( currentheight != "20px"){
            widget.restoreHeight =currentheight ;
            d3.select(id).style('height','20px');
        }
        else{
            d3.select(id).style("height",widget.restoreHeight);
        }
    });

    
    //SVG container
    var svg = d3.select(id).append("svg").append("g");

    //Title
    svg.append("text").attr('y',-5);
    
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

    xAxis.orient("bottom").ticks(3,",.1s");
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
            
            widget.redraw(res);
        });
    },
    
    flattenData: function(res){
        var widget = this;
        return Object.keys(res).reduce(function(prev,curr){         
            var label = curr.split('-'); 
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
        var fdata = this.flattenData(res);
        var x =this.x;
        var y0 =this.y0;
        var y1 =this.y1;
        var svg =this.svg;
        var selection = this.selection;
        
        //svg.on('click', this.click_callback);
        
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
            .transition().duration(500)
            .attr('width',function(d){
                var w = widget.x(d.val);
                if(isNaN(w) && d.val <=0 ){
                    w = 0;
                }
                return w;
            });
        
        //add tool tip
        bars.select('title').text(function(d){
            return d3.format(',')(d.val);
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
        var rect=svgframe.node().parentNode.getBoundingClientRect();

        //calculate width and height in side the margin
        var width = rect.width-margin.left-margin.right;
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

        x.domain([d3.min(data, function(d) {return +d.val;})*0.5,
                  d3.max(data, function(d) {return +d.val;})]);

        if(this._opts.logaxis){ // prevent zeros for log
            var d = x.domain();
            d[0] = Math.max(d[0],1e-6);
            d[1] = Math.max(d[1],d[0]+1e-6);
            x.domain(d);
        }

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
        
        y0.domain(data.map(function(d){return d.cat;}).sort());
        if (y0.domain().every(function(d) {return !isNaN(d);})){
            y0.domain(y0.domain().sort(function(a,b){return a-b;}));
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
        this.margin.left = svg.select('.y.axis').node().getBBox().width+10;

        //update title with cat count
        svg.select('text').text(this._name+' ('+y0.domain().length+')');
    }    
};
