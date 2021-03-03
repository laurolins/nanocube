import jquery from 'jquery';
let $ = window.$ = jquery;

import * as d3 from 'd3';

function GroupedBarChart(opts, getDataCallback, updateCallback){
    this.getDataCallback=getDataCallback;
    this.updateCallback=updateCallback;

    var name=opts.name;
    var id = "#"+name.replace(/\./g,'\\.');
    var margin = {top: 20, right: 20, bottom: 30, left: 40};

    //set param
    this.selection = {global:[]};
    if(opts.args){ // set selection from arguments
        this._decodeArgs(opts.args);
    }
    
    var widget = this;
    //Make draggable and resizable
    d3.select(id).attr("class","barchart");
    
    d3.select(id).on("divresize",function(){
        widget.update();
    });

    //Add clear button
    this.clearbtn = d3.select(id)
        .append('button')
        .attr('class','btn')
        .on('click',function(event){
            event.stopPropagation();
            
            delete widget.selection.brush; //clear selection
            widget.update(); //redraw itself
            widget.updateCallback(widget._encodeArgs());            
        }).html('clear');
    
    //Add sort button
    this.sortbtn = d3.select(id)
        .append('button')
        .attr('class','btn')
        .on('click',function(event){
            event.stopPropagation();
            widget._opts.alpha_order = !widget._opts.alpha_order;
            widget.redraw(widget.lastres);
        })
        .html('A');
    
    //Add percent button
    this.percentbtn = d3.select(id)
        .append('button')
        .attr('class','percent-btn')
        .on('click',function(event){
            event.stopPropagation();
            widget._opts.percent = !widget._opts.percent;
            widget.redraw(widget.lastres);
        });
    
    //SVG container
    var svg = d3.select(id).append("svg").append("g");

    //Title
    svg.append('text').attr('y',-5);
    
    //Axes
    svg.append("g").attr("class", "y axis")
        .attr("transform", "translate(-3,0)");
    svg.append("g").attr("class", "x axis");
    
    //Scales
    var y0 = d3.scaleBand();
    var y1 = d3.scaleBand();
    var x = d3.scaleLinear();
    if (opts.logaxis){
        x = d3.scaleLog();
    }

    //Axis
    var xAxis = d3.axisBottom();
    var yAxis = d3.axisLeft();

    //set default values 
    this._numformat = opts.numformat = opts.numformat || ",";    
    if(!opts.hasOwnProperty('alpha_order')) {
        opts.alpha_order = true;
    }

    //set 'percent' or 'raw' display
    if(!opts.hasOwnProperty('percent')) {
        opts.percent = false;
    }
        
    //Save vars to "this"
    this.margin = margin;
    this.svg=svg;
    this.y0=y0;
    this.y1=y1;
    this.x=x;
    this.xAxis = xAxis;
    this.yAxis = yAxis;
    
    this._datasrc = opts.datasrc;
    this._opts = opts;
    this._logaxis = opts.logaxis;
    this._name = name;
}

