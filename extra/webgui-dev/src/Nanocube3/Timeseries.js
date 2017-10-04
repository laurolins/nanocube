/*global $,d3 */

function Timeseries(opts,getDataCallback,updateCallback, getXYCallback){
    var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;

    //Make draggable and resizable
    d3.select(id).attr("class","timeseries resize-drag"); //add resize-drag later

    d3.select(id).on("divresize",function(){
        widget.update();
    });

    this._opts = opts;

    var name = opts.name;

    opts.numformat = opts.numformat || ",";

    this._datasrc = opts.datasrc;

    widget.getDataCallback = getDataCallback;
    widget.updateCallback =  updateCallback;
    widget.getXYCallback = getXYCallback;

    this.retbrush = {
        color:'',
        x:'',
        y:''
    };

    this.retx = ['default'];
    this.rety = ['default'];

    var margin = opts.margin;
    if (margin === undefined)
        margin = {top: 10, right: 30, bottom: 20, left: 50};

    widget.permleft = margin.left;

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

    widget.midleft = widget.midlayer.append("div")
        .style("width", 30)
        .style("height", height + margin.top + margin.bottom + "px")
        .attr("class", "midleft");

    widget.timespace = widget.midlayer.append("div")
        .style("width", width + margin.left + margin.right + "px")
        .style("height", height + margin.top + margin.bottom + "px")
        .attr("class", "timespace");

    widget.midright = widget.midlayer.append("div")
        .style("width", 30)
        .style("height", height + margin.top + margin.bottom + "px")
        .attr("class", "midright");

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
            //iterates current color brush
            var curcolor = widget.brushcolors[widget.currentcolor];

            if(d3.brushSelection(widget.anygbrush[curcolor].node()) !== null){
                widget.iterateTime(currentstep, 1);
            }
        }).html(">");

    this.playbtn = widget.toplayer.append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            var curcolor = widget.brushcolors[widget.currentcolor];
            if(d3.brushSelection(widget.anygbrush[curcolor].node()) !== null){
                play_stop = !play_stop;
                widget.playTime(play_stop, currentspeed, currentstep, ref);
            }
        }).html("Play");

    this.backbtn = widget.toplayer.append('button')
        .attr('class', 'play-btn')
        .on('click',function(){
            var curcolor = widget.brushcolors[widget.currentcolor];
            if(d3.brushSelection(widget.anygbrush[curcolor].node()) !== null){
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

    width = (width + margin.left + margin.right) - 
            (margin.left + margin.right) * widget.retx.length;
    height = (height + margin.top + margin.bottom) - 
            (margin.top + margin.bottom) * widget.rety.length;

    widget.x = d3.scaleUtc().range([0, width / widget.retx.length]);
    widget.y = d3.scaleLinear().range([height / widget.rety.length, 0]);

    widget.xAxis = d3.axisBottom(widget.x)
        .tickSize(-height / widget.rety.length);
        

    widget.yAxis = d3.axisLeft(widget.y)
        .ticks(3)
        .tickFormat(d3.format(opts.numformat))
        .tickSize(-(width / widget.retx.length)-3);

    //Zoom
    widget.x_new = widget.x;
    widget.zoom=d3.zoom()
        .extent([[0, 0], [width, height]])
        .on('zoom', function(){
            var t = d3.event.transform;
            widget.x_new = t.rescaleX(widget.x);
            Object.keys(widget.gX).map(function(i){
                Object.keys(widget.gX[i]).map(function(j){
                    widget.gX[i][j].call(widget.xAxis.scale(widget.x_new));
                });
            });

            Object.keys(widget.gbrush).map(function(i){
                Object.keys(widget.gbrush[i]).map(function(j){
                    Object.keys(widget.brushtime).map(function(color){
                        widget.brush[0].move(widget.gbrush[i][j][color],
                            widget.brushtime[color].map(widget.x_new));
                    });
                });
            });

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
    this.brushcolors = colorbrewer.Set1[5].slice(0).reverse(0);

    //BRUSH
    widget.brush = {};
    var col;
    for(col = 0; col < 5; col++){
        widget.brush[col] = d3.brushX()
            .extent([[0, 0], [width, height]])
            .on("end", brushend(col));
    }

    function brushend(bindex){
        return function(){
            // set brush for selected brush color
            var selcolor = widget.brushcolors[bindex];

            if(widget.iterating){
                widget.iterating = false;
                return;
            }
            if(!(d3.event.sourceEvent instanceof MouseEvent)) return;
            if(!d3.event.sourceEvent) return;
            if(!d3.event.selection){
                delete widget.brushtime[selcolor];
                Object.keys(widget.gbrush).map(function(i){
                    Object.keys(widget.gbrush[i]).map(function(j){
                        widget.brush[bindex].move(widget.gbrush[i][j][selcolor], null);
                    });
                });
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
            widget.brushtime[selcolor] = d1;
            Object.keys(widget.gbrush).map(function(i){
                Object.keys(widget.gbrush[i]).map(function(j){
                    widget.brush[bindex].move(widget.gbrush[i][j][selcolor], 
                                                d1.map(widget.x_new));
                });
            });
            widget.updateCallback(widget._encodeArgs());
        };
    }

    // Timeline Left Pan button
    var arc = d3.symbol().type(d3.symbolDiamond)
        .size([height] * 20);

    var leftpan = widget.midleft.append("svg")
        .attr("width", 30)
        .attr("height", height + margin.top + margin.bottom)
        .append('path')
        .attr('d', arc)
        .attr('fill', 'gray')
        .attr('stroke','#000')
        .attr('stroke-width',1)
        .attr('transform', 'translate(' + 30 + ',' + 
            (height + margin.top + margin.bottom)/2 + ')');
    
    var pan;
    leftpan.on("mouseover", function(){
        // console.log("Mouseover");
        leftpan.attr('fill', 'blue');
        pan = setInterval(function(){
            var transform = d3.zoomTransform(widget.anyts.node());
            widget.zoom.translateBy(widget.anyts, 20 / transform.k, 0);
        }, 10);
    });

    leftpan.on('mouseleave', function(){
        leftpan.attr('fill', 'gray');
        clearInterval(pan);
    });


    widget.brushnumber = 1;
    widget.currentcolor = 0;
    widget.brushtime = {};

    if(opts.args){
        widget._decodeArgs(opts.args);
    }
    else{
        widget.x.domain(opts.timerange);
    }

    // Timeline svg
    widget.tssvg = {};
    widget.ts = {};
    widget.gX = {};
    widget.gY = {};
    widget.gbrush = {};
    for (var j in widget.rety){
        var rj = widget.rety[j];
        widget.tssvg[rj] = {};
        widget.ts[rj] = {};
        widget.gX[rj] = {};
        widget.gY[rj] = {};
        widget.gbrush[rj] = {};
        for (var i in widget.retx){
            var ri = widget.retx[i];
            widget.tssvg[rj][ri] = widget.timespace.append("svg")
                .attr("width", (width/widget.retx.length) + margin.left + margin.right)
                .attr("height", (height/widget.rety.length) + margin.top + margin.bottom)
                .attr("class", "tssvg");

            widget.ts[rj][ri] = widget.tssvg[rj][ri].append("g")
                .attr("transform", "translate(" + (margin.left) + "," +
                      (margin.top)+ ")")
                .call(widget.zoom);

            widget.gX[rj][ri] = widget.ts[rj][ri].append("g")
                .attr("class", "axis axis--x")
                .attr("transform", "translate(0," + (height/widget.rety.length) + ")")
                .call(widget.xAxis);

            widget.gY[rj][ri] = widget.ts[rj][ri].append("g")
                .attr("class", "axis axis--y")
                .call(widget.yAxis);

            widget.gbrush[rj][ri] = {};

            for (col = 0; col < 5; col++){
                var curcol = this.brushcolors[col];
                widget.gbrush[rj][ri][curcol] = widget.ts[rj][ri]
                    .append("g")
                    .attr("class", "brush")
                    .call(widget.brush[col]);

                d3.select(widget.gbrush[rj][ri][curcol].node())
                    .select(".selection")
                    .attr("fill", curcol);
            }
            
        }
    }



    Object.keys(widget.ts).map(function(i){
        Object.keys(widget.ts[i]).map(function(j){
            widget.ts[i][j].on("mousedown", function(){d3.event.stopPropagation();});
        });
    });

    widget.anygbrush = widget.getAny(widget.gbrush);
    widget.anyts = widget.getAny(widget.ts);

    var rightpan = widget.midright.append("svg")
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
            var transform = d3.zoomTransform(widget.anyts.node());
            widget.zoom.translateBy(widget.anyts, -20 / transform.k, 0);
        }, 10);
    });

    rightpan.on('mouseleave', function(){
        rightpan.attr('fill', 'gray');
        clearInterval(pan2);
    });


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


    widget.brushbtn = {};
    for(col = 0; col < 5; col++){
        widget.brushbtn[col] = widget.botlayer.append('button')
            .attr('id',(name + col + 'brush'))
            .on('click', brushPEToggle(col))
            .html("Brush " + (col + 1));
        $('#' + name + col + 'brush').css('color', widget.brushcolors[col]);
        if(col > widget.brushnumber - 1){
            $('#' + name + col + 'brush').hide();
        }
    }

    widget.brushbtn[0].attr('class', 'clicked');
    $('#' + name + 0 + 'brush').click();

    function brushPEToggle(index){
        return function(){
            $('#' + name + index + 'brush').toggleClass('clicked');
            $('#' + name + widget.currentcolor + 'brush').toggleClass('clicked');
            widget.currentcolor = index;
            // for(var brushnum = 0; brushnum < 5; brushnum++){
            //     if(brushnum == index){
            //         widget.brush[brushnum].selectAll('.overlay')
            //             .style('pointer-events', 'all');
            //     }
            //     else{
            //         widget.brush[brushnum].selectAll('.overlay')
            //             .style('pointer-events', 'none');
            //     }
            // }

            Object.keys(widget.gbrush).map(function(i){
                Object.keys(widget.gbrush[i]).map(function(j){
                    for(var brushnum = 0; brushnum < 5; brushnum++){
                        var brushnode = widget.gbrush[i][j][widget.brushcolors[brushnum]].node();
                        if(brushnum == index){
                            console.log(brushnode);
                            d3.select(brushnode)
                                .selectAll('.overlay')
                                .attr('pointer-events', 'all');
                            var firstchild = brushnode.parentNode.children[2];
                            brushnode.parentNode.insertBefore(brushnode, firstchild);
                        }
                        else{
                            d3.select(brushnode)
                                .selectAll('.overlay')
                                .attr('pointer-events', 'none');
                        }
                    }
                });
            });
            widget.updateCallback(widget._encodeArgs());
        };
    }

    widget.newbrushbtn = widget.botlayer.append('button')
        .attr('class', 'newbrush-btn')
        .on('click', function(){
            if(widget.brushnumber == 5) return;
            $('#' + name + widget.brushnumber + 'brush').show();
            $('#' + name + widget.brushnumber + 'brush').click();
            // widget.currentcolor = brushnumber;
            widget.brushnumber += 1;
            widget.updateCallback(widget._encodeArgs());
        }).html("New Brush");

    widget.delbrushbtn = widget.botlayer.append('button')
        .attr('class', 'delbrush-btn')
        .on('click', function(){
            if(widget.brushnumber == 1) return;

            widget.brushnumber -= 1;
            $('#' + name + widget.brushnumber + 'brush').hide();

            //move brush[brushcolor] to null

            var lastbrush = widget.brushcolors[widget.brushnumber];

            delete widget.brushtime[lastbrush];
            Object.keys(widget.gbrush).map(function(i){
                Object.keys(widget.gbrush[i]).map(function(j){
                    widget.brush[widget.brushnumber].move(widget.gbrush[i][j][lastbrush], null);
                });
            });
            if(widget.brushnumber == widget.currentcolor){
                $('#' + name + (widget.brushnumber - 1) + 'brush').click();
            }
            widget.updateCallback(widget._encodeArgs());
        }).html("Del Brush");

    widget.margin = margin;
    widget.width = width;
    widget.height = height;
    // widget.gX = gX;
    // widget.gY = gY;
    widget.iterating = false;
    widget.compare = false;
    widget.name = name;
    widget.update();
}


Timeseries.brushcolors = colorbrewer.Set1[5].slice(0).reverse(0);

function arraysEqual(arr1, arr2) {
    if(arr1.length !== arr2.length)
        return false;
    for(var i = arr1.length; i--;) {
        if(arr1[i] !== arr2[i])
            return false;
    }

    return true;
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
        var wi = widget.interval;
        if(Math.floor(wi / (3600 * 24 * 365)) > 0)
            widget.interval = Math.floor(wi / (3600 * 24 * 365)) * (3600 * 24 * 365);
        else if(Math.floor(wi / (3600 * 24 * 7)) > 0)
            widget.interval = Math.floor(wi / (3600 * 24 * 7)) * (3600 * 24 * 7);
        else if(Math.floor(wi / (3600 * 24)) > 0)
            widget.interval = Math.floor(wi / (3600 * 24)) * (3600 * 24);
        else if(Math.floor(wi / 3600) > 0)
            widget.interval = Math.floor(wi / 3600) * 3600;
        else if(Math.floor(wi / 60) > 0)
            widget.interval = Math.floor(wi / 60) * 60;

        //updating time aggregation text

        widget.tatext.text(function(){
            // console.log(widget.interval);
            var bucketsize = widget.interval / widget.unitTime;
            bucketsize = Math.max(1,Math.floor(bucketsize+0.5));

            return "Unit Time: " + widget.timeUnit(widget.unitTime) + 
                " Current time aggregation: " + 
                widget.timeUnit(widget.unitTime * bucketsize);
        });

        var xydata = this.getXYCallback();
        // console.log(xydata);
        // console.log(this.retx, this.rety);

        if(!arraysEqual(this.retx,xydata[0]) || !arraysEqual(this.rety,xydata[1])){
            console.log("Rebuilding..");

            if(widget.yext){
                widget.margin.left = widget.yext.toString().length * 3.5;
            }
            this.retx = xydata[0];
            this.rety = xydata[1];

            widget.width = (widget.width + widget.permleft + widget.margin.right) - 
                            ((widget.margin.left + widget.margin.right) * widget.retx.length);
            widget.height = (widget.height + widget.margin.top + widget.margin.bottom) - 
                            ((widget.margin.top + widget.margin.bottom) * widget.rety.length);
            widget.x.range([0, widget.width / widget.retx.length]);
            widget.x_new = widget.x;
            widget.y.range([widget.height / widget.rety.length, 0]);

            widget.xAxis = d3.axisBottom(widget.x)
                .tickSize(-(widget.height / widget.rety.length));
                

            widget.yAxis = d3.axisLeft(widget.y)
                .ticks(3)
                .tickFormat(d3.format(widget._opts.numformat))
                .tickSize(-(widget.width / widget.retx.length));

            widget.timespace.selectAll("*").remove();
            widget.tssvg = {};
            widget.ts = {};
            widget.gX = {};
            widget.gY = {};
            widget.gbrush = {};
            widget.zoom.extent([[0,0], [widget.width/widget.retx.length,
                                        widget.height/widget.rety.length]]);
            for(var col = 0; col < 5; col++){
                widget.brush[col].extent([[0,0], [widget.width/widget.retx.length, 
                                                widget.height/widget.rety.length]]);
            }
            
            for (var j in widget.rety){
                var rj = widget.rety[j];
                widget.tssvg[rj] = {};
                widget.ts[rj] = {};
                widget.gX[rj] = {};
                widget.gY[rj] = {};
                widget.gbrush[rj] = {};
                for (var i in widget.retx){
                    var ri = widget.retx[i];
                    // console.log(ri);
                    widget.tssvg[rj][ri] = widget.timespace.append("svg")
                        .attr("width", (widget.width/widget.retx.length) + 
                                widget.margin.left + widget.margin.right)
                        .attr("height", (widget.height/widget.rety.length) +
                                    widget.margin.top + widget.margin.bottom)
                        .attr("class", "tssvg");

                    var xtext = widget.tssvg[rj][ri].append("text")
                        .attr("x", (widget.margin.left + widget.margin.right + 
                                    widget.width/widget.retx.length) / 2)
                        .attr("y", 10)
                        .attr("font-family", "sans-serif")
                        .attr("font-size", "10px")
                        .attr("text-anchor", "end")
                        .text("X COLOR    .");

                    if(ri != 'default')
                        xtext.attr("fill", ri);
                    else
                        xtext.attr("fill", "#ffffff");

                    var ytext = widget.tssvg[rj][ri].append("text")
                        .attr("x", (widget.margin.left + widget.margin.right + 
                                    widget.width/widget.retx.length) / 2)
                        .attr("y", 10)
                        .attr("font-family", "sans-serif")
                        .attr("font-size", "10px")
                        .attr("text-anchor", "start")
                        .text(".     Y COLOR");

                    if(rj != 'default')
                        ytext.attr("fill", rj);
                    else
                        ytext.attr("fill", "#ffffff");

                    widget.ts[rj][ri] = widget.tssvg[rj][ri].append("g")
                        .attr("transform", "translate(" + (widget.margin.left) + "," +
                              (widget.margin.top) + ")")
                        .call(widget.zoom);

                    widget.gX[rj][ri] = widget.ts[rj][ri].append("g")
                        .attr("class", "axis axis--x")
                        .attr("transform", "translate(0," + (widget.height/widget.rety.length) + ")")
                        .call(widget.xAxis);

                    widget.gY[rj][ri] = widget.ts[rj][ri].append("g")
                        .attr("class", "axis axis--y")
                        .call(widget.yAxis);

                    widget.gbrush[rj][ri] = {};

                    for (col = 0; col < 5; col++){
                        var curcol = widget.brushcolors[col];
                        widget.gbrush[rj][ri][curcol] = widget.ts[rj][ri]
                            .append("g")
                            .attr("class", "brush")
                            .call(widget.brush[col]);

                        d3.select(widget.gbrush[rj][ri][curcol].node())
                            .select(".selection")
                            .attr("fill", curcol);
                    }

                }
            }

            $('#' + widget.name + widget.currentcolor + 'brush').click();

            Object.keys(widget.ts).map(function(a){
                Object.keys(widget.ts[a]).map(function(b){
                    widget.ts[a][b].on("mousedown", function(){d3.event.stopPropagation();});
                });
            });

            widget.anygbrush = widget.getAny(widget.gbrush);
            widget.anyts = widget.getAny(widget.ts);
        }

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
                console.log(err);
                return;
            }
            for (var k in p){
                promises[k] = p[k];
            }
        }
            
        var promarray = Object.keys(promises).map(function(k){
            return promises[k];
        });

        // console.log(promises);

        var promkeys = Object.keys(promises);
        $.when.apply($,promarray).done(function(){
            var results = arguments;
            var res = {};
            Object.keys(widget.ts).map(function(a){
                res[a] = {};
                Object.keys(widget.ts[a]).map(function(b){
                    res[a][b] = {};
                    promkeys.forEach(function(d,i){
                        // res[d] = results[i];

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

                        if(ret.c){
                            res[a][b][ret.c] = results[i];
                            res[a][b][ret.c].color = ret.c;
                        }
                        else{
                            res[a][b].global = results[i];
                            var colormap = widget._datasrc[label[1]].colormap;
                            var cidx = Math.floor(colormap.length/2);
                            res[a][b].global.color = colormap[cidx];
                        }
                    });
                });
            });

            // console.log(res);
            widget.lastres = res;
            widget.redraw(res);
            
        });

    },

    getSelection: function(){
        var sel = {};
        var timedom = this.x_new.domain();
        sel.global = {start:timedom[0], end:timedom[1]};
        var curcolor = this.brushcolors[this.currentcolor];

        var widget = this;
        for(var i = 0; i < widget.brushnumber; i++){
            var selcolor = this.brushcolors[i];
            var brushnode = this.anygbrush[selcolor].node();
            if (brushnode !== null && d3.brushSelection(brushnode) !== null){
                var bext = d3.brushSelection(brushnode).map(this.x_new.invert);
                if(i == widget.currentcolor){
                    widget.bstext.text("(" + bext[0].toUTCString() + ", " + 
                        bext[1].toUTCString() + ")");
                }
                sel[selcolor] = {start:bext[0], end:bext[1]};
            }
            else{
                if(i == widget.currentcolor){
                    widget.bstext.text("No Brush Selected");
                }
                // sel[selcolor] = undefined;
            }
        }

        if(sel.hasOwnProperty(curcolor))
            sel.brush = sel[curcolor];
        else
            sel.brush = sel.global;

        return sel;
    },

    _encodeArgs: function(){
        var args = this.getSelection();
        Object.keys(args).map(function(color){
            if(color.startsWith("#")){
                args[color.substr(1)] = {start: args[color].start,
                                         end: args[color].end};
                delete args[color];
            }
        });
        return JSON.stringify(args);
    },
    
    _decodeArgs: function(s){
        
        var widget = this;
        console.log(s);
        var args = JSON.parse(s);
        this.x.domain([new Date(args.global.start),
                       new Date(args.global.end)]);

        var xydata = this.getXYCallback();
        widget.retx = xydata[0];
        widget.rety = xydata[1];

        var colors = Object.keys(args);

        for(var col = 0; col < 5; col++){
            var curcolor = widget.brushcolors[col];
            if(colors.indexOf(curcolor.substr(1)) != -1){
                widget.brushnumber = col + 1;
                widget.brushtime[curcolor] = [args[curcolor.substr(1)].start, 
                                              args[curcolor.substr(1)].end];
            }
        }

    },

    redraw: function(res){
        // console.log(res);
        Object.keys(res).map(function(i){
            Object.keys(res[i]).map(function(j){
                var lines = res[i][j];
                var empty = true;
                Object.keys(lines).forEach(function(k){
                    if(lines[k].data.length > 1){ 
                        var last = lines[k].data[lines[k].data.length-1];
                        lines[k].data.push(last); //dup the last point for step line
                        empty = false;
                    }
                    else{
                        delete res[i][j][k];
                    }
                });
                if(empty)
                    delete res[i][j];
            });
        });

        //update y axis
        var yext = Object.keys(res).reduce(function(p1,c1){
            var g = Object.keys(res[c1]).reduce(function(p2,c2){
                var f = Object.keys(res[c1][c2]).reduce(function(p3,c3){
                    var e = d3.extent(res[c1][c2][c3].data, function (d){
                        return (d.val || 0);
                    });
                    return [Math.min(p3[0],e[0]),
                            Math.max(p3[1],e[1])];
                }, [Infinity,-Infinity]);
                return [Math.min(p2[0],f[0]),
                        Math.max(p2[1],f[1])];
            }, [Infinity,-Infinity]);
            return [Math.min(p1[0],g[0]),
                    Math.max(p1[1],g[1])];  
        }, [Infinity,-Infinity]);

        
        // var yext = Object.keys(lines).reduce(function(p,c){
        //     var e = d3.extent(lines[c].data, function(d){
        //         return (d.val || 0);
        //     });
        //     return [ Math.min(p[0],e[0]),
        //              Math.max(p[1],e[1])];
        // }, [Infinity,-Infinity]);


        yext[0]= yext[0]-0.05*(yext[1]-yext[0]); //show the line around min
        yext[0]= Math.min(yext[0],yext[1]*0.5);

        var widget = this;


        widget.yext = yext;

        widget.updateSVG();

        widget.x.range([0, widget.width / widget.retx.length]);
        widget.x_new.range([0, widget.width / widget.retx.length]);
        widget.y.range([widget.height / widget.rety.length, 0]);
        widget.y.domain(yext);


        widget.xAxis.scale(widget.x_new)
            .tickSize(-widget.height / widget.rety.length);
            
        widget.yAxis.scale(widget.y)
            .tickSize(-(widget.width / widget.retx.length)-3);

        
        Object.keys(widget.ts).map(function(i){
            Object.keys(widget.ts[i]).map(function(j){

                //update the axis
                widget.gX[i][j].call(widget.xAxis)
                    .attr("transform", "translate(0," + 
                        (widget.height / widget.rety.length) + ")");
                widget.gY[i][j].call(widget.yAxis);

                for(k = 0; k < 5; k++){
                    var curcol = widget.brushcolors[k];
                    widget.brush[k].extent([[0,0], [widget.width / widget.retx.length, 
                                                 widget.height/ widget.rety.length]]);
                    widget.gbrush[i][j][curcol].call(widget.brush[k]);
                    if(widget.brushtime[curcol] !== undefined){
                        widget.brush[k].move(widget.gbrush[i][j][curcol], 
                                          widget.brushtime[curcol].map(widget.x_new));
                    }
                }

                //Remove paths obsolete paths
                var paths = widget.ts[i][j].selectAll('path.line');
                paths.each(function(){
                    var p = this;
                    var exists;
                    if(res[i][j] === undefined)
                        exists = false;
                    else{
                        exists = Object.keys(res[i][j]).some(function(d){
                            return d3.select(p).classed(d);
                        });
                    }
                    if (!exists){ // remove obsolete
                        d3.select(p).remove();
                    }
                });

                if(res[i][j] !== undefined){
                    //Draw Lines
                    Object.keys(res[i][j]).forEach(function(k){
                        res[i][j][k].data.sort(function(a,b){return a.time - b.time;});
                        widget.drawLine(res[i][j][k].data,res[i][j][k].color,i,j);
                    });
                }


            });
        });
        
    },

    drawLine:function(data,color,i,j){
        var colorid = 'color_'+color.replace('#','');


        var widget = this;

        if (data.length < 2)
            return;
        
        //create unexisted paths
        var path = widget.ts[i][j].select('path.line.'+colorid);

        if (path.empty()){
            path = widget.ts[i][j].append('path');
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
        var width = idwidth - (this.margin.left + this.margin.right) * this.retx.length - 60;
        var height;

        if(idwidth < 1200){
            widget.toplayer.style("height", 80 + "px");
            height = idheight - ((this.margin.top + this.margin.bottom) * this.rety.length) - 110;
        }
        else{
            widget.toplayer.style("height", 40 + "px");
            height = idheight - ((this.margin.top + this.margin.bottom) * this.rety.length) - 70;
        }

        widget.toplayer.style("width", idwidth + "px");
        widget.midlayer.style("width", idwidth + "px");
        widget.midlayer.style("height", height +
                            ((this.margin.top + this.margin.bottom) * this.rety.length) + "px");
        widget.midleft.style("height", height +
                            ((this.margin.top + this.margin.bottom) * this.rety.length) + "px");
        widget.midright.style("height", height +
                            ((this.margin.top + this.margin.bottom) * this.rety.length) + "px");
        widget.timespace.style("width", idwidth - 60 + "px");
        widget.timespace.style("height", height + 
                            ((this.margin.top + this.margin.bottom) * this.rety.length) + "px");
        widget.botlayer.style("width", idwidth + "px");

        Object.keys(widget.tssvg).map(function(i){
            Object.keys(widget.tssvg[i]).map(function(j){

                widget.tssvg[i][j].attr("width", (width/widget.retx.length) + 
                                        widget.margin.left + widget.margin.right);
                widget.tssvg[i][j].attr("height", (height/widget.rety.length) + 
                                        widget.margin.top + widget.margin.bottom);

            });
        });
        
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
        var widget = this;
        var curcolor = widget.brushcolors[widget.currentcolor];
        var bsel = d3.brushSelection(widget.anygbrush[curcolor].node());
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
        widget.brushtime[curcolor] = newbsel.map(widget.x_new.invert);
        Object.keys(widget.gbrush).map(function(i){
            Object.keys(widget.gbrush[i]).map(function(j){
                widget.brush[widget.currentcolor].move(widget.gbrush[i][j][curcolor], newbsel);
            });
        });
        
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
    },

    getAny: function(obj){
        var temp = obj[Object.keys(obj)[0]];
        return temp[Object.keys(temp)[0]];
    },

    adjustToCompare: function(){
        return;
    }

};



