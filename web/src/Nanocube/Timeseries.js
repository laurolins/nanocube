//JQuery
import jquery from 'jquery';
let $ = window.$ = jquery;

//d3
import * as d3 from 'd3';

//fontawesome
import { library, dom } from '@fortawesome/fontawesome-svg-core';
import { faForward } from '@fortawesome/free-solid-svg-icons/faForward';
import { faPlay } from '@fortawesome/free-solid-svg-icons/faPlay';
import { faBackward } from '@fortawesome/free-solid-svg-icons/faBackward';
import { faPause } from '@fortawesome/free-solid-svg-icons/faPause';

library.add(faForward,faPlay,faBackward,faPause);
dom.watch();

function Timeseries(opts,getDataCallback,updateCallback){
    var id = '#'+ opts.name.replace(/\./g,'\\.');
    var widget = this;
    this._opts = opts;
    
    //Make draggable and resizable
    d3.select(id).attr("class","timeseries");

    d3.select(id).on("divresize",function(){
        widget.redraw(widget.lastres);
    });

    //Add playback buttons
    d3.select(id).append('button')
        .attr('class','btn')
        .on('click',function(){
            widget.moveOneStep();
        })
        .append('i').attr('class','fas fa-forward');
    
    d3.select(id).append('button')
        .attr('class','btn')
        .on('click',function(){
            let btn = d3.select(this);
            let icon = btn.select('[data-fa-i2svg]');
            if(widget.animationStartStop()){
                    icon.attr('class', 'fas fa-pause');
            }
            else{
                icon.attr('class', 'fas fa-play');
            }
        })
        .append('i').attr('class', 'fas fa-play');

    d3.select(id).append('button')
        .attr('class','btn')
        .on('click',function(){
            widget.moveOneStep(false);
        })
        .append('i').attr('class', 'fas fa-backward');


    //Add percent button
    this.percentbtn = d3.select(id).append('button')
        .on('click',function(event){
            event.stopPropagation();
            widget._opts.percent = !widget._opts.percent;
            widget.redraw(widget.lastres);
        });
    
    //num format
    opts.numformat = opts.numformat || ",";
    this._datasrc = opts.datasrc;

    //Set time limit
    this.timelimits = opts.timelimits;

    
    widget.getDataCallback = getDataCallback;
    widget.updateCallback =  updateCallback;


    var margin = opts.margin;
    if (margin===undefined){
        margin = {top: 30, right: 20, bottom: 35, left: 35};
    }

    var width = $(id).width() - margin.left - margin.right;
    var height = $(id).height() - margin.top - margin.bottom;

    widget.x = d3.scaleUtc().range([0, width]);
    widget.y = d3.scaleLinear().range([height, 0]);

    //x axis
    widget.xz = widget.x; //zoomed scale

    widget.xAxis = d3.axisBottom(widget.x)
        .tickSizeInner(-height);

    widget.yAxis = d3.axisLeft(widget.y)
        .ticks(3)
        .tickFormat(d3.format(opts.numformat))
        .tickSizeInner(-width-3);

    //SVG
    widget.svg = d3.select(id)
        .append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
        .append("g")
        .attr("transform", "translate(" + margin.left + "," +
              margin.top + ")");

    //add svg stuffs
    //add title
    widget.svg.append("text")
        .attr("x", -10)
        .attr("y", -10)
        .text(opts.title);

    widget.svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + height + ")")
        .call(widget.xAxis);

    widget.svg.append("g")
        .attr("class", "y axis")
        .attr("transform", "translate(-3,0)")
        .call(widget.yAxis);


    //Zoom    
    widget.zoom=d3.zoom()
        .on('zoom', function(event){
            //ignore mousemove without button down
            if(event.sourceEvent.buttons < 1 && 
               event.sourceEvent.type=="mousemove"){
                return;
            }
            
            //rescale
            widget.xz=event.transform.rescaleX(widget.x);

            //redraw
            widget.redraw(widget.lastres);

            //update brush
            if(widget.brush.selection){
                widget.svg.select('g.brush')
                    .call(widget.brush.move,
                          widget.brush.selection.map(widget.xz));
            }
        })
        .on('end', function(){
            widget.update();
            widget.updateCallback(widget._encodeArgs());
        });


    //apply zoom to axis
    widget.svg.call(widget.zoom);

    //Brush
    widget.brush = d3.brushX()
        .extent([[0, 0], [width, height]])
        .on('end', function(event){
            if(!event.sourceEvent){
                return;
            }
            
            if (event.selection) {
                var sel = event.selection;
                //save selection
                widget.brush.selection = sel.map(widget.xz.invert);
            }
            else{
                delete widget.brush.selection;
            }
            widget.updateCallback(widget._encodeArgs());
        });

    widget.svg.append("g")
        .attr("class", "brush").call(widget.brush);

    //Load config
    if(opts.args){
        widget._decodeArgs(opts.args);
    }
    else{
        //set initial domain
        widget.xz.domain(opts.timerange);
    }

    widget.animating = null;
    widget.width = width;
}