GroupedBarChart.prototype = {
    getSelection: function(){        
        return this.selection;
    },
    
    _encodeArgs: function(){
        return JSON.stringify(this.getSelection());
    },
    
    _decodeArgs: function(s){
        this.selection = JSON.parse(s);
    },
    
    update: function(){        
        //check it
        if(this._opts.tab != null){ //if the chart is in a tab
            let display = d3.select('#'+this._opts.tab).style('display');
            if(display == 'none'){ // no need to draw
                return;
            }
        }
        
        var widget = this;        
        var promises = {};
        
        //generate promise for each expr
        for (var d in widget._datasrc){
            if (widget._datasrc[d].disabled){
                continue;
            }
            var p = this.getDataCallback(d);
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
            });

            //rename results by aliases
            var aliases = widget._opts.aliases || {};
            Object.keys(res).forEach(d =>{
                var data = res[d].data;
                res[d].data.forEach(dd => {
                    if(dd['cat'] in aliases){
                        dd['cat'] = aliases[dd['cat']];
                    }
                });
            });
            
            
            widget.lastres = res;
            widget.redraw(res);
        });
    },
    
    flattenData: function(res){
        var widget = this;        
        return Object.keys(res).reduce(function(prev,curr){         
            var label = curr.split('&-&'); 
            var c = label[0];

            var isColor  = /^#[0-9A-F]{6}$/i.test(label[0]);                
            if(!isColor){
                var colormap = widget._datasrc[label[1]].colormap;
                var cidx = Math.floor(colormap.length/2);
                c = colormap[cidx];
            }
            
            //Add color
            var row = res[curr].data.map(function(d){
                d.color = c;
                return d;
            });
            return prev.concat(row);
        }, []);
    },

    redraw :function(res){        
        var topn = this._opts.topn;
        if(topn > 0){
            var agg = {};
            Object.keys(res).forEach(function(k){
                res[k].data.forEach(function(d){
                    agg[d.cat]= (agg[d.cat] + d.val) || d.val;
                });
            });
            var kvlist =Object.keys(agg)
                .map(function(d){return {cat: d, val:agg[d]};});
            kvlist.sort(function(x,y) { return y.val - x.val; });
            kvlist = kvlist.slice(0,topn);
            var kvhash = {};
            kvlist.forEach(function(d){ kvhash[d.cat] = d.val; });
            Object.keys(res).forEach(function(k){
                res[k].data = res[k].data.filter(function(d){
                    return (d.cat in kvhash);
                });
            });
            console.log(res);
        }

        var fdata = this.flattenData(res);
        
        var x =this.x;
        var y0 =this.y0;
        var y1 =this.y1;
        var svg =this.svg;
        var selection = this.selection;
        
        
        //update the axis and svgframe
        fdata = this.updateYAxis(fdata);
        this.updateXAxis(fdata);
        this.updateSVG();

        var widget = this;
        
        //bind data
        var bars = this.svg.selectAll('.bar').data(fdata);

        //exit
        bars.exit()
            .transition()
            .duration(100)
            .remove();

        //append
        var newbars = bars.enter()
            .append('rect')
            .attr('class', 'bar')
            .on('click', function(event,d) {
                widget.clickFunc(event,d); //toggle callback
            });

        newbars.append('svg:title');

        //merge new with old
        bars = newbars.merge(bars);

        //update
        bars.attr('x',0)
            .attr('y', function(d){
                return widget.y0(d.cat)+widget.y1(d.color);
            })
            .attr('height',function(d){
                return Math.max(0,widget.y1.bandwidth()-1);
            })
            .attr('width',function(d){
                return Math.max(0,widget.x(d.val));
            })
            .style('fill', function(d){
                if (!widget.selection.brush || //no selection
                    widget.selection.brush.findIndex(function(b){
                        return (b.cat == d.cat); }) != -1){//in selection
                    return d.color;
                }
                else{
                    return 'gray';
                }
            });

        //add tool tip
        bars.select('title').text(function(d){
            return d3.format(widget._numformat)(d.val);
        });
    },

    clickFunc:function(event,d){
        var widget = this;
        if(!widget.selection.brush){
            widget.selection.brush = [];
        }
            
        var idx = widget.selection.brush.findIndex(function(b){
            return (b.cat == d.cat);
        });
        
        if (idx != -1){
            widget.selection.brush.splice(idx,1);
        }
        else{
            if(event.shiftKey){
                widget.selection.brush.push({id:d.id, cat:d.cat});
            }
            else{
                widget.selection.brush = [{id:d.id, cat:d.cat}];
            }                        
        }
        
        if(widget.selection.brush.length < 1){
            delete widget.selection.brush;
        }            
            
        widget.update(); //redraw itself
        widget.updateCallback(widget._encodeArgs());            
    },

    updateSVG : function(){
        var svg = this.svg;
        var margin = this.margin;

        var svgframe = d3.select(svg.node().parentNode);
        
        var width=d3.select(svgframe.node().parentNode).style('width');       
        width = parseFloat(width);
        
        //calculate width and height in side the margin
        width = width-margin.left-margin.right;
        var height = this.totalheight;

        //resize the frame
        svgframe.attr("width", width + margin.left + margin.right);
        svgframe.attr("height", height + margin.top + margin.bottom);

        svg.attr("transform", "translate("+margin.left+","+margin.top+")");

        this.width = width;
        this.height = height;
    },

    updateXAxis: function(data){
        var margin = this.margin;
        var x=this.x;
        var xAxis=this.xAxis;
        var svg=this.svg;

        var svgframe = d3.select(svg.node().parentNode);
        var rect=svgframe.node().parentNode.getBoundingClientRect();
        var width = rect.width - this.margin.left-this.margin.right;

        var d = [d3.min(data, function(d) {return +d.val;}),
                 d3.max(data, function(d) {return +d.val;})];

        if(this._opts.logaxis){ // prevent zeros for log
            d[0] = Math.max(d[0]-1e-6,1e-6);
        }
        else{
            d[0] = Math.min(d[0],d[1]-Math.abs(d[1]/2));
            d[0] = d[0]-0.1*Math.abs(d[0]);
        }
        
        //set domain from options
        if(this._opts.domain){
            if(this._opts.domain.min != undefined){
                d[0] = this._opts.domain.min;
            }
                
            if(this._opts.domain.max != undefined){
                d[1] = this._opts.domain.max;
            }
        }
        
        //update formatting
        xAxis.ticks(3)
            .tickFormat(d3.format(this._numformat)); 

        //set domain
        x.domain(d);        
        x.range([0,width]);        
        xAxis.scale(x);

        //move and draw the axis
        svg.select('.x.axis')
            .attr("transform", "translate(0,"+this.totalheight+")")
            .call(xAxis);
        this.width=width;
    },

    updateYAxis:function(odata){
        var y0=this.y0;
        var y1=this.y1;
        var yAxis=this.yAxis;
        var svg = this.svg;
        var opts = this._opts;
        var sortbtn = this.sortbtn;
        var percentbtn = this.percentbtn;
        
        //Percent
        let data = JSON.parse(JSON.stringify(odata)); //deep copy data
        if (opts.percent){
            //compute total
            let totals = {};
            data.forEach(function(d){
                totals[d.color] = totals[d.color]+d.val || d.val;
            });
            data.forEach(function(d,i){
                data[i].val= d.val/totals[d.color];
            });
            
            percentbtn.html('raw');
            this._numformat='0.2%';
        }
        else{ //sort by data value
            percentbtn.html('%');            
            this._numformat = this._opts.numformat;
        }
        
        //Remove hidden cats
        data = data.filter(function(d){
            var hidden = opts.hidden || [];
            return hidden.indexOf(d.cat)==-1;
        });
        
        //Sort y axis
        y0.domain(data.map(function(d){return d.cat;}));


        if(opts.labels_order){
            y0.domain(opts.labels_order);
        }
        
        if (opts.alpha_order && !opts.labels_order){
            //sort the numbers
            let numlabels = y0.domain().filter(function(d){
                return !isNaN(parseFloat(d));
            });

            let strlabels = y0.domain().filter(function(d){
                return isNaN(parseFloat(d));
            });

            //sort them independently
            numlabels.sort(function(a,b){return parseFloat(a)-parseFloat(b);});
            strlabels.sort();

            //concat
            y0.domain(numlabels.concat(strlabels));

            sortbtn.html('#');
        }

        if(!opts.alpha_order){ //sort by data value
            var d = data.sort(function(x,y){ return y.val - x.val;});
            y0.domain(d.map(function(d){return d.cat;}));
            sortbtn.html('A');
        }
        
        y1.domain(data.map(function(d){return d.color;}));
        var totalheight = y0.domain().length* y1.domain().length * 18;

        y0.range([0, totalheight]);
        y1.range([0, y0.bandwidth()]);
        yAxis.scale(y0);
        svg.select('.y.axis').call(yAxis);

        //enable axis click
        var widget = this;
        svg.select('.y.axis').selectAll('.tick')
            .on('click',function(event,d){
                var obj = data.filter(function(e){return e.cat==d;})[0];
                widget.clickFunc(event,obj);
            });
        
        this.totalheight = totalheight;

        //set margin
        this.margin.left = svg.select('.y.axis').node().getBBox().width + 3;

        //update title with cat count
        svg.select('text').text(this._opts.title+' ('+y0.domain().length+')');

        return data;
    }
    
};


export default GroupedBarChart;
