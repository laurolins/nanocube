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
