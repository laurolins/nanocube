/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback){
	var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;

    //Make draggable and resizable
    d3.select(id).attr("class","timeseries resize-drag"); //add resize-drag later

    d3.select(id).on("divresize",function(){
        widget.update();
    });

    //Collapse on dbl click
    // d3.select(id).on('dblclick',function(d){
    //     var currentheight = d3.select(id).style("height");
    //     if ( currentheight != "40px"){
    //         widget.restoreHeight = currentheight ;
    //         d3.select(id).style('height','40px');
    //     }
    //     else{
    //         d3.select(id).style('height',widget.restoreHeight);
    //     }
    // });

    opts.numformat = opts.numformat || ",";

    this._datasrc = opts.datasrc;

    widget.getDataCallback = getDataCallback;
    widget.updateCallback =  updateCallback;


    var margin = opts.margin;
    if (margin === undefined)
        margin = {top: 10, right: 30, bottom: 20, left: 30};

    var width = $(id).width() - margin.left - margin.right - 60;
    var height = $(id).height() - margin.top - margin.bottom - 70;
    								//30 from sliders above and below

    //Nested SVG layers
    widget.toplayer = d3.select(id).append("div")
    	.style("width", $(id).width() + "px")
    	.style("height", 40 + "px")
    	.attr("class", "toplayer");

    widget.midlayer = d3.select(id).append("div")
    	.style("width", $(id).width() + "px")
    	.style("height", height + margin.top + margin.bottom + "px")
    	.attr("class", "midlayer");

    widget.botlayer = d3.select(id).append("div")
    	.style("width", $(id).width() + "px")
    	.style("height", 30 + "px")
    	.attr("class", "botlayer");


	//Animation step slider
	var asx = d3.scaleLinear()
    	.domain([0, 5])
    	.range([0, 200])
    	.clamp(true);

    widget.asslider = widget.toplayer.append("svg")
    	.attr("width", 250)
    	.attr("height", 40)
    	.attr("class", "as-slider")
    	.append("g")
    	.attr("transform", "translate(" + 25 + "," + 20 + ")");

    d3.select(widget.asslider.node().parentNode).append("text")
    	.attr("x", 100)
    	.attr("y", 12)
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "10px")
    	.attr("text-anchor", "center")
    	.attr("fill", "white")
    	.text("Animation Step");

    widget.asslider.append("line")
    	.attr("class", "track")
    	.attr("x1", asx.range()[0])
    	.attr("x2", asx.range()[1])
    .select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-inset")
	.select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-overlay")
    	.call(d3.drag()
    		.on("start.interrupt", function(){widget.asslider.interrupt(); })
    		.on("drag", function(){                                                                   
    			var h = Math.round(asx.invert(d3.event.x));
    			currentstep = h;
    			ashandle.attr("cx", asx(h));
    			widget.playTime(play_stop, currentspeed, h, ref);
    		}));

    var aslist = ["Auto", "Hour", "Day", "Week", "Month", "Year"];
    widget.asslider.insert("g", ".track-overlay")
    	.attr("class", "ticks")
    	.attr("transform", "translate(0," + 18 + ")")
    .selectAll("text")
    .data([5, 4, 3, 2, 1, 0])
    .enter().append("text")
    	.attr("x", asx)
    	.attr("text-anchor", "middle")
    	.attr("fill", "white")
    	.text(function(d) { return aslist[d];});

    var ashandle = widget.asslider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);


	//Speed Slider
    var sx = d3.scaleLinear()
    	.domain([0, 999])
    	.range([0, 200])
    	.clamp(true);

    widget.slider = widget.toplayer.append("svg")
    	.attr("width", 250)
    	.attr("height", 40)
    	.attr("class", "spd-slider")
    	.append("g")
    	.attr("transform", "translate(" + 25 + "," + 20 + ")");

    d3.select(widget.slider.node().parentNode).append("text")
    	.attr("x", 110)
    	.attr("y", 12)
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "10px")
    	.attr("text-anchor", "center")
    	.attr("fill", "white")
    	.text("Speed");

    widget.slider.append("line")
    	.attr("class", "track")
    	.attr("x1", sx.range()[0])
    	.attr("x2", sx.range()[1])
    .select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-inset")
	.select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-overlay")
    	.call(d3.drag()
    		.on("start.interrupt", function(){widget.slider.interrupt(); })
    		.on("drag", function(){                                                            
    			var h = sx.invert(d3.event.x);
    			currentspeed = h;
    			handle.attr("cx", sx(h));
    			widget.playTime(play_stop, h, currentstep, ref);
    		}));

    widget.slider.insert("g", ".track-overlay")
    	.attr("class", "ticks")
    	.attr("transform", "translate(0," + 18 + ")")
    .selectAll("text")
    .data([999, 800, 600, 400, 200, 0])
    .enter().append("text")
    	.attr("x", sx)
    	.attr("text-anchor", "middle")
    	.attr("fill", "white")
    	.text(function(d) { return (1000 - d) + " ms";});

    var handle = widget.slider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);

	//Brush play button
    var play_stop = false;
    var ref = {};
    var currentspeed = 0;
    var currentstep = 0;

    this.forwardbtn = widget.toplayer.append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            if(d3.brushSelection(widget.gbrush.node()) !== null){
                widget.iterateTime(currentstep, 1);
            }
        }).html(">");

    this.playbtn = widget.toplayer.append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            if(d3.brushSelection(widget.gbrush.node()) !== null){
                play_stop = !play_stop;
                widget.playTime(play_stop, currentspeed, currentstep, ref);
            }
        }).html("Play");

    this.backbtn = widget.toplayer.append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            if(d3.brushSelection(widget.gbrush.node()) !== null){
                widget.iterateTime(currentstep, -1);
            }
        }).html("<");

    //Brush snapping slider
    var bslist = ["Hour", "Day", "Week", "Month", "Year"];

    var bsx = d3.scaleLinear()
    	.domain([0, 4])
    	.range([0, 150])
    	.clamp(true);

    widget.bsslider = widget.toplayer.append("svg")
    	.attr("width", 200)
    	.attr("height", 40)
		.append("g")
    	.attr("class", "slider")
    	.attr("transform", "translate(" + 25 + "," + 20 + ")");

    d3.select(widget.bsslider.node().parentNode).append("text")
    	.attr("x", 75)
    	.attr("y", 12)
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "10px")
    	.attr("text-anchor", "center")
    	.attr("fill", "white")
    	.text("Snap-to-grid");


    widget.bsslider.append("line")
    	.attr("class", "track")
    	.attr("x1", bsx.range()[0])
    	.attr("x2", bsx.range()[1])
    .select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-inset")
	.select(function() { return this.parentNode.appendChild(this.cloneNode(true)); })
    	.attr("class", "track-overlay")
    	.call(d3.drag()
    		.on("start.interrupt", function(){widget.bsslider.interrupt(); })
    		.on("drag", function(){                                                                       
    			var h = Math.round(bsx.invert(d3.event.x));
    			brushsnap = h;
    			bshandle.attr("cx", bsx(h));
    		}));

    widget.bsslider.insert("g", ".track-overlay")
    	.attr("class", "ticks")
    	.attr("transform", "translate(0," + 18 + ")")
    .selectAll("text")
    .data([4, 3, 2, 1, 0])
    .enter().append("text")
    	.attr("x", bsx)
    	.attr("text-anchor", "middle")
    	.attr("fill", "white")
    	.text(function(d) { return bslist[d];});

    var bshandle = widget.bsslider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);

    //Brush Selection Text
    widget.bstextsvg = widget.toplayer.append("svg")
    	.attr("width", 400)
    	.attr("height", 30);

    widget.bstext = widget.bstextsvg.append("text")
    	.attr("x", 10)
    	.attr("y", 15)
    	.text("No Brush Selected")
    	.attr("font-family", "Courier New, monospace")
    	.attr("font-size", "10px")
    	.attr("text-anchor", "start")
    	.attr("fill", "white");

    widget.x = d3.scaleUtc().range([0, width]);
    widget.y = d3.scaleLinear().range([height, 0]);

    widget.xAxis = d3.axisBottom(widget.x)
    	.tickSize(-height);
    	

    widget.yAxis = d3.axisLeft(widget.y)
    	.ticks(3)
        .tickFormat(d3.format(opts.numformat))
        .tickSize(-width-3);

    //Zoom
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
            widget.redraw(widget.lastres);
        })
        .on('end', function(){
        	//update data
        	widget.update();
            widget.updateCallback(widget._encodeArgs());
        });

    //Brush and Brushsnapping
    var bsfunc = [d3.utcHour, d3.utcDay, d3.utcWeek, d3.utcMonth, d3.utcYear];
    var brushsnap = 0;

    widget.brush = d3.brushX()
		.extent([[0, 0], [width, height]])
		.on("end", function(){
			if(widget.iterating){
				widget.iterating = false;
				return;
			}
			if(!(d3.event.sourceEvent instanceof MouseEvent)) return;
			if(!d3.event.sourceEvent) return;
			if(!d3.event.selection){
				widget.brushtime = undefined;
				widget.updateCallback(widget._encodeArgs());
				return;
			}
			var d0 = d3.event.selection.map(widget.x_new.invert);
			var d1 = d0.map(bsfunc[brushsnap].round);

			// If empty when rounded, use floor & cbexteil instead.
			if (d1[0] >= d1[1]) {
				d1[0] = bsfunc[brushsnap].floor(d0[0]);
				d1[1] = bsfunc[brushsnap].ceil(d0[1]);
			}
			widget.brushtime = d1;
			d3.event.target.move(widget.gbrush, d1.map(widget.x_new));
			widget.updateCallback(widget._encodeArgs());
		});

    // Timeline Left Pan button
    var arc = d3.symbol().type(d3.symbolDiamond)
    	.size([height] * 20);


	var leftpan = widget.midlayer.append("svg")
		.attr("width", 30)
		.attr("height", height + margin.top + margin.bottom)
		.append('path')
		.attr('d', arc)
		.attr('fill', 'gray')
		.attr('stroke','#000')
		.attr('stroke-width',1)
		.attr('transform', 'translate(' + margin.left + ',' + 
			(height + margin.top + margin.bottom)/2 + ')');
	
	var pan;
	leftpan.on("mouseover", function(){
		console.log("Mouseover");
		leftpan.attr('fill', 'blue');
		pan = setInterval(function(){
			var transform = d3.zoomTransform(widget.ts.node());
            widget.zoom.translateBy(widget.ts, 20 / transform.k, 0);
        }, 10);
	});

	leftpan.on('mouseleave', function(){
		leftpan.attr('fill', 'gray');
		clearInterval(pan);
	});

    // Timeline svg
    widget.tssvg = widget.midlayer.append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom);
    widget.ts = widget.tssvg.append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")")
        .call(widget.zoom)
        .on("mousedown", function() { d3.event.stopPropagation(); });

    // Timeline Right Pan button

	var rightpan = widget.midlayer.append("svg")
		.attr("width", 30)
		.attr("height", height + margin.top + margin.bottom)
		.append('path')
		.attr('d', arc)
		.attr('fill', 'gray')
		.attr('stroke','#000')
		.attr('stroke-width',1)
		.attr('transform', 'translate(' + 0 + ',' + 
			(height + margin.top + margin.bottom)/2 + ')');
	
	var pan2;
	rightpan.on("mouseover", function(){
		rightpan.attr('fill', 'blue');
		pan2 = setInterval(function(){
            var transform = d3.zoomTransform(widget.ts.node());
            widget.zoom.translateBy(widget.ts, -20 / transform.k, 0);
        }, 10);
	});

	rightpan.on('mouseleave', function(){
		rightpan.attr('fill', 'gray');
		clearInterval(pan2);
	});

    if(opts.args){
    	widget._decodeArgs(opts.args);
    }
    else{
    	widget.x.domain(opts.timerange);
    }

    var gX = widget.ts.append("g")
        .attr("class", "axis axis--x")
        .attr("transform", "translate(0," + height + ")")
        .call(widget.xAxis);

    var gY = widget.ts.append("g")
    	.attr("class", "axis axis--y")
    	.call(widget.yAxis);

    widget.gbrush = widget.ts.append("g")
    	.attr("class", "brush")
    	.call(widget.brush);

    //Time Aggregation
    widget.unitTime = opts.binsec;

    widget.tatext = widget.botlayer.append("svg")
    	.attr("width", 250)
    	.attr("height", 30)
    	.attr("class", "tatext")
    	.append("text")
    	.attr("x", 10)
    	.attr("y", 15)
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "12px")
    	.attr("text-anchor", "start")
    	.attr("fill", "white");

    widget.tapbtn = widget.botlayer.append('button')
        .attr('class', 'tap-btn')
        .on('click',function(){
            widget.tafactor = 1;
            widget.update();
        }).html("+");
    widget.tambtn = widget.botlayer.append('button')
        .attr('class', 'tam-btn')
        .on('click',function(){
            widget.tafactor = -1;
            widget.update();
        }).html("-");
    widget.tambtn = widget.botlayer.append('button')
        .attr('class', 'taa-btn')
        .on('click',function(){
            widget.tafactor = undefined;
            widget.update();
        }).html("auto");

    var rst = {
    	x_new: widget.x_new,
    	x: widget.x,
    	y: widget.y,
    	playstop: false,
    	ref: {},
    	currentstep: 0,
    	currentspeed: 0,
    	brushsnap: 0,
    	brushselection: null,
    	brushtime: undefined,
    	tainterval: null,
    	tafactor: undefined,
    	zoomtransform: d3.zoomTransform(widget.ts.node())
    };

    widget.resetbtn = widget.botlayer.append('button')
    	.attr('class', 'rst-btn')
    	.on('click', function(){
    		play_stop = rst.playstop;
    		ref = rst.ref;
    		widget.x = rst.x;
    		widget.x_new = rst.x_new;
    		widget.y = rst.y;
    		currentstep = rst.currentstep;
    		currentspeed = rst.currentspeed;
    		brushsnap = rst.brushsnap;
    		widget.brushtime = rst.brushtime;
    		if(rst.tainterval !== null)
    			widget.interval = rst.tainterval;
    		widget.tafactor = rst.tafactor;
    		ashandle.attr("cx", asx(currentstep));
    		handle.attr("cx", sx(currentspeed));
    		bshandle.attr("cx", bsx(brushsnap));

    		widget.zoom.transform(widget.ts, rst.zoomtransform);
    		widget.brush.move(widget.gbrush, rst.brushselection);

    		widget.update();
    		widget.playTime(play_stop, currentspeed, currentstep, ref);
    	}).html("Reset");

    widget.sdbtn = widget.botlayer.append('button')
    	.attr('class', 'rst-btn')
    	.on('click', function(){
    		rst.x_new = widget.x_new;
    		rst.x = widget.x;
    		rst.y = widget.y;
    		rst.playstop = play_stop;
    		rst.ref = ref;
    		rst.currentstep = currentstep;
    		rst.currentspeed = currentspeed;
    		rst.brushsnap = brushsnap;
    		rst.brushselection = d3.brushSelection(widget.gbrush.node());
    		rst.brushtime = widget.brushtime;
    		rst.tainterval = widget.interval;
    		rst.tafactor = widget.tafactor;
    		rst.zoomtransform = d3.zoomTransform(widget.ts.node());

    	}).html("Set default");

    widget.margin = margin;
    widget.width = width;
    widget.height = height;
    widget.gX = gX;
    widget.gY = gY;
    widget.iterating = false;

}

