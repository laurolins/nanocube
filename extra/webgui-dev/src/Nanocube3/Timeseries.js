/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback){
	var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;

    //Make draggable and resizable
    d3.select(id).attr("class","timeseries"); //add resize-drag later

    var margin = opts.margin;
    if (margin === undefined)
        margin = {top: 30, right: 30, bottom: 30, left: 30};

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height() - margin.top - margin.bottom;

    widget.x = d3.scaleUtc().range([0, width])
    	.domain([new Date(2013, 7, 1), new Date(2013, 7, 15) - 1]); //temporary
    widget.y = d3.scaleLinear().range([0,height]);

    widget.xAxis = d3.axisBottom(widget.x)
    	.ticks(d3.utcDay)
    	.tickPadding(0);

    widget.brush = d3.brushX()
		.extent([[0, 0], [width, height]])
		.on("end", brushended);

	function brushended() {
		console.log(d3.event, d3.event.sourceEvent);
		if(!d3.event.sourceEvent) return;
		if(!d3.event.selection) return;
		var d0 = d3.event.selection.map(widget.x.invert),
			d1 = d0.map(d3.utcDay.round);

		// If empty when rounded, use floor & ceil instead.
		if (d1[0] >= d1[1]) {
			d1[0] = d3.timeDay.floor(d0[0]);
			d1[1] = d3.timeDay.offset(d1[0]);
		}
		widget.svg.select("g.brush").call(d3.event.target.move, d1.map(widget.x));
		// d3.event.target.move(widget.svg.select("g.brush"), d1.map(widget.x));
	}


    widget.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")");

    widget.svg.append("g")
    	.attr("class", "axis axis--grid")
    	.attr("transform", "translate(0," + height + ")")
    	.call(d3.axisBottom(widget.x)
    		.ticks(d3.utcHour, 12)
    		.tickSize(-height)
    		.tickFormat(function(){ return null; }))
      .selectAll(".tick")
      	.classed("tick--minor", function(d){ return d.getHours(); });


    widget.svg.append("g")
        .attr("class", "axis axis--x")
        .attr("transform", "translate(0," + height + ")")
        .call(widget.xAxis)
        .attr("text-anchor", null)
      .selectAll("text")
      	.attr("x", 6);

    widget.svg.append("g")
    	.attr("class", "brush")
    	.call(widget.brush);
}

Timeseries.prototype={
	update: function(){
		return;
	},

	getSelection: function(){
        var sel = {};
        var timedom = this.x.domain();
        sel.global = {start:timedom[0], end:timedom[1]};

        // if (!this.brush.empty()){
        //     var bext = this.brush.extent();
        //     sel.brush = {start:bext[0], end:bext[1]};
        // }
        return sel;
    }

};