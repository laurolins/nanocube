/*global d3 */

function GroupedBarChart(name,logaxis){
    var id = '#'+name;
    var margin = {top: 20, right: 20, bottom: 30, left: 40};

    //Add CSS to the div
    d3.select(id).style({
        "overflow-y":"auto",
        "overflow-x":"hidden"
    });

    //Make draggable
    d3.select(id).attr("class","draggable");

    var widget = this;
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
    svg.append("text").attr('y',-5).text(name);

    //Axes
    svg.append("g").attr("class", "y axis");
    svg.append("g").attr("class", "x axis");
   
    //Scales
    var y0 = d3.scale.ordinal();
    var y1 = d3.scale.ordinal();
    var x = d3.scale.linear();
    if (logaxis){
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

    this.id = id;
    this.data = {};
    this.selection=null;
}

GroupedBarChart.prototype.redraw = function(){
    var fdata = this.flattenData(this.data);
    var x =this.x;
    var y0 =this.y0;
    var y1 =this.y1;
    var svg =this.svg;
    var selection = this.selection;
    
    svg.on('click', this.click_callback);
    
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
        .append("svg:title"); //tooltip

    //set shape
    bars.attr('x', 0)
        .attr('y', function(d){
            return y0(d.cat) + y1(d.color);
        }) //selection group
        .attr('height',function(d){ return y1.rangeBand(); })
        .attr('width',function(d){
            return x(d.value);
        })
        .on('click', widget.click_callback) //toggle callback
        .style('fill', function(d){
            if (selection.length < 1  //no selection (everything)
                || (selection.indexOf(d.addr)!=-1)){//addr in selection
                return d.color;
            }
            else{
                return 'gray';
            }
        })
        .transition().duration(100);

    //add tool tip
    bars.select('title').text(function(d){
        return d3.format(',')(d.value);
    });

    //remove bars with no data
    bars.exit().remove();
};

GroupedBarChart.prototype.updateSVG = function(){
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
};

GroupedBarChart.prototype.updateXAxis=function(data){
    var margin = this.margin;
    var x=this.x;
    var xAxis=this.xAxis;
    var svg=this.svg;


    var svgframe = d3.select(svg.node().parentNode);
    var rect=svgframe.node().parentNode.getBoundingClientRect();
    var width = rect.width - this.margin.left-this.margin.right;

    x.domain([d3.min(data, function(d) {return +d.value;})*0.5 ,
              d3.max(data, function(d) {return +d.value;})]);

    x.range([0,width]);

    xAxis.scale(x);

    //move and draw the axis
    svg.select('.x.axis')
        .attr("transform", "translate(0,"+this.totalheight+")")
        .call(xAxis);
    this.width=width;
};

GroupedBarChart.prototype.updateYAxis=function(data){
    var y0=this.y0;
    var y1=this.y1;
    var yAxis=this.yAxis;
    var svg = this.svg;

    y0.domain(data.map(function(d){return d.cat;}));
    y1.domain(data.map(function(d){return d.color;}));
    var totalheight = y0.domain().length* y1.domain().length * 18;

    y0.rangeRoundBands([0, totalheight],0.05);
    y1.rangeRoundBands([0, y0.rangeBand()]);
    yAxis.scale(y0);
    svg.select('.y.axis').call(yAxis);

    //enable axis click
    var widget = this;
    svg.select('.y.axis').selectAll('.tick')
        .on('click',function(d){
            var obj = data.filter(function(e){return e.cat==d;})[0];
            widget.click_callback(obj);
        });
    
    this.totalheight = totalheight;
    this.margin.left = svg.select('.y.axis').node().getBBox().width+5;
};


GroupedBarChart.prototype.setData = function(data,id,color){
    this.data[id] = {color:color, data: data};
};

GroupedBarChart.prototype.setSelection = function(sel){
    this.selection = sel;
};

GroupedBarChart.prototype.removeData = function(id){
    if (id in this.data){ delete this.data[id]; }
};

GroupedBarChart.prototype.flattenData = function(data){
    return Object.keys(data).reduce(function(prev,curr){
        var row = Object.keys(data[curr].data).map(function(k){
            return { addr: data[curr].data[k].addr,
                     cat: data[curr].data[k].cat,
                     color: data[curr].color,
                     value:data[curr].data[k].value };
        });
        return prev.concat(row);
    }, []);
};


GroupedBarChart.prototype.setClickCallback = function(cb){
    this.click_callback = cb;
};
