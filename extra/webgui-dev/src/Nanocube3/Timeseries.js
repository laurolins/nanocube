/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback){
	var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;

    //Make draggable and resizable
    d3.select(id).attr("class","timeseries resize-drag"); //add resize-drag later

    //Collapse on dbl click
    d3.select(id).on('dblclick',function(d){
        var currentheight = d3.select(id).style("height");
        if ( currentheight != "40px"){
            widget.restoreHeight = currentheight ;
            d3.select(id).style('height','40px');
        }
        else{
            d3.select(id).style('height',widget.restoreHeight);
        }
    });

    // opts.numformat = opts.numformat || ",";

    // this._datasrc = opts.datasrc;

    // widget.getDataCallback = getDataCallback;
    // widget.updateCallback =  updateCallback;


    var margin = opts.margin;
    if (margin === undefined)
        margin = {top: 30, right: 30, bottom: 30, left: 30};

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height() - margin.top - margin.bottom;

    widget.x = d3.scaleUtc().range([0, width])
    	.domain([new Date(2013, 7, 1), new Date(2013, 7, 15) - 1]); //temporary
    widget.y = d3.scaleLinear().range([0,height]);

    widget.xAxis = d3.axisBottom(widget.x)
    	.tickSize(-height);
    	

    widget.yAxis = d3.axisLeft(widget.y);

    widget.x_new = widget.x;
    widget.zoom=d3.zoom()
    	.extent([[0, 0], [width, height]])
        .on('zoom', function(){
        	var t = d3.event.transform;
        	widget.x_new = t.rescaleX(widget.x);
        	gX.call(widget.xAxis.scale(widget.x_new));
        	if(widget.brushtime !== undefined){
        		widget.brush.move(widget.gbrush, widget.brushtime.map(widget.x_new));
        	}
            // widget.redraw(widget.lastres);
        })
        .on('end', function(){
        	//update data
        	// widget.update();
            // widget.updateCallback(widget._encodeArgs());
        });

    widget.brush = d3.brushX()
		.extent([[0, 0], [width, height]])
		.on("end", function(){
			if(!(d3.event.sourceEvent instanceof MouseEvent)) return;
			if(!d3.event.selection) return;
			var d0 = d3.event.selection.map(widget.x_new.invert),
				d1 = d0.map(d3.utcDay.round);

			// If empty when rounded, use floor & cbexteil instead.
			if (d1[0] >= d1[1]) {
				d1[0] = d3.timeDay.floor(d0[0]);
				d1[1] = d3.timeDay.offset(d1[0]);
			}
			widget.brushtime = d1;
			d3.event.target.move(widget.gbrush, d1.map(widget.x_new));
			// widget.updateCallback(widget._encodeArgs());
		});

	//Brush play button
    var play_stop = false;
    var ref = {};
    this.playbtn = d3.select(id)
        .append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            if(d3.brushSelection(widget.gbrush.node()) !== null){
                play_stop = !play_stop;
                widget.playTime(play_stop, 0, ref);
            }
        }).html("Play");

    widget.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")")
        .call(widget.zoom)
        .on("mousedown", function() { d3.event.stopPropagation(); });


    var gX = widget.svg.append("g")
        .attr("class", "axis axis--x")
        .attr("transform", "translate(0," + height + ")")
        .call(widget.xAxis);

    var gY = widget.svg.append("g")
    	.attr("class", "axis axis--y")
    	.call(widget.yAxis);

    widget.gbrush = widget.svg.append("g")
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
    },

    playTime: function(play_stop, speed, ref){
    	var widget = this;
    	if(play_stop){
    		widget.playbtn.html("Stop");
            // widget.slider.show();
            // if("repeat" in ref)
            //     clearInterval(ref.repeat);
            ref.repeat = setInterval(function(){
                var bsel = d3.brushSelection(widget.gbrush.node());
                var newbsel = [bsel[1], 2 * bsel[1] - bsel[0]];
                widget.brushtime = newbsel.map(widget.x_new.invert);
                widget.brush.move(widget.gbrush, newbsel);
                // widget.update();
                // widget.updateCallback(widget._encodeArgs());
            }, (1000 - speed));

    	}
    	else{
    		widget.playbtn.html("Play");
            // widget.slider.hide();
            clearInterval(ref.repeat);
    	}
    }

};