Timeseries.prototype={
	update: function(){
        var widget = this;
        var sel = this.getSelection();
        var start = sel.global.start;
        var end = sel.global.end;

        if(widget.tafactor === undefined){
        	widget.interval = (end - start+1) / 1000 / this.width * 1;
        }
        else{
        	widget.interval = widget.interval * Math.pow(2, widget.tafactor);
        	if(widget.interval < widget.unitTime)
        		widget.interval = widget.unitTime;
        	widget.tafactor = 0;
        }

        //Round to nearest time unit
        var i = widget.interval;
        if(Math.floor(i / (3600 * 24 * 365)) > 0)
        	widget.interval = Math.floor(i / (3600 * 24 * 365)) * (3600 * 24 * 365);
        else if(Math.floor(i / (3600 * 24 * 7)) > 0)
        	widget.interval = Math.floor(i / (3600 * 24 * 7)) * (3600 * 24 * 7);
        else if(Math.floor(i / (3600 * 24)) > 0)
        	widget.interval = Math.floor(i / (3600 * 24)) * (3600 * 24);
        else if(Math.floor(i / 3600) > 0)
        	widget.interval = Math.floor(i / 3600) * 3600;
        else if(Math.floor(i / 60) > 0)
        	widget.interval = Math.floor(i / 60) * 60;

        //updating time aggregation text

        widget.tatext.text(function(){
    		// console.log(widget.interval);
    		var bucketsize = widget.interval / widget.unitTime;
        	bucketsize = Math.max(1,Math.floor(bucketsize+0.5));

    		return "Unit Time: " + widget.timeUnit(widget.unitTime) + 
    			" Current time aggregation: " + 
    			widget.timeUnit(widget.unitTime * bucketsize);
    	});

        var promises = {};

        //generate promise for each expr
        for (var d in widget._datasrc){
            if (widget._datasrc[d].disabled){
                continue;
            }
            var p;
            try{
            	p = this.getDataCallback(d,start, end, widget.interval);
            }
            catch(err){
            	return;
            }
            for (var k in p){
                promises[k] = p[k];
            }
        }
            
        var promarray = Object.keys(promises).map(function(k){
            return promises[k];
        });

        var promkeys = Object.keys(promises);
        console.log(promkeys);
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
        var timedom = this.x_new.domain();
        sel.global = {start:timedom[0], end:timedom[1]};

        widget = this;
        brushnode = this.gbrush.node();
        if (brushnode !== null && d3.brushSelection(brushnode) !== null){
            var bext = d3.brushSelection(brushnode).map(this.x_new.invert);
            widget.bstext.text(function(){
				return "(" + bext[0].toUTCString() + ", " + bext[1].toUTCString() + ")";
			});
            sel.brush = {start:bext[0], end:bext[1]};
        }
        else{
        	widget.bstext.text("No Brush Selected");
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
            this.brush.move(this.brushg, 
                            [this.x(new Date(args.brush.start)),
                             this.x(new Date(args.brush.end))]);
        }
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
        

        var widget = this;

        widget.y.domain(yext);

        widget.updateSVG();

        widget.x.range([0, widget.width]);
        widget.x_new.range([0, widget.width]);
		widget.y.range([widget.height, 0]);

		widget.xAxis.scale(widget.x_new)
			.tickSize(-widget.height);
			
		widget.yAxis.scale(widget.y)
		    .tickSize(-widget.width-3);

        //update the axis
        widget.gX.call(widget.xAxis)
        	.attr("transform", "translate(0," + this.height + ")");
        widget.gY.call(widget.yAxis);

        widget.brush.extent([[0,0], [this.width, this.height]]);
        widget.gbrush.call(widget.brush);

        if(widget.brushtime !== undefined){
    		widget.brush.move(widget.gbrush, widget.brushtime.map(widget.x_new));
    	}

        

        

        //Remove paths obsolete paths
        var paths = widget.ts.selectAll('path.line');
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
        var path = widget.ts.select('path.line.'+colorid);
        if (path.empty()){
            path = widget.ts.append('path');
            path.attr('class', 'line '+colorid);
            
            path.style('stroke-width','2px')
                .style('fill','none')
                .style('stroke',color);
        }


        //Transit to new data
        var lineFunc = d3.line()
                .x(function(d) { return widget.x_new(d.time); })
                .y(function(d) { return widget.y(d.val); })
                .curve(d3.curveStepBefore);
        var zeroFunc = d3.line()
                .x(function(d) { return widget.x_new(d.time); })
                .y(function(d) { return widget.y(0); });

        path.transition()
            .duration(500)
            .attr('d', lineFunc(data));
    },

    updateSVG: function(){

    	var widget = this;

    	var idwidth = parseFloat(d3.select(widget.toplayer.node().parentNode)
    		.style('width'));
    	var idheight = parseFloat(d3.select(widget.toplayer.node().parentNode)
    		.style('height'));
    	var width = idwidth - this.margin.left - this.margin.right - 60;
    	var height;

    	if(idwidth < 1200){
    		widget.toplayer.style("height", 80 + "px");
    		height = idheight - this.margin.top - this.margin.bottom - 110;
    	}
    	else{
    		widget.toplayer.style("height", 30 + "px");
    		height = idheight - this.margin.top - this.margin.bottom - 70;
    	}

    	widget.toplayer.style("width", idwidth + "px");
    	widget.midlayer.style("width", idwidth + "px");
    	widget.midlayer.style("height", height + this.margin.top + this.margin.bottom + "px");
    	widget.botlayer.style("width", idwidth + "px");

    	widget.tssvg.attr("width", width + this.margin.left + this.margin.right);
    	widget.tssvg.attr("height", height + this.margin.top + this.margin.bottom);
    	this.width = width;
    	this.height = height;
    },


    playTime: function(play_stop, speed, step, ref){
    	var widget = this;
    	if(play_stop){
    		widget.playbtn.html("Stop");
            if("repeat" in ref)
                clearInterval(ref.repeat);
            ref.repeat = setInterval(function(){
                widget.iterateTime(step, 1);
            }, (1000 - speed));

    	}
    	else{
    		widget.playbtn.html("Play");
            clearInterval(ref.repeat);
    	}
    },

    iterateTime: function(step, direction){
    	var bsel = d3.brushSelection(widget.gbrush.node());
        if(bsel === null)
        	return;
        var asfunc = [d3.utcHour, d3.utcDay, d3.utcWeek, d3.utcMonth, d3.utcYear];
        var newbsel;
        if(step === 0){
        	var diff = bsel[1] - bsel[0];
        	newbsel = [bsel[0] + (diff * direction), bsel[1] + (diff * direction)];
        }
        else{
        	bseldate = bsel.map(widget.x_new.invert);
        	newbsel = [asfunc[step-1].offset(bseldate[0], direction),
        			   asfunc[step-1].offset(bseldate[1], direction)]
        			   .map(widget.x_new);
        }
        widget.iterating = true;
        widget.brushtime = newbsel.map(widget.x_new.invert);
        widget.brush.move(widget.gbrush, newbsel);
        widget.updateCallback(widget._encodeArgs());
    },

    timeUnit: function(t){
    	var unit = 's';
    	if((t % 60) === 0 && Math.floor(t / 60) > 0){
    		t = t / 60;
    		unit = 'm';
    		if((t % 60) === 0 && Math.floor(t / 60) > 0){
	    		t = t / 60;
	    		unit = 'h';
	    		if((t % 24) === 0 && Math.floor(t / 24) > 0){
					t = t / 24;
					unit = 'd';
					if((t % 365) === 0 && Math.floor(t / 365) > 0){
			    		t = t / 365;
			    		unit = 'y';
			    	}
					else if((t % 7) === 0 && Math.floor(t / 7) > 0){
			    		t = t / 7;
			    		unit = 'w';
			    	}
			    	
				}
	    	}
    	}
    	
    	return "" + t + unit;
    }

};
