/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback){
    var id = '#'+ opts.name.replace(/\./g,'\\.');
    //Add CSS to the div
    d3.select(id).style({
        "overflow-y":"auto",
        "overflow-x":"hidden"
    });

    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","resize-drag");
    
    d3.select(id).on("divresize",function(){ widget.redraw(); });

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
        .orient("bottom");
    widget.yAxis = d3.svg.axis().scale(widget.y)
        .orient("left").ticks(3,"s");

    //Zoom
    widget.zoom=d3.behavior.zoom();
    widget.zoom.size([width,height]);

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

    if(opts.args){
        widget._decodeArgs(opts.args);
    }
    else{
        //set initial domain    
        widget.x.domain(opts.timerange);
    }

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
        var interval = (end - start+1) / 1000 / this.width * 5;

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
                var label = d.split('-');
                var colormap = widget._datasrc[label[1]].colormap;
                var cidx = Math.floor(colormap.length/2);
                res[d] = results[i];
                res[d].color = colormap[cidx];
            });

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
            var last = lines[k].data[lines[k].data.length-1];
            lines[k].data.push(last); //add one last data point for step line
            //.pop(); //avoid extreme values at the end?
        });

        //update y axis
        var yext = Object.keys(lines).reduce(function(p,c){
            var e = d3.extent(lines[c].data, function(d){ return d.val; });
            return [ Math.min(p[0],e[0]),
                     Math.max(p[1],e[1])];
        }, [Infinity,-Infinity]);


        //yext = yext.map(function(d){ return d3.round(d,2); });
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
        /*
         var colors = color.split('-');
         color = colors[1];
         */
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

            //var is_color=/(^#[0-9A-Fa-f]{6}$)|(^#[0-9A-Fa-f]{3}$)/i.test(color);
            //if (!is_color){
              //  color = '#f00'; //make it red as default
           // }
            
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
