/*global $,d3 */

function Timeseries(name, margin){
    var id = '#'+name;

    this.data = {};
    this.brush_callback = null;

    //code
    if (margin==undefined){
        margin = {top: 30, right: 10, bottom: 30, left: 50};
    }

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height()- margin.top - margin.bottom;

    
    this.x = d3.time.scale()
        .range([0, width]);
    
    this.y = d3.scale.linear()
        .range([height, 0]);
    
    this.xAxis = d3.svg.axis().scale(this.x)
        .orient("bottom");
    
    this.yAxis = d3.svg.axis()
        .scale(this.y)
        .orient("left")
        .ticks(3,',s');

    var that = this;

    //Brush
    this.brush = d3.svg.brush().x(this.x);

    this.brush.on("brushstart", function(){
        d3.event.sourceEvent.stopPropagation();
    });

    this.brush.on("brushend", function(){
        that.brushended(that);
    });
    
    //Zoom
    this.zoom= d3.behavior.zoom();
    this.zoom.size([width,height]);

    var bext;
    this.zoom.on("zoomstart", function(){
        if(!that.brush.empty()){ //save the size of the brush
            bext = that.brush.extent();
        }
    });

    this.zoom.on("zoom", function(){
        that.zoomed();
        if(!that.brush.empty()){ //set the size of the brush
            d3.select(this).call(that.brush.extent(bext));
        }
    });

    this.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," 
              + margin.top + ")").call(this.zoom);

    
    //add svg stuffs
    this.svg.append("text")
        .attr("x", 5)
        .attr("y", 5)
        .text("");

    //add title
    this.svg.append("text")
	.attr("x", -10)
        .attr("y", -10)
	.text(name);    

    this.svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + (height) + ")")
        .call(this.xAxis);

    this.svg.append("g")
        .attr("class", "y axis")
        .attr("transform", "translate(-3,0)")
        .call(this.yAxis);

    //brush    
    this.svg.append("g").attr("class", "x brush")
        .call(this.brush)
        .selectAll("rect")
        .attr("y", 0)
        .attr("height", height);
}

Timeseries.prototype.setBinSizeTxt=function(timeinh){
    var timestr = "";
    if (timeinh < 1/60.0){
        timestr = d3.round(timeinh*3600,1) + "s";
    }
    else if (timeinh < 1){
        timestr = d3.round(timeinh*60,1) + "m";
    }
    else if (timeinh < 24){
        timestr = d3.round(timeinh,1) + "h";
    }
    else if (timeinh >= 24){
        timestr = d3.round(timeinh/24.0,1) + "D";
    }

    this.svg.select("text")
        .text(timestr);
};

Timeseries.prototype.setData=function(line,key){
    this.data[key] = line; 
    this.updateRanges();
};

Timeseries.prototype.keys=function(){
    return Object.keys(this.data);
}

Timeseries.prototype.removeData=function(key){
    delete this.data[key];
    this.updateRanges();
};


Timeseries.prototype.brushended=function(that){    
    var start=0,end=0;
    if(!that.brush.empty()){ //reset
        var timerange = that.brush.extent();
        start = timerange[0];
        end = timerange[1];
    }

    //use this callback to update the model
    that.brush_callback(start,end);
};

Timeseries.prototype.zoomed=function(){
    //redraw axes and lines
    this.svg.select("g.x.axis").call(this.xAxis);
    this.svg.select("g.y.axis").call(this.yAxis);
    this.svg.selectAll(".line").attr("class", "line").attr("d", this.line);

    var xdom = this.x.domain();
    this.update_display_callback(xdom[0],xdom[1]);
};

Timeseries.prototype.updateRanges=function(){
    var that = this;
    var data = Object.keys(that.data).reduce(function(previous,current){ 
        return previous.concat(that.data[current].data);
    },  []);
    
    //set domain
    var xext = d3.extent(data, function(d) { return d.date; });
    var yext = d3.extent(data, function(d) { return d.value; });

    this.y.domain(yext);
    var xdom = this.x.domain();
    if(xdom[0].getTime()==0 && xdom[1].getTime()==1){ 
        //set x to xext for init only
        this.x.domain(xext);
    }
    
    //update zoom domain
    this.zoom.x(this.x);
};

Timeseries.prototype.redraw=function(){
    //update the axis
    this.svg.select("g.x.axis").call(this.xAxis);
    this.svg.select("g.y.axis").call(this.yAxis);

    //Remove lines
    this.svg.selectAll("path.line").data([]).exit().remove();
    //Remove points
    //this.svg.selectAll("circle.dot").data([]).exit().remove();

    var that = this;
    //Draw Lines
    Object.keys(that.data).forEach(function(l){ 
        var line = that.data[l];
        that.drawLine(line.data, line.color); 
    });    
};

Timeseries.prototype.drawLine=function(data,color){
    if (data.length < 2){
        return;    
    }
    
    var that = this;
    this.line = d3.svg.line()
        .x(function(d) { return that.x(d.date); })
        .y(function(d) { return that.y(d.value); }); 
    
    //add data
    this.svg.append('path')
        .datum(data)
        .attr('class', 'line')
        .attr('d',this.line)
        .style('stroke-width','2px')
        .style('fill','none')
        .style('stroke',color);

    /*
     //add data points
     if(this.ptidx != null){
     var points = this.ptidx.map(function(idx){return data[idx];});

     this.svg.selectAll('circle.dot')
     .data(points)
     .enter()
     .append("circle")
     .attr('class', 'dot')
     .attr('r', 5.0)
     .attr('cx', function(d){return that.x(d.date);})
     .attr('cy', function(d){return that.y(d.value);})
     .style('stroke','black')
     .style('stroke-width',2)
     .style('fill','none');
     }
     */
};