Timeseries.prototype={
    update: function(){
        let display = d3.select('#'+this._opts.tab).style('display');
        if(display == 'none'){ // no need to draw
            return;
        }
        
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
        var timedom = this.xz.domain();
        sel.global = {start:timedom[0], end:timedom[1]};

        if (this.brush.selection){
            var bext = this.brush.selection;
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
            this.brush.selection = [new Date(args.brush.start),
                                    new Date(args.brush.end)];
            this.svg.select('g.brush')
                .call(this.brush.move,
                      this.brush.selection.map(this.xz));
        }
    },

    redraw: function(olines){
        let lines = JSON.parse(JSON.stringify(olines));
        Object.keys(lines).forEach(function(k){  //Fix time after serialization
            lines[k].data = lines[k].data.map(function(d){
                return {time:new Date(d.time), val:+d.val};
            });
        });
        
        let opts = this._opts;
        let percentbtn = this.percentbtn;
        if (opts.percent){            
            percentbtn.html('raw');
            this._numformat='0.2%';

            //compute percentage
            Object.keys(lines).forEach(function(k){
                var total=lines[k].data.reduce(function(x,y){ return x+y.val;},0);
                lines[k].data.forEach(function(d,i){
                    lines[k].data[i].val /=total;
                });
            });
        }
        else{ //sort by data value
            percentbtn.html('%');            
            this._numformat = this._opts.numformat;
        }

        //construct the lines
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

        //update y axis
        this.yAxis.ticks(3).tickFormat(d3.format(this._numformat));
        
        //update axis
        this.svg.select(".y.axis").call(this.yAxis.scale(this.y));
        this.svg.select(".x.axis").call(this.xAxis.scale(this.xz));

        //Remove paths obsolete paths
        var widget = this;
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
        var m = color.match(/rgba\((.+),(.+),(.+),(.+)\)/);
        if(m){
            color='#'+
                (+m[1]).toString(16).padStart(2, '0')+
                (+m[2]).toString(16).padStart(2, '0')+
                (+m[3]).toString(16).padStart(2, '0');
        }
        
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
            .x(function(d) { return widget.xz(d.time); })
            .y(function(d) { return widget.y(d.val); })
            .curve(d3.curveStepAfter);

        path.transition()
            .duration(500)
            .attr('d', lineFunc(data));
    },

    moveOneStep: function(forward,stepsize){
        var widget = this;
        if(!widget.brush.selection){
            return;
        }

        if(forward == undefined){
            forward = true;
        }

        if(stepsize == undefined){
            stepsize = widget.brush.selection[1]-widget.brush.selection[0];
        }

        if(!forward){
            stepsize = -stepsize;
        }

        //move the selection
        var sel = [widget.brush.selection[0].getTime()+stepsize,
                   widget.brush.selection[1].getTime()+stepsize];

        //cycle
        if(sel[0] > widget.timelimits[1]){
            sel = [widget.timelimits[0],
                   +widget.timelimits[0]+(sel[1]-sel[0])];
        }
        
        if(sel[1] < widget.timelimits[0]){
            sel = [+widget.timelimits[1]-(sel[1]-sel[0]),
                   widget.timelimits[1]];            
        }        

        //make Dates
        var newsel = [Math.min.apply(null, sel),
                      Math.max.apply(null, sel)].map(function(d){
                          return new Date(d);
                      });

        widget.brush.selection = newsel; 
        
        //move the domain if needed
        //use + to convert dates to int for date arithmetic
        var xzdom = widget.xz.domain();
        if(xzdom[1] < newsel[1]){
            widget.xz.domain([+newsel[1]-(xzdom[1]-xzdom[0]),
                              newsel[1]]);
        }

        if(xzdom[0] > newsel[0]){
            widget.xz.domain([newsel[0],
                              +newsel[0]+(xzdom[1]-xzdom[0])]);
        }

        //move the brush
        widget.svg.select('g.brush')
            .call(widget.brush.move,
                  widget.brush.selection.map(widget.xz));

        widget.update(); //redraw itself
        widget.updateCallback(widget._encodeArgs());
    },

    animationStartStop: function(){
        var widget = this;

        if(!widget.brush.selection){ //no selection
            return false;
        }

        if(this.animating==null){
            this.animating = window.setInterval(function(){
                widget.moveOneStep();
            }, 1000);
            return true;
        }
        else{
            window.clearInterval(widget.animating);
            widget.animating=null;
        }
        return false;
    }
};

export default Timeseries;
