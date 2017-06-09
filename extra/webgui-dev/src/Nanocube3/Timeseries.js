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

    opts.numformat = opts.numformat || ",";

    this._datasrc = opts.datasrc;

    widget.getDataCallback = getDataCallback;
    widget.updateCallback =  updateCallback;


    var margin = opts.margin;
    if (margin === undefined)
        margin = {top: 10, right: 30, bottom: 20, left: 30};

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height() - margin.top - margin.bottom - 60;
    								//30 from sliders above and below

    widget.x = d3.scaleUtc().range([0, width]);
    widget.y = d3.scaleLinear().range([height, 0]);

    widget.xAxis = d3.axisBottom(widget.x)
    	.tickSize(-height);
    	

    widget.yAxis = d3.axisLeft(widget.y)
    	.ticks(3)
        .tickFormat(d3.format(opts.numformat))
        .tickSize(-width-3);

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

	//Animation step slider
	var asx = d3.scaleLinear()
    	.domain([1, 0])
    	.range([0, 150])
    	.clamp(true);

    widget.asslider = d3.select(id).append("svg")
    	.attr("width", 200)
    	.attr("height", 30)
    	.attr("class", "as-slider")
    	.append("g")
    	.attr("transform", "translate(" + 25 + "," + 10 + ")");

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
    			var h = asx.invert(d3.event.x);
    			currentstep = h;
    			ashandle.attr("cx", asx(h));
    			widget.playTime(play_stop, currentspeed, h, ref);
    		}));

    widget.asslider.insert("g", ".track-overlay")
    	.attr("class", "ticks")
    	.attr("transform", "translate(0," + 18 + ")")
    .selectAll("text")
    .data([0, 0.5, 1])
    .enter().append("text")
    	.attr("x", asx)
    	.attr("text-anchor", "middle")
    	.text(function(d) { return d + " step";});

    var ashandle = widget.asslider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);




	//Speed Slider
    var sx = d3.scaleLinear()
    	.domain([0, 999])
    	.range([0, 200])
    	.clamp(true);

    widget.slider = d3.select(id).append("svg")
    	.attr("width", 250)
    	.attr("height", 30)
    	.attr("class", "spd-slider")
    	.append("g")
    	.attr("transform", "translate(" + 25 + "," + 10 + ")");

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
    	.text(function(d) { return (1000 - d) + " ms";});

    var handle = widget.slider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);

	//Brush play button
    var play_stop = false;
    var ref = {};
    var currentspeed = 0;
    var currentstep = 1;
    this.playbtn = d3.select(id)
        .append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            if(d3.brushSelection(widget.gbrush.node()) !== null){
                play_stop = !play_stop;
                widget.playTime(play_stop, currentspeed, currentstep, ref);
            }
        }).html("Play");

    //Brush snapping slider
    var bslist = ["Hour", "Day", "Week", "Month", "Year"];

    var bsx = d3.scaleLinear()
    	.domain([0, 4])
    	.range([0, 150])
    	.clamp(true);

    widget.bsslider = d3.select(id).append("svg")
    	.attr("width", 200)
    	.attr("height", 30)
		.append("g")
    	.attr("class", "slider")
    	.attr("transform", "translate(" + 25 + "," + 10 + ")");

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
    	.text(function(d) { return bslist[d];});

    var bshandle = widget.bsslider.insert("circle", ".track-overlay")
    	.attr("class", "handle")
    	.attr("r", 6);

    //Brush Selection Text
    widget.bstextsvg = d3.select(id).append("svg")
    	.attr("width", 400)
    	.attr("height", 30);

    widget.bstext = widget.bstextsvg.append("text")
    	.attr("x", 10)
    	.attr("y", 15)
    	.text("No Brush Selected")
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "12px")
    	.attr("text-anchor", "start")
    	.attr("fill", "white");


    // Timeline svg
    widget.svg = d3.select(id).append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .attr("class", "resize")
        .append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")")
        .call(widget.zoom)
        .on("mousedown", function() { d3.event.stopPropagation(); });

    if(opts.args){
    	widget._decodeArgs(opts.args);
    }
    else{
    	widget.x.domain(opts.timerange);
    }

    // widget.svg.append("text")
    //     .attr("x", -10)
    //     .attr("y", -10)
    //     .text(opts.title);

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

    widget.tatext = d3.select(id).append("svg")
    	.attr("width", 400)
    	.attr("height", 30)
    	.append("text")
    	.attr("x", 10)
    	.attr("y", 15)
    	.attr("font-family", "sans-serif")
    	.attr("font-size", "12px")
    	.attr("text-anchor", "start")
    	.attr("fill", "white")
    	.text(function(){
    		return "Current time aggregation: ";
    	});

    

    widget.width = width;
    widget.gX = gX;
    widget.gY = gY;

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
            console.log(res);
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
        
        this.y.domain(yext);

        //update the axis
        this.gX.call(this.xAxis);
        this.gY.call(this.yAxis);

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


    playTime: function(play_stop, speed, step, ref){
    	var widget = this;
    	if(play_stop){
    		widget.playbtn.html("Stop");
            if("repeat" in ref)
                clearInterval(ref.repeat);
            ref.repeat = setInterval(function(){
                var bsel = d3.brushSelection(widget.gbrush.node());
                if(newbsel === null)
                	return;
                var diff = (bsel[1] - bsel[0]) * step;
                var newbsel = [bsel[0] + diff, bsel[1] + diff];
                widget.brushtime = newbsel.map(widget.x_new.invert);
                widget.brush.move(widget.gbrush, newbsel);
                widget.update();
                widget.updateCallback(widget._encodeArgs());
            }, (1000 - speed));

    	}
    	else{
    		widget.playbtn.html("Play");
            clearInterval(ref.repeat);
    	}
    }

